#include "IslandSpectator.h"
#include "AgentSocialSubsystem.h"
#include "AutonomousAgentCharacter.h"
#include "IslandDayNight.h"
#include "IslandWorldStateSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/TraceAuxiliary.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandSpectator, Log, All);

FString AIslandSpectatorDirector::ResolveScreenshotDirectory(const FString& ProjectSavedDir, const FString& Override)
{
	FString SavedDirectory = FPaths::ConvertRelativePathToFull(ProjectSavedDir);
	FPaths::NormalizeDirectoryName(SavedDirectory);
	FString Directory = Override.TrimStartAndEnd();
	if (Directory.IsEmpty())
		Directory = FPaths::Combine(SavedDirectory, TEXT("Screenshots"), TEXT("Spectator"));
	else if (FPaths::IsRelative(Directory))
		Directory = FPaths::Combine(SavedDirectory, Directory);
	FPaths::NormalizeDirectoryName(Directory);
	return FPaths::ConvertRelativePathToFull(Directory);
}

namespace
{
	/** "02b_InnFromPath" -> "Inn From Path". */
	FString TitleFromViewpointName(const FString& Name)
	{
		int32 Underscore = INDEX_NONE;
		const FString Bare = Name.FindChar(TEXT('_'), Underscore) ? Name.Mid(Underscore + 1) : Name;
		FString Title;
		for (int32 Index = 0; Index < Bare.Len(); ++Index)
		{
			const TCHAR Character = Bare[Index];
			if (Index > 0 && FChar::IsUpper(Character) && !FChar::IsUpper(Bare[Index - 1])) Title.AppendChar(TEXT(' '));
			Title.AppendChar(Character == TEXT('_') ? TEXT(' ') : Character);
		}
		return Title;
	}

	bool ResolveViewpointPoint(UWorld* World, const TSharedPtr<FJsonObject>& Spec, FVector& Out, AActor** OutAnchor = nullptr)
	{
		if (OutAnchor) *OutAnchor = nullptr;
		auto ReadVector = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& Vector)
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!Object->TryGetArrayField(Field, Values) || Values->Num() != 3) return false;
			Vector = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
			return true;
		};
		if (!Spec.IsValid()) return false;
		if (ReadVector(Spec, TEXT("at"), Out)) return true;
		FString Tag;
		if (!Spec->TryGetStringField(TEXT("tag"), Tag)) return false;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->ActorHasTag(FName(*Tag))) continue;
			FVector Offset = FVector::ZeroVector;
			ReadVector(Spec, TEXT("offset"), Offset);
			Out = It->GetActorLocation() + Offset;
			if (OutAnchor) *OutAnchor = *It;
			UE_LOG(LogIslandSpectator, Display, TEXT("Viewpoint anchor tag '%s' resolved to '%s' at %s"),
				*Tag, *It->GetName(), *Out.ToCompactString());
			return true;
		}
		return false;
	}

	void KeepAboveGround(UWorld* World, FVector& Point, float MinimumHeight)
	{
		FHitResult Ground;
		if (MinimumHeight >= 0.f && World->LineTraceSingleByChannel(Ground, Point + FVector(0, 0, 5000), Point - FVector(0, 0, 20000), ECC_Visibility) &&
			Point.Z < Ground.ImpactPoint.Z + MinimumHeight)
			Point.Z = Ground.ImpactPoint.Z + MinimumHeight;
	}
}

AIslandSpectatorDirector::AIslandSpectatorDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AIslandSpectatorDirector::LoadEstablishingShots()
{
	Establishing.Reset();
	UWorld* World = GetWorld();
	FString CommandLineViewpointFile;
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("SpectatorViewpointFile="), CommandLineViewpointFile) && !CommandLineViewpointFile.IsEmpty())
		Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), CommandLineViewpointFile);
	else
		Path = ViewpointFileOverride.IsEmpty() ? FPaths::ProjectConfigDir() / TEXT("IslandViewpoints.json") : ViewpointFileOverride;
	double ShotDuration = EstablishingSeconds;
	FParse::Value(FCommandLine::Get(), TEXT("SpectatorEstablishingSeconds="), ShotDuration);
	ShotDuration = FMath::Clamp(FMath::IsFinite(ShotDuration) ? ShotDuration : static_cast<double>(EstablishingSeconds), 1.0, 600.0);
	FString Json;
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (FFileHelper::LoadFileToString(Json, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) && Root.IsValid() &&
		Root->TryGetArrayField(TEXT("viewpoints"), Entries))
	{
		for (const TSharedPtr<FJsonValue>& Entry : *Entries)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry->TryGetObject(Object)) continue;
			FString Name;
			(*Object)->TryGetStringField(TEXT("name"), Name);
			if (Name.StartsWith(TEXT("00_"))) continue; // the overhead survey is a map, not a view to linger on
			const TSharedPtr<FJsonObject>* From = nullptr;
			const TSharedPtr<FJsonObject>* Look = nullptr;
			FIslandShot Shot;
			AActor* FromAnchor = nullptr;
			AActor* LookAnchor = nullptr;
			if (!(*Object)->TryGetObjectField(TEXT("from"), From) || !(*Object)->TryGetObjectField(TEXT("look"), Look) ||
				!ResolveViewpointPoint(World, *From, Shot.From, &FromAnchor) || !ResolveViewpointPoint(World, *Look, Shot.LookAt, &LookAnchor)) continue;
			if (FromAnchor && FromAnchor == LookAnchor)
			{
				Shot.AnchorActor = FromAnchor;
				Shot.AnchorLocationAtStart = FromAnchor->GetActorLocation();
			}
			double FieldOfView = 60.0, MinimumHeight = 160.0;
			(*Object)->TryGetNumberField(TEXT("fov"), FieldOfView);
			(*Object)->TryGetNumberField(TEXT("min_height"), MinimumHeight);
			KeepAboveGround(World, Shot.From, MinimumHeight);
			// A slow drift sideways and a little forward keeps a held view alive without calling attention to itself.
			const FVector Toward = (Shot.LookAt - Shot.From).GetSafeNormal2D();
			const float Reach = MinimumHeight < 0.0 ? 60.f : FMath::Min(300.f, FVector::Dist(Shot.From, Shot.LookAt) * 0.04f);
			Shot.To = Shot.From + FVector(-Toward.Y, Toward.X, 0.f) * (MinimumHeight < 0.0 ? 40.f : 150.f) + Toward * Reach;
			KeepAboveGround(World, Shot.To, MinimumHeight);
			Shot.Kind = EIslandShotKind::Establishing;
			Shot.Title = TitleFromViewpointName(Name);
			Shot.FieldOfView = FieldOfView;
			Shot.Duration = static_cast<float>(ShotDuration);
			Shot.Focus = Shot.LookAt;
			Establishing.Add(Shot);
		}
	}
	if (Establishing.Num() > 0)
		UE_LOG(LogIslandSpectator, Log, TEXT("Loaded %d establishing viewpoints from %s at %.1f seconds per shot"), Establishing.Num(), *Path, ShotDuration);
	if (Establishing.Num() > 0) return;
	// Without viewpoints, circle the landmarks instead.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("IslandLandmark")) || It->Tags.Num() == 0) continue;
		FIslandShot Shot;
		Shot.LookAt = It->GetActorLocation();
		Shot.From = Shot.LookAt + FVector(900.f, 0.f, 300.f);
		Shot.To = Shot.LookAt + FVector(0.f, 900.f, 300.f);
		KeepAboveGround(World, Shot.From, 160.f);
		KeepAboveGround(World, Shot.To, 160.f);
		Shot.Title = It->Tags[0].ToString();
		Shot.Duration = static_cast<float>(ShotDuration);
		Shot.Focus = Shot.LookAt;
		Establishing.Add(Shot);
	}
}

void AIslandSpectatorDirector::BeginSpectating(APlayerController* Player)
{
	UWorld* World = GetWorld();
	if (!World || !Player) return;
	int32 RequestedCSVFrames = 0;
	double CSVDelaySeconds = 0.0;
	if (FParse::Value(FCommandLine::Get(), TEXT("SpectatorCSVProfileFrames="), RequestedCSVFrames) &&
		FParse::Value(FCommandLine::Get(), TEXT("SpectatorCSVProfileDelaySeconds="), CSVDelaySeconds) && RequestedCSVFrames > 0)
	{
		CSVProfileFrames = FMath::Clamp(RequestedCSVFrames, 1, 2000);
		CSVProfileStartAt = FPlatformTime::Seconds() + FMath::Clamp(FMath::IsFinite(CSVDelaySeconds) ? CSVDelaySeconds : 60.0, 1.0, 1800.0);
		UE_LOG(LogIslandSpectator, Log, TEXT("Will start a %d-frame CSV profile %.1f real seconds after spectator startup."), CSVProfileFrames, CSVProfileStartAt - FPlatformTime::Seconds());
	}
	int32 TraceDelaySeconds = 45;
	int32 TraceDurationSeconds = 30;
	if (FParse::Value(FCommandLine::Get(), TEXT("SpectatorTraceProfileFile="), TraceProfilePath) && !TraceProfilePath.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("SpectatorTraceProfileDelaySeconds="), TraceDelaySeconds);
		FParse::Value(FCommandLine::Get(), TEXT("SpectatorTraceProfileDurationSeconds="), TraceDurationSeconds);
		TraceDelaySeconds = FMath::Clamp(TraceDelaySeconds, 1, 1800);
		TraceDurationSeconds = FMath::Clamp(TraceDurationSeconds, 5, 180);
		TraceProfileDurationSeconds = TraceDurationSeconds;
		TraceProfilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), TraceProfilePath);
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(TraceProfilePath), true);
		TraceProfileStartAt = FPlatformTime::Seconds() + TraceDelaySeconds;
		TraceProfileStopAt = TraceProfileStartAt + TraceDurationSeconds;
		UE_LOG(LogIslandSpectator, Log, TEXT("Will capture a %d-second CPU/GPU trace to %s, starting %.1f real seconds after spectator startup."),
			TraceDurationSeconds, *TraceProfilePath, TraceProfileStartAt - FPlatformTime::Seconds());
	}
	Viewer = Player;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Camera = World->SpawnActor<ACameraActor>(GetActorLocation(), FRotator::ZeroRotator, Spawn);
	Camera->GetCameraComponent()->bConstrainAspectRatio = false;

	// The pawn stays in the world, invisible and inert, and travels with each shot's subject.
	if (APawn* Pawn = Player->GetPawn())
	{
		HiddenPawn = Pawn;
		PawnHome = Pawn->GetActorLocation();
		Pawn->SetActorHiddenInGame(true);
		Pawn->SetActorEnableCollision(false);
		if (ACharacter* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->DisableMovement();
	}
	Player->SetIgnoreMoveInput(true);
	Player->SetIgnoreLookInput(true);
	Player->SetViewTargetWithBlend(Camera, 0.f);
	if (UAgentSocialSubsystem* Social = World->GetSubsystem<UAgentSocialSubsystem>())
		Social->OnAmbientSpeech.AddDynamic(this, &AIslandSpectatorDirector::HandleSpeech);

	if (GEngine && GEngine->GameViewport && World->IsGameWorld())
	{
		CaptionWidget = SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(48.f, 0.f, 0.f, 36.f))
			[
				SAssignNew(CaptionText, STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
				.ColorAndOpacity(FLinearColor(1.f, 0.96f, 0.9f, 0.8f))
				.ShadowOffset(FVector2D(1.f, 1.f))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
			];
		GEngine->GameViewport->AddViewportWidgetContent(CaptionWidget.ToSharedRef(), 5);
	}

	LoadEstablishingShots();
	SincePoll = 0.f;
	WorldStateSignature.Reset();
	PollLastingChanges(); // baseline only
	NextEstablishingShot();
	UE_LOG(LogIslandSpectator, Log, TEXT("Spectating with %d establishing views."), Establishing.Num());
}

void AIslandSpectatorDirector::EndSpectating()
{
	UWorld* World = GetWorld();
	if (World)
		if (UAgentSocialSubsystem* Social = World->GetSubsystem<UAgentSocialSubsystem>())
			Social->OnAmbientSpeech.RemoveDynamic(this, &AIslandSpectatorDirector::HandleSpeech);
	if (APawn* Pawn = HiddenPawn.Get())
	{
		Pawn->SetActorLocation(PawnHome, false, nullptr, ETeleportType::TeleportPhysics);
		Pawn->SetActorHiddenInGame(false);
		Pawn->SetActorEnableCollision(true);
		if (ACharacter* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	if (APlayerController* Player = Viewer.Get())
	{
		Player->ResetIgnoreMoveInput();
		Player->ResetIgnoreLookInput();
		if (HiddenPawn.IsValid()) Player->SetViewTargetWithBlend(HiddenPawn.Get(), 0.f);
	}
	if (CaptionWidget.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(CaptionWidget.ToSharedRef());
	CaptionWidget.Reset();
	CaptionText.Reset();
	if (Camera) Camera->Destroy();
	Camera = nullptr;
	Viewer.Reset();
	HiddenPawn.Reset();
}

void AIslandSpectatorDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bTraceProfileActive)
	{
		bTraceProfileActive = false;
		const bool bStopped = FTraceAuxiliary::Stop();
		UE_LOG(LogIslandSpectator, Log, TEXT("Stopped delayed Insights trace during spectator shutdown (%s)."), bStopped ? TEXT("success") : TEXT("failure"));
	}
	if (Viewer.IsValid()) EndSpectating();
	Super::EndPlay(EndPlayReason);
}

FString AIslandSpectatorDirector::DescribeClock() const
{
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
		return FString::Printf(TEXT("Day %d, %02d:%02d"), It->DayNumber, FMath::FloorToInt(It->CurrentHour), FMath::FloorToInt(FMath::Frac(It->CurrentHour) * 60.f));
	return FString();
}

void AIslandSpectatorDirector::StartShot(const FIslandShot& Shot)
{
	Current = Shot;
	if (AActor* Anchor = Current.AnchorActor.Get())
	{
		const FVector AnchorLocation = Anchor->GetActorLocation();
		const FVector AnchorDelta = AnchorLocation - Current.AnchorLocationAtStart;
		Current.From += AnchorDelta;
		Current.To += AnchorDelta;
		Current.LookAt += AnchorDelta;
		Current.Focus += AnchorDelta;
		Current.AnchorLocationAtStart = AnchorLocation;
	}
	Elapsed = 0.f;
	++ShotIndex;
	bShotCaptured = false;
	if (Camera)
	{
		Camera->SetActorLocationAndRotation(Shot.From, (Shot.LookAt - Shot.From).Rotation());
		Camera->GetCameraComponent()->SetFieldOfView(Shot.FieldOfView);
	}
	if (APawn* Pawn = HiddenPawn.Get()) Pawn->SetActorLocation(Shot.Focus + FVector(0, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
	const FString Clock = DescribeClock();
	if (CaptionText.IsValid()) CaptionText->SetText(FText::FromString(Clock.IsEmpty() ? Shot.Title : Shot.Title + TEXT("  ·  ") + Clock));
}

void AIslandSpectatorDirector::NextEstablishingShot()
{
	if (Establishing.Num() == 0) return;
	// At night the inn's lit windows are the warmest thing to watch, so every other view goes there.
	float Hour = 12.f;
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It) { Hour = It->CurrentHour; break; }
	const bool bNight = Hour >= 19.5f || Hour < 5.5f;
	TArray<int32> InnShots;
	for (int32 Index = 0; Index < Establishing.Num(); ++Index)
		if (Establishing[Index].Title.Contains(TEXT("Inn"))) InnShots.Add(Index);
	bInnTurn = bNight && InnShots.Num() > 0 && !bInnTurn;
	if (bInnTurn)
	{
		StartShot(Establishing[InnShots[NextInn++ % InnShots.Num()]]);
		return;
	}
	// The full journey keeps its own place in the rotation, so every view still comes round at night.
	StartShot(Establishing[NextEstablishing++ % Establishing.Num()]);
}

FVector AIslandSpectatorDirector::FrameSubject(UWorld* World, const FVector& Head, const FVector& Facing, float Distance, const TArray<const AActor*>& Ignore)
{
	const FVector Forward = Facing.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : Facing.GetSafeNormal2D();
	FCollisionQueryParams Query(SCENE_QUERY_STAT(SpectatorFraming), false);
	for (const AActor* Actor : Ignore) Query.AddIgnoredActor(Actor);
	FVector Fallback = Head + Forward * Distance + FVector(0, 0, 40);
	// Three-quarter views first, then profiles, straight on, and from behind.
	for (const float Yaw : { 35.f, -35.f, 70.f, -70.f, 0.f, 115.f, -115.f, 180.f })
	{
		const FVector Direction = Forward.RotateAngleAxis(Yaw, FVector::UpVector);
		const FVector Candidate = Head + Direction * Distance + FVector(0, 0, 40);
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Head, Candidate, ECC_Visibility, Query)) return Candidate;
		if (Yaw == 35.f) Fallback = Hit.ImpactPoint - Direction * 30.f;
	}
	return Fallback;
}

void AIslandSpectatorDirector::FocusOnSpeaker(AActor* Speaker, const FString& Speech)
{
	UWorld* World = GetWorld();
	if (!World || !Speaker) return;
	const FBox Bounds = Speaker->GetComponentsBoundingBox(false);
	const FVector Head = Bounds.IsValid ? FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z - 15.f) : Speaker->GetActorLocation();
	// Frame the listener too when another resident is close: a two-shot from the side of their line.
	const AActor* Listener = nullptr;
	float Nearest = 800.f;
	for (TActorIterator<AAutonomousAgentCharacter> It(World); It; ++It)
		if (*It != Speaker && FVector::Dist(It->GetActorLocation(), Speaker->GetActorLocation()) < Nearest) { Nearest = FVector::Dist(It->GetActorLocation(), Speaker->GetActorLocation()); Listener = *It; }
	FIslandShot Shot;
	Shot.Kind = EIslandShotKind::Speech;
	const AAutonomousAgentCharacter* AgentSpeaker = Cast<AAutonomousAgentCharacter>(Speaker);
	Shot.Title = AgentSpeaker ? AgentSpeaker->GetAgentDisplayName() : Speaker->GetActorNameOrLabel();
	Shot.FieldOfView = 50.f;
	Shot.Duration = FMath::Clamp(4.f + Speech.Len() / 15.f, 5.f, 12.f);
	Shot.Focus = Speaker->GetActorLocation();
	TArray<const AActor*> Ignore = { Speaker };
	if (HiddenPawn.IsValid()) Ignore.Add(HiddenPawn.Get());
	if (Listener)
	{
		Ignore.Add(Listener);
		const FVector Between = Listener->GetActorLocation() - Speaker->GetActorLocation();
		Shot.LookAt = (Head + Listener->GetActorLocation() + FVector(0, 0, Head.Z - Speaker->GetActorLocation().Z)) * 0.5f;
		const FVector Side(-Between.Y, Between.X, 0.f);
		Shot.From = FrameSubject(World, Shot.LookAt, Side, FMath::Max(420.f, Between.Size() * 1.3f), Ignore);
	}
	else
	{
		Shot.LookAt = Head;
		Shot.From = FrameSubject(World, Head, Speaker->GetActorForwardVector(), 380.f, Ignore);
	}
	// A barely perceptible push-in.
	Shot.To = Shot.From + (Shot.LookAt - Shot.From).GetSafeNormal() * 40.f;
	StartShot(Shot);
}

void AIslandSpectatorDirector::FocusOnChange(const FVector& Location, const FString& Title)
{
	UWorld* World = GetWorld();
	if (!World) return;
	FIslandShot Shot;
	Shot.Kind = EIslandShotKind::Made;
	Shot.Title = Title;
	Shot.LookAt = Location + FVector(0, 0, 40);
	const FVector Facing = Camera ? Camera->GetActorLocation() - Location : FVector::ForwardVector;
	TArray<const AActor*> Ignore;
	if (HiddenPawn.IsValid()) Ignore.Add(HiddenPawn.Get());
	Shot.From = FrameSubject(World, Shot.LookAt, Facing, 500.f, Ignore) + FVector(0, 0, 120);
	Shot.To = Shot.From + FVector(0, 0, -60) + (Shot.LookAt - Shot.From).GetSafeNormal2D() * 80.f;
	Shot.FieldOfView = 55.f;
	Shot.Duration = 12.f;
	Shot.Focus = Location;
	StartShot(Shot);
}

void AIslandSpectatorDirector::HandleSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech)
{
	FocusOnSpeaker(Speaker, Speech);
}

void AIslandSpectatorDirector::PollLastingChanges()
{
	const UIslandWorldStateSubsystem* State = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!State) return;
	// One line per lasting thing; the first line that differs from the last poll is what changed.
	struct FLasting { FString Key; FVector Where; FString Title; };
	TArray<FLasting> Items;
	for (const FIslandNestRecord& Nest : State->GetNests())
		Items.Add({ FString::Printf(TEXT("nest:%s:%d"), *Nest.SiteTag.ToString(), Nest.Layers), Nest.Location,
			FString::Printf(TEXT("A nest at %s, %d of %d layers"), *Nest.SiteTag.ToString(), Nest.Layers, UIslandWorldStateSubsystem::MaxNestLayers) });
	for (const FIslandCurioRecord& Curio : State->GetCurios())
	{
		if (Curio.Kind == EIslandCurioKind::Cairn)
			Items.Add({ FString::Printf(TEXT("cairn:%d"), Curio.State), Curio.Location, FString::Printf(TEXT("The cairn, %d stones high"), Curio.State) });
		if (Curio.Kind == EIslandCurioKind::SeedPod)
			Items.Add({ FString::Printf(TEXT("pod:%d"), Curio.State), Curio.Location, FString(Curio.State >= 3 ? TEXT("The seed pod, open") : TEXT("The seed pod, opening")) });
	}
	for (const FIslandArrangementSite& Site : State->GetArrangementSites())
	{
		if (!Site.bHasWork) continue;
		const FString Form = UIslandWorldStateSubsystem::FormName(Site.Form);
		Items.Add({ FString::Printf(TEXT("work:%s:%d"), *Site.Id.ToString(), Site.Responses.Num()), Site.Location,
			Site.Responses.Num() == 0 ? FString::Printf(TEXT("A stone %s, newly arranged"), *Form)
				: FString::Printf(TEXT("A stone %s, answered %d time%s"), *Form, Site.Responses.Num(), Site.Responses.Num() == 1 ? TEXT("") : TEXT("s")) });
	}

	FString Signature;
	for (const FLasting& Item : Items) Signature += Item.Key + LINE_TERMINATOR;
	const bool bBaseline = WorldStateSignature.IsEmpty();
	if (!bBaseline && Signature != WorldStateSignature)
	{
		TArray<FString> Before;
		WorldStateSignature.ParseIntoArrayLines(Before);
		for (const FLasting& Item : Items)
			if (!Before.Contains(Item.Key)) { FocusOnChange(Item.Where, Item.Title); break; }
	}
	WorldStateSignature = Signature.IsEmpty() ? TEXT("-") : Signature;
}

void AIslandSpectatorDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Viewer.IsValid() || !Camera) return;
	const double RealNow = FPlatformTime::Seconds();
	if (!TraceProfilePath.IsEmpty() && !bTraceProfileStarted && RealNow >= TraceProfileStartAt)
	{
		bTraceProfileStarted = true;
		FTraceAuxiliary::FOptions TraceOptions;
		TraceOptions.bExcludeTail = true;
		bTraceProfileActive = FTraceAuxiliary::Start(FTraceAuxiliary::EConnectionType::File, *TraceProfilePath,
			TEXT("cpu,gpu,frame,bookmark"), &TraceOptions);
		if (bTraceProfileActive) TraceProfileStopAt = RealNow + TraceProfileDurationSeconds;
		UE_LOG(LogIslandSpectator, Log, TEXT("Delayed Insights trace %s: %s"),
			bTraceProfileActive ? TEXT("started") : TEXT("failed to start"), *TraceProfilePath);
	}
	if (bTraceProfileActive && RealNow >= TraceProfileStopAt)
	{
		bTraceProfileActive = false;
		const bool bStopped = FTraceAuxiliary::Stop();
		UE_LOG(LogIslandSpectator, Log, TEXT("Delayed Insights trace %s: %s"),
			bStopped ? TEXT("stopped") : TEXT("failed to stop"), *TraceProfilePath);
	}
	if (CSVProfileFrames > 0 && !bCSVProfileStarted && FPlatformTime::Seconds() >= CSVProfileStartAt)
	{
		bCSVProfileStarted = true;
		const FString Command = FString::Printf(TEXT("csvprofile frames=%d"), CSVProfileFrames);
		const bool bCommandAccepted = GEngine && GEngine->Exec(GetWorld(), *Command);
		UE_LOG(LogIslandSpectator, Log, TEXT("Delayed CSV profile command %s: %s"), bCommandAccepted ? TEXT("accepted") : TEXT("rejected"), *Command);
	}
	if (AActor* Anchor = Current.AnchorActor.Get())
	{
		const FVector AnchorLocation = Anchor->GetActorLocation();
		const FVector AnchorDelta = AnchorLocation - Current.AnchorLocationAtStart;
		if (!AnchorDelta.IsNearlyZero())
		{
			Current.From += AnchorDelta;
			Current.To += AnchorDelta;
			Current.LookAt += AnchorDelta;
			Current.Focus += AnchorDelta;
			if (APawn* Pawn = HiddenPawn.Get())
				Pawn->SetActorLocation(Pawn->GetActorLocation() + AnchorDelta, false, nullptr, ETeleportType::TeleportPhysics);
			Current.AnchorLocationAtStart = AnchorLocation;
		}
	}
	Elapsed += DeltaSeconds;
	const float Alpha = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Elapsed / FMath::Max(1.f, Current.Duration), 0.f, 1.f));
	const FVector Location = FMath::Lerp(Current.From, Current.To, Alpha);
	Camera->SetActorLocationAndRotation(Location, (Current.LookAt - Location).Rotation());
	SincePoll += DeltaSeconds;
	if (SincePoll >= 5.f)
	{
		SincePoll = 0.f;
		PollLastingChanges();
	}
	// -SpectatorShots saves one frame per shot, a few seconds in, for reviewing a run afterwards.
	if (!bShotCaptured && Elapsed >= 3.f && FParse::Param(FCommandLine::Get(), TEXT("SpectatorShots")))
	{
		bShotCaptured = true;
		FString Name = Current.Title;
		for (TCHAR& Character : Name) if (!FChar::IsAlnum(Character)) Character = TEXT('_');
		FString ScreenshotDirectoryOverride;
		FParse::Value(FCommandLine::Get(), TEXT("SpectatorScreenshotDir="), ScreenshotDirectoryOverride);
		const FString ScreenshotDirectory = ResolveScreenshotDirectory(FPaths::ProjectSavedDir(), ScreenshotDirectoryOverride);
		IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
		const FString ScreenshotPath = ScreenshotDirectory / FString::Printf(TEXT("%03d_%s.png"), ShotIndex, *Name);
		FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
		UE_LOG(LogIslandSpectator, Log, TEXT("Spectator screenshot queued at %s"), *ScreenshotPath);
	}
	if (Elapsed >= Current.Duration) NextEstablishingShot();
}

bool UIslandSpectatorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandSpectatorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!FParse::Param(FCommandLine::Get(), TEXT("Spectator"))) return;
	// The player and their pawn arrive during play start; begin once they exist.
	InWorld.GetTimerManager().SetTimer(StartRetry, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		APlayerController* Player = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (!Player || !Player->GetPawn()) return;
		GetWorld()->GetTimerManager().ClearTimer(StartRetry);
		StartSpectating();
	}), 1.f, true, 1.f);
}

void UIslandSpectatorSubsystem::StartSpectating()
{
	UWorld* World = GetWorld();
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Player || IsSpectating()) return;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Director = World->SpawnActor<AIslandSpectatorDirector>(Spawn);
	if (Director.IsValid()) Director->BeginSpectating(Player);
}

void UIslandSpectatorSubsystem::StopSpectating()
{
	if (!Director.IsValid()) return;
	Director->EndSpectating();
	Director->Destroy();
	Director.Reset();
}

static FAutoConsoleCommandWithWorld GIslandSpectateCommand(
	TEXT("Island.Spectate"),
	TEXT("Toggles spectator mode: a slow camera over the Island that cuts to residents when they speak or make something."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (UIslandSpectatorSubsystem* Spectator = World ? World->GetSubsystem<UIslandSpectatorSubsystem>() : nullptr)
			Spectator->IsSpectating() ? Spectator->StopSpectating() : Spectator->StartSpectating();
	}));
