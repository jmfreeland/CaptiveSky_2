// Copyright Epic Games, Inc. All Rights Reserved.

#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandDayNight.h"
#include "IslandPoolRippleEffect.h"
#include "IslandListeningStonesChime.h"
#include "IslandFirefly.h"
#include "IslandTidepoolCrab.h"
#include "IslandWindMoteEffect.h"
#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "Navigation/PathFollowingComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogAutonomousAgentAI, Log, All);

AAutonomousAgentAIController::AAutonomousAgentAIController()
{
	// Necessary for EnvQueries to work correctly, matching ACombatAIController's setup.
	bAttachToPawn = true;
}

void AAutonomousAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AAutonomousAgentCharacter* Agent = Cast<AAutonomousAgentCharacter>(InPawn))
	{
		if (Agent->Brain)
		{
			Agent->Brain->OnDecisionReady.AddDynamic(this, &AAutonomousAgentAIController::HandleDecisionReady);
		}
	}

	NextThinkAt = FPlatformTime::Seconds() + 2;
	NextRestAt = FPlatformTime::Seconds() + 600;
	AutonomousRequests = RepeatedActions = 0;
	LastActionKey.Empty();
	InspectedUntil.Reset();
	GetWorldTimerManager().SetTimer(ThinkTimerHandle, this, &AAutonomousAgentAIController::Think, 1.f, true, 2.f);
}

void AAutonomousAgentAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimerHandle);

	if (AAutonomousAgentCharacter* Agent = Cast<AAutonomousAgentCharacter>(GetPawn()))
	{
		if (Agent->Brain)
		{
			Agent->Brain->OnDecisionReady.RemoveDynamic(this, &AAutonomousAgentAIController::HandleDecisionReady);
		}
	}

	Super::OnUnPossess();
}

void AAutonomousAgentAIController::Think()
{
	AAutonomousAgentCharacter* Agent = Cast<AAutonomousAgentCharacter>(GetPawn());
	if (!Agent || !Agent->Brain || Agent->Brain->bRequestInFlight || IsResting() || IsActionInProgress())
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (Now >= NextRestAt && CanRest())
	{
		for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
			if ((It->CurrentHour >= 20 || It->CurrentHour < 5) && TryRest()) return;
	}
	if (Now < NextThinkAt || AutonomousRequests >= 30) return;
	++AutonomousRequests;
	NextThinkAt = Now + BackgroundDelay(RepeatedActions, ThinkIntervalSeconds);
	Agent->Brain->RequestDecision(FString());
}

double AAutonomousAgentAIController::BackgroundDelay(int32 Repeats, double BaseSeconds)
{
	return FMath::Clamp(FMath::Max(60.0, BaseSeconds) * FMath::Pow(2.0, FMath::Clamp(Repeats - 1, 0, 3)), 60.0, 300.0);
}

bool AAutonomousAgentAIController::IsActionInProgress() const { return GetMoveStatus() == EPathFollowingStatus::Moving; }
bool AAutonomousAgentAIController::CanRest() const
{
	const ACharacter* Body = Cast<ACharacter>(GetPawn());
	return Body && Body->GetCharacterMovement()->IsMovingOnGround() && !IsActionInProgress();
}
bool AAutonomousAgentAIController::IsResting() const
{
	const UAgentConsolidationComponent* Rest = GetPawn() ? GetPawn()->FindComponentByClass<UAgentConsolidationComponent>() : nullptr;
	return Rest && !Rest->IsAwake();
}
bool AAutonomousAgentAIController::TryRest()
{
	UAgentConsolidationComponent* Rest = GetPawn() ? GetPawn()->FindComponentByClass<UAgentConsolidationComponent>() : nullptr;
	if (!CanRest() || !Rest || !Rest->BeginSleep(120.f)) return false;
	StopMovement();
	NextRestAt = FPlatformTime::Seconds() + 900;
	NextThinkAt = FPlatformTime::Seconds() + 180;
	RepeatedActions = 0;
	ReportAction(TEXT("Settled safely to rest. Ordinary thoughts pause during sleep; new lived memories may be consolidated."));
	return true;
}
void AAutonomousAgentAIController::ReportAction(const FString& Outcome)
{
	LastActionOutcome = Outcome;
	UE_LOG(LogAutonomousAgentAI, Log, TEXT("%s outcome: %s"), *GetName(), *Outcome);
}
FString AAutonomousAgentAIController::DescribeActionState() const
{
	FString Result = TEXT(" Last physical action result: ") + LastActionOutcome;
	Result += IsActionInProgress() ? TEXT(" Movement is still in progress.") : TEXT(" No movement is currently in progress.");
	for (const auto& Entry : InspectedUntil)
		if (Entry.Value > FPlatformTime::Seconds()) Result += FString::Printf(TEXT(" %s was already inspected; no new interaction is available for %.0f more real seconds."), *Entry.Key.ToString(), Entry.Value - FPlatformTime::Seconds());
	if (RepeatedActions >= 2) Result += TEXT(" You have repeated this choice without new progress. Rest, wait quietly, speak, or choose something different; do not invent a discovery.");
	return Result;
}
void AAutonomousAgentAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	ReportAction(Result.IsSuccess() ? TEXT("Reached the requested destination. Arrival is complete; it does not imply an interaction or a discovery.") : TEXT("Movement did not complete (blocked, cancelled, or unreachable). Choose a reachable destination instead of repeating this route."));
}
void AAutonomousAgentAIController::InspectTarget(FName Target)
{
	if (Target.IsNone()) { ReportAction(TEXT("Inspection failed: no target supplied.")); return; }
	APawn* Observer = GetPawn();
	if (!Observer) { ReportAction(TEXT("Inspection failed: no character is currently under your control.")); return; }
	if (const double* Until = InspectedUntil.Find(Target); Until && *Until > FPlatformTime::Seconds())
	{
		ReportAction(TEXT("Already inspected that place recently; no additional interaction is available yet."));
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(Target)) continue;
		if (It->IsHidden() && (Target == FName(TEXT("Firefly")) || Target == FName(TEXT("TidepoolCrab")))) continue;
		if ((Target == FName(TEXT("Firefly")) || Target == FName(TEXT("TidepoolCrab"))) &&
			FVector::DistSquared(It->GetActorLocation(), Observer->GetActorLocation()) > FMath::Square(400.f)) continue;
		// Ecology habitat markers can share a name with the interactable pool but are not themselves landmarks.
		// Skip them before distance checks so an unrelated habitat cannot mask the actual TideglassPool landmark.
		if (Target == FName(TEXT("TideglassPool")) && !It->ActorHasTag(TEXT("IslandLandmark"))) continue;
		if (FVector::DistSquared(It->GetActorLocation(), Observer->GetActorLocation()) > FMath::Square(400.f)) { ReportAction(TEXT("Too far away to inspect; move within four metres first.")); return; }
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(AgentInspect), false, Observer);
		Query.AddIgnoredActor(*It);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Observer->GetActorLocation(), It->GetActorLocation(), ECC_Visibility, Query)) { ReportAction(TEXT("The inspection point is occluded; find a clear approach.")); return; }
		FString Fact;
		if (It->ActorHasTag(TEXT("RavenNestSite")))
		{
			if (const ARavenAgentAIController* RavenController = Cast<ARavenAgentAIController>(this))
				Fact = RavenController->AssessRoostSite(*It);
			else
				Fact = TEXT("You inspected a candidate bird roost, but this body cannot perch there; no movement or persistent change was made.");
			for (TActorIterator<AIslandWeather> WeatherIt(GetWorld()); WeatherIt; ++WeatherIt)
			{
				Fact += TEXT(" ") + WeatherIt->DescribeWindShelterAt(It->GetActorLocation(), Observer);
				if (WeatherIt->SampleRainIntensity(GetWorld()->GetTimeSeconds()) >= 0.55f)
					Fact += TEXT(" A stronger shower is currently passing; the geometry probes do not guarantee that this site stays dry.");
				break;
			}
			Fact += TEXT(" No nest, ownership, or assigned home has been created.");
		}
		else if (It->ActorHasTag(TEXT("IslandCurio")))
		{
			UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
			const UAgentMemoryComponent* Memory = Observer->FindComponentByClass<UAgentMemoryComponent>();
			const FString ContributorAgentId = Memory ? Memory->GetResolvedAgentId() : Observer->GetName();
			Fact = WorldState ? WorldState->ExamineCurio(Target, UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()), ContributorAgentId) : TEXT("You examined it, but nothing about it can change here.");
		}
		else if (It->ActorHasTag(TEXT("IslandLife")) && Target == FName(TEXT("Firefly")))
		{
			if (AIslandFirefly* Firefly = Cast<AIslandFirefly>(*It)) Firefly->RespondToQuietObservation();
			Fact = TEXT("You quietly watched a nearby firefly. Its glow briefly brightened within its ordinary pulse; it remains wild and independent. You did not touch, catch, or claim it, and it may drift away.");
		}
		else if (It->ActorHasTag(TEXT("IslandLife")) && Target == FName(TEXT("TidepoolCrab")))
		{
			if (AIslandTidepoolCrab* Crab = Cast<AIslandTidepoolCrab>(*It)) Crab->RespondToQuietObservation(Observer->GetActorLocation());
			Fact = TEXT("You quietly watched a small shore crab. It scuttled a short way toward cover, paused, then resumed its usual Tideglass path. It remains wild and independent; you did not touch, catch, or claim it, and nothing persistent changed.");
		}
		else if (It->ActorHasTag(TEXT("IslandLandmark")) && Target == FName(TEXT("ListeningStones")))
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AIslandListeningStonesChime* Chime = GetWorld()->SpawnActor<AIslandListeningStonesChime>(It->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
			if (Chime) Chime->BeginChime();
			Fact = Chime
				? TEXT("Your inspection woke a quiet, layered resonance in the ListeningStones. It rings softly nearby and fades within a few seconds; the sound is synthesized locally, reveals nothing, and leaves no lasting change.")
				: TEXT("You inspected the ListeningStones, but their short-lived resonance could not be created. No persistent change occurred.");
		}
		else if (It->ActorHasTag(TEXT("IslandLandmark")) && Target == FName(TEXT("WindArch")))
		{
			bool bWindResponded = false;
			bool bVisibleMotesCreated = false;
			for (TActorIterator<AIslandWeather> WeatherIt(GetWorld()); WeatherIt; ++WeatherIt)
			{
				const FVector ExistingWind = WeatherIt->GetLocalWind(It->GetActorLocation(), *It);
				const FVector GustDirection = ExistingWind.IsNearlyZero() ? It->GetActorForwardVector() : ExistingWind.GetSafeNormal();
				WeatherIt->AddTransientGust(It->GetActorLocation(), GustDirection, 220.f, 1400.f, 18.f);
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (AIslandWindMoteEffect* Motes = GetWorld()->SpawnActor<AIslandWindMoteEffect>(It->GetActorLocation() + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, SpawnParameters))
				{
					Motes->InitializeGust(GustDirection, 1400.f, 18.f);
					bVisibleMotesCreated = true;
				}
				bWindResponded = true;
				break;
			}
			Fact = bWindResponded && bVisibleMotesCreated
				? TEXT("Your interaction with the WindArch created a short-lived gust in the simulated local wind. Three small illuminated motes briefly trace its changing airflow; both effects fade over eighteen seconds of Island time and across fourteen metres. Nearby residents can sense the changed wind, and the raven's flight responds to it. No sound is implemented, and no lasting weather change occurred.")
				: bWindResponded
				? TEXT("Your interaction with the WindArch created a short-lived gust in the simulated local wind. It fades over eighteen seconds of Island time and across fourteen metres. Nearby residents can sense the changed wind, and the raven's flight responds to it, but its temporary visual cue could not be created. No lasting weather change occurred.")
				: TEXT("You inspected the WindArch, but no IslandWeather actor is active, so no gust was created. This landmark has no visible wind effect or sound yet.");
		}
		else if (It->ActorHasTag(TEXT("IslandLandmark")) && Target == FName(TEXT("TideglassPool")))
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector SurfaceLocation = It->GetActorLocation() + FVector(0.f, 0.f, 20.f);
			AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(SurfaceLocation, FRotator::ZeroRotator, SpawnParameters);
			Fact = Ripple
				? TEXT("Your interaction sent a short ring of cool highlights across the flattened TideglassPool prototype surface. It expands over about one and a half seconds and fades; it changes no permanent level state, and reveals no hidden item or reward. Separately, stronger showers can create fainter ripples on their own; those are a weather response, not an interaction you caused.")
				: TEXT("You inspected the TideglassPool, but the temporary surface-light response could not be created. No persistent change occurred.");
		}
		else if (It->ActorHasTag(TEXT("IslandLandmark"))) Fact = TEXT("You inspected a visible Island landmark. It is currently static prototype scenery: no hidden item, puzzle response, sound, or other interactive effect is implemented. Inspection is complete; returning immediately provides no new result.");
		else { ReportAction(TEXT("This target has no implemented inspection interaction.")); return; }
		InspectedUntil.Add(Target, FPlatformTime::Seconds() + 300);
		ReportAction(Target.ToString() + TEXT(": ") + Fact);
		if (UAgentMemoryComponent* Memory = Observer->FindComponentByClass<UAgentMemoryComponent>())
			Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Observation, Target.ToString() + TEXT(": ") + Fact, 0.45f, {TEXT("action-result"), Target.ToString()}));
		return;
	}
	if (Target == FName(TEXT("Firefly"))) ReportAction(TEXT("No firefly is close enough to watch quietly; move within four metres and let it come near."));
	else if (Target == FName(TEXT("TidepoolCrab"))) ReportAction(TEXT("No shore crab is close enough to watch quietly; return to the TideglassPool and look near its edge."));
	else ReportAction(TEXT("Inspection failed: that target does not exist in this level."));
}

bool AAutonomousAgentAIController::CanArrangeStones() const
{
	const ACharacter* Body = Cast<ACharacter>(GetPawn());
	return Body && Body->GetCharacterMovement()->IsMovingOnGround() && !IsActionInProgress();
}

void AAutonomousAgentAIController::ArrangeStones(const FAgentDecision& Decision)
{
	APawn* Body = GetPawn();
	UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	const FName SiteId(*Decision.ActionTarget);
	const FIslandArrangementSite* Site = WorldState ? WorldState->FindArrangementSite(SiteId) : nullptr;
	if (!Body || !Site) { ReportAction(TEXT("There is no arranging ground by that name here. Nothing changed.")); return; }
	if (FVector::Dist2D(Body->GetActorLocation(), Site->Location) > 300.f || FMath::Abs(Body->GetActorLocation().Z - Site->Location.Z) > 250.f)
	{
		ReportAction(TEXT("You are too far from that arranging ground; move_to it and stand within three metres first. Nothing changed."));
		return;
	}
	if (!CanArrangeStones()) { ReportAction(TEXT("Stones can only be arranged while settled on the ground. Nothing changed.")); return; }
	UAgentMemoryComponent* Memory = Body->FindComponentByClass<UAgentMemoryComponent>();
	bool bChanged = false;
	const FString Fact = WorldState->ArrangeStones(SiteId, Decision.Form, Decision.Title, Decision.Intent,
		Memory ? Memory->GetResolvedAgentId() : Body->GetName(), UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()), bChanged);
	ReportAction(SiteId.ToString() + TEXT(": ") + Fact);
	if (bChanged && Memory)
		Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Observation, SiteId.ToString() + TEXT(": ") + Fact, 0.65f, {TEXT("action-result"), TEXT("arrangement"), SiteId.ToString()}));
}

void AAutonomousAgentAIController::HandleDecisionReady(const FAgentDecision& Decision)
{
	if (!Decision.bValid)
	{
		UE_LOG(LogAutonomousAgentAI, Warning, TEXT("%s: brain returned an invalid decision."), *GetName());
		return;
	}
	if (IsResting()) return;
	const UAgentBrainComponent* Brain = GetPawn() ? GetPawn()->FindComponentByClass<UAgentBrainComponent>() : nullptr;
	if (!Brain || Brain->LastConversationContext.Text.IsEmpty())
	{
		const FString Key = FString::FromInt(static_cast<int32>(Decision.ActionType)) + TEXT(":") + Decision.ActionTarget.ToLower();
		// Different speech and random wandering are not identical failed actions.
		const bool bRepeatSensitive = Decision.ActionType == EAgentActionType::MoveTo || Decision.ActionType == EAgentActionType::Interact || Decision.ActionType == EAgentActionType::Idle || Decision.ActionType == EAgentActionType::Build;
		RepeatedActions = bRepeatSensitive ? (Key == LastActionKey ? RepeatedActions + 1 : 1) : 0;
		LastActionKey = Key;
		NextThinkAt = FPlatformTime::Seconds() + BackgroundDelay(RepeatedActions, ThinkIntervalSeconds);
		if (RepeatedActions >= 3 && Decision.ActionType != EAgentActionType::Idle)
		{
			ReportAction(TEXT("Repeated action suppressed: it produced no new choice or progress. Background thinking is cooling down; choose a different activity or rest."));
			return;
		}
	}

	UE_LOG(LogAutonomousAgentAI, Log, TEXT("%s decided: \"%s\" (Action=%d)"), *GetName(), *Decision.Thought, static_cast<int32>(Decision.ActionType));

	ActOnDecision(Decision);
}

void AAutonomousAgentAIController::ActOnDecision(const FAgentDecision& Decision)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	switch (Decision.ActionType)
	{
	case EAgentActionType::Wander:
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation Result;
			if (NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), WanderRadius, Result))
			{
				const EPathFollowingRequestResult::Type Request = MoveToLocation(Result.Location);
				ReportAction(Request == EPathFollowingRequestResult::Failed ? TEXT("Wandering failed: no navigable route.") : Request == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("Already at the wandering destination; waiting quietly.") : TEXT("Wandering movement started; arrival is not yet complete."));
				break;
			}
		}
		ReportAction(TEXT("Wandering failed: no reachable ground was found nearby."));
		break;
	}
	case EAgentActionType::MoveTo:
	{
		const FName TargetTag(*Decision.ActionTarget);
		AActor* TargetActor = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TargetTag))
			{
				TargetActor = *It;
				break;
			}
		}

		if (TargetActor)
		{
			// Shared landmarks can be elevated bird targets. Grounded bodies need a
			// nearby walkable goal, not the airborne marker or a partial-path endpoint.
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			FNavLocation GroundGoal;
			if (!NavSys || !NavSys->ProjectPointToNavigation(TargetActor->GetActorLocation(), GroundGoal, FVector(250, 250, 1000), &ControlledPawn->GetNavAgentPropertiesRef()))
			{
				ReportAction(TEXT("Movement failed: no walkable ground near that marker. Choose another destination."));
				break;
			}
			const EPathFollowingRequestResult::Type Result = MoveToLocation(GroundGoal.Location, 50.f, true, true, false, true, nullptr, false);
			ReportAction(Result == EPathFollowingRequestResult::Failed ? TEXT("Movement failed: no navigable route to that target.") : Result == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("Already at this destination. Do not keep requesting arrival; inspect once, wait, or rest.") : TEXT("Movement started; arrival is not yet complete."));
		}
		else
		{
			UE_LOG(LogAutonomousAgentAI, Warning, TEXT("%s: MoveTo target tag '%s' not found in level."), *GetName(), *Decision.ActionTarget);
			ReportAction(TEXT("Movement failed: target tag was not found in this level."));
		}
		break;
	}
	case EAgentActionType::Speak:
		UE_LOG(LogAutonomousAgentAI, Log, TEXT("%s says: \"%s\""), *GetName(), *Decision.Speech);
		break;
	case EAgentActionType::Interact:
		InspectTarget(FName(*Decision.ActionTarget));
		break;
	case EAgentActionType::Build:
		if (Decision.ActionTarget.StartsWith(TEXT("ArrangingGround"))) ArrangeStones(Decision);
		else ReportAction(TEXT("This body has no way to build anything yet; nothing changed."));
		break;
	case EAgentActionType::Sleep:
		if (!TryRest()) ReportAction(TEXT("Cannot sleep here yet: finish moving and settle on the ground or a solid perch first."));
		break;
	case EAgentActionType::Idle:
	default:
		StopMovement();
		ReportAction(TEXT("Waiting quietly. No new action or discovery occurred."));
		break;
	}
}
