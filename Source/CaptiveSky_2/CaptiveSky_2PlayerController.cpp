// Copyright Epic Games, Inc. All Rights Reserved.


#include "CaptiveSky_2PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSky_2.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "CaptiveSkyConversationWidget.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "CaptiveSkyGuestBookWidget.h"
#include "CaptiveSkyArrangementWidget.h"
#include "Agent/AutonomousAgentCharacter.h"
#include "Agent/AgentMemoryComponent.h"
#include "Agent/AgentBrainComponent.h"
#include "Agent/AgentSocialSubsystem.h"
#include "Agent/IslandGuestBook.h"
#include "Agent/IslandArrangement.h"
#include "Agent/IslandRainBasin.h"
#include "Agent/IslandInteractionUtility.h"
#include "Agent/IslandVegetationUtility.h"
#include "Agent/IslandTrail.h"
#include "Agent/IslandWorldStateSubsystem.h"
#include "Agent/IslandSpectator.h"
#include "Agent/IslandEnvironmentSubsystem.h"
#include "Agent/IslandDayNight.h"
#include "Agent/IslandWeather.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

void ACaptiveSky_2PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCaptiveSky_2, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	if (IsLocalPlayerController())
	{
		ConversationWidget = CreateWidget<UCaptiveSkyConversationWidget>(this, UCaptiveSkyConversationWidget::StaticClass());
		if (ConversationWidget)
		{
			ConversationWidget->AddToPlayerScreen(20);
			ConversationWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		GuestBookWidget = CreateWidget<UCaptiveSkyGuestBookWidget>(this, UCaptiveSkyGuestBookWidget::StaticClass());
		if (GuestBookWidget)
		{
			GuestBookWidget->AddToPlayerScreen(25);
			GuestBookWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		ArrangementWidget = CreateWidget<UCaptiveSkyArrangementWidget>(this, UCaptiveSkyArrangementWidget::StaticClass());
		if (ArrangementWidget)
		{
			ArrangementWidget->AddToPlayerScreen(24);
			ArrangementWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		AmbientSpeechWidget = CreateWidget<UCaptiveSkyAmbientSpeechWidget>(this, UCaptiveSkyAmbientSpeechWidget::StaticClass());
		if (AmbientSpeechWidget)
		{
			AmbientSpeechWidget->AddToPlayerScreen(10);
			AmbientSpeechWidget->SetVisibility(ESlateVisibility::Collapsed);
			GetWorldTimerManager().SetTimer(InteractionHintRefreshTimer, this,
				&ACaptiveSky_2PlayerController::UpdateWorldInteractionHint,
				FMath::Max(0.1f, InteractionHintRefreshSeconds), true);
			UpdateWorldInteractionHint();
		}
		if (UAgentSocialSubsystem* SocialSubsystem = GetWorld()->GetSubsystem<UAgentSocialSubsystem>())
		{
			SocialSubsystem->OnAmbientSpeech.AddDynamic(this, &ACaptiveSky_2PlayerController::HandleAmbientAgentSpeech);
		}
	}
}

void ACaptiveSky_2PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		if (UAgentSocialSubsystem* SocialSubsystem = GetWorld()->GetSubsystem<UAgentSocialSubsystem>())
		{
			SocialSubsystem->OnAmbientSpeech.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleAmbientAgentSpeech);
		}
		GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
		GetWorldTimerManager().ClearTimer(InteractionHintRefreshTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ACaptiveSky_2PlayerController::HandleAmbientAgentSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech)
{
	if (!AmbientSpeechWidget || !Speaker || !GetPawn() ||
		FVector::DistSquared(Speaker->GetActorLocation(), GetPawn()->GetActorLocation()) > FMath::Square(AmbientSpeechRadius))
	{
		return;
	}
	AmbientSpeechWidget->ShowSpeech(Speaker->GetAgentDisplayName(), Speech);
	GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	GetWorldTimerManager().SetTimer(AmbientSpeechHideTimer, this,
		&ACaptiveSky_2PlayerController::HideAmbientSpeech, AmbientSpeechDurationSeconds, false);
}

void ACaptiveSky_2PlayerController::HideAmbientSpeech()
{
	if (AmbientSpeechWidget)
	{
		AmbientSpeechWidget->HideCaption();
	}
}

void ACaptiveSky_2PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACaptiveSky_2PlayerController::ToggleConversation);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ACaptiveSky_2PlayerController::InteractWithNearestWorldObject);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACaptiveSky_2PlayerController::HandleEscape);

		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

AActor* ACaptiveSky_2PlayerController::FindNearestWorldInteraction() const
{
	const APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn || !GetWorld()) return nullptr;
	return IslandInteractionUtility::FindNearestVisibleTarget(PlayerPawn, GetWorld(), IslandInteractionRadius);
}

FString ACaptiveSky_2PlayerController::DescribeTrailUnderfoot() const
{
	const APawn* PlayerPawn = GetPawn();
	UWorld* World = GetWorld();
	const UIslandTrailSubsystem* Trail = World ? World->GetSubsystem<UIslandTrailSubsystem>() : nullptr;
	if (!PlayerPawn || !Trail) return FString();

	float Wetness = 0.f;
	if (const UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>())
		Wetness = Environment->GetWetness();
	return Trail->DescribeUnderfoot(PlayerPawn->GetActorLocation(), Wetness);
}

FString ACaptiveSky_2PlayerController::DescribeNearbyWildlife() const
{
	const APawn* Visitor = GetPawn();
	UWorld* World = GetWorld();
	if (!Visitor || !World) return FString();

	constexpr float WildlifeObservationRadiusCm = 1800.f;
	AActor* NearestWildlife = nullptr;
	FString WildlifeDescription;
	float NearestDistanceSquared = FMath::Square(WildlifeObservationRadiusCm);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("IslandLife"))) continue;

		const FName TargetTag = IslandInteractionUtility::GetTargetTag(*It);
		const TCHAR* Description = nullptr;
		if (TargetTag == FName(TEXT("WoodlandDeer"))) Description = TEXT("a stag browsing at the woodland edge");
		else if (TargetTag == FName(TEXT("WoodlandFox"))) Description = TEXT("a fox moving along the woodland margin");
		else if (TargetTag == FName(TEXT("Firefly"))) Description = TEXT("a firefly glimmering nearby");
		else if (TargetTag == FName(TEXT("TidepoolCrab"))) Description = TEXT("a small shore crab working along the tide line");
		else if (TargetTag == FName(TEXT("TidepoolPeriwinkle"))) Description = TEXT("a periwinkle clinging to the shore rocks");
		else if (TargetTag == FName(TEXT("TideglassDragonfly"))) Description = TEXT("a dragonfly hovering above the shore");
		else if (TargetTag == FName(TEXT("TideglassFrog"))) Description = TEXT("a common frog resting among damp plants");
		else if (TargetTag == FName(TEXT("MinnowSchool"))) Description = TEXT("a small school of minnows turning through the shallows");
		if (!Description) continue;

		const float DistanceSquared = FVector::DistSquared(Visitor->GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared >= NearestDistanceSquared || !IslandInteractionUtility::CanInteract(Visitor, *It, WildlifeObservationRadiusCm)) continue;
		NearestWildlife = *It;
		WildlifeDescription = Description;
		NearestDistanceSquared = DistanceSquared;
	}

	return NearestWildlife
		? FString::Printf(TEXT("You notice %s nearby and leave it undisturbed."), *WildlifeDescription)
		: FString();
}

FString ACaptiveSky_2PlayerController::DescribeLocalWeather() const
{
	const UWorld* World = GetWorld();
	const UIslandEnvironmentSubsystem* Environment = World ? World->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
	if (!Environment || Environment->GetIndoors() >= 0.5f) return FString();
	return UIslandEnvironmentSubsystem::DescribeWeather(Environment->GetRainIntensity(), Environment->GetWetness(),
		Environment->GetWind(), Environment->GetMist());
}

FString ACaptiveSky_2PlayerController::DescribeLocalWindShelter() const
{
	const APawn* PlayerPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!PlayerPawn || !World) return FString();

	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		const FVector Position = PlayerPawn->GetActorLocation();
		if (It->SampleWind(Position, World->GetTimeSeconds()).Size() < 35.f) return FString();

		// Reuse the same conservative six-metre upwind test used by residents.
		// This describes horizontal wind only; the result deliberately does not
		// claim overhead rain cover or a safe place to perch or nest.
		return FString::Printf(TEXT("You pause to notice the wind. %s"),
			*It->DescribeWindShelterAt(Position, PlayerPawn));
	}
	return FString();
}

FString ACaptiveSky_2PlayerController::DescribeLocalSky() const
{
	UWorld* World = GetWorld();
	if (!World) return FString();

	const UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>();
	if (Environment && Environment->GetIndoors() >= 0.5f) return FString();
	const float CloudCover = Environment ? Environment->GetCloudCover() : 0.f;

	for (TActorIterator<AIslandDayNight> It(World); It; ++It)
	{
		const int32 Day = It->DayNumber;
		const float Hour = It->CurrentHour;
		FString Observation = AIslandDayNight::DescribeSunEdge(Hour, Day) +
			AIslandDayNight::DescribeHorizonColour(Hour, CloudCover, Day);
		if (Observation.IsEmpty())
		{
			const float Night = AIslandDayNight::NightAmount(Hour, Day);
			const float Moonlight = AIslandDayNight::LunarIllumination(Day, Hour);
			const float MoonHeight = AIslandDayNight::MoonHeight(Day, Hour);
			Observation = AIslandDayNight::DescribeStars(Night, CloudCover, Moonlight, MoonHeight);
		}
		return Observation.IsEmpty() ? FString() : FString::Printf(TEXT("You pause to read the sky.%s"), *Observation);
	}
	return FString();
}

FString ACaptiveSky_2PlayerController::DescribeNearbyTide() const
{
	const APawn* PlayerPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!PlayerPawn || !World) return FString();

	constexpr float TideglassObservationRadiusCm = 1200.f;
	const AActor* NearestTideglass = nullptr;
	float NearestDistanceSquared = FMath::Square(TideglassObservationRadiusCm);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("TideglassPool"))) continue;
		const float DistanceSquared = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared <= NearestDistanceSquared)
		{
			NearestTideglass = *It;
			NearestDistanceSquared = DistanceSquared;
		}
	}
	if (!NearestTideglass) return FString();

	for (TActorIterator<AIslandDayNight> It(World); It; ++It)
		return FString::Printf(TEXT("You pause beside Tideglass.%s"), *AIslandDayNight::DescribeTide(It->DayNumber, It->CurrentHour));
	return FString();
}

FString ACaptiveSky_2PlayerController::DescribeWorldInteractionHint(AActor* Target) const
{
	if (!Target) return FString();
	const FName Tag = IslandInteractionUtility::GetTargetTag(Target);
	FString Action;
	if (Tag == FName(TEXT("GuestBook"))) Action = TEXT("read or leave a public guest-book line");
	else if (Tag == FName(TEXT("IslandArrangement")))
	{
		const AIslandArrangement* WorkActor = Cast<AIslandArrangement>(Target);
		const UIslandWorldStateSubsystem* State = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
		const FIslandArrangementSite* Site = WorkActor && State ? State->FindArrangementSite(WorkActor->GetSiteId()) : nullptr;
		Action = Site && Site->bHasWork ? TEXT("see or answer the stone work") : TEXT("arrange a few stones");
	}
	else if (Tag == FName(TEXT("Firefly"))) Action = TEXT("quietly watch a wild firefly");
	else if (Tag == FName(TEXT("WoodlandFox"))) Action = TEXT("quietly watch a wild fox at the woodland edge");
	else if (Tag == FName(TEXT("WoodlandDeer"))) Action = TEXT("quietly watch a wild stag grazing");
	else if (Tag == FName(TEXT("TidepoolCrab"))) Action = TEXT("quietly watch a wild shore crab");
	else if (Tag == FName(TEXT("TidepoolPeriwinkle"))) Action = TEXT("quietly watch a small wild periwinkle");
	else if (Tag == FName(TEXT("TideglassDragonfly"))) Action = TEXT("quietly watch a Tideglass dragonfly");
	else if (Tag == FName(TEXT("TideglassFrog"))) Action = TEXT("quietly watch a wild common frog");
	else if (Tag == FName(TEXT("MinnowSchool"))) Action = TEXT("quietly watch the wild minnows");
	else if (Tag == FName(TEXT("RainBasin")))
	{
		const UIslandRainBasinSubsystem* Basin = GetWorld() ? GetWorld()->GetSubsystem<UIslandRainBasinSubsystem>() : nullptr;
		Action = Basin && Basin->GetState().Water >= FIslandBasinState::FloatLevel
			? TEXT("float a leaf on the rainwater") : TEXT("inspect the rain basin");
	}
	else if (Tag == FName(TEXT("InnHearth"))) Action = TEXT("tend the inn hearth");
	else if (Tag == FName(TEXT("ListeningStones"))) Action = TEXT("listen to the ListeningStones");
	else if (Tag == FName(TEXT("TideglassPool"))) Action = TEXT("send a brief ripple across the pool");
	else if (Tag == FName(TEXT("WindArch"))) Action = TEXT("stir a brief local gust at the WindArch");
	else Action = TEXT("inspect ") + Tag.ToString();

	const TWeakObjectPtr<AActor> TargetKey(Target);
	if (const double* CooldownUntil = WorldInteractionCooldowns.Find(TargetKey);
		CooldownUntil && *CooldownUntil > FPlatformTime::Seconds())
	{
		Action = TEXT("let that response settle");
	}

	const FString Input = ShouldUseTouchControls() ? TEXT("Interact") : TEXT("E");
	return FString::Printf(TEXT("%s: %s"), *Input, *Action);
}

FString ACaptiveSky_2PlayerController::FormatConversationHint(const FString& AgentName, bool bTouchControls)
{
	if (bTouchControls || AgentName.IsEmpty()) return FString();
	return FString::Printf(TEXT("Enter: talk with %s"), *AgentName);
}

void ACaptiveSky_2PlayerController::UpdateWorldInteractionHint()
{
	if (!AmbientSpeechWidget) return;
	const UIslandSpectatorSubsystem* Spectator = GetWorld() ? GetWorld()->GetSubsystem<UIslandSpectatorSubsystem>() : nullptr;
	if (bGuestBookPanelOpen || bArrangementPanelOpen ||
		(ConversationWidget && ConversationWidget->IsVisible()) || (Spectator && Spectator->IsSpectating()))
	{
		AmbientSpeechWidget->HideInteractionHint();
		return;
	}
	FString Hint = DescribeWorldInteractionHint(FindNearestWorldInteraction());
	if (Hint.IsEmpty())
	{
		const UIslandEnvironmentSubsystem* Environment = GetWorld() ? GetWorld()->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
		if (!Environment || Environment->GetIndoors() < 0.5f)
		{
			const FString Input = ShouldUseTouchControls() ? TEXT("Interact") : TEXT("E");
			Hint = FString::Printf(TEXT("%s: notice your surroundings"), *Input);
		}
	}
	if (!ShouldUseTouchControls())
	{
		if (AAutonomousAgentCharacter* ConversationAgent = FindNearestConversationAgent())
		{
			const FString ConversationHint = FormatConversationHint(ConversationAgent->GetAgentDisplayName(), false);
			if (!ConversationHint.IsEmpty())
				Hint = Hint.IsEmpty() ? ConversationHint : Hint + LINE_TERMINATOR + ConversationHint;
		}
	}
	if (Hint.IsEmpty()) AmbientSpeechWidget->HideInteractionHint();
	else AmbientSpeechWidget->ShowInteractionHint(Hint);
}

void ACaptiveSky_2PlayerController::ShowWorldInteractionCaption(const FString& Caption)
{
	if (!AmbientSpeechWidget || !GetWorld()) return;
	AmbientSpeechWidget->ShowCaption(Caption);
	GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	GetWorldTimerManager().SetTimer(AmbientSpeechHideTimer, this,
		&ACaptiveSky_2PlayerController::HideAmbientSpeech, AmbientSpeechDurationSeconds, false);
}

void ACaptiveSky_2PlayerController::InteractWithNearestWorldObject()
{
	if (!IsLocalPlayerController() || (ConversationWidget && ConversationWidget->IsVisible()) || bGuestBookPanelOpen || bArrangementPanelOpen) return;
	AActor* Target = FindNearestWorldInteraction();
	if (!Target)
	{
		TArray<FString> EnvironmentalObservations;
		FString TrailFact = DescribeTrailUnderfoot();
		TrailFact.TrimStartAndEndInline();
		if (!TrailFact.IsEmpty())
		{
			EnvironmentalObservations.Add(FString::Printf(TEXT("You pause to read the ground. %s"), *TrailFact));
		}

		const FString WildlifeFact = DescribeNearbyWildlife();
		if (!WildlifeFact.IsEmpty()) EnvironmentalObservations.Add(WildlifeFact);

		if (const APawn* PlayerPawn = GetPawn())
		{
			const TArray<FIslandVegetationPatch> NearbyPlants = IslandVegetationUtility::FindNearbyVisiblePatches(
				PlayerPawn, 650.f, 3);
			const FString PlantFact = IslandVegetationUtility::FormatVisitorObservation(NearbyPlants, PlayerPawn);
			if (!PlantFact.IsEmpty()) EnvironmentalObservations.Add(PlantFact);
		}

		const FString WeatherFact = DescribeLocalWeather();
		if (!WeatherFact.IsEmpty())
		{
			EnvironmentalObservations.Add(FString::Printf(TEXT("You pause outdoors. %s"), *WeatherFact));
		}

		const FString WindShelterFact = DescribeLocalWindShelter();
		if (!WindShelterFact.IsEmpty()) EnvironmentalObservations.Add(WindShelterFact);

		const FString SkyFact = DescribeLocalSky();
		if (!SkyFact.IsEmpty()) EnvironmentalObservations.Add(SkyFact);

		const FString TideFact = DescribeNearbyTide();
		if (!TideFact.IsEmpty()) EnvironmentalObservations.Add(TideFact);

		if (!EnvironmentalObservations.IsEmpty())
		{
			const int32 ObservationIndex = NextEnvironmentalObservationIndex % EnvironmentalObservations.Num();
			NextEnvironmentalObservationIndex = (ObservationIndex + 1) % EnvironmentalObservations.Num();
			ShowWorldInteractionCaption(EnvironmentalObservations[ObservationIndex]);
			return;
		}
		ShowWorldInteractionCaption(TEXT("Nothing nearby catches your eye. Move closer to a landmark or wild creature and try E again."));
		return;
	}

	const FName TargetTag = IslandInteractionUtility::GetTargetTag(Target);
	if (TargetTag == FName(TEXT("GuestBook")))
	{
		OpenGuestBook(Target);
		return;
	}
	if (TargetTag == FName(TEXT("IslandArrangement")))
	{
		OpenArrangement(Target);
		return;
	}

	const double Now = FPlatformTime::Seconds();
	for (auto CooldownIt = WorldInteractionCooldowns.CreateIterator(); CooldownIt; ++CooldownIt)
		if (!CooldownIt.Key().IsValid() || CooldownIt.Value() <= Now) CooldownIt.RemoveCurrent();
	const TWeakObjectPtr<AActor> TargetKey(Target);
	if (const double* CooldownUntil = WorldInteractionCooldowns.Find(TargetKey); CooldownUntil && *CooldownUntil > Now)
	{
		ShowWorldInteractionCaption(FString::Printf(TEXT("%s has already answered your attention; let the moment settle."), *TargetTag.ToString()));
		return;
	}

	FString Fact;
	if (!IslandInteractionUtility::Perform(GetPawn(), Target, Fact))
	{
		ShowWorldInteractionCaption(TEXT("You cannot reach or clearly see that from here."));
		return;
	}
	WorldInteractionCooldowns.Add(TargetKey, Now + FMath::Max(30.f, IslandInteractionCooldownSeconds));
	UpdateWorldInteractionHint();
	ShowWorldInteractionCaption(TargetTag.ToString() + TEXT(": ") + Fact);
}

void ACaptiveSky_2PlayerController::OpenArrangement(AActor* Target)
{
	AIslandArrangement* WorkActor = Cast<AIslandArrangement>(Target);
	APawn* Visitor = GetPawn();
	UWorld* World = GetWorld();
	UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	const FIslandArrangementSite* Site = WorkActor && State ? State->FindArrangementSite(WorkActor->GetSiteId()) : nullptr;
	if (!ArrangementWidget || !IsValid(Visitor) || !WorkActor || !Site ||
		!IslandInteractionUtility::CanInteract(Visitor, WorkActor, IslandInteractionRadius))
	{
		ShowWorldInteractionCaption(TEXT("You cannot reach or clearly see that arranging ground."));
		return;
	}

	const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(World);
	const FString VisitorId(TEXT("Visitor"));
	const bool bUsedToday = State->HasArrangedStonesToday(VisitorId, Today);
	const bool bOwnWork = Site->bHasWork && Site->MakerAgentId == VisitorId;
	const bool bAlreadyResponded = Site->Responses.ContainsByPredicate([&VisitorId](const FIslandArrangementResponse& Response)
		{ return Response.AgentId == VisitorId; });
	const bool bCanCreate = !Site->bHasWork && !bUsedToday;
	const bool bCanRespond = Site->bHasWork && !bOwnWork && !bAlreadyResponded && !bUsedToday &&
		Site->Responses.Num() < AIslandArrangement::MaxResponses;
	FString Description;
	if (!Site->bHasWork)
	{
		Description = TEXT("This is an open patch of level ground beside the ListeningStones. Choose a simple shape and leave a few stones here.");
	}
	else
	{
		const bool bWeathered = Today - Site->Day >= AIslandArrangement::DaysToWeather;
		Description = FString::Printf(TEXT("A %s of stones rests here, %s, with %d small response(s) nearby."),
			*UIslandWorldStateSubsystem::FormName(Site->Form), bWeathered ? TEXT("mossy and settled") : TEXT("still taking shape"),
			Site->Responses.Num());
	}

	ArrangementTarget = WorkActor;
	ArrangementWidget->OpenFor(Site->Id, Description, Site->bHasWork, bCanCreate, bCanRespond, bOwnWork);
	SetArrangementInputMode(true);
	bArrangementPanelOpen = true;
	if (AmbientSpeechWidget) AmbientSpeechWidget->HideInteractionHint();
	GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	HideAmbientSpeech();
}

void ACaptiveSky_2PlayerController::SubmitVisitorArrangement(const FString& Form, const FString& Title, const FString& Intent)
{
	if (!ArrangementWidget || !bArrangementPanelOpen) return;
	APawn* Visitor = GetPawn();
	AIslandArrangement* Target = ArrangementTarget.Get();
	UWorld* World = GetWorld();
	UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	const FIslandArrangementSite* Site = State ? State->FindArrangementSite(ArrangementWidget->GetSiteId()) : nullptr;
	if (!IsValid(Visitor) || !IsValid(Target) || !Site || Target->GetSiteId() != Site->Id ||
		!IslandInteractionUtility::CanInteract(Visitor, Target, IslandInteractionRadius))
	{
		ArrangementWidget->ShowResult(TEXT("That arranging ground is no longer within reach and view; nothing was saved."),
			Site && Site->bHasWork, false, false, false);
		return;
	}

	bool bChanged = false;
	const FString Result = State->ArrangeStones(Site->Id, Form, Title, Intent, TEXT("Visitor"),
		UIslandWorldStateSubsystem::CurrentIslandDay(World), bChanged);
	Site = State->FindArrangementSite(Target->GetSiteId());
	const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(World);
	const bool bUsedToday = State->HasArrangedStonesToday(TEXT("Visitor"), Today);
	const bool bOwnWork = Site && Site->bHasWork && Site->MakerAgentId == TEXT("Visitor");
	const bool bAlreadyResponded = Site && Site->Responses.ContainsByPredicate([](const FIslandArrangementResponse& Response)
		{ return Response.AgentId == TEXT("Visitor"); });
	ArrangementWidget->ShowResult(Result, Site && Site->bHasWork,
		Site && !Site->bHasWork && !bUsedToday,
		Site && Site->bHasWork && !bOwnWork && !bAlreadyResponded && !bUsedToday && Site->Responses.Num() < AIslandArrangement::MaxResponses,
		bChanged);
}

void ACaptiveSky_2PlayerController::SubmitVisitorArrangementResponse(const FString& Intent)
{
	SubmitVisitorArrangement(FString(), FString(), Intent);
}

void ACaptiveSky_2PlayerController::CloseArrangement()
{
	if (ArrangementWidget) ArrangementWidget->SetVisibility(ESlateVisibility::Collapsed);
	bArrangementPanelOpen = false;
	ArrangementTarget.Reset();
	SetArrangementInputMode(false);
}

void ACaptiveSky_2PlayerController::SetArrangementInputMode(bool bOpen)
{
	if (bOpen && ArrangementWidget)
	{
		FInputModeGameAndUI InputMode;
		const TSharedPtr<SWidget> FocusWidget = ArrangementWidget->GetPreferredInputWidget();
		InputMode.SetWidgetToFocus(FocusWidget.IsValid() ? FocusWidget : ArrangementWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		return;
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ACaptiveSky_2PlayerController::OpenGuestBook(AActor* Target)
{
	APawn* Visitor = GetPawn();
	UWorld* World = GetWorld();
	UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!GuestBookWidget || !IsValid(Visitor) || !IsValid(Target) || !State ||
		!IslandInteractionUtility::CanInteract(Visitor, Target, IslandInteractionRadius))
	{
		ShowWorldInteractionCaption(TEXT("You cannot reach or clearly see the guest book from here."));
		return;
	}

	FString LatestEntries;
	if (!IslandInteractionUtility::Perform(Visitor, Target, LatestEntries))
	{
		ShowWorldInteractionCaption(TEXT("The inn guest book cannot be opened here. Nothing changed."));
		return;
	}

	GuestBookTarget = Cast<AIslandGuestBook>(Target);
	if (!GuestBookTarget.IsValid()) return;
	GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	HideAmbientSpeech();
	GuestBookWidget->OpenFor(LatestEntries, !State->HasGuestBookEntryToday(TEXT("Visitor")));
	bGuestBookPanelOpen = true;
	if (AmbientSpeechWidget) AmbientSpeechWidget->HideInteractionHint();
	SetGuestBookInputMode(true);
}

void ACaptiveSky_2PlayerController::SubmitGuestBookEntry(const FString& Line)
{
	if (!GuestBookWidget || !bGuestBookPanelOpen) return;
	APawn* Visitor = GetPawn();
	AIslandGuestBook* Book = GuestBookTarget.Get();
	UWorld* World = GetWorld();
	UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!IsValid(Visitor) || !IsValid(Book) || !State ||
		!IslandInteractionUtility::CanInteract(Visitor, Book, IslandInteractionRadius))
	{
		GuestBookWidget->ShowWriteResult(TEXT("The guest book is no longer within reach and view; nothing was recorded."),
			TEXT("Move back to the counter and open it again."), false, false);
		return;
	}

	bool bChanged = false;
	const FString Result = State->WriteVisitorGuestBook(Line, bChanged);
	FString LatestEntries;
	IslandInteractionUtility::Perform(Visitor, Book, LatestEntries);
	GuestBookWidget->ShowWriteResult(Result, LatestEntries,
		!State->HasGuestBookEntryToday(TEXT("Visitor")), bChanged);
}

void ACaptiveSky_2PlayerController::CloseGuestBook()
{
	if (GuestBookWidget) GuestBookWidget->SetVisibility(ESlateVisibility::Collapsed);
	bGuestBookPanelOpen = false;
	GuestBookTarget.Reset();
	SetGuestBookInputMode(false);
}

void ACaptiveSky_2PlayerController::SetGuestBookInputMode(bool bOpen)
{
	if (bOpen && GuestBookWidget)
	{
		FInputModeGameAndUI InputMode;
		const TSharedPtr<SWidget> FocusWidget = GuestBookWidget->GetPreferredInputWidget();
		InputMode.SetWidgetToFocus(FocusWidget.IsValid() ? FocusWidget : GuestBookWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		return;
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ACaptiveSky_2PlayerController::HandleEscape()
{
	if (bGuestBookPanelOpen)
	{
		CloseGuestBook();
		return;
	}
	if (bArrangementPanelOpen)
	{
		CloseArrangement();
		return;
	}
	CloseConversation();
}

AAutonomousAgentCharacter* ACaptiveSky_2PlayerController::FindNearestConversationAgent() const
{
	const APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn) return nullptr;
	AAutonomousAgentCharacter* Nearest = nullptr;
	float BestDistanceSquared = FMath::Square(ConversationRadius);
	for (TActorIterator<AAutonomousAgentCharacter> It(GetWorld()); It; ++It)
	{
		const float DistanceSquared = FVector::DistSquared(PlayerPawn->GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			Nearest = *It;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return Nearest;
}

void ACaptiveSky_2PlayerController::ToggleConversation()
{
	if (!ConversationWidget || ConversationWidget->IsVisible() || bGuestBookPanelOpen || bArrangementPanelOpen) return;
	ConversationTarget = FindNearestConversationAgent();
	if (!ConversationTarget)
	{
		if (AmbientSpeechWidget) AmbientSpeechWidget->HideInteractionHint();
		ConversationWidget->SetVisibility(ESlateVisibility::Visible);
		ConversationWidget->ShowMessage(TEXT("No one is close enough to hear you."), false);
		FTimerHandle HideHandle;
		GetWorldTimerManager().SetTimer(HideHandle, this, &ACaptiveSky_2PlayerController::CloseConversation, 2.f, false);
		return;
	}
	ConversationTargetDisplayName = ConversationTarget->GetAgentDisplayName();
	ConversationWidget->OpenFor(ConversationTargetDisplayName);
	if (AmbientSpeechWidget) AmbientSpeechWidget->HideInteractionHint();
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ConversationWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ACaptiveSky_2PlayerController::SubmitConversation(const FString& Utterance)
{
	const FString Clean = Utterance.TrimStartAndEnd();
	if (Clean.IsEmpty() || !ConversationWidget || !ConversationTarget || !ConversationTarget->Brain || !GetPawn()) return;
	if (FVector::DistSquared(GetPawn()->GetActorLocation(), ConversationTarget->GetActorLocation()) > FMath::Square(ConversationRadius))
	{
		ConversationWidget->ShowMessage(TEXT("They are no longer close enough to hear you."), true);
		return;
	}
	if (ConversationTarget->Brain->bRequestInFlight)
	{
		ConversationWidget->ShowMessage(FString::Printf(TEXT("%s is still thinking."), *ConversationTargetDisplayName), true);
		return;
	}
	ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationTarget->Brain->OnDecisionReady.AddDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationWidget->ShowMessage(FString::Printf(TEXT("%s is thinking…"), *ConversationTargetDisplayName), false);
	ConversationTarget->Brain->RequestDecision(Clean);
}

void ACaptiveSky_2PlayerController::HandleConversationDecision(const FAgentDecision& Decision)
{
	if (ConversationTarget && ConversationTarget->Brain)
		ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	if (!ConversationWidget) return;
	if (!Decision.bValid)
	{
		ConversationWidget->ShowMessage(FString::Printf(TEXT("%s could not gather their thoughts."), *ConversationTargetDisplayName), true);
		return;
	}
	ConversationWidget->ShowMessage(Decision.Speech.IsEmpty()
		? FString::Printf(TEXT("%s considers what you said, but does not answer aloud."), *ConversationTargetDisplayName)
		: FString::Printf(TEXT("%s: %s"), *ConversationTargetDisplayName, *Decision.Speech), true);
}

void ACaptiveSky_2PlayerController::CloseConversation()
{
	if (ConversationTarget && ConversationTarget->Brain)
		ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationTarget = nullptr;
	ConversationTargetDisplayName.Reset();
	if (ConversationWidget) ConversationWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

bool ACaptiveSky_2PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
