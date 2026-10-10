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
#include "IslandTidepoolMinnows.h"
#include "IslandTrail.h"
#include "IslandWorldStateSubsystem.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
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
bool AAutonomousAgentAIController::IsCapsulePathPhysicallyClear(const UWorld* World, const UNavigationPath* Path,
	const UCapsuleComponent* Capsule, const AActor* IgnoredActor, int32* OutBlockingSegment, FHitResult* OutBlocker)
{
	if (OutBlockingSegment) *OutBlockingSegment = INDEX_NONE;
	if (OutBlocker) *OutBlocker = FHitResult();
	if (!World || !Path || !Capsule || Path->PathPoints.Num() < 2) return false;

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FCollisionShape CapsuleShape = Capsule->GetCollisionShape();
	const ACharacter* Character = Cast<ACharacter>(IgnoredActor);
	const UCharacterMovementComponent* CharacterMovement = Character ? Character->GetCharacterMovement() : nullptr;
	const float WalkableFloorZ = CharacterMovement ? CharacterMovement->GetWalkableFloorZ() : 0.7f;

	for (int32 PointIndex = 1; PointIndex < Path->PathPoints.Num(); ++PointIndex)
	{
		const FVector SegmentStart = Path->PathPoints[PointIndex - 1] + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
		const FVector SegmentEnd = Path->PathPoints[PointIndex] + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AgentCapsulePathClearance), false, IgnoredActor);
		int32 IgnoredLandscapeContacts = 0;
		FHitResult Hit;
		while (World->SweepSingleByChannel(Hit, SegmentStart, SegmentEnd, FQuat::Identity,
			ECC_Pawn, CapsuleShape, QueryParams))
		{
			const ULandscapeHeightfieldCollisionComponent* LandscapeCollision =
				Cast<ULandscapeHeightfieldCollisionComponent>(Hit.GetComponent());
			const bool bWalkableGroundContact = LandscapeCollision && Hit.ImpactNormal.Z >= WalkableFloorZ;
			if (bWalkableGroundContact && IgnoredLandscapeContacts < 16)
			{
				// The navmesh already establishes this as walkable support. A capsule sweep
				// otherwise reports the heightfield floor itself as an obstacle on every path.
				QueryParams.AddIgnoredComponent(LandscapeCollision);
				++IgnoredLandscapeContacts;
				continue;
			}

			if (OutBlockingSegment) *OutBlockingSegment = PointIndex;
			if (OutBlocker) *OutBlocker = Hit;
			return false;
		}
	}
	return true;
}

bool AAutonomousAgentAIController::FindGroundedLandmarkApproachGoal(UNavigationSystemV1* Navigation, UWorld* World,
	APawn* Pawn, const FVector& MoverLocation, const AActor* Target,
	const FNavAgentProperties& AgentProperties, FNavLocation& OutGoal, AActor* PathfindingContext)
{
	const UCapsuleComponent* Capsule = Pawn ? Pawn->FindComponentByClass<UCapsuleComponent>() : nullptr;
	if (!Navigation || !World || !Pawn || !Capsule || !IsValid(Target) || Target->GetWorld() != World) return false;
	const FVector TargetLocation = Target->GetActorLocation();

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FCollisionQueryParams StartOverlapQuery(SCENE_QUERY_STAT(AgentLandmarkSpawnClearance), false, Pawn);
	FHitResult StartOverlapHit;
	const bool bStartsOverlapped = World->SweepSingleByChannel(StartOverlapHit, MoverLocation, MoverLocation,
		FQuat::Identity, ECC_Pawn, Capsule->GetCollisionShape(), StartOverlapQuery);
	if (bStartsOverlapped)
	{
		UE_LOG(LogAutonomousAgentAI, Log, TEXT("Grounded landmark mover starts overlapping %s at %s."),
			StartOverlapHit.GetActor() ? *StartOverlapHit.GetActor()->GetName() : TEXT("<no actor>"),
			*StartOverlapHit.ImpactPoint.ToCompactString());
	}
	FNavLocation Start;
	const FVector StartOnFloor = MoverLocation - FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
	if (!ProjectGroundedTarget(Navigation, StartOnFloor, AgentProperties, Start)) return false;

	TArray<FVector> TestedGoals;
	float BestPathLength = TNumericLimits<float>::Max();
	bool bFoundGoal = false;
	int32 ProjectedCandidates = 0;
	int32 InRangeCandidates = 0;
	int32 CompleteRoutes = 0;
	int32 ClearRoutes = 0;
	int32 BlockedRoutes = 0;
	int32 InspectionVisibleRoutes = 0;
	int32 OccludedRoutes = 0;
	FVector PreferredApproach = BuildGroundedResidentApproachPoint(Start.Location, TargetLocation);
	FVector PreferredDirection = PreferredApproach - TargetLocation;
	PreferredDirection.Z = 0.f;
	if (!PreferredDirection.Normalize()) PreferredDirection = FVector::ForwardVector;
	const float PreferredAngle = FMath::Atan2(PreferredDirection.Y, PreferredDirection.X);
	for (const float Radius : { 150.f, 200.f, 250.f, 325.f, 375.f })
	{
		for (int32 Side = 0; Side < 8; ++Side)
		{
			const float Angle = PreferredAngle + Side * (PI / 4.f);
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			const FVector DesiredApproach = TargetLocation + Direction * Radius;
			FNavLocation CandidateGoal;
			// Match the grounded target projection used elsewhere: a narrow single
			// projection can miss a nearby floor polygon around rocks, arches, or
			// map markers, even when a clear stand-off position is within range.
			if (!ProjectGroundedTarget(Navigation, DesiredApproach, AgentProperties, CandidateGoal)) continue;
			++ProjectedCandidates;
			if (TestedGoals.ContainsByPredicate([&CandidateGoal](const FVector& Existing)
				{ return FVector::DistSquared2D(Existing, CandidateGoal.Location) < FMath::Square(50.f); })) continue;
			TestedGoals.Add(CandidateGoal.Location);

			const FVector BodyCenter = CandidateGoal.Location + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
			if (FVector::DistSquared(BodyCenter, TargetLocation) >
				FMath::Square(IslandInteractionUtility::DefaultInteractionRange - IslandInteractionUtility::GroundedApproachRangeMargin)) continue;
			++InRangeCandidates;

			const UNavigationPath* Route = Navigation->FindPathToLocationSynchronously(
				World, Start.Location, CandidateGoal.Location, PathfindingContext ? PathfindingContext : Pawn);
			if (!Route || !Route->IsValid() || Route->IsPartial()) continue;
			++CompleteRoutes;
			if (Route->PathPoints.Num() < 2) continue;

			FHitResult FirstBlocker;
			int32 BlockedSegment = INDEX_NONE;
			if (!IsCapsulePathPhysicallyClear(World, Route, Capsule, Pawn, &BlockedSegment, &FirstBlocker))
			{
				++BlockedRoutes;
				if (BlockedRoutes == 1)
				{
					const UPrimitiveComponent* BlockingComponent = FirstBlocker.GetComponent();
					const UStaticMeshComponent* BlockingMesh = Cast<UStaticMeshComponent>(BlockingComponent);
					const UStaticMesh* StaticMesh = BlockingMesh ? BlockingMesh->GetStaticMesh() : nullptr;
					UE_LOG(LogAutonomousAgentAI, Log,
						TEXT("Landmark capsule sweep first blocked at segment %d near %s by %s (%s, mesh %s, affects-nav %d, pawn-response %d, bounds %s +/- %s)."),
						BlockedSegment, *FirstBlocker.ImpactPoint.ToCompactString(),
						FirstBlocker.GetActor() ? *FirstBlocker.GetActor()->GetName() : TEXT("<no actor>"),
						BlockingComponent ? *BlockingComponent->GetName() : TEXT("<no component>"),
						StaticMesh ? *StaticMesh->GetPathName() : TEXT("<no static mesh>"),
						BlockingComponent && BlockingComponent->CanEverAffectNavigation(),
						BlockingComponent ? static_cast<int32>(BlockingComponent->GetCollisionResponseToChannel(ECC_Pawn)) : -1,
						BlockingComponent ? *BlockingComponent->Bounds.Origin.ToCompactString() : TEXT("<unknown>"),
						BlockingComponent ? *BlockingComponent->Bounds.BoxExtent.ToCompactString() : TEXT("<unknown>"));
				}
				continue;
			}
			++ClearRoutes;
			if (!IslandInteractionUtility::CanInspectFromLocation(Pawn, Target, BodyCenter))
			{
				++OccludedRoutes;
				continue;
			}
			++InspectionVisibleRoutes;
			if (Route->GetPathLength() >= BestPathLength) continue;

			OutGoal = CandidateGoal;
			BestPathLength = Route->GetPathLength();
			bFoundGoal = true;
		}
	}
	UE_LOG(LogAutonomousAgentAI, Log,
		TEXT("Grounded landmark approach at %s: %d projected, %d in inspection range, %d complete routes, %d capsule-blocked, %d capsule-clear, %d visibility-blocked, %d inspection-visible; %s."),
		*TargetLocation.ToCompactString(), ProjectedCandidates, InRangeCandidates, CompleteRoutes, BlockedRoutes, ClearRoutes,
		OccludedRoutes, InspectionVisibleRoutes,
		bFoundGoal ? *FString::Printf(TEXT("selected %s at %.0f cm path length"), *OutGoal.Location.ToCompactString(), BestPathLength)
			: TEXT("no candidate qualified"));
	return bFoundGoal;
}
bool AAutonomousAgentAIController::FindGroundedClearanceWaypoint(UNavigationSystemV1* Navigation, UWorld* World,
	APawn* Pawn, const FVector& TargetLocation, FNavLocation& OutGoal, AActor* PathfindingContext)
{
	if (!Navigation || !World || !Pawn) return false;
	ANavigationData* NavData = Navigation->GetNavDataForProps(Pawn->GetNavAgentPropertiesRef());
	if (!NavData) return false;

	const FVector Origin = Pawn->GetActorLocation();
	const float StartingTargetDistance = FVector::Dist2D(Origin, TargetLocation);
	float BestScore = -TNumericLimits<float>::Max();
	float BestProgress = 0.f;
	int32 ClearRouteCount = 0;
	for (int32 Attempt = 0; Attempt < 96; ++Attempt)
	{
		FNavLocation Candidate;
		if (!Navigation->GetRandomReachablePointInRadius(Origin, 2500.f, Candidate, NavData) ||
			FVector::DistSquared2D(Origin, Candidate.Location) < FMath::Square(175.f)) continue;

		const UNavigationPath* Route = Navigation->FindPathToLocationSynchronously(
			World, Origin, Candidate.Location, PathfindingContext ? PathfindingContext : Pawn);
		if (!IsUsableWanderPath(Route, Origin, Candidate.Location) ||
			!IsWanderPathPhysicallyClear(World, Route, Pawn)) continue;

		++ClearRouteCount;
		const float TargetProgress = StartingTargetDistance - FVector::Dist2D(Candidate.Location, TargetLocation);
		// Prefer a genuinely useful detour toward the requested destination. A
		// shortest-hop waypoint can merely move a few metres around one blocker,
		// leaving the next physically clear route blocked by the same scenery.
		const float Score = TargetProgress - Route->GetPathLength() * 0.1f;
		if (Score <= BestScore) continue;
		BestScore = Score;
		BestProgress = TargetProgress;
		OutGoal = Candidate;
	}
	UE_LOG(LogAutonomousAgentAI, Log,
		TEXT("Grounded landmark recovery found %d capsule-clear staging routes; %s (target progress %.0f cm)."),
		ClearRouteCount, ClearRouteCount > 0
			? *FString::Printf(TEXT("selected %s"), *OutGoal.Location.ToCompactString())
			: TEXT("no safe staging waypoint"), BestProgress);
	return ClearRouteCount > 0;
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
	return IsCapsulePathPhysicallyClear(World, Path, Capsule, Pawn);
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
float AAutonomousAgentAIController::WanderPathTrailAffinityScore(const TArray<FVector>& PathPoints,
	const FIslandTrailLedger& TrailLedger)
{
	if (PathPoints.Num() < 2) return 0.f;

	float PathLength = 0.f;
	for (int32 Index = 1; Index < PathPoints.Num(); ++Index)
		PathLength += FVector::Dist2D(PathPoints[Index - 1], PathPoints[Index]);
	if (PathLength <= KINDA_SMALL_NUMBER) return 0.f;

	// Evenly sample the full polyline at half-cell intervals, with a hard cap so
	// a malformed or unusually long nav route cannot make candidate scoring costly.
	constexpr int32 MaxSamples = 256;
	const int32 NumSamples = FMath::Clamp(
		FMath::CeilToInt(PathLength / (FIslandTrailLedger::CellSize * 0.5f)) + 1, 2, MaxSamples);
	float FamiliaritySum = 0.f;
	int32 Segment = 1;
	float SegmentStartDistance = 0.f;
	float SegmentLength = FVector::Dist2D(PathPoints[0], PathPoints[1]);
	for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		const float DistanceAlongPath = PathLength * static_cast<float>(SampleIndex) / (NumSamples - 1);
		while (Segment < PathPoints.Num() - 1 &&
			(SegmentLength <= KINDA_SMALL_NUMBER || DistanceAlongPath > SegmentStartDistance + SegmentLength))
		{
			SegmentStartDistance += SegmentLength;
			++Segment;
			SegmentLength = FVector::Dist2D(PathPoints[Segment - 1], PathPoints[Segment]);
		}
		if (SegmentLength <= KINDA_SMALL_NUMBER) continue;
		const float Alpha = FMath::Clamp((DistanceAlongPath - SegmentStartDistance) / SegmentLength, 0.f, 1.f);
		const FVector Sample = FMath::Lerp(PathPoints[Segment - 1], PathPoints[Segment], Alpha);
		FamiliaritySum += UIslandTrailSubsystem::WearAmount(TrailLedger.StepsAt(Sample));
	}
	return FamiliaritySum / NumSamples;
}
float AAutonomousAgentAIController::WanderTrailAdjustedScore(float BaseScore, float TrailAffinity)
{
	if (!FMath::IsFinite(BaseScore) || !FMath::IsFinite(TrailAffinity)) return 0.f;
	return BaseScore * (1.f + FMath::Clamp(TrailAffinity, 0.f, 1.f) * WanderTrailAffinityWeight);
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
			Rest->QueueSleepExperience(MemoryText, { TEXT("sleep"), TEXT("rest"), TEXT("inn"), TEXT("shelter-geometry") });
		}
		ReportAction(bMemoryWillBeRecorded
			? TEXT("Settled to sleep beside the tagged inn bed. A roof was overhead and the enclosing walls passed the current indoor-geometry check; the lived rest will be added to your memory when the rest interval completes. Waking early will cancel that note. That evidence does not promise warmth, complete dryness, comfort, or recovery; ordinary thoughts pause during sleep.")
			: TEXT("Settled to sleep beside the tagged inn bed. A roof was overhead and the enclosing walls passed the current indoor-geometry check, but no memory store is attached to this body. That evidence does not promise warmth, complete dryness, comfort, or recovery; ordinary thoughts pause during sleep."));
	}
	else if (BedAtRest)
	{
		ReportAction(TEXT("Settled to sleep near the tagged inn bed, but the roof-and-wall enclosure did not pass the current check. This will not be recorded as sheltered inn rest; ordinary thoughts pause during sleep."));
	}
	else if (Cast<ARavenAgentAIController>(this))
	{
		AActor* RestingNestSite = RequestedPerch;
		if (!RestingNestSite)
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
				if (It->ActorHasTag(TEXT("RavenPerch")) && It->ActorHasTag(TEXT("RavenNestSite")) &&
					FVector::Dist2D(Body->GetActorLocation(), It->GetActorLocation()) <= 250.f &&
					FMath::Abs(Body->GetActorLocation().Z - It->GetActorLocation().Z) <= 250.f)
				{
					RestingNestSite = *It;
					break;
				}
		const UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
		const FName NestSiteTag = RestingNestSite && RestingNestSite->Tags.Num() > 0 ? RestingNestSite->Tags[0] : NAME_None;
		const FIslandNestRecord* Nest = WorldState && RestingNestSite &&
			RestingNestSite->ActorHasTag(TEXT("RavenPerch")) && RestingNestSite->ActorHasTag(TEXT("RavenNestSite"))
			? WorldState->FindNest(NestSiteTag) : nullptr;
		if (Nest)
		{
			UAgentMemoryComponent* Memory = Body->FindComponentByClass<UAgentMemoryComponent>();
			if (Memory)
			{
				const bool bHelpedWeave = Nest->Builders.Contains(Memory->GetResolvedAgentId());
				const FString Experience = FString::Printf(TEXT("I slept at %s woven nest at %s; it had %d of %d layers. This is a place I rested, not proof of shelter or safety."),
					bHelpedWeave ? TEXT("my") : TEXT("a"), *NestSiteTag.ToString(), Nest->Layers, UIslandWorldStateSubsystem::MaxNestLayers);
				Rest->QueueSleepExperience(Experience, { TEXT("sleep"), TEXT("rest"), TEXT("nest"), TEXT("roost") });
			}
			ReportAction(Memory
				? TEXT("Settled to sleep at the persistent woven nest beneath this roost. A memory of this rest will be kept only if the sleep finishes; waking early cancels it. The nest's presence is not evidence of shelter, safety, comfort, or recovery; ordinary thoughts pause during sleep.")
				: TEXT("Settled to sleep at the persistent woven nest beneath this roost, but this body has no memory store. The nest's presence is not evidence of shelter, safety, comfort, or recovery; ordinary thoughts pause during sleep."));
		}
		else
			ReportAction(RequestedPerch
				? TEXT("Settled to sleep at the tagged perch. Ordinary thoughts pause during sleep; no sheltered inn rest was verified.")
				: TEXT("Settled to sleep. Ordinary thoughts pause during sleep; no sheltered inn rest was verified."));
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
	if (const APawn* Observer = GetPawn())
	{
		for (TActorIterator<AIslandTidepoolMinnows> It(GetWorld()); It; ++It)
		{
			const FString WildlifeCue = It->DescribeRecentSurfaceBreak(Observer->GetActorLocation());
			if (WildlifeCue.IsEmpty()) continue;
			Result += TEXT(" Nearby transient wildlife cue: ") + WildlifeCue;
			break;
		}
	}
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
	if (bPendingGroundLandmarkRecoveryMove)
	{
		const FName TargetActorName = PendingGroundMoveTargetName;
		const FName TargetTag = PendingGroundMoveTargetTag;
		bPendingGroundLandmarkRecoveryMove = false;
		if (Result.IsSuccess())
		{
			AActor* Target = nullptr;
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
				if (It->GetFName() == TargetActorName) { Target = *It; break; }
			if (Target && !TargetTag.IsNone())
			{
				UE_LOG(LogAutonomousAgentAI, Log,
					TEXT("Reached capsule-clear staging ground for %s; recomputing the landmark approach from the new position."),
					*TargetTag.ToString());
				FAgentDecision Retry;
				Retry.ActionType = EAgentActionType::MoveTo;
				Retry.ActionTarget = TargetTag.ToString();
				ActOnDecision(Retry);
				return;
			}
		}
		PendingGroundMoveTargetName = NAME_None;
		PendingGroundMoveTargetTag = NAME_None;
		GroundedLandmarkRecoveryAttempts = 0;
		ReportAction(Result.IsSuccess()
			? TEXT("Movement did not resume: the landmark disappeared before the clear approach could continue.")
			: TEXT("The clear-ground detour was blocked or cancelled; choose a different destination."));
		return;
	}
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
			PendingGroundMoveTargetTag = NAME_None;
			GroundedLandmarkRecoveryAttempts = 0;
			ReportAction(FString::Printf(
				TEXT("The route to the projected ground point was blocked, but you stopped within clear inspection range of %s. You may inspect or interact from this reachable approach; you did not reach the marker itself."),
				*TargetTag.ToString()));
			return;
		}
	}
	PendingGroundMoveTargetName = NAME_None;
	PendingGroundMoveTargetTag = NAME_None;
	GroundedLandmarkRecoveryAttempts = 0;
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
		Memory ? Memory->GetResolvedAgentId() : Body->GetName(), UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()), bChanged, Decision.Influence);
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

	GroundedLandmarkRecoveryAttempts = 0;
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
	PendingGroundMoveTargetTag = NAME_None;
	bPendingGroundLandmarkRecoveryMove = false;

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
			float BestTrailAffinityScore = 0.f;
			const UIslandTrailSubsystem* Trail = GetWorld()->GetSubsystem<UIslandTrailSubsystem>();
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
				const float BaseWanderScore = WanderFrontierScore(Candidate.Location, WanderExplorationOrigin,
					FurthestWanderDistance, RecentWanderDestinations) + LandmarkProgressScore;
				const float TrailAffinity = Trail
					? WanderPathTrailAffinityScore(Route->PathPoints, Trail->GetLedger())
					: 0.f;
				const float WanderScore = WanderTrailAdjustedScore(BaseWanderScore, TrailAffinity);
				if (bFoundFullRoute && WanderScore <= BestWanderScore) continue;
				Destination = Candidate;
				bFoundFullRoute = true;
				BestWanderScore = WanderScore;
				BestLandmarkProgressScore = LandmarkProgressScore;
				BestTrailAffinityScore = TrailAffinity;
				BestWanderPathPoints = Route->PathPoints;
			}
			if (bFoundFullRoute)
			{
				FString PathDescription;
				for (const FVector& PathPoint : BestWanderPathPoints)
					PathDescription += (PathDescription.IsEmpty() ? TEXT("") : TEXT(" -> ")) + PathPoint.ToCompactString();
				UE_LOG(LogAutonomousAgentAI, Log,
					TEXT("%s selected wander destination %s with %d visible nearby landmarks; landmark progress %.0f cm; worn-trail affinity %.0f%%; rejected %d capsule-blocked paths; nav path: %s."),
					*GetName(), *Destination.Location.ToCompactString(), VisibleLandmarks.Num(),
					BestLandmarkProgressScore / WanderLandmarkProgressWeight, BestTrailAffinityScore * 100.f,
					BlockedPathCount, *PathDescription);
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

			// Grounded residents should approach a landmark from clear ground rather than
			// walking toward its marker, which may sit inside the landmark's collision.
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			FNavLocation GroundGoal;
			const ACharacter* MoverCharacter = Cast<ACharacter>(ControlledPawn);
			const bool bGroundedWalker = MoverCharacter && MoverCharacter->GetCharacterMovement() &&
				MoverCharacter->GetCharacterMovement()->IsMovingOnGround();
			const bool bHasGroundedApproach = bGroundedWalker
				? FindGroundedLandmarkApproachGoal(NavSys, GetWorld(), ControlledPawn, ControlledPawn->GetActorLocation(),
					TargetActor, ControlledPawn->GetNavAgentPropertiesRef(), GroundGoal, ControlledPawn)
				: ProjectGroundedTarget(NavSys, TargetActor->GetActorLocation(), ControlledPawn->GetNavAgentPropertiesRef(), GroundGoal);
			if (!bHasGroundedApproach)
			{
				FNavLocation StagingGoal;
				if (bGroundedWalker && GroundedLandmarkRecoveryAttempts < 3 &&
					FindGroundedClearanceWaypoint(NavSys, GetWorld(), ControlledPawn, TargetActor->GetActorLocation(),
						StagingGoal, ControlledPawn))
				{
					const EPathFollowingRequestResult::Type RecoveryResult = MoveToLocation(StagingGoal.Location,
						WanderAcceptanceRadius, true, true, false, false, nullptr, false);
					if (RecoveryResult == EPathFollowingRequestResult::RequestSuccessful)
					{
						PendingGroundMoveTargetName = TargetActor->GetFName();
						PendingGroundMoveTargetTag = TargetTag;
						bPendingGroundLandmarkRecoveryMove = true;
						++GroundedLandmarkRecoveryAttempts;
						ReportAction(TEXT("The direct landmark route conflicts with nearby scenery. Moving to a physically clear nearby point to find a path around it; no interaction has happened yet."));
						break;
					}
				}
				GroundedLandmarkRecoveryAttempts = 0;
				ReportAction(bGroundedWalker
					? TEXT("Movement failed: no physically clear, complete ground route reaches an inspectable approach to that landmark. Choose another destination.")
					: TEXT("Movement failed: no walkable ground near that marker. Choose another destination."));
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
			PendingGroundMoveTargetTag = TargetTag;
			const EPathFollowingRequestResult::Type Result = MoveToLocation(GroundGoal.Location,
				50.f, true, true, false, true, nullptr, false);
			if (Result != EPathFollowingRequestResult::RequestSuccessful)
			{
				PendingGroundMoveTargetName = NAME_None;
				PendingGroundMoveTargetTag = NAME_None;
			}
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
