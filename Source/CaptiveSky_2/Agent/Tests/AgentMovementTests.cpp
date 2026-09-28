#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"

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
	return true;
}
