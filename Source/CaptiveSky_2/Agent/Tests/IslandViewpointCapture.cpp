// Visual-regression captures of fixed Island viewpoints, rendered offscreen from the open editor
// map. Runs in the editor world only: no play session, agents, gateway, or world-state writes.
// Viewpoints live in Config/IslandViewpoints.json so compositions can change without recompiling.
// Run through Scripts/Capture-Viewpoints.ps1 (it needs a real RHI, so never with -nullrhi).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "IslandDayNight.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandNest.h"
#include "IslandCurio.h"
#include "IslandArrangement.h"
#include "IslandGuestBook.h"
#include "IslandFirefly.h"
#include "IslandWeather.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "ContentStreaming.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TextureResource.h"

namespace
{
	struct FIslandViewpoint
	{
		FString Name;
		FVector From = FVector::ZeroVector;
		FVector LookAt = FVector::ForwardVector;
		float FieldOfView = 60.f;
	};

	/** Resolves {"tag": T, "offset": [x,y,z]} or {"at": [x,y,z]} against the actors in World. */
	bool ResolvePoint(UWorld* World, const TSharedPtr<FJsonObject>& Spec, FVector& Out, FString& Error)
	{
		if (!Spec.IsValid()) { Error = TEXT("missing point"); return false; }
		auto ReadVector = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& Vector)
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!Object->TryGetArrayField(Field, Values) || Values->Num() != 3) return false;
			Vector = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
			return true;
		};
		if (ReadVector(Spec, TEXT("at"), Out)) return true;
		FString Tag;
		if (!Spec->TryGetStringField(TEXT("tag"), Tag)) { Error = TEXT("point needs \"tag\" or \"at\""); return false; }
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->ActorHasTag(FName(*Tag))) continue;
			FVector Offset = FVector::ZeroVector;
			ReadVector(Spec, TEXT("offset"), Offset);
			Out = It->GetActorLocation() + Offset;
			return true;
		}
		Error = FString::Printf(TEXT("no actor tagged %s"), *Tag);
		return false;
	}

	/** Captures each viewpoint over several frames (so exposure and streaming settle), then saves a PNG. */
	class FIslandViewpointCaptureCommand : public IAutomationLatentCommand
	{
	public:
		FIslandViewpointCaptureCommand(UWorld* InWorld, TArray<FIslandViewpoint> InViewpoints, float InHour, FIntPoint InSize, FString InDirectory,
			FAutomationTestBase* InTest, AIslandDayNight* InClock, float InOriginalStartHour, TArray<TWeakObjectPtr<AActor>> InPreviewActors,
			AIslandWeather* InGroundCoverWeather, bool bInClearGroundCover)
			: PreviewActors(MoveTemp(InPreviewActors)), World(InWorld), Viewpoints(MoveTemp(InViewpoints)), Hour(InHour), Size(InSize),
			  Directory(MoveTemp(InDirectory)), Test(InTest), Clock(InClock), GroundCoverWeather(InGroundCoverWeather),
			  OriginalStartHour(InOriginalStartHour), bClearGroundCover(bInClearGroundCover) {}

		virtual bool Update() override
		{
			if (!World.IsValid()) return true;
			if (!bStarted)
			{
				bStarted = true;
				for (TActorIterator<AIslandDayNight> It(World.Get()); It; ++It)
				{
					// Preview lighting at the requested hour, exactly as editing Start Hour would; restored afterwards.
					if (!Clock.IsValid())
					{
						Clock = *It;
						OriginalStartHour = It->StartHour;
					}
					It->StartHour = Hour;
					It->OnConstruction(It->GetActorTransform());
					break;
				}
				SpawnLastingChanges();
				Target.Reset(NewObject<UTextureRenderTarget2D>());
				Target->RenderTargetFormat = RTF_RGBA8_SRGB;
				Target->InitAutoFormat(Size.X, Size.Y);
				Target->UpdateResourceImmediate(true);
			}
			if (Index >= Viewpoints.Num())
			{
				if (Clock.IsValid())
				{
					Clock->StartHour = OriginalStartHour;
					Clock->OnConstruction(Clock->GetActorTransform());
				}
				for (const TWeakObjectPtr<AActor>& Preview : PreviewActors)
					if (Preview.IsValid()) Preview->Destroy();
				PreviewActors.Reset();
				if (bClearGroundCover && GroundCoverWeather.IsValid()) GroundCoverWeather->ClearGroundCoverPreview();
				Test->AddInfo(FString::Printf(TEXT("Viewpoint captures saved in %s"), *Directory));
				return true;
			}
			const FIslandViewpoint& View = Viewpoints[Index];
			if (!Capture.IsValid())
			{
				FActorSpawnParameters Spawn;
				Spawn.ObjectFlags |= RF_Transient;
				Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Capture = World->SpawnActor<ASceneCapture2D>(View.From, (View.LookAt - View.From).Rotation(), Spawn);
				USceneCaptureComponent2D* Component = Capture->GetCaptureComponent2D();
				Component->TextureTarget = Target.Get();
				Component->CaptureSource = SCS_FinalColorLDR;
				Component->FOVAngle = View.FieldOfView;
				Component->bCaptureEveryFrame = false;
				Component->bCaptureOnMovement = false;
				Component->bAlwaysPersistRenderingState = true;
				IStreamingManager::Get().StreamAllResources(2.f);
				Frames = 0;
			}
			Capture->GetCaptureComponent2D()->CaptureScene();
			if (++Frames < 60) return false;

			FlushRenderingCommands();
			TArray<FColor> Pixels;
			if (Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) && Pixels.Num() == Size.X * Size.Y)
			{
				for (FColor& Pixel : Pixels) Pixel.A = 255;
				const FString Path = Directory / (View.Name + TEXT(".png"));
				if (FImageUtils::SaveImageByExtension(*Path, FImageView(Pixels.GetData(), Size.X, Size.Y)))
					Test->AddInfo(FString::Printf(TEXT("%s: camera %s looking at %s -> %s"), *View.Name, *View.From.ToString(), *View.LookAt.ToString(), *Path));
				else Test->AddError(FString::Printf(TEXT("Could not write %s"), *Path));
			}
			else Test->AddError(FString::Printf(TEXT("Could not read pixels for %s"), *View.Name));
			Capture->Destroy();
			Capture.Reset();
			++Index;
			return false;
		}

	private:
		/**
		 * Shows what residents have left behind (nests, curios, stone arrangements, and guest-book lines)
		 * by spawning transient previews from WorldState/<Map>.json into the editor world for the capture.
		 * Reading the file changes nothing; -ViewpointNoWorldState captures the bare map instead.
		 */
		void SpawnLastingChanges()
		{
			AIslandGuestBook* Book = nullptr;
			for (TActorIterator<AActor> It(World.Get()); It; ++It)
				if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnCounter")))
				{
					const FVector Location = It->GetActorTransform().TransformPosition(FVector(0.f, 0.f, 60.f));
					FActorSpawnParameters BookSpawn;
					BookSpawn.ObjectFlags |= RF_Transient;
					BookSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					Book = World->SpawnActor<AIslandGuestBook>(AIslandGuestBook::StaticClass(), FTransform(It->GetActorRotation(), Location), BookSpawn);
					if (Book) PreviewActors.Add(Book);
					break;
				}
			if (FParse::Param(FCommandLine::Get(), TEXT("ViewpointNoWorldState"))) return;
			const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("WorldState") / (World->GetMapName() + TEXT(".json")));
			UIslandWorldStateSubsystem* Reader = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
			if (!FPaths::FileExists(Path) || !Reader->ReadStateFile(Path))
			{
				Test->AddInfo(FString::Printf(TEXT("No readable world state at %s; capturing the bare map."), *Path));
				return;
			}
			if (Book)
			{
				FString Display;
				const TArray<FIslandGuestBookEntry>& Entries = Reader->GetGuestBookEntries();
				for (int32 EntryIndex = FMath::Max(0, Entries.Num() - 3); EntryIndex < Entries.Num(); ++EntryIndex)
				{
					const FIslandGuestBookEntry& Entry = Entries[EntryIndex];
					if (!Display.IsEmpty()) Display += TEXT("\n");
					Display += FString::Printf(TEXT("Day %d - %s\n%s"), Entry.Day, *Entry.AgentId.Left(12), *Entry.Line.Left(34));
				}
				Book->SetDisplayText(Display);
			}
			const int32 Today = Reader->GetSavedDay().Get(1);
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			for (const FIslandNestRecord& Nest : Reader->GetNests())
				if (AIslandNest* Actor = World->SpawnActor<AIslandNest>(Nest.Location, FRotator::ZeroRotator, Spawn))
				{
					Actor->SetWoven(Nest.SiteTag, Nest.Layers);
					PreviewActors.Add(Actor);
				}
			for (const FIslandCurioRecord& Curio : Reader->GetCurios())
				if (AIslandCurio* Actor = World->SpawnActor<AIslandCurio>(Curio.Location + FVector(0.f, 0.f, AIslandCurio::GroundClearance), FRotator::ZeroRotator, Spawn))
				{
					Actor->ShowRecord(Curio);
					PreviewActors.Add(Actor);
				}
			for (const FIslandArrangementSite& Site : Reader->GetArrangementSites())
				if (AIslandArrangement* Actor = World->SpawnActor<AIslandArrangement>(Site.Location, FRotator::ZeroRotator, Spawn))
				{
					Actor->ShowSite(Site, Today);
					PreviewActors.Add(Actor);
				}
			Test->AddInfo(FString::Printf(TEXT("Showing %d nest(s), %d curio(s), and %d arranging ground(s) from %s (Island day %d)."),
				Reader->GetNests().Num(), Reader->GetCurios().Num(), Reader->GetArrangementSites().Num(), *Path, Today));
			if (Book) Test->AddInfo(TEXT("The transient open guest book is showing its latest saved lines at the inn counter."));
		}

		TArray<TWeakObjectPtr<AActor>> PreviewActors;
		TWeakObjectPtr<UWorld> World;
		TArray<FIslandViewpoint> Viewpoints;
		float Hour;
		FIntPoint Size;
		FString Directory;
		FAutomationTestBase* Test;
		TStrongObjectPtr<UTextureRenderTarget2D> Target;
		TWeakObjectPtr<ASceneCapture2D> Capture;
		TWeakObjectPtr<AIslandDayNight> Clock;
		TWeakObjectPtr<AIslandWeather> GroundCoverWeather;
		float OriginalStartHour = 9.f;
		int32 Index = 0;
		int32 Frames = 0;
		bool bStarted = false;
		bool bClearGroundCover = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandViewpointCaptureTest, "CaptiveSky2.Visual.Viewpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandViewpointCaptureTest::RunTest(const FString& Parameters)
{
	if (FApp::CanEverRender() == false)
	{
		AddError(TEXT("Viewpoint capture needs a real RHI; run it without -nullrhi (see Scripts/Capture-Viewpoints.ps1)."));
		return false;
	}
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() && Context.World()->GetMapName() == TEXT("Island")) Island = Context.World();
	if (!TestNotNull(TEXT("Island is the open editor map"), Island)) return false;

	const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("IslandViewpoints.json");
	FString Json;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Json, *ConfigPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
	{
		AddError(FString::Printf(TEXT("Could not read %s"), *ConfigPath));
		return false;
	}
	double Hour = 18.5, Width = 1600, Height = 900;
	Root->TryGetNumberField(TEXT("hour"), Hour);
	Root->TryGetNumberField(TEXT("width"), Width);
	Root->TryGetNumberField(TEXT("height"), Height);
	FParse::Value(FCommandLine::Get(), TEXT("ViewpointHour="), Hour);
	const bool bNightFireflyPreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointNightFireflies"));
	const bool bGroundCoverPreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointGroundCover"));
	FString Only;
	FParse::Value(FCommandLine::Get(), TEXT("ViewpointOnly="), Only);
	AIslandDayNight* PreviewClock = nullptr;
	float OriginalStartHour = 9.f;
	TArray<TWeakObjectPtr<AActor>> PreviewActors;
	AIslandWeather* PreviewWeather = nullptr;
	if (bNightFireflyPreview)
	{
		if (!(Hour >= 19.0 || Hour < 5.0))
		{
			AddError(TEXT("Night firefly preview requires a night hour (19:00–05:00); pass -Hour 20."));
			return false;
		}
		for (TActorIterator<AIslandDayNight> It(Island); It; ++It)
		{
			PreviewClock = *It;
			OriginalStartHour = It->StartHour;
			It->StartHour = Hour;
			It->OnConstruction(It->GetActorTransform());
			break;
		}
		for (TActorIterator<AIslandWeather> It(Island); It; ++It) { PreviewWeather = *It; break; }
		if (!PreviewClock || !PreviewWeather)
		{
			if (PreviewClock)
			{
				PreviewClock->StartHour = OriginalStartHour;
				PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
			}
			AddError(TEXT("Night firefly preview needs the Island day/night and weather actors."));
			return false;
		}
		TSet<AIslandFirefly*> ExistingFireflies;
		for (TActorIterator<AIslandFirefly> It(Island); It; ++It) ExistingFireflies.Add(*It);
		PreviewWeather->RefreshNightEcology();
		AActor* Stones = nullptr;
		for (TActorIterator<AActor> It(Island); It; ++It)
			if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"))) { Stones = *It; break; }
		AIslandFirefly* RouteFirefly = nullptr;
		float NearestStoneDistance = TNumericLimits<float>::Max();
		for (TActorIterator<AIslandFirefly> It(Island); It; ++It)
		{
			if (ExistingFireflies.Contains(*It)) continue;
			PreviewActors.Add(*It);
			if (UPointLightComponent* Glow = It->FindComponentByClass<UPointLightComponent>())
			{
				Glow->SetIntensity(It->GlowIntensity);
				Glow->SetVisibility(true);
			}
			if (Stones)
			{
				const float Distance = FVector::Dist(It->GetActorLocation(), Stones->GetActorLocation());
				if (Distance < NearestStoneDistance) { NearestStoneDistance = Distance; RouteFirefly = *It; }
			}
		}
		if (!RouteFirefly || PreviewActors.IsEmpty())
		{
			for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
			PreviewActors.Reset();
			PreviewClock->StartHour = OriginalStartHour;
			PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
			AddError(TEXT("The Island map did not produce a route-side night firefly for capture."));
			return false;
		}
		RouteFirefly->Tags.AddUnique(TEXT("NightPreviewFirefly"));
		AddInfo(FString::Printf(TEXT("Night ecology preview spawned %d transient firefly actors; route-side light is %.0f cm from ListeningStones."),
			PreviewActors.Num(), NearestStoneDistance));
	}
	if (bGroundCoverPreview)
	{
		if (!PreviewWeather)
			for (TActorIterator<AIslandWeather> It(Island); It; ++It) { PreviewWeather = *It; break; }
		if (!PreviewWeather)
		{
			for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
			if (PreviewClock)
			{
				PreviewClock->StartHour = OriginalStartHour;
				PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
			}
			AddError(TEXT("Ground-cover preview needs the Island weather actor."));
			return false;
		}
		PreviewWeather->InitializeGroundCover();
		AddInfo(FString::Printf(TEXT("Transient ground-cover preview placed %d nonblocking grass instances near Tideglass and ListeningStones."), PreviewWeather->GroundCoverInstanceCount));
	}

	// Log the landmarks so compositions can be planned against real coordinates.
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->Tags.Num() > 0 && (It->ActorHasTag(TEXT("IslandLandmark")) || It->ActorHasTag(TEXT("RavenPerch")) || It->GetName().Contains(TEXT("Ocean"))))
			AddInfo(FString::Printf(TEXT("Landmark %s (%s) at %s"), *It->Tags[0].ToString(), *It->GetName(), *It->GetActorLocation().ToString()));

	TArray<FIslandViewpoint> Viewpoints;
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (Root->TryGetArrayField(TEXT("viewpoints"), Entries))
	{
		for (const TSharedPtr<FJsonValue>& Entry : *Entries)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry->TryGetObject(Object)) continue;
			FIslandViewpoint View;
			(*Object)->TryGetStringField(TEXT("name"), View.Name);
			if (!Only.IsEmpty() && !View.Name.Contains(Only)) continue;
			bool bRequiresNightFireflies = false;
			(*Object)->TryGetBoolField(TEXT("night_fireflies"), bRequiresNightFireflies);
			if (bRequiresNightFireflies && !bNightFireflyPreview) continue;
			double FieldOfView = 60.0, MinimumHeight = 160.0;
			(*Object)->TryGetNumberField(TEXT("fov"), FieldOfView);
			(*Object)->TryGetNumberField(TEXT("min_height"), MinimumHeight);
			View.FieldOfView = FieldOfView;
			const TSharedPtr<FJsonObject>* From = nullptr;
			const TSharedPtr<FJsonObject>* Look = nullptr;
			FString Error;
			if (!(*Object)->TryGetObjectField(TEXT("from"), From) || !(*Object)->TryGetObjectField(TEXT("look"), Look) ||
				!ResolvePoint(Island, *From, View.From, Error) || !ResolvePoint(Island, *Look, View.LookAt, Error))
			{
				AddError(FString::Printf(TEXT("Viewpoint %s: %s"), *View.Name, Error.IsEmpty() ? TEXT("needs \"from\" and \"look\"") : *Error));
				continue;
			}
			// Keep the camera at eye level or above whatever ground lies beneath it; a negative min_height
			// (interior shots) opts out, since the trace from above would land on the roof.
			FHitResult Ground;
			if (MinimumHeight >= 0.0 && Island->LineTraceSingleByChannel(Ground, View.From + FVector(0, 0, 5000), View.From - FVector(0, 0, 20000), ECC_Visibility) &&
				View.From.Z < Ground.ImpactPoint.Z + MinimumHeight)
				View.From.Z = Ground.ImpactPoint.Z + MinimumHeight;
			Viewpoints.Add(View);
		}
	}
	if (!TestTrue(TEXT("At least one viewpoint resolved"), Viewpoints.Num() > 0))
	{
		for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
		if (bGroundCoverPreview && PreviewWeather) PreviewWeather->ClearGroundCoverPreview();
		if (PreviewClock)
		{
			PreviewClock->StartHour = OriginalStartHour;
			PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
		}
		return false;
	}
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Viewpoints") /
		FString::Printf(TEXT("%s_h%04.1f"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H%M%S")), Hour));
	ADD_LATENT_AUTOMATION_COMMAND(FIslandViewpointCaptureCommand(Island, MoveTemp(Viewpoints), static_cast<float>(Hour),
		FIntPoint(FMath::Clamp(static_cast<int32>(Width), 64, 3840), FMath::Clamp(static_cast<int32>(Height), 64, 2160)), Directory, this,
		PreviewClock, OriginalStartHour, MoveTemp(PreviewActors), PreviewWeather, bGroundCoverPreview));
	return true;
}

#endif
