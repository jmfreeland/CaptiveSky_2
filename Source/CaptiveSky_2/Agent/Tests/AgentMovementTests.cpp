#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "NavigationPath.h"

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
	TestTrue(TEXT("Wander stop tolerance allows capsule overlap at navigation endpoints"),
		AAutonomousAgentAIController::WanderAcceptanceRadius > 0.f);
	return true;
}
