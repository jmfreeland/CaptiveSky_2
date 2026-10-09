#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "RavenAgentAIController.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "UnrealClient.h"

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
		State->QueuedAt = World->GetTimeSeconds();
		TArray<FString> PositionalArgs;
		for (const FString& Arg : Args)
		{
			double OptionValue = 0.0;
			if (Arg.StartsWith(TEXT("DelaySeconds="), ESearchCase::IgnoreCase))
			{
				if (!LexTryParseString(OptionValue, *Arg.RightChop(13)) || OptionValue < 0.0 || OptionValue > 30.0)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("DelaySeconds must be a number from 0 to 30."));
					return;
				}
				State->DispatchDelaySeconds = OptionValue;
			}
			else if (Arg.StartsWith(TEXT("HoldSeconds="), ESearchCase::IgnoreCase))
			{
				if (!LexTryParseString(OptionValue, *Arg.RightChop(12)) || OptionValue < 0.0 || OptionValue > 20.0)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("HoldSeconds must be a number from 0 to 20."));
					return;
				}
				State->PostCompletionHoldSeconds = OptionValue;
			}
			else
			{
				PositionalArgs.Add(Arg);
			}
		}
		State->MoverTag = FName(PositionalArgs.IsEmpty() ? TEXT("IslandInnkeeper") : *PositionalArgs[0]);
		State->bApproachProbe = PositionalArgs.Num() > 1 && PositionalArgs[1].Equals(TEXT("Approach"), ESearchCase::IgnoreCase);
		State->bWanderProbe = PositionalArgs.Num() > 1 && PositionalArgs[1].Equals(TEXT("Wander"), ESearchCase::IgnoreCase);
		State->bForceCuriosityProbe = State->bWanderProbe && PositionalArgs.Num() > 2 && PositionalArgs[2].Equals(TEXT("Curious"), ESearchCase::IgnoreCase);
		State->bInteractAfterMove = !State->bWanderProbe && !State->bApproachProbe && PositionalArgs.Num() > 2 && PositionalArgs[2].Equals(TEXT("Interact"), ESearchCase::IgnoreCase);
		State->TargetTag = State->bApproachProbe
			? FName(PositionalArgs.Num() > 2 ? *PositionalArgs[2] : TEXT("ApproachAgent_Raven_01"))
			: FName(PositionalArgs.Num() > 1 ? *PositionalArgs[1] : TEXT("InnDoorLantern"));
		State->PerchTag = State->bApproachProbe ? FName(PositionalArgs.Num() > 3 ? *PositionalArgs[3] : TEXT("Roost_East")) : NAME_None;
		const int32 StartOverrideIndex = State->bApproachProbe ? 4 : (State->bWanderProbe ? (State->bForceCuriosityProbe ? 3 : 2) : (State->bInteractAfterMove ? 3 : INDEX_NONE));
		if (StartOverrideIndex != INDEX_NONE && PositionalArgs.Num() > StartOverrideIndex)
		{
			if (PositionalArgs.Num() <= StartOverrideIndex + 2)
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Start override needs all three numeric world coordinates."));
				return;
			}
			double StartX = 0.0;
			double StartY = 0.0;
			double StartZ = 0.0;
			if (!LexTryParseString(StartX, *PositionalArgs[StartOverrideIndex]) ||
				!LexTryParseString(StartY, *PositionalArgs[StartOverrideIndex + 1]) ||
				!LexTryParseString(StartZ, *PositionalArgs[StartOverrideIndex + 2]))
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Start override needs three numeric world coordinates."));
				return;
			}
			State->bHasMoverStartOverride = true;
			State->MoverStartOverride = FVector(StartX, StartY, StartZ);
		}
		World->GetTimerManager().SetTimer(State->PollTimer,
			FTimerDelegate::CreateLambda([State]() { Poll(State); }), State->bForceCuriosityProbe ? 0.1f : 0.5f, true);
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Queued isolated %s%s probe for %s; camera warm-up %.1f s, post-probe hold %.1f s; waiting for runtime actors."),
			State->bForceCuriosityProbe ? TEXT("forced-curiosity ") : (State->bWanderProbe ? TEXT("wander ") : TEXT("")),
			State->bWanderProbe ? TEXT("flight-wander") : TEXT("movement"), *State->MoverTag.ToString(),
			State->DispatchDelaySeconds, State->PostCompletionHoldSeconds);
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
		FTimerHandle ExitTimer;
		FVector StartLocation = FVector::ZeroVector;
		double StartedAt = 0.0;
		double QueuedAt = 0.0;
		double DispatchDelaySeconds = 0.0;
		double PostCompletionHoldSeconds = 0.0;
		double PerchRequestedAt = 0.0;
		double GroundingStartedAt = 0.0;
		int32 StartupPolls = 0;
		bool bMoveStarted = false;
		bool bInteractAfterMove = false;
		bool bWaitingForGround = false;
		bool bWanderProbe = false;
		bool bForceCuriosityProbe = false;
		bool bApproachProbe = false;
		bool bPerchRequested = false;
		bool bHasMoverStartOverride = false;
		FVector MoverStartOverride = FVector::ZeroVector;
		TWeakObjectPtr<AActor> TargetActor;
		bool bHasMinnowBaseline = false;
		bool bLoggedMinnowBandEntry = false;
		FVector MinnowBaselineCentroid = FVector::ZeroVector;
		int32 MinnowBodyCount = 0;
		int32 MinnowLowFlybySampleCount = 0;
		float MaxMinnowCentroidShiftInBand = 0.f;
		bool bLoggedMinnowStartleCue = false;
		bool bRequestedMinnowStartleScreenshot = false;
		FTimerHandle MinnowScreenshotTimer;
		FString MinnowStartleScreenshotPath;
	};

	static bool SampleMinnowSchool(UWorld* World, FVector& OutSchoolLocation, FVector& OutBodyCentroid, int32& OutBodyCount)
	{
		if (!World) return false;
		AActor* School = nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("MinnowSchool")))
			{
				School = *It;
				break;
			}
		}
		if (!School) return false;

		TArray<UActorComponent*> Components;
		School->GetComponents(Components);
		FVector BodyLocationSum = FVector::ZeroVector;
		OutBodyCount = 0;
		for (UActorComponent* Component : Components)
		{
			const USceneComponent* Scene = Cast<USceneComponent>(Component);
			if (!Scene || !Scene->GetName().StartsWith(TEXT("Minnow_"), ESearchCase::CaseSensitive)) continue;
			BodyLocationSum += Scene->GetRelativeLocation();
			++OutBodyCount;
		}
		if (OutBodyCount == 0) return false;

		OutSchoolLocation = School->GetActorLocation();
		OutBodyCentroid = BodyLocationSum / static_cast<float>(OutBodyCount);
		return true;
	}

	static void SampleRavenMinnowProximity(const TSharedRef<FProbeState>& State, UWorld* World,
		const ARavenAgentAIController* RavenController, const APawn* Raven)
	{
		if (!State->bForceCuriosityProbe || !RavenController || !Raven || !State->bHasMinnowBaseline) return;

		FVector SchoolLocation = FVector::ZeroVector;
		FVector BodyCentroid = FVector::ZeroVector;
		int32 BodyCount = 0;
		if (!SampleMinnowSchool(World, SchoolLocation, BodyCentroid, BodyCount)) return;

		const FVector Offset = Raven->GetActorLocation() - SchoolLocation;
		const float HorizontalDistance = Offset.Size2D();
		const bool bLowFlybyEnvelope = RavenController->LocomotionState == ERavenLocomotionState::Flying &&
			Offset.Z >= 150.f && Offset.Z <= 700.f && HorizontalDistance <= 550.f;
		if (!bLowFlybyEnvelope) return;

		++State->MinnowLowFlybySampleCount;
		const float CentroidShift = FVector::Dist(BodyCentroid, State->MinnowBaselineCentroid);
		State->MaxMinnowCentroidShiftInBand = FMath::Max(State->MaxMinnowCentroidShiftInBand, CentroidShift);
		if (!State->bLoggedMinnowStartleCue)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->ActorHasTag(TEXT("MinnowStartleImpact"))) continue;
				State->bLoggedMinnowStartleCue = true;
				UE_LOG(LogIslandMovementProbe, Log,
					TEXT("Observed the live minnow startle cue at %s during Raven low-flyby sample %d."),
					*It->GetActorLocation().ToCompactString(), State->MinnowLowFlybySampleCount);
				if (!State->bRequestedMinnowStartleScreenshot && FParse::Param(FCommandLine::Get(), TEXT("SpectatorShots")))
				{
					FString ScreenshotDirectory;
					if (FParse::Value(FCommandLine::Get(), TEXT("SpectatorScreenshotDir="), ScreenshotDirectory) && !ScreenshotDirectory.IsEmpty())
					{
						IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
						State->MinnowStartleScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("000_MinnowStartleMidPulse.png"));
						// The ring deliberately grows from a quiet first frame. Capture near
						// its mid-pulse so the probe judges the cue at its intended visibility.
						World->GetTimerManager().SetTimer(State->MinnowScreenshotTimer,
							FTimerDelegate::CreateLambda([State]()
							{
								FScreenshotRequest::RequestScreenshot(State->MinnowStartleScreenshotPath, true, false);
								UE_LOG(LogIslandMovementProbe, Log, TEXT("Requested mid-pulse startle screenshot at %s."), *State->MinnowStartleScreenshotPath);
							}), 0.45f, false);
						State->bRequestedMinnowStartleScreenshot = true;
						UE_LOG(LogIslandMovementProbe, Log, TEXT("Scheduled mid-pulse startle screenshot at %s."), *State->MinnowStartleScreenshotPath);
					}
				}
				break;
			}
		}
		if (!State->bLoggedMinnowBandEntry)
		{
			State->bLoggedMinnowBandEntry = true;
			UE_LOG(LogIslandMovementProbe, Log,
				TEXT("Raven entered the minnow low-flyby envelope: horizontal %.0f cm, vertical %.0f cm, school bodies %d, centroid shift %.0f cm."),
				HorizontalDistance, Offset.Z, BodyCount, CentroidShift);
		}
	}

	static void LogMinnowProbeSummary(const TSharedRef<FProbeState>& State)
	{
		if (!State->bHasMinnowBaseline) return;
		UE_LOG(LogIslandMovementProbe, Log,
			TEXT("Raven/minnow probe summary: %d low-flyby samples; maximum fish-body centroid shift inside the envelope %.0f cm from the pre-action baseline."),
			State->MinnowLowFlybySampleCount, State->MaxMinnowCentroidShiftInBand);
	}

	static void Poll(const TSharedRef<FProbeState>& State)
	{
		UWorld* World = State->World.Get();
		if (!World) return;

		if (!State->bMoveStarted)
		{
			if (World->GetTimeSeconds() - State->QueuedAt < State->DispatchDelaySeconds) return;

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
			State->TargetActor = Target;
			State->StartLocation = Pawn->GetActorLocation();
			State->StartedAt = World->GetTimeSeconds();
			if (State->bForceCuriosityProbe)
			{
				ARavenAgentAIController* RavenController = Cast<ARavenAgentAIController>(Controller);
				if (!RavenController)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("Curiosity flight probe requires an embodied raven controller."));
					Finish(State, false);
					return;
				}
				FVector SchoolLocation = FVector::ZeroVector;
				if (SampleMinnowSchool(World, SchoolLocation, State->MinnowBaselineCentroid, State->MinnowBodyCount))
				{
					State->bHasMinnowBaseline = true;
					UE_LOG(LogIslandMovementProbe, Log,
						TEXT("Captured pre-flight minnow baseline: %d fish bodies at school %s (relative centroid %s)."),
						State->MinnowBodyCount, *SchoolLocation.ToCompactString(), *State->MinnowBaselineCentroid.ToCompactString());
				}
				RavenController->BeginTakeoff(RavenController->MakeCruiseTarget(true));
			}
			else
			{
				FAgentDecision Decision;
				Decision.bValid = true;
				Decision.ActionType = State->bWanderProbe ? EAgentActionType::Wander : EAgentActionType::MoveTo;
				if (!State->bWanderProbe) Decision.ActionTarget = State->TargetTag.ToString();
				Controller->ActOnDecision(Decision);
			}
			State->bMoveStarted = true;
			if (Target)
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Move issued from %s toward %s (%s)."),
					*State->StartLocation.ToCompactString(), *Target->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			else
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("%s issued from %s (%s)."),
					State->bForceCuriosityProbe ? TEXT("Forced-curiosity flight-wander") : TEXT("Wander"),
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
		SampleRavenMinnowProximity(State, World, RavenController, Pawn);
		const FString ActionState = Controller->DescribeActionState();
		const bool bComplete = RavenController
			? !Controller->IsActionInProgress() &&
				(ActionState.Contains(TEXT("Reached the flight destination")) || ActionState.Contains(TEXT("short ground hop")))
			: Controller->GetMoveStatus() != EPathFollowingStatus::Moving &&
				ActionState.Contains(TEXT("Reached the requested destination"));
		// The Raven may need to cross a large portion of the Island to reach a
		// shore marker. Keep the probe bounded, but allow a real long-distance
		// flight to complete before reporting it as stuck.
		constexpr double MovementTimeoutSeconds = 210.0;
		const bool bTimedOut = World->GetTimeSeconds() - State->StartedAt >= MovementTimeoutSeconds;
		const bool bMovementStopped = RavenController ? !Controller->IsActionInProgress()
			: Controller->GetMoveStatus() != EPathFollowingStatus::Moving;
		if (bComplete || bTimedOut || bMovementStopped)
		{
			const float Distance = FVector::Dist2D(State->StartLocation, Pawn->GetActorLocation());
			if (bComplete)
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Probe completed: %.0f cm in %.1f simulated seconds; final location %s; %s"),
					Distance, World->GetTimeSeconds() - State->StartedAt, *Pawn->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
				if (State->bInteractAfterMove)
				{
					if (!State->TargetActor.IsValid())
					{
						UE_LOG(LogIslandMovementProbe, Error, TEXT("Movement arrived, but the interaction target no longer exists."));
						Finish(State, false);
						return;
					}
					FAgentDecision Decision;
					Decision.bValid = true;
					Decision.ActionType = EAgentActionType::Interact;
					Decision.ActionTarget = State->TargetTag.ToString();
					Controller->ActOnDecision(Decision);
					const FString InteractionResult = Controller->DescribeActionState();
					UE_LOG(LogIslandMovementProbe, Log, TEXT("Issued the resident's normal interact action for %s after arrival: %s"),
						*State->TargetTag.ToString(), *InteractionResult);
					if (!InteractionResult.Contains(TEXT("You crouched by"), ESearchCase::IgnoreCase))
					{
						UE_LOG(LogIslandMovementProbe, Error, TEXT("Movement arrived, but the normal interaction did not turn over %s."),
							*State->TargetTag.ToString());
						Finish(State, false);
						return;
					}
				}
			}
			else
			{
				UE_LOG(LogIslandMovementProbe, Warning, TEXT("Probe did not complete within 210 simulated seconds: %.0f cm in %.1f simulated seconds; final location %s; %s"),
					Distance, World->GetTimeSeconds() - State->StartedAt, *Pawn->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			Finish(State, bComplete);
		}
	}

	static void Finish(const TSharedRef<FProbeState>& State, bool bSuccess)
	{
		UWorld* World = State->World.Get();
		if (World) World->GetTimerManager().ClearTimer(State->PollTimer);
		LogMinnowProbeSummary(State);
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Isolated movement probe finished: %s."), bSuccess ? TEXT("success") : TEXT("failure"));
		if (bSuccess && State->PostCompletionHoldSeconds > 0.0 && World)
		{
			UE_LOG(LogIslandMovementProbe, Log, TEXT("Holding the runtime view for %.1f seconds before the bounded diagnostic exit."), State->PostCompletionHoldSeconds);
			World->GetTimerManager().SetTimer(State->ExitTimer,
				FTimerDelegate::CreateLambda([]() { FPlatformMisc::RequestExit(false); }),
				static_cast<float>(State->PostCompletionHoldSeconds), false);
			return;
		}
		FPlatformMisc::RequestExit(false);
	}
};

static FAutoConsoleCommandWithWorldAndArgs GIslandMovementProbeCommand(
	TEXT("Island.MoveProbe"),
	TEXT("Safely probes a runtime resident move, wander, or approach with agent thinking disabled. Add DelaySeconds=0..30 for camera warm-up and HoldSeconds=0..20 for post-success observation. Usage: Island.MoveProbe [mover-tag] [target-tag [Interact] [optional-start-x start-y start-z]|Wander [Curious] [optional-start-x start-y start-z]]; for a perched-raven approach: Island.MoveProbe [mover-tag] Approach [raven-approach-tag] [roost-tag] [optional-start-x start-y start-z]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FIslandMovementProbeCommand::Run));
