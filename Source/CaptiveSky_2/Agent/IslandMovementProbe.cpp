#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "RavenAgentAIController.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandMovementProbe, Log, All);

class FIslandMovementProbeCommand
{
public:
	static void Run(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Island.MoveProbe requires a running game world."));
			return;
		}

		const TSharedRef<FProbeState> State = MakeShared<FProbeState>();
		State->World = World;
		State->MoverTag = FName(Args.IsEmpty() ? TEXT("IslandInnkeeper") : *Args[0]);
		State->bApproachProbe = Args.Num() > 1 && Args[1].Equals(TEXT("Approach"), ESearchCase::IgnoreCase);
		State->TargetTag = State->bApproachProbe
			? FName(Args.Num() > 2 ? *Args[2] : TEXT("ApproachAgent_Raven_01"))
			: FName(Args.Num() > 1 ? *Args[1] : TEXT("InnDoorLantern"));
		State->PerchTag = State->bApproachProbe ? FName(Args.Num() > 3 ? *Args[3] : TEXT("Roost_East")) : NAME_None;
		if (State->bApproachProbe && Args.Num() > 6)
		{
			double StartX = 0.0;
			double StartY = 0.0;
			double StartZ = 0.0;
			if (!LexTryParseString(StartX, *Args[4]) || !LexTryParseString(StartY, *Args[5]) || !LexTryParseString(StartZ, *Args[6]))
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Approach start override needs three numeric world coordinates."));
				return;
			}
			State->bHasMoverStartOverride = true;
			State->MoverStartOverride = FVector(StartX, StartY, StartZ);
		}
		State->bWanderProbe = State->TargetTag == FName(TEXT("Wander"));
		World->GetTimerManager().SetTimer(State->PollTimer,
			FTimerDelegate::CreateLambda([State]() { Poll(State); }), 0.5f, true);
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Queued isolated %s probe for %s; waiting for runtime actors."),
			State->bWanderProbe ? TEXT("wander") : TEXT("movement"), *State->MoverTag.ToString());
	}

private:
	struct FProbeState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AAutonomousAgentAIController> Controller;
		TWeakObjectPtr<APawn> Pawn;
		FName MoverTag;
		FName TargetTag;
		FName PerchTag;
		FTimerHandle PollTimer;
		FVector StartLocation = FVector::ZeroVector;
		double StartedAt = 0.0;
		double PerchRequestedAt = 0.0;
		double GroundingStartedAt = 0.0;
		int32 StartupPolls = 0;
		bool bMoveStarted = false;
		bool bWaitingForGround = false;
		bool bWanderProbe = false;
		bool bApproachProbe = false;
		bool bPerchRequested = false;
		bool bHasMoverStartOverride = false;
		FVector MoverStartOverride = FVector::ZeroVector;
	};

	static void Poll(const TSharedRef<FProbeState>& State)
	{
		UWorld* World = State->World.Get();
		if (!World) return;

		if (!State->bMoveStarted)
		{
			AActor* Mover = nullptr;
			AActor* Target = nullptr;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!Mover && It->ActorHasTag(State->MoverTag)) Mover = *It;
				if (!Mover)
				{
					const AAutonomousAgentCharacter* Resident = Cast<AAutonomousAgentCharacter>(*It);
					if (Resident && Resident->Memory &&
						Resident->Memory->GetResolvedAgentId().Equals(State->MoverTag.ToString(), ESearchCase::IgnoreCase))
					{
						Mover = *It;
					}
				}
				if (!State->bWanderProbe && !Target && It->ActorHasTag(State->TargetTag)) Target = *It;
			}
			APawn* Pawn = Cast<APawn>(Mover);
			AAutonomousAgentAIController* Controller = Pawn ? Cast<AAutonomousAgentAIController>(Pawn->GetController()) : nullptr;
			if (!Pawn || !Controller || (!State->bWanderProbe && !Target))
			{
				if (++State->StartupPolls <= 40) return;
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Timed out finding mover/controller/target (mover=%s controller=%s target=%s)."),
					Pawn ? TEXT("yes") : TEXT("no"), Controller ? TEXT("yes") : TEXT("no"),
					(State->bWanderProbe || Target) ? TEXT("yes") : TEXT("no"));
				Finish(State, false);
				return;
			}
			if (State->bWanderProbe && !Cast<ARavenAgentAIController>(Controller))
			{
				const ACharacter* Character = Cast<ACharacter>(Pawn);
				const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
				if (Movement && !Movement->IsMovingOnGround())
				{
					if (!State->bWaitingForGround)
					{
						State->bWaitingForGround = true;
						State->GroundingStartedAt = World->GetTimeSeconds();
						UE_LOG(LogIslandMovementProbe, Log,
							TEXT("Wander probe is waiting for the grounded resident to land (location=%s mode=%d)."),
							*Pawn->GetActorLocation().ToCompactString(), static_cast<int32>(Movement->MovementMode));
					}
					if (World->GetTimeSeconds() - State->GroundingStartedAt < 12.0) return;
					UE_LOG(LogIslandMovementProbe, Warning,
						TEXT("Wander probe timed out before its grounded resident landed (location=%s mode=%d)."),
						*Pawn->GetActorLocation().ToCompactString(), static_cast<int32>(Movement->MovementMode));
					Finish(State, false);
					return;
				}
				if (State->bWaitingForGround)
				{
					UE_LOG(LogIslandMovementProbe, Log, TEXT("Grounded resident landed at %s; issuing wander now."),
						*Pawn->GetActorLocation().ToCompactString());
				}
			}
			if (State->bApproachProbe)
			{
				const AAutonomousAgentCharacter* TargetResident = Cast<AAutonomousAgentCharacter>(Target);
				ARavenAgentAIController* RavenController = TargetResident
					? Cast<ARavenAgentAIController>(TargetResident->GetController()) : nullptr;
				if (!RavenController)
				{
					UE_LOG(LogIslandMovementProbe, Error,
						TEXT("Approach probe target %s is not an embodied raven with a raven controller."), *State->TargetTag.ToString());
					Finish(State, false);
					return;
				}
				if (RavenController->LocomotionState != ERavenLocomotionState::Perched)
				{
					if (!State->bPerchRequested)
					{
						State->PerchRequestedAt = World->GetTimeSeconds();
						State->bPerchRequested = RavenController->RequestPerch(State->PerchTag);
						if (!State->bPerchRequested)
						{
							UE_LOG(LogIslandMovementProbe, Error,
								TEXT("Approach probe could not start the raven's perch request for %s."), *State->PerchTag.ToString());
							Finish(State, false);
							return;
						}
						UE_LOG(LogIslandMovementProbe, Log,
							TEXT("Approach probe is preparing the raven at %s before issuing resident movement."), *State->PerchTag.ToString());
					}
					else if (World->GetTimeSeconds() - State->PerchRequestedAt >= 45.0)
					{
						UE_LOG(LogIslandMovementProbe, Warning,
							TEXT("Approach probe timed out waiting for the raven to perch at %s."), *State->PerchTag.ToString());
						Finish(State, false);
					}
					return;
				}
			}
			if (State->bHasMoverStartOverride &&
				!Pawn->SetActorLocation(State->MoverStartOverride, false, nullptr, ETeleportType::TeleportPhysics))
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Could not place the mover at the requested diagnostic start %s."),
					*State->MoverStartOverride.ToCompactString());
				Finish(State, false);
				return;
			}

			State->Pawn = Pawn;
			State->Controller = Controller;
			State->StartLocation = Pawn->GetActorLocation();
			State->StartedAt = World->GetTimeSeconds();
			FAgentDecision Decision;
			Decision.bValid = true;
			Decision.ActionType = State->bWanderProbe ? EAgentActionType::Wander : EAgentActionType::MoveTo;
			if (!State->bWanderProbe) Decision.ActionTarget = State->TargetTag.ToString();
			Controller->ActOnDecision(Decision);
			State->bMoveStarted = true;
			if (Target)
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Move issued from %s toward %s (%s)."),
					*State->StartLocation.ToCompactString(), *Target->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			else
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Wander issued from %s (%s)."),
					*State->StartLocation.ToCompactString(), *Controller->DescribeActionState());
			}
			return;
		}

		AAutonomousAgentAIController* Controller = State->Controller.Get();
		APawn* Pawn = State->Pawn.Get();
		if (!Controller || !Pawn)
		{
			Finish(State, false);
			return;
		}
		const ARavenAgentAIController* RavenController = Cast<ARavenAgentAIController>(Controller);
		const FString ActionState = Controller->DescribeActionState();
		const bool bComplete = RavenController
			? !Controller->IsActionInProgress() &&
				(ActionState.Contains(TEXT("Reached the flight destination")) || ActionState.Contains(TEXT("short ground hop")))
			: Controller->GetMoveStatus() != EPathFollowingStatus::Moving &&
				ActionState.Contains(TEXT("Reached the requested destination"));
		const bool bTimedOut = World->GetTimeSeconds() - State->StartedAt >= 45.0;
		const bool bMovementStopped = RavenController ? !Controller->IsActionInProgress()
			: Controller->GetMoveStatus() != EPathFollowingStatus::Moving;
		if (bComplete || bTimedOut || bMovementStopped)
		{
			const float Distance = FVector::Dist2D(State->StartLocation, Pawn->GetActorLocation());
			if (bComplete)
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Probe completed: %.0f cm in %.1f simulated seconds; final location %s; %s"),
					Distance, World->GetTimeSeconds() - State->StartedAt, *Pawn->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			else
			{
				UE_LOG(LogIslandMovementProbe, Warning, TEXT("Probe did not complete: %.0f cm in %.1f simulated seconds; final location %s; %s"),
					Distance, World->GetTimeSeconds() - State->StartedAt, *Pawn->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			Finish(State, bComplete);
		}
	}

	static void Finish(const TSharedRef<FProbeState>& State, bool bSuccess)
	{
		if (UWorld* World = State->World.Get()) World->GetTimerManager().ClearTimer(State->PollTimer);
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Isolated movement probe finished: %s."), bSuccess ? TEXT("success") : TEXT("failure"));
		FPlatformMisc::RequestExit(false);
	}
};

static FAutoConsoleCommandWithWorldAndArgs GIslandMovementProbeCommand(
	TEXT("Island.MoveProbe"),
	TEXT("Safely probes a runtime resident move, wander, or approach with agent thinking disabled. Usage: Island.MoveProbe [mover-tag] [target-tag|Wander]; for a perched-raven approach: Island.MoveProbe [mover-tag] Approach [raven-approach-tag] [roost-tag] [optional-start-x start-y start-z]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FIslandMovementProbeCommand::Run));
