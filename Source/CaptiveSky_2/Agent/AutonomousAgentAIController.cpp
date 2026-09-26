// Copyright Epic Games, Inc. All Rights Reserved.

#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
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
	if (const double* Until = InspectedUntil.Find(Target); Until && *Until > FPlatformTime::Seconds())
	{
		ReportAction(TEXT("Already inspected that place recently; no additional interaction is available yet."));
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(Target)) continue;
		if (FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation()) > FMath::Square(400.f)) { ReportAction(TEXT("Too far away to inspect; move within four metres first.")); return; }
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(AgentInspect), false, GetPawn());
		Query.AddIgnoredActor(*It);
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetPawn()->GetActorLocation(), It->GetActorLocation(), ECC_Visibility, Query)) { ReportAction(TEXT("The inspection point is occluded; find a clear approach.")); return; }
		FString Fact;
		if (It->ActorHasTag(TEXT("RavenNestSite"))) Fact = TEXT("You inspected a candidate resting site. This visual inspection alone does not prove support or shelter: a successful perch result confirms support, and shelter varies with solid geometry and wind direction. No nest, ownership, or assigned home has been created.");
		else if (It->ActorHasTag(TEXT("IslandLandmark")) && Target == FName(TEXT("WindArch")))
		{
			bool bWindResponded = false;
			for (TActorIterator<AIslandWeather> WeatherIt(GetWorld()); WeatherIt; ++WeatherIt)
			{
				const FVector ExistingWind = WeatherIt->GetLocalWind(It->GetActorLocation(), *It);
				const FVector GustDirection = ExistingWind.IsNearlyZero() ? It->GetActorForwardVector() : ExistingWind.GetSafeNormal();
				WeatherIt->AddTransientGust(It->GetActorLocation(), GustDirection, 220.f, 1400.f, 18.f);
				bWindResponded = true;
				break;
			}
			Fact = bWindResponded
				? TEXT("Your interaction with the WindArch created a short-lived gust in the simulated local wind. It fades over eighteen seconds of Island time and across fourteen metres. Nearby residents can sense the changed wind, and the raven's flight responds to it. No sound or visible wind effect is implemented yet.")
				: TEXT("You inspected the WindArch, but no IslandWeather actor is active, so no gust was created. This landmark has no visible wind effect or sound yet.");
		}
		else if (It->ActorHasTag(TEXT("IslandLandmark"))) Fact = TEXT("You inspected a visible Island landmark. It is currently static prototype scenery: no hidden item, puzzle response, sound, or other interactive effect is implemented. Inspection is complete; returning immediately provides no new result.");
		else { ReportAction(TEXT("This target has no implemented inspection interaction.")); return; }
		InspectedUntil.Add(Target, FPlatformTime::Seconds() + 300);
		ReportAction(Target.ToString() + TEXT(": ") + Fact);
		if (UAgentMemoryComponent* Memory = GetPawn()->FindComponentByClass<UAgentMemoryComponent>())
			Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Observation, Target.ToString() + TEXT(": ") + Fact, 0.45f, {TEXT("action-result"), Target.ToString()}));
		return;
	}
	ReportAction(TEXT("Inspection failed: that target does not exist in this level."));
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
		const bool bRepeatSensitive = Decision.ActionType == EAgentActionType::MoveTo || Decision.ActionType == EAgentActionType::Interact || Decision.ActionType == EAgentActionType::Idle;
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
