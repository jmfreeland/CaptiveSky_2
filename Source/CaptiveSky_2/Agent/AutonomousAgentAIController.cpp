// Copyright Epic Games, Inc. All Rights Reserved.

#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "AgentSocialComponent.h"
#include "AgentPlaySessionSubsystem.h"
#include "IslandInteractionUtility.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWorldStateSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
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
	BoundedAutonomousRequests = RepeatedActions = 0;
	RecentWanderDestinations.Reset();
	WanderExplorationOrigin = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
	FurthestWanderDistance = 0.f;
	bCurrentMoveIsWander = false;
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
	if (FParse::Param(FCommandLine::Get(), TEXT("CaptiveSkyDisableAgentThinking"))) return;
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
	if (const UAgentSocialComponent* Social = Agent->FindComponentByClass<UAgentSocialComponent>();
		Social && Social->IsAutomaticConversationActive())
	{
		return;
	}
	const UAgentPlaySessionSubsystem* Session = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UAgentPlaySessionSubsystem>() : nullptr;
	const bool bContinuousPlay = Session && Session->IsContinuous();
	if (Now < NextThinkAt || IsAutonomousRequestLimitReached(BoundedAutonomousRequests, bContinuousPlay)) return;
	if (!bContinuousPlay) ++BoundedAutonomousRequests;
	NextThinkAt = Now + BackgroundDelay(RepeatedActions, ThinkIntervalSeconds);
	Agent->Brain->RequestDecision(FString());
}

double AAutonomousAgentAIController::BackgroundDelay(int32 Repeats, double BaseSeconds)
{
	return FMath::Clamp(FMath::Max(60.0, BaseSeconds) * FMath::Pow(2.0, FMath::Clamp(Repeats - 1, 0, 3)), 60.0, 300.0);
}

bool AAutonomousAgentAIController::IsActionInProgress() const { return GetMoveStatus() == EPathFollowingStatus::Moving; }
FVector AAutonomousAgentAIController::BuildResidentApproachPoint(const FVector& MoverLocation, const FVector& TargetLocation)
{
	FVector Direction = MoverLocation - TargetLocation;
	Direction.Z = 0.f;
	if (!Direction.Normalize()) Direction = FVector::ForwardVector;
	return TargetLocation + Direction * ResidentApproachStandOffDistance + FVector(0.f, 0.f, ResidentApproachAltitudeOffset);
}
FVector AAutonomousAgentAIController::BuildGroundedResidentApproachPoint(const FVector& MoverLocation, const FVector& TargetLocation)
{
	FVector Direction = MoverLocation - TargetLocation;
	Direction.Z = 0.f;
	if (!Direction.Normalize()) Direction = FVector::ForwardVector;
	return FVector(TargetLocation.X, TargetLocation.Y, MoverLocation.Z) + Direction * ResidentApproachStandOffDistance;
}
TArray<FVector> AAutonomousAgentAIController::BuildGroundedResidentApproachCandidates(const FVector& MoverLocation, const FVector& TargetLocation)
{
	const FVector PreferredApproach = BuildGroundedResidentApproachPoint(MoverLocation, TargetLocation);
	FVector PreferredDirection = PreferredApproach - TargetLocation;
	PreferredDirection.Z = 0.f;
	if (!PreferredDirection.Normalize()) PreferredDirection = FVector::ForwardVector;
	const float PreferredAngle = FMath::Atan2(PreferredDirection.Y, PreferredDirection.X);
	TArray<FVector> Candidates;
	Candidates.Reserve(8);
	for (int32 Side = 0; Side < 8; ++Side)
	{
		const float Angle = PreferredAngle + Side * (PI / 4.f);
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Candidates.Add(FVector(TargetLocation.X, TargetLocation.Y, MoverLocation.Z) +
			Direction * ResidentApproachStandOffDistance);
	}
	return Candidates;
}
bool AAutonomousAgentAIController::IsElevatedResidentForApproach(const ACharacter* TargetCharacter)
{
	if (!TargetCharacter) return false;
	if (const ARavenAgentAIController* RavenController = Cast<ARavenAgentAIController>(TargetCharacter->GetController()))
		return RavenController->LocomotionState != ERavenLocomotionState::Grounded;
	const UCharacterMovementComponent* Movement = TargetCharacter->GetCharacterMovement();
	return Movement && !Movement->IsMovingOnGround();
}
bool AAutonomousAgentAIController::FindGroundedResidentApproachGoal(UNavigationSystemV1* Navigation,
	const FVector& MoverLocation, const FVector& TargetLocation, const FNavAgentProperties& AgentProperties,
	float CapsuleHalfHeight, float SpeakingRadius, FNavLocation& OutStart, FNavLocation& OutGoal, AActor* PathfindingContext)
{
	if (!Navigation || CapsuleHalfHeight <= 0.f || SpeakingRadius <= 0.f) return false;
	const FVector StartOnFloor = MoverLocation - FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
	if (!ProjectGroundedTarget(Navigation, StartOnFloor, AgentProperties, OutStart)) return false;

	// A single arrival-side goal can land on water or the wrong side of a cliff,
	// trunk, or building even when another place around the elevated resident is
	// reachable. Search a small ring of grounded stand-offs and choose the shortest
	// complete path that remains inside their shared speaking radius.
	float BestPathLength = TNumericLimits<float>::Max();
	bool bFoundGoal = false;
	TArray<FVector> TestedGoals;
	for (const FVector& DesiredApproach : BuildGroundedResidentApproachCandidates(OutStart.Location, TargetLocation))
	{
		FNavLocation CandidateGoal;
		if (!ProjectGroundedTarget(Navigation, DesiredApproach, AgentProperties, CandidateGoal)) continue;
		if (TestedGoals.ContainsByPredicate([&CandidateGoal](const FVector& Existing)
			{ return FVector::DistSquared2D(Existing, CandidateGoal.Location) < FMath::Square(50.f); })) continue;
		TestedGoals.Add(CandidateGoal.Location);

		const FVector GoalBodyCenter = CandidateGoal.Location + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
		if (FVector::DistSquared(GoalBodyCenter, TargetLocation) > FMath::Square(SpeakingRadius)) continue;
		const UNavigationPath* Route = Navigation->FindPathToLocationSynchronously(
			Navigation->GetWorld(), OutStart.Location, CandidateGoal.Location, PathfindingContext);
		if (!Route || !Route->IsValid() || Route->IsPartial() || Route->GetPathLength() >= BestPathLength) continue;

		OutGoal = CandidateGoal;
		BestPathLength = Route->GetPathLength();
		bFoundGoal = true;
	}
	return bFoundGoal;
}
bool AAutonomousAgentAIController::IsAutonomousRequestLimitReached(int32 RequestCount, bool bContinuousPlay)
{
	return !bContinuousPlay && RequestCount >= BoundedAutonomousRequestLimit;
}
bool AAutonomousAgentAIController::IsUsableWanderPath(const UNavigationPath* Path, const FVector& Origin, const FVector& Goal)
{
	return Path && Path->IsValid() && !Path->IsPartial() &&
		FVector::DistSquared2D(Origin, Goal) >= FMath::Square(WanderMinimumDistance);
}
bool AAutonomousAgentAIController::IsWanderPathPhysicallyClear(const UWorld* World, const UNavigationPath* Path, const APawn* Pawn)
{
	const UCapsuleComponent* Capsule = Pawn ? Pawn->FindComponentByClass<UCapsuleComponent>() : nullptr;
	if (!World || !Path || !Capsule || Path->PathPoints.Num() < 2) return false;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgentWanderCapsuleClearance), false, Pawn);
	const FCollisionShape CapsuleShape = Capsule->GetCollisionShape();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	for (int32 PointIndex = 1; PointIndex < Path->PathPoints.Num(); ++PointIndex)
	{
		const FVector SegmentStart = Path->PathPoints[PointIndex - 1] + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
		const FVector SegmentEnd = Path->PathPoints[PointIndex] + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
		FHitResult Hit;
		if (World->SweepSingleByChannel(Hit, SegmentStart, SegmentEnd, FQuat::Identity,
			ECC_Pawn, CapsuleShape, QueryParams)) return false;
	}
	return true;
}
float AAutonomousAgentAIController::WanderNoveltyScore(const FVector& Candidate, const TArray<FVector>& RecentDestinations)
{
	if (RecentDestinations.IsEmpty()) return 0.f;
	float NearestRecentDistanceSquared = TNumericLimits<float>::Max();
	for (const FVector& Recent : RecentDestinations)
		NearestRecentDistanceSquared = FMath::Min(NearestRecentDistanceSquared, FVector::DistSquared2D(Candidate, Recent));
	return NearestRecentDistanceSquared;
}
float AAutonomousAgentAIController::WanderFrontierScore(const FVector& Candidate, const FVector& ExplorationOrigin,
	float FurthestExploredDistance, const TArray<FVector>& RecentDestinations)
{
	const float CandidateDistance = FVector::Dist2D(Candidate, ExplorationOrigin);
	const float FrontierProgress = FMath::Max(0.f, CandidateDistance - FurthestExploredDistance);
	return WanderNoveltyScore(Candidate, RecentDestinations) + FrontierProgress * WanderFrontierProgressWeight;
}
float AAutonomousAgentAIController::WanderLandmarkProgressScore(const FVector& Candidate, const FVector& Origin,
	const TArray<FVector>& VisibleLandmarks)
{
	float BestProgress = 0.f;
	for (const FVector& Landmark : VisibleLandmarks)
	{
		BestProgress = FMath::Max(BestProgress,
			FVector::Dist2D(Origin, Landmark) - FVector::Dist2D(Candidate, Landmark));
	}
	return BestProgress * WanderLandmarkProgressWeight;
}
bool AAutonomousAgentAIController::IsWanderLandmarkEligible(const AActor* Landmark,
	const TMap<FName, double>& RecentInspections, double Now)
{
	if (!IsValid(Landmark) || Landmark->IsHidden() || !Landmark->ActorHasTag(TEXT("IslandLandmark")) ||
		!IslandInteractionUtility::IsMovementTargetAllowed(Landmark) || !FMath::IsFinite(Now)) return false;
	const FName TargetTag = IslandInteractionUtility::GetTargetTag(Landmark);
	if (TargetTag.IsNone()) return false;
	const double* Until = RecentInspections.Find(TargetTag);
	return !Until || !FMath::IsFinite(*Until) || *Until <= Now;
}
bool AAutonomousAgentAIController::CanFollowWanderCuriosityToward(const AActor* Landmark, double Now) const
{
	return IsWanderLandmarkEligible(Landmark, InspectedUntil, Now);
}
bool AAutonomousAgentAIController::ProjectGroundedTarget(UNavigationSystemV1* Navigation, const FVector& Target, const FNavAgentProperties& AgentProperties, FNavLocation& OutLocation)
{
	if (!Navigation) return false;
	// Indoors, a large Z extent can select an upper floor or roof hundreds of
	// centimetres above a floor-level marker. Keep the first search close to the
	// marker's height so walkers don't mistake another level for the floor.
	const FVector FloorExtent(250.f, 250.f, 120.f);
	if (Navigation->ProjectPointToNavigation(Target, OutLocation, FloorExtent, &AgentProperties)) return true;

	// Furniture may remove the nav polygon directly beneath its marker. Search
	// outward in small rings for a nearby floor point, choosing the closest clear
	// surface before considering a deliberately elevated landmark projection.
	for (const float Radius : { 100.f, 200.f, 300.f, 400.f })
	{
		bool bFoundAtRadius = false;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		FNavLocation BestLocation;
		for (int32 AngleDegrees = 0; AngleDegrees < 360; AngleDegrees += 45)
		{
			const float Angle = FMath::DegreesToRadians(static_cast<float>(AngleDegrees));
			const FVector Candidate = Target + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
			FNavLocation Projected;
			if (!Navigation->ProjectPointToNavigation(Candidate, Projected, FVector(75.f, 75.f, 120.f), &AgentProperties)) continue;
			const float DistanceSquared = FVector::DistSquared2D(Target, Projected.Location);
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestLocation = Projected;
				bFoundAtRadius = true;
			}
		}
		if (bFoundAtRadius)
		{
			OutLocation = BestLocation;
			return true;
		}
	}

	// Elevated landmarks without nearby floor nav (for example a raised stone)
	// still remain usable, but only after all floor-level options were checked.
	return Navigation->ProjectPointToNavigation(Target, OutLocation, FVector(250.f, 250.f, 1000.f), &AgentProperties);
}
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
bool AAutonomousAgentAIController::TryRest(FName RequestedRestSite)
{
	APawn* Body = GetPawn();
	UAgentConsolidationComponent* Rest = Body ? Body->FindComponentByClass<UAgentConsolidationComponent>() : nullptr;
	if (!CanRest() || !Rest)
	{
		ReportAction(TEXT("Cannot sleep here yet: finish moving and settle on the ground or a solid perch first."));
		return false;
	}

	AActor* RequestedBed = nullptr;
	AActor* RequestedPerch = nullptr;
	if (!RequestedRestSite.IsNone())
	{
		if (RequestedRestSite.ToString().StartsWith(TEXT("InnBed_")))
		{
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
				if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(RequestedRestSite)) { RequestedBed = *It; break; }
			if (!RequestedBed)
			{
				ReportAction(TEXT("That tagged inn bed is not present here. Nothing changed."));
				return false;
			}
		}
		else
		{
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
				if (It->ActorHasTag(TEXT("RavenPerch")) && It->ActorHasTag(RequestedRestSite)) { RequestedPerch = *It; break; }
			if (!RequestedPerch)
			{
				ReportAction(TEXT("That is not a supported bed or raven perch. Nothing changed."));
				return false;
			}
		}
		AActor* RequestedLocation = RequestedBed ? RequestedBed : RequestedPerch;
		if (FVector::Dist2D(Body->GetActorLocation(), RequestedLocation->GetActorLocation()) > 250.f ||
			FMath::Abs(Body->GetActorLocation().Z - RequestedLocation->GetActorLocation().Z) > 250.f)
		{
			ReportAction(FString::Printf(TEXT("You are not at %s yet. Move there first; sleep does not teleport you."), *RequestedRestSite.ToString()));
			return false;
		}
	}

	if (!Rest->BeginSleep(120.f))
	{
		ReportAction(TEXT("Sleep could not begin; an ordinary thought may still be in progress."));
		return false;
	}
	StopMovement();
	NextRestAt = FPlatformTime::Seconds() + 900;
	NextThinkAt = FPlatformTime::Seconds() + 180;
	RepeatedActions = 0;

	AActor* BedAtRest = RequestedBed;
	if (!BedAtRest && !RequestedPerch)
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			if (It->ActorHasTag(TEXT("IslandInn")) && It->Tags.ContainsByPredicate([](FName Tag) { return Tag.ToString().StartsWith(TEXT("InnBed_")); }) &&
				FVector::Dist2D(Body->GetActorLocation(), It->GetActorLocation()) <= 250.f &&
				FMath::Abs(Body->GetActorLocation().Z - It->GetActorLocation().Z) <= 250.f)
			{
				BedAtRest = *It;
				break;
			}
	const bool bShelteredInnRest = BedAtRest && UIslandEnvironmentSubsystem::IsInsideInnAt(GetWorld(), Body->GetActorLocation(), Body);
	if (bShelteredInnRest)
	{
		const bool bMemoryWillBeRecorded = Body->FindComponentByClass<UAgentMemoryComponent>() != nullptr;
		if (bMemoryWillBeRecorded)
		{
			FString BedName = TEXT("a tagged bed");
			for (const FName Tag : BedAtRest->Tags)
				if (Tag.ToString().StartsWith(TEXT("InnBed_"))) { BedName = Tag.ToString(); break; }
			const FString MemoryText = FString::Printf(TEXT("I rested near %s at the Island inn. Its tagged roof was overhead and its walls met the current geometric enclosure check."), *BedName);
			Rest->QueueSleepExperience(MemoryText);
		}
		ReportAction(bMemoryWillBeRecorded
			? TEXT("Settled to sleep beside the tagged inn bed. A roof was overhead and the enclosing walls passed the current indoor-geometry check; the lived rest will be added to your memory when the rest interval completes. Waking early will cancel that note. That evidence does not promise warmth, complete dryness, comfort, or recovery; ordinary thoughts pause during sleep.")
			: TEXT("Settled to sleep beside the tagged inn bed. A roof was overhead and the enclosing walls passed the current indoor-geometry check, but no memory store is attached to this body. That evidence does not promise warmth, complete dryness, comfort, or recovery; ordinary thoughts pause during sleep."));
	}
	else if (BedAtRest)
	{
		ReportAction(TEXT("Settled to sleep near the tagged inn bed, but the roof-and-wall enclosure did not pass the current check. This will not be recorded as sheltered inn rest; ordinary thoughts pause during sleep."));
	}
	else
	{
		ReportAction(RequestedPerch
			? TEXT("Settled to sleep at the tagged perch. Ordinary thoughts pause during sleep; no sheltered inn rest was verified.")
			: TEXT("Settled to sleep. Ordinary thoughts pause during sleep; no sheltered inn rest was verified."));
	}
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
	if (bCurrentMoveIsWander)
	{
		const APawn* ControlledPawn = GetPawn();
		UE_LOG(LogAutonomousAgentAI, Log,
			TEXT("%s wander move completed with result code %d, flags %u; location %s, velocity %s."),
			*GetName(), static_cast<int32>(Result.Code), static_cast<uint32>(Result.Flags),
			ControlledPawn ? *ControlledPawn->GetActorLocation().ToCompactString() : TEXT("<no pawn>"),
			ControlledPawn ? *ControlledPawn->GetVelocity().ToCompactString() : TEXT("<no pawn>"));
	}
	else if (Result.Code == EPathFollowingResult::Blocked)
	{
		const APawn* ControlledPawn = GetPawn();
		UE_LOG(LogAutonomousAgentAI, Warning,
			TEXT("%s non-wander movement was blocked (result code %d, flags %u); location %s, velocity %s."),
			*GetName(), static_cast<int32>(Result.Code), static_cast<uint32>(Result.Flags),
			ControlledPawn ? *ControlledPawn->GetActorLocation().ToCompactString() : TEXT("<no pawn>"),
			ControlledPawn ? *ControlledPawn->GetVelocity().ToCompactString() : TEXT("<no pawn>"));
	}
	if (bCurrentMoveIsWander && Result.IsSuccess())
	{
		if (const APawn* ControlledPawn = GetPawn())
		{
			const FVector WanderLocation = ControlledPawn->GetActorLocation();
			RecentWanderDestinations.Add(WanderLocation);
			if (RecentWanderDestinations.Num() > 8) RecentWanderDestinations.RemoveAt(0);
			FurthestWanderDistance = FMath::Max(FurthestWanderDistance,
				FVector::Dist2D(WanderLocation, WanderExplorationOrigin));
		}
	}
	bCurrentMoveIsWander = false;
	if (Result.Code == EPathFollowingResult::Blocked)
	{
		APawn* ControlledPawn = GetPawn();
		AActor* Target = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			if (It->GetFName() == PendingGroundMoveTargetName) { Target = *It; break; }
		const FName TargetTag = IslandInteractionUtility::GetTargetTag(Target);
		if (ControlledPawn && !TargetTag.IsNone() && IslandInteractionUtility::CanInspect(ControlledPawn, Target))
		{
			PendingGroundMoveTargetName = NAME_None;
			ReportAction(FString::Printf(
				TEXT("The route to the projected ground point was blocked, but you stopped within clear inspection range of %s. You may inspect or interact from this reachable approach; you did not reach the marker itself."),
				*TargetTag.ToString()));
			return;
		}
	}
	PendingGroundMoveTargetName = NAME_None;
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
	bool bNearbyWildlifeOccluded = false;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(Target)) continue;
		const bool bWildlifeTarget = Target == FName(TEXT("WoodlandDeer")) || Target == FName(TEXT("Firefly")) || Target == FName(TEXT("TidepoolCrab")) ||
			Target == FName(TEXT("TideglassDragonfly")) || Target == FName(TEXT("MinnowSchool"));
		if (bWildlifeTarget && It->IsHidden()) continue;
		if (bWildlifeTarget && FVector::DistSquared(It->GetActorLocation(), Observer->GetActorLocation()) > FMath::Square(400.f)) continue;
		// Ecology habitat markers can share a name with the interactable pool but are not themselves landmarks.
		// Skip them before distance checks so an unrelated habitat cannot mask the actual TideglassPool landmark.
		if (Target == FName(TEXT("TideglassPool")) && !It->ActorHasTag(TEXT("IslandLandmark"))) continue;
		if (FVector::DistSquared(It->GetActorLocation(), Observer->GetActorLocation()) > FMath::Square(400.f))
		{
			ReportAction(TEXT("Too far away to inspect; move within four metres first."));
			return;
		}
		if (!IslandInteractionUtility::CanInspect(Observer, *It))
		{
			if (bWildlifeTarget) { bNearbyWildlifeOccluded = true; continue; }
			ReportAction(TEXT("The inspection point is occluded; find a clear approach."));
			return;
		}
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
		else if (It->ActorHasTag(TEXT("IslandLife")) || It->ActorHasTag(TEXT("IslandLandmark")) || IslandInteractionUtility::GetTargetTag(*It) == Target)
		{
			if (!IslandInteractionUtility::Perform(Observer, *It, Fact)) { ReportAction(TEXT("This target has no implemented inspection interaction.")); return; }
		}
		else { ReportAction(TEXT("This target has no implemented inspection interaction.")); return; }
		// The hearth is deliberately reversible: residents may tend it again before its light expires.
		InspectedUntil.Add(Target, FPlatformTime::Seconds() + (Target == FName(TEXT("InnHearth")) ? 60.0 : 300.0));
		ReportAction(Target.ToString() + TEXT(": ") + Fact);
		if (UAgentMemoryComponent* Memory = Observer->FindComponentByClass<UAgentMemoryComponent>())
			Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Observation, Target.ToString() + TEXT(": ") + Fact, 0.45f, {TEXT("action-result"), Target.ToString()}));
		return;
	}
	if (bNearbyWildlifeOccluded) ReportAction(TEXT("That nearby wildlife is hidden from view; find a clear approach and watch without disturbing it."));
	else if (Target == FName(TEXT("Firefly"))) ReportAction(TEXT("No firefly is close enough to watch quietly; move within four metres and let it come near."));
	else if (Target == FName(TEXT("TidepoolCrab"))) ReportAction(TEXT("No shore crab is close enough to watch quietly; return to the TideglassPool and look near its edge."));
	else if (Target == FName(TEXT("TideglassDragonfly"))) ReportAction(TEXT("No Tideglass dragonfly is close enough to watch; look above the Tideglass shore during daylight and let it come near."));
	else if (Target == FName(TEXT("MinnowSchool"))) ReportAction(TEXT("No minnow school is close enough to watch; return to TideglassPool and look into the shallows."));
	else if (Target == FName(TEXT("WoodlandDeer"))) ReportAction(TEXT("No wild stag is close enough to watch; look near Wind Arch and keep a respectful distance."));
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

void AAutonomousAgentAIController::WriteGuestBook(const FAgentDecision& Decision)
{
	APawn* Body = GetPawn();
	UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	AActor* Counter = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnCounter"))) { Counter = *It; break; }
	if (!Body || !WorldState || !Counter)
	{
		ReportAction(TEXT("There is no writable inn guest book here. Nothing changed."));
		return;
	}
	if (FVector::Dist2D(Body->GetActorLocation(), Counter->GetActorLocation()) > 250.f ||
		FMath::Abs(Body->GetActorLocation().Z - Counter->GetActorLocation().Z) > 250.f)
	{
		ReportAction(TEXT("The guest book is at the inn counter; come within two and a half metres before writing. Nothing changed."));
		return;
	}
	if (!CanArrangeStones())
	{
		ReportAction(TEXT("You need to be settled on the ground beside the counter to write. Nothing changed."));
		return;
	}
	UAgentMemoryComponent* Memory = Body->FindComponentByClass<UAgentMemoryComponent>();
	const FString AgentId = Memory ? Memory->GetResolvedAgentId() : Body->GetName();
	bool bChanged = false;
	const FString Fact = WorldState->WriteGuestBook(AgentId, Decision.Intent,
		UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()), bChanged);
	ReportAction(TEXT("GuestBook: ") + Fact);
	if (bChanged && Memory)
		Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Observation, TEXT("Inn guest book: ") + Fact, 0.65f, {TEXT("action-result"), TEXT("guest-book")}));
}

void AAutonomousAgentAIController::HandleDecisionReady(const FAgentDecision& Decision)
{
	if (!Decision.bValid)
	{
		// The brain has already logged parse/provider failures at their source. It also broadcasts
		// default decisions for intentional deferrals (for example, a spent continuous-play allowance).
		UE_LOG(LogAutonomousAgentAI, Verbose, TEXT("%s: brain returned no actionable decision."), *GetName());
		return;
	}
	if (IsResting()) return;
	const UAgentBrainComponent* Brain = GetPawn() ? GetPawn()->FindComponentByClass<UAgentBrainComponent>() : nullptr;
	if (!Brain || Brain->LastConversationContext.Text.IsEmpty())
	{
		const FString Key = FString::FromInt(static_cast<int32>(Decision.ActionType)) + TEXT(":") + Decision.ActionTarget.ToLower();
		// Different speech and random wandering are not identical failed actions.
		const bool bRepeatSensitive = Decision.ActionType == EAgentActionType::MoveTo || Decision.ActionType == EAgentActionType::Interact || Decision.ActionType == EAgentActionType::Idle || Decision.ActionType == EAgentActionType::Build || Decision.ActionType == EAgentActionType::Land;
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
	PendingGroundMoveTargetName = NAME_None;

	switch (Decision.ActionType)
	{
	case EAgentActionType::Wander:
	{
		bCurrentMoveIsWander = false;
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			const FVector Origin = ControlledPawn->GetActorLocation();
			const FNavAgentProperties& AgentProperties = ControlledPawn->GetNavAgentPropertiesRef();
			ANavigationData* NavData = NavSys->GetNavDataForProps(AgentProperties);
			FNavLocation Destination;
			bool bFoundFullRoute = false;
			float BestWanderScore = -1.f;
			float BestLandmarkProgressScore = 0.f;
			TArray<FVector> BestWanderPathPoints;
			TArray<FVector> VisibleLandmarks;
			VisibleLandmarks.Reserve(6);
			const double CuriosityNow = FPlatformTime::Seconds();
			for (TActorIterator<AActor> It(GetWorld()); It && VisibleLandmarks.Num() < 6; ++It)
			{
				if (!CanFollowWanderCuriosityToward(*It, CuriosityNow) ||
					FVector::DistSquared(Origin, It->GetActorLocation()) > FMath::Square(5000.f)) continue;
				FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(AgentWanderLandmarkVisibility), false, ControlledPawn);
				VisibilityParams.AddIgnoredActor(*It);
				FHitResult VisibilityHit;
				if (GetWorld()->LineTraceSingleByChannel(VisibilityHit, Origin, It->GetActorLocation(), ECC_Visibility, VisibilityParams)) continue;
				VisibleLandmarks.Add(It->GetActorLocation());
			}
			int32 BlockedPathCount = 0;
			for (int32 Attempt = 0; Attempt < 24; ++Attempt)
			{
				FNavLocation Candidate;
				if (!NavData || !NavSys->GetRandomReachablePointInRadius(Origin, WanderRadius, Candidate, NavData)) break;
				const UNavigationPath* Route = NavSys->FindPathToLocationSynchronously(GetWorld(), Origin, Candidate.Location, ControlledPawn);
				if (!IsUsableWanderPath(Route, Origin, Candidate.Location)) continue;
				if (!IsWanderPathPhysicallyClear(GetWorld(), Route, ControlledPawn))
				{
					++BlockedPathCount;
					continue;
				}
				const float LandmarkProgressScore = WanderLandmarkProgressScore(Candidate.Location, Origin, VisibleLandmarks);
				const float WanderScore = WanderFrontierScore(Candidate.Location, WanderExplorationOrigin,
					FurthestWanderDistance, RecentWanderDestinations) + LandmarkProgressScore;
				if (bFoundFullRoute && WanderScore <= BestWanderScore) continue;
				Destination = Candidate;
				bFoundFullRoute = true;
				BestWanderScore = WanderScore;
				BestLandmarkProgressScore = LandmarkProgressScore;
				BestWanderPathPoints = Route->PathPoints;
			}
			if (bFoundFullRoute)
			{
				FString PathDescription;
				for (const FVector& PathPoint : BestWanderPathPoints)
					PathDescription += (PathDescription.IsEmpty() ? TEXT("") : TEXT(" -> ")) + PathPoint.ToCompactString();
				UE_LOG(LogAutonomousAgentAI, Log,
					TEXT("%s selected wander destination %s with %d visible nearby landmarks; landmark progress %.0f cm; rejected %d capsule-blocked paths; nav path: %s."),
					*GetName(), *Destination.Location.ToCompactString(), VisibleLandmarks.Num(),
					BestLandmarkProgressScore / WanderLandmarkProgressWeight, BlockedPathCount, *PathDescription);
				// A random reachable point is usually not the exact point a capsule can occupy.
				// Stop with overlap tolerance and reject partial paths instead of timing out at a wall.
				const EPathFollowingRequestResult::Type Request = MoveToLocation(Destination.Location,
					WanderAcceptanceRadius, true, true, false, false, nullptr, false);
				bCurrentMoveIsWander = Request == EPathFollowingRequestResult::RequestSuccessful;
				ReportAction(Request == EPathFollowingRequestResult::Failed ? TEXT("Wandering failed: no navigable route.") : Request == EPathFollowingRequestResult::AlreadyAtGoal ? TEXT("Already at the wandering destination; waiting quietly.") : TEXT("Wandering movement started; arrival is not yet complete."));
				break;
			}
		}
		ReportAction(TEXT("Wandering failed: no full route to a distinct nearby point was found."));
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
			if (!IslandInteractionUtility::IsMovementTargetAllowed(TargetActor))
			{
				ReportAction(TEXT("Wildlife are not movement targets. Observe from a respectful distance; interact quietly only when already close, and do not chase, feed, touch, or claim them."));
				break;
			}
			if (Cast<AAutonomousAgentCharacter>(TargetActor) && TargetActor != ControlledPawn)
			{
				// Resident movement targets represent a willing approach, not a request to occupy
				// another body's capsule. Keep a comfortable stand-off; the target may move while
				// we are travelling, so let path following track the actor rather than a stale point.
				ACharacter* MoverCharacter = Cast<ACharacter>(ControlledPawn);
				ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
				if (MoverCharacter && TargetCharacter && MoverCharacter->GetCharacterMovement() &&
					MoverCharacter->GetCharacterMovement()->IsMovingOnGround() && IsElevatedResidentForApproach(TargetCharacter))
				{
					UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
					const float HalfHeight = MoverCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					FNavLocation GroundStart;
					FNavLocation GroundGoal;
					const UAgentSocialComponent* MoverSocial = MoverCharacter->FindComponentByClass<UAgentSocialComponent>();
					const UAgentSocialComponent* TargetSocial = TargetCharacter->FindComponentByClass<UAgentSocialComponent>();
					const float SpeakingRadius = MoverSocial && TargetSocial
						? FMath::Min(MoverSocial->SpeakingRadius, TargetSocial->SpeakingRadius) : 0.f;
					if (!FindGroundedResidentApproachGoal(NavSys, MoverCharacter->GetActorLocation(),
						TargetActor->GetActorLocation(), MoverCharacter->GetNavAgentPropertiesRef(), HalfHeight,
						SpeakingRadius, GroundStart, GroundGoal, MoverCharacter))
					{
						ReportAction(TEXT("Approach failed: no complete ground route reaches conversational range of the elevated resident."));
						break;
					}
					else
					{
						const EPathFollowingRequestResult::Type GroundedResult = MoveToLocation(GroundGoal.Location,
							WanderAcceptanceRadius, true, true, false, false, nullptr, false);
						ReportAction(GroundedResult == EPathFollowingRequestResult::Failed
							? TEXT("Approach failed: the raven's elevated position has no grounded conversational route that could start.")
							: GroundedResult == EPathFollowingRequestResult::AlreadyAtGoal
								? TEXT("Already at a reachable ground position near the elevated resident. Speaking remains optional.")
								: TEXT("Approach started toward a reachable ground position near the elevated resident; arriving does not begin a conversation."));
						break;
					}
				}
				const EPathFollowingRequestResult::Type Result = MoveToActor(TargetActor,
					ResidentApproachStandOffDistance, true, true, false, nullptr, false);
				ReportAction(Result == EPathFollowingRequestResult::Failed
					? TEXT("Approach failed: no complete route to a reachable conversational space near the other resident.")
					: Result == EPathFollowingRequestResult::AlreadyAtGoal
						? TEXT("Already near the other resident. Speaking remains optional; wait, observe, or choose another activity.")
						: TEXT("Approach started toward the other resident's conversational space; arriving does not begin a conversation."));
				break;
			}

			// Shared landmarks can be elevated bird targets. Grounded bodies need a
			// nearby walkable goal, not the airborne marker or a partial-path endpoint.
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			FNavLocation GroundGoal;
			if (!ProjectGroundedTarget(NavSys, TargetActor->GetActorLocation(), ControlledPawn->GetNavAgentPropertiesRef(), GroundGoal))
			{
				ReportAction(TEXT("Movement failed: no walkable ground near that marker. Choose another destination."));
				break;
			}
			const UNavigationPath* Route = NavSys
				? NavSys->FindPathToLocationSynchronously(GetWorld(), ControlledPawn->GetActorLocation(), GroundGoal.Location, ControlledPawn)
				: nullptr;
			UE_LOG(LogAutonomousAgentAI, Log,
				TEXT("%s projected %s to grounded goal %s; nav route %s%s (%d points)."),
				*GetName(), *Decision.ActionTarget, *GroundGoal.Location.ToCompactString(),
				!Route || !Route->IsValid() ? TEXT("missing") : Route->IsPartial() ? TEXT("partial") : TEXT("complete"),
				Route && Route->IsValid() ? *FString::Printf(TEXT(", %.0f m"), Route->GetPathLength() / 100.f) : TEXT(""),
				Route ? Route->PathPoints.Num() : 0);
			PendingGroundMoveTargetName = TargetActor->GetFName();
			const EPathFollowingRequestResult::Type Result = MoveToLocation(GroundGoal.Location,
				50.f, true, true, false, true, nullptr, false);
			if (Result != EPathFollowingRequestResult::RequestSuccessful) PendingGroundMoveTargetName = NAME_None;
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
		if (UAgentSocialComponent* Social = ControlledPawn->FindComponentByClass<UAgentSocialComponent>();
			Social && !Decision.ActionTarget.IsEmpty() && Social->GetConversationCooldownRemainingWith(Decision.ActionTarget) > 0.f)
		{
			ReportAction(FString::Printf(TEXT("Your automatic conversation with %s is still resting; the other resident did not hear that speech. Choose another activity and wait for the pause to end."), *Decision.ActionTarget));
			break;
		}
		UE_LOG(LogAutonomousAgentAI, Log, TEXT("%s chose speech: \"%s\""), *GetName(), *Decision.Speech);
		break;
	case EAgentActionType::Interact:
		InspectTarget(FName(*Decision.ActionTarget));
		break;
	case EAgentActionType::Build:
		if (Decision.ActionTarget.StartsWith(TEXT("ArrangingGround"))) ArrangeStones(Decision);
		else if (Decision.ActionTarget == TEXT("GuestBook")) WriteGuestBook(Decision);
		else ReportAction(TEXT("This body has no way to build anything yet; nothing changed."));
		break;
	case EAgentActionType::Sleep:
		TryRest(FName(*Decision.ActionTarget));
		break;
	case EAgentActionType::Land:
		StopMovement();
		ReportAction(TEXT("Landing at an open-ground site is available to the raven only; nothing changed."));
		break;
	case EAgentActionType::Idle:
	default:
		StopMovement();
		ReportAction(TEXT("Waiting quietly. No new action or discovery occurred."));
		break;
	}
}
