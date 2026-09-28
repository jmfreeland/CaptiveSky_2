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

	UAgentSocialComponent* RavenSocial = NewObject<UAgentSocialComponent>();
	UAgentSocialComponent* InnkeeperSocial = NewObject<UAgentSocialComponent>();
	const FString ConversationId = TEXT("social-pacing-fixture");
	TestTrue(TEXT("A spontaneous exchange reserves both residents before delivering its opener"),
		Social->TryReserveAutomaticConversation(RavenSocial, TEXT("aster"), TEXT("raven"), ConversationId));
	TestTrue(TEXT("An engaged resident exposes its busy state to the autonomous-thought scheduler"),
		Social->IsAutomaticConversationActive());
	TestTrue(TEXT("The counterpart sees the same active exchange reservation"),
		RavenSocial->IsReservedFor(TEXT("aster"), ConversationId));
	TestFalse(TEXT("The pair cannot open a simultaneous reverse exchange"),
		RavenSocial->TryReserveAutomaticConversation(Social, TEXT("raven"), TEXT("aster"), TEXT("reverse")));
	TestFalse(TEXT("A resident already speaking cannot begin a second exchange with another resident"),
		Social->TryReserveAutomaticConversation(InnkeeperSocial, TEXT("aster"), TEXT("innkeeper"), TEXT("overlap")));

	Social->ReleaseAutomaticConversation(RavenSocial, ConversationId);
	TestFalse(TEXT("Ending an exchange releases the initiator reservation"),
		Social->IsReservedFor(TEXT("raven"), ConversationId));
	TestFalse(TEXT("Ending an exchange releases the counterpart reservation"),
		RavenSocial->IsReservedFor(TEXT("aster"), ConversationId));
	TestFalse(TEXT("Ending an exchange makes the resident eligible for later independent thoughts"),
		Social->IsAutomaticConversationActive());

	TestTrue(TEXT("A fresh pair can begin after its prior reservation is released"),
		Social->TryReserveAutomaticConversation(RavenSocial, TEXT("aster"), TEXT("raven"), ConversationId));
	Social->CooldownUntilByAgentId.Add(TEXT("raven"), FPlatformTime::Seconds() + 300.0);
	RavenSocial->CooldownUntilByAgentId.Add(TEXT("aster"), FPlatformTime::Seconds() + 300.0);
	Social->ReleaseAutomaticConversation(RavenSocial, ConversationId);
	TestFalse(TEXT("The mutual cooldown blocks an immediate repeat exchange"),
		Social->TryReserveAutomaticConversation(RavenSocial, TEXT("aster"), TEXT("raven"), TEXT("repeat")));

	Social->MaximumConversationTurns = 4;
	TestFalse(TEXT("The third line may receive a reply within the four-line limit"),
		Social->IsConversationAtTurnLimit(3));
	TestTrue(TEXT("The fourth spoken line ends the exchange without requesting a fifth line"),
		Social->IsConversationAtTurnLimit(4));
	Social->MaximumConversationTurns = 1;
	TestTrue(TEXT("A one-line configured exchange does not queue a reply to its opener"),
		Social->IsConversationAtTurnLimit(1));
	return true;
}
