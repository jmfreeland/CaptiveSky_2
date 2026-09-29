#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentPlacesTest, "CaptiveSky2.Agent.Places",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentPlacesTest::RunTest(const FString& Parameters)
{
	// No world or model requests: only the bookkeeping that decides which remembered curios a resident is reminded of.
	TArray<UAgentBrainComponent::FRememberedPlace> Places;
	UAgentBrainComponent::AddRememberedPlace(Places, TEXT("PaleStone_1"), TEXT("a small pale stone"), FVector(-4000.f, 0.f, 0.f));
	UAgentBrainComponent::AddRememberedPlace(Places, TEXT("Cairn"), TEXT("the small cairn"), FVector(2000.f, 0.f, 0.f));
	UAgentBrainComponent::AddRememberedPlace(Places, TEXT("Seedpod"), TEXT("a strange pod"), FVector(500.f, 0.f, 0.f));
	UAgentBrainComponent::AddRememberedPlace(Places, TEXT("Cairn"), TEXT("the small cairn"), FVector(2100.f, 0.f, 0.f));
	TestEqual(TEXT("Remembering a place again updates it rather than duplicating it"), Places.Num(), 3);

	const FVector Here = FVector::ZeroVector;
	FString Text = UAgentBrainComponent::DescribeRememberedPlaces(Places, Here, {}, 1500.f, 3);
	TestTrue(TEXT("Cairn is offered with its updated distance"), Text.Contains(TEXT("the small cairn, about 21 metres away")));
	TestTrue(TEXT("Farther stone is offered too"), Text.Contains(TEXT("move_to target: PaleStone_1")));
	TestFalse(TEXT("A place within the notice distance is not listed"), Text.Contains(TEXT("Seedpod")));
	TestTrue(TEXT("Nearest place comes first"), Text.Find(TEXT("Cairn")) < Text.Find(TEXT("PaleStone_1")));

	Text = UAgentBrainComponent::DescribeRememberedPlaces(Places, Here, { FName(TEXT("Cairn")) }, 1500.f, 3);
	TestFalse(TEXT("A place in view right now is not repeated as a memory"), Text.Contains(TEXT("Cairn")));
	Text = UAgentBrainComponent::DescribeRememberedPlaces(Places, Here, {}, 1500.f, 1);
	TestFalse(TEXT("The list is capped"), Text.Contains(TEXT("PaleStone_1")));

	TArray<UAgentBrainComponent::FRememberedPlace> Many;
	for (int32 Index = 0; Index < 20; ++Index)
		UAgentBrainComponent::AddRememberedPlace(Many, FName(*FString::Printf(TEXT("PaleStone_%d"), Index)), TEXT("a small pale stone"), FVector::ZeroVector);
	TestEqual(TEXT("Only the most recent places are kept"), Many.Num(), 12);
	TestEqual(TEXT("The oldest are dropped first"), Many[0].Target, FName(TEXT("PaleStone_8")));
	return true;
}
