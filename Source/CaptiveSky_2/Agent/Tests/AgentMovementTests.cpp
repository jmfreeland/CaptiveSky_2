#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "RavenAgentAIController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentMovementTest, "CaptiveSky2.Agent.ResidentApproach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentMovementTest::RunTest(const FString& Parameters)
{
	const float StandOff = AAutonomousAgentAIController::ResidentApproachStandOffDistance;
	TestTrue(TEXT("The approach stand-off exceeds the combined radius of two placeholder capsules"), StandOff > 2.f * 42.f);
	TestEqual(TEXT("The raven's approach height is explicit and bounded"),
		AAutonomousAgentAIController::ResidentApproachAltitudeOffset, 180.f);

	const FVector Target(100.f, -50.f, 20.f);
	const FVector EastApproach = AAutonomousAgentAIController::BuildResidentApproachPoint(Target + FVector(1000.f, 0.f, 0.f), Target);
	TestTrue(TEXT("A walker/raven stops outside the target centre along its arrival side"),
		FMath::IsNearlyEqual(FVector::Dist2D(EastApproach, Target), StandOff));
	TestTrue(TEXT("The approach point preserves a small aerial offset"),
		FMath::IsNearlyEqual(EastApproach.Z - Target.Z, AAutonomousAgentAIController::ResidentApproachAltitudeOffset));
	TestTrue(TEXT("The approach side faces the mover"), FVector::DotProduct(EastApproach - Target, FVector::ForwardVector) > 0.f);

	const FVector WestApproach = AAutonomousAgentAIController::BuildResidentApproachPoint(Target + FVector(-1000.f, 0.f, 0.f), Target);
	TestTrue(TEXT("The approach side updates when the mover is on the opposite side"),
		FVector::Dist2D(WestApproach, Target) > 0.f && FVector::DotProduct(WestApproach - Target, FVector::ForwardVector) < 0.f);

	const FVector CoincidentApproach = AAutonomousAgentAIController::BuildResidentApproachPoint(Target, Target);
	TestTrue(TEXT("Coincident residents receive a deterministic nonzero fallback direction"),
		FMath::IsNearlyEqual(FVector::Dist2D(CoincidentApproach, Target), StandOff));

	const FVector GroundMover(0.f, 0.f, 120.f);
	const FVector PerchedRaven(1000.f, 0.f, 2400.f);
	const FVector GroundedApproach = AAutonomousAgentAIController::BuildGroundedResidentApproachPoint(GroundMover, PerchedRaven);
	TestTrue(TEXT("Ground fallback keeps a stand-off from an airborne resident"),
		FMath::IsNearlyEqual(FVector::Dist2D(GroundedApproach, PerchedRaven), StandOff));
	TestEqual(TEXT("Ground fallback stays on the mover's walkable plane"), GroundedApproach.Z, GroundMover.Z);
	TestTrue(TEXT("Ground fallback approaches from the mover's side"),
		FVector::DotProduct(GroundedApproach - PerchedRaven, FVector::ForwardVector) < 0.f);
	const TArray<FVector> GroundedCandidates = AAutonomousAgentAIController::BuildGroundedResidentApproachCandidates(GroundMover, PerchedRaven);
	TestEqual(TEXT("Grounded resident approach checks eight sides around a perched resident"), GroundedCandidates.Num(), 8);
	if (GroundedCandidates.Num() == 8)
	{
		TestTrue(TEXT("The mover-facing ground stand-off is considered first"), GroundedCandidates[0].Equals(GroundedApproach));
		for (int32 Index = 0; Index < GroundedCandidates.Num(); ++Index)
		{
			TestTrue(*FString::Printf(TEXT("Grounded candidate %d remains at the shared stand-off radius"), Index),
				FMath::IsNearlyEqual(FVector::Dist2D(GroundedCandidates[Index], PerchedRaven), StandOff, 0.1f));
			TestEqual(*FString::Printf(TEXT("Grounded candidate %d stays on the mover's plane"), Index),
				GroundedCandidates[Index].Z, GroundMover.Z);
			for (int32 OtherIndex = Index + 1; OtherIndex < GroundedCandidates.Num(); ++OtherIndex)
				TestTrue(*FString::Printf(TEXT("Grounded candidates %d and %d are distinct"), Index, OtherIndex),
					!GroundedCandidates[Index].Equals(GroundedCandidates[OtherIndex], 1.f));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentBlockedGroundMoveTest, "CaptiveSky2.Agent.BlockedGroundMoveApproach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentBlockedGroundMoveTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Blocked-move fixture world created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Observer = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AActor* Target = World->SpawnActor<AActor>(FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>(Spawn);
	if (!TestNotNull(TEXT("Inspection-capable resident spawned"), Observer) ||
		!TestNotNull(TEXT("Tagged landmark spawned"), Target) ||
		!TestNotNull(TEXT("Resident controller spawned"), Controller))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Target->Tags = { TEXT("IslandLandmark"), TEXT("InnDoorLantern") };
	USceneComponent* TargetRoot = NewObject<USceneComponent>(Target);
	Target->SetRootComponent(TargetRoot);
	TargetRoot->RegisterComponent();
	Target->SetActorLocation(FVector(200.f, 0.f, 0.f));
	TestTrue(TEXT("Landmark fixture retains its intended target location"), Target->GetActorLocation().Equals(FVector(200.f, 0.f, 0.f)));
	World->BeginPlay();
	Controller->Possess(Observer);

	auto ReportBlockedMove = [Controller, Target]()
	{
		Controller->PendingGroundMoveTargetName = Target->GetFName();
		Controller->OnMoveCompleted(FAIRequestID(), FPathFollowingResult(EPathFollowingResult::Blocked, FPathFollowingResultFlags::None));
	};
	ReportBlockedMove();
	TestTrue(TEXT("A blocked resident who can clearly inspect the landmark gets a reachable-approach outcome"),
		Controller->DescribeActionState().Contains(TEXT("within clear inspection range")) &&
		Controller->DescribeActionState().Contains(TEXT("did not reach the marker itself")));

	Observer->SetActorLocation(FVector(-300.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	ReportBlockedMove();
	TestTrue(TEXT("A blocked resident beyond four metres receives the ordinary failure, not the approach fallback"),
		Controller->DescribeActionState().Contains(TEXT("Movement did not complete")) &&
		!Controller->DescribeActionState().Contains(TEXT("within clear inspection range")));

	AActor* Occluder = World->SpawnActor<AActor>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Visibility blocker spawned"), Occluder))
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(Occluder);
		Occluder->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(15.f, 100.f, 100.f));
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Occluder->SetActorLocation(FVector(100.f, 0.f, 0.f));
		Observer->SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
		ReportBlockedMove();
		TestTrue(TEXT("A blocked resident behind an occlusion receives the ordinary failure"),
			Controller->DescribeActionState().Contains(TEXT("Movement did not complete")) &&
			!Controller->DescribeActionState().Contains(TEXT("within clear inspection range")));
	}
	Controller->UnPossess();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentWanderPathTest, "CaptiveSky2.Agent.ResidentWanderPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentWanderPathTest::RunTest(const FString& Parameters)
{

	const FVector WanderOrigin(0.f, 0.f, 0.f);
	const FVector WanderGoal(500.f, 0.f, 0.f);
	auto MakePath = [&WanderOrigin, &WanderGoal](bool bPartial)
	{
		FNavPathSharedPtr NativePath = MakeShared<FNavigationPath>(TArray<FVector>{ WanderOrigin, WanderGoal });
		NativePath->MarkReady();
		NativePath->DoneUpdating(ENavPathUpdateType::Custom);
		NativePath->SetIsPartial(bPartial);
		UNavigationPath* Path = NewObject<UNavigationPath>();
		Path->SetPath(NativePath);
		return Path;
	};
	TestTrue(TEXT("Wander accepts a valid, complete route far enough to constitute a choice"),
		AAutonomousAgentAIController::IsUsableWanderPath(MakePath(false), WanderOrigin, WanderGoal));
	TestFalse(TEXT("Wander rejects a partial route instead of entering a likely movement timeout"),
		AAutonomousAgentAIController::IsUsableWanderPath(MakePath(true), WanderOrigin, WanderGoal));
	TestFalse(TEXT("Wander rejects a route that only returns to its current position"),
		AAutonomousAgentAIController::IsUsableWanderPath(MakePath(false), WanderOrigin, WanderOrigin));
	TestFalse(TEXT("Wander rejects a missing route"),
		AAutonomousAgentAIController::IsUsableWanderPath(nullptr, WanderOrigin, WanderGoal));
	const TArray<FVector> RecentDestinations = { FVector::ZeroVector, FVector(1000.f, 0.f, 0.f) };
	TestTrue(TEXT("Wander gives a higher score to a candidate far from recent destinations"),
		AAutonomousAgentAIController::WanderNoveltyScore(FVector(0.f, 1500.f, 0.f), RecentDestinations) >
		AAutonomousAgentAIController::WanderNoveltyScore(FVector(0.f, 100.f, 0.f), RecentDestinations));
	TestEqual(TEXT("A new resident's first wander remains unbiased by nonexistent history"),
		AAutonomousAgentAIController::WanderNoveltyScore(WanderGoal, {}), 0.f);
	const TArray<FVector> FrontierRecentDestinations = { FVector::ZeroVector, FVector(200.f, 0.f, 0.f), FVector(0.f, 500.f, 0.f) };
	TestTrue(TEXT("An explicit wander prefers extending a new frontier over circling familiar ground"),
		AAutonomousAgentAIController::WanderFrontierScore(FVector(0.f, 1800.f, 0.f), FVector::ZeroVector,
			1200.f, FrontierRecentDestinations) >
		AAutonomousAgentAIController::WanderFrontierScore(FVector(100.f, 0.f, 0.f), FVector::ZeroVector,
			1200.f, FrontierRecentDestinations));
	TestTrue(TEXT("Recent-place novelty still guides wander choices without outward frontier progress"),
		AAutonomousAgentAIController::WanderFrontierScore(FVector(0.f, 1500.f, 0.f), FVector::ZeroVector,
			2000.f, RecentDestinations) >
		AAutonomousAgentAIController::WanderFrontierScore(FVector(0.f, 100.f, 0.f), FVector::ZeroVector,
			2000.f, RecentDestinations));
	const TArray<FVector> VisibleLandmarks = { FVector(2000.f, 0.f, 0.f) };
	TestTrue(TEXT("Wander gives an equal-quality candidate a gentle pull toward a visible landmark"),
		AAutonomousAgentAIController::WanderLandmarkProgressScore(FVector(1500.f, 0.f, 0.f), FVector::ZeroVector, VisibleLandmarks) >
		AAutonomousAgentAIController::WanderLandmarkProgressScore(FVector(-1500.f, 0.f, 0.f), FVector::ZeroVector, VisibleLandmarks));
	TestEqual(TEXT("Wander is not biased toward landmarks that are not visible to the resident"),
		AAutonomousAgentAIController::WanderLandmarkProgressScore(FVector(1500.f, 0.f, 0.f), FVector::ZeroVector, {}), 0.f);
	TestEqual(TEXT("A candidate moving away from a visible landmark receives no curiosity bonus"),
		AAutonomousAgentAIController::WanderLandmarkProgressScore(FVector(-1500.f, 0.f, 0.f), FVector::ZeroVector, VisibleLandmarks), 0.f);
	TestTrue(TEXT("Wander stop tolerance allows capsule overlap at navigation endpoints"),
		AAutonomousAgentAIController::WanderAcceptanceRadius > 0.f);
	return true;
}
