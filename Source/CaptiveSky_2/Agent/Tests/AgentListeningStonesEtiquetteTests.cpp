#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentListeningStonesEtiquetteTest,
	"CaptiveSky2.Agent.ListeningStonesEtiquette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentListeningStonesEtiquetteTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A resident who learned the practice yields to a different quiet listener"),
		AAutonomousAgentAIController::ShouldUseListeningStonesEtiquette(true, true, false));
	TestFalse(TEXT("Without witnessed practice, the resident keeps its ordinary choice"),
		AAutonomousAgentAIController::ShouldUseListeningStonesEtiquette(false, true, false));
	TestFalse(TEXT("No quiet listener means there is no reason to yield"),
		AAutonomousAgentAIController::ShouldUseListeningStonesEtiquette(true, false, false));
	TestFalse(TEXT("The resident already listening does not yield to itself"),
		AAutonomousAgentAIController::ShouldUseListeningStonesEtiquette(true, true, true));

	const FVector Stones(-1000.f, 500.f, 40.f);
	const FVector QuietResident = Stones + FVector(300.f, 0.f, 0.f);
	const FVector Visitor = Stones + FVector(0.f, -1200.f, 0.f);
	const FVector WaitPoint = AAutonomousAgentAIController::BuildListeningStonesWaitPoint(
		Stones, QuietResident, Visitor, AAutonomousAgentAIController::ListeningStonesWaitStandOffDistance);
	TestTrue(TEXT("The wait point is on the opposite outer edge from the listener"),
		FVector::Dist2D(WaitPoint, Stones) >= 849.f && WaitPoint.X < Stones.X);
	TestTrue(TEXT("The edge wait stays comfortably beyond the listener"),
		FVector::Dist2D(WaitPoint, QuietResident) >= 1100.f);
	TestTrue(TEXT("The wait point remains on the landmark's ground plane"), FMath::IsNearlyEqual(WaitPoint.Z, Stones.Z));

	const FVector VisitorSideFallback = AAutonomousAgentAIController::BuildListeningStonesWaitPoint(
		Stones, Stones, Visitor, 850.f);
	TestTrue(TEXT("If the listener is at the marker, the fallback remains on the visitor's side"),
		VisitorSideFallback.Y < Stones.Y);
	TestTrue(TEXT("A non-positive stand-off does not invent a new location"),
		AAutonomousAgentAIController::BuildListeningStonesWaitPoint(Stones, QuietResident, Visitor, 0.f).Equals(Stones));
	return true;
}
