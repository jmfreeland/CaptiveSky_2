#include "Misc/AutomationTest.h"
#include "AgentModelTier.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentModelTierTest, "CaptiveSky2.Agent.ModelTier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentModelTierTest::RunTest(const FString& Parameters)
{
	using namespace AgentModelTier;

	FTurnFacts Quiet;
	TestFalse(TEXT("Nothing near and nobody speaking is a light turn"), NeedsFullModel(Quiet));

	FTurnFacts Spoken;
	Spoken.bSomeoneAddressedResident = true;
	TestTrue(TEXT("Being spoken to needs the full model"), NeedsFullModel(Spoken));

	FTurnFacts Company;
	Company.NearestBeingCm = BeingRangeCm - 1.f;
	TestTrue(TEXT("Another being within earshot needs the full model"), NeedsFullModel(Company));
	Company.NearestBeingCm = BeingRangeCm + 1.f;
	TestFalse(TEXT("A being farther off does not"), NeedsFullModel(Company));

	FTurnFacts Reach;
	Reach.NearestAffordanceCm = AffordanceRangeCm - 1.f;
	TestTrue(TEXT("Something within reach to act on needs the full model"), NeedsFullModel(Reach));
	Reach.NearestAffordanceCm = AffordanceRangeCm + 1.f;
	TestFalse(TEXT("Something only in the distance does not"), NeedsFullModel(Reach));

	FTurnFacts Building;
	Building.bHasBuildOptions = true;
	TestTrue(TEXT("An offered build needs the full model"), NeedsFullModel(Building));

	TestTrue(TEXT("Idle is a light action"), IsLightAction(EAgentActionType::Idle));
	TestTrue(TEXT("Wander is a light action"), IsLightAction(EAgentActionType::Wander));
	TestTrue(TEXT("Moving is a light action"), IsLightAction(EAgentActionType::MoveTo));
	TestTrue(TEXT("Sleeping is a light action"), IsLightAction(EAgentActionType::Sleep));
	TestFalse(TEXT("Speaking is not"), IsLightAction(EAgentActionType::Speak));
	TestFalse(TEXT("Interacting is not"), IsLightAction(EAgentActionType::Interact));
	TestFalse(TEXT("Building is not"), IsLightAction(EAgentActionType::Build));

	const FString Prompt = BuildLightSystemPrompt(TEXT("You are Aster."), TEXT("- [2026-09-28 20:51] I walked to the cairn.\n"));
	TestTrue(TEXT("The light prompt names the resident"), Prompt.Contains(TEXT("You are Aster.")));
	TestTrue(TEXT("The light prompt offers only the four quiet actions"), Prompt.Contains(TEXT("idle|move_to|wander|sleep")));
	TestFalse(TEXT("The light prompt does not offer speech"), Prompt.Contains(TEXT("|speak")));
	TestTrue(TEXT("The light prompt carries recent memories"), Prompt.Contains(TEXT("I walked to the cairn.")));
	TestTrue(TEXT("The light prompt stays compact"), Prompt.Len() < 1500);

	TestEqual(TEXT("Timestamps are trimmed to the minute"), CompactTimestamp(FDateTime(2026, 9, 28, 20, 51, 47, 818)), FString(TEXT("2026-09-28 20:51")));
	return true;
}
