#include "AutonomousAgentAIController.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
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
		State->TargetTag = FName(Args.Num() > 1 ? *Args[1] : TEXT("InnDoorLantern"));
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
		FTimerHandle PollTimer;
		FVector StartLocation = FVector::ZeroVector;
		double StartedAt = 0.0;
		int32 StartupPolls = 0;
		bool bMoveStarted = false;
		bool bWanderProbe = false;
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
		const bool bComplete = Controller->GetMoveStatus() != EPathFollowingStatus::Moving &&
			Controller->DescribeActionState().Contains(TEXT("Reached the requested destination"));
		const bool bTimedOut = World->GetTimeSeconds() - State->StartedAt >= 45.0;
		if (bComplete || bTimedOut || Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
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
	TEXT("Safely probes a runtime resident move or wander with agent thinking disabled. Usage: Island.MoveProbe [mover-tag] [target-tag|Wander]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FIslandMovementProbeCommand::Run));
