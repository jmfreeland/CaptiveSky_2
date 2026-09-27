#include "Misc/AutomationTest.h"
#include "AgentSocialComponent.h"
#include "HAL/PlatformTime.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentSocialPacingTest, "CaptiveSky2.Agent.SocialPacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentSocialPacingTest::RunTest(const FString& Parameters)
{
	const UAgentSocialComponent* Defaults = GetDefault<UAgentSocialComponent>();
	if (!TestNotNull(TEXT("Social component defaults exist"), Defaults)) return false;

	TestEqual(TEXT("Autonomous conversations remain bounded to four spoken lines"), Defaults->MaximumConversationTurns, 4);
	TestEqual(TEXT("Pair cooldown gives residents five minutes between automatic exchanges"), Defaults->ConversationCooldownSeconds, 300.f);
	TestEqual(TEXT("An old short Blueprint cooldown cannot restore rapid exchanges"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(45.f), 300.f);
	TestEqual(TEXT("Zero cannot disable the autonomous social pause"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(0.f), 300.f);
	TestEqual(TEXT("Longer designer-authored cooldowns remain configurable"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(420.f), 420.f);
	TestEqual(TEXT("Non-finite overrides fall back to the safe five-minute pause"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(std::numeric_limits<float>::quiet_NaN()), 300.f);

	UAgentSocialComponent* Social = NewObject<UAgentSocialComponent>();
	TestEqual(TEXT("A resident with no previous exchange has no pair cooldown"),
		Social->GetConversationCooldownRemainingWith(TEXT("aster")), 0.f);
	Social->CooldownUntilByAgentId.Add(TEXT("aster"), FPlatformTime::Seconds() + 90.0);
	const float Remaining = Social->GetConversationCooldownRemainingWith(TEXT("aster"));
	TestTrue(TEXT("The pair cooldown exposes remaining real time to the resident's situation summary"),
		Remaining > 89.f && Remaining <= 90.f);
	Social->CooldownUntilByAgentId.Add(TEXT("aster"), FPlatformTime::Seconds() - 1.0);
	TestEqual(TEXT("An elapsed pair cooldown is reported as zero"),
		Social->GetConversationCooldownRemainingWith(TEXT("aster")), 0.f);
	return true;
}
