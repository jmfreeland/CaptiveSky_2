#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentLingeringTest, "CaptiveSky2.Agent.Lingering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentLingeringTest::RunTest(const FString& Parameters)
{
	// No world or model requests: only the counting that decides when a resident is told it has not moved on.
	const FVector Here(0.f, 0.f, 0.f);
	TArray<FVector> Spots;
	TestEqual(TEXT("No history means no lingering"), UAgentBrainComponent::CountLingeringDecisions(Spots, Here, 1200.f), 0);
	Spots = { FVector(5000.f, 0.f, 0.f), FVector(300.f, 0.f, 0.f), FVector(-400.f, 200.f, 0.f), FVector(0.f, 900.f, 800.f) };
	TestEqual(TEXT("Counts back until a spot lies elsewhere; height is ignored"), UAgentBrainComponent::CountLingeringDecisions(Spots, Here, 1200.f), 3);
	Spots.Add(FVector(1300.f, 0.f, 0.f));
	TestEqual(TEXT("A recent move away resets the count"), UAgentBrainComponent::CountLingeringDecisions(Spots, Here, 1200.f), 0);
	return true;
}
