// Copyright Epic Games, Inc. All Rights Reserved.

#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "EngineUtils.h"

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

	GetWorldTimerManager().SetTimer(ThinkTimerHandle, this, &AAutonomousAgentAIController::Think, ThinkIntervalSeconds, true, 2.f);
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
	if (!Agent || !Agent->Brain || Agent->Brain->bRequestInFlight)
	{
		return;
	}

	Agent->Brain->RequestDecision(FString());
}

void AAutonomousAgentAIController::HandleDecisionReady(const FAgentDecision& Decision)
{
	if (!Decision.bValid)
	{
		UE_LOG(LogAutonomousAgentAI, Warning, TEXT("%s: brain returned an invalid decision."), *GetName());
		return;
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
				MoveToLocation(Result.Location);
			}
		}
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
			MoveToActor(TargetActor);
		}
		else
		{
			UE_LOG(LogAutonomousAgentAI, Warning, TEXT("%s: MoveTo target tag '%s' not found in level."), *GetName(), *Decision.ActionTarget);
		}
		break;
	}
	case EAgentActionType::Speak:
		UE_LOG(LogAutonomousAgentAI, Log, TEXT("%s says: \"%s\""), *GetName(), *Decision.Speech);
		break;
	case EAgentActionType::Interact:
		UE_LOG(LogAutonomousAgentAI, Warning, TEXT("%s: Interact with '%s' requested but not implemented yet."), *GetName(), *Decision.ActionTarget);
		break;
	case EAgentActionType::Idle:
	default:
		StopMovement();
		break;
	}
}
