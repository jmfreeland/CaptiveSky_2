#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentArrangementReflectionPromptTest,
	"CaptiveSky2.Agent.ArrangementReflectionPrompt", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentArrangementReflectionPromptTest::RunTest(const FString& Parameters)
{
	UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>();
	if (!TestNotNull(TEXT("A brain can build its ordinary thought prompt without a live provider"), Brain)) return false;

	const FString Prompt = Brain->BuildSystemPrompt({});
	TestTrue(TEXT("A completed, actually experienced arrangement may inspire an optional private impression"),
		Prompt.Contains(TEXT("you may optionally keep one brief private new memory")));
	TestTrue(TEXT("Aesthetic reflection must not become mandatory or a shared score"),
		Prompt.Contains(TEXT("not a shared CultureScore or a duty")) &&
		Prompt.Contains(TEXT("do not write a memory just because you saw the work")));
	TestTrue(TEXT("The creator's private intention stays unknown unless deliberately shared"),
		Prompt.Contains(TEXT("do not invent the maker's feelings, title, intent, or authorship beyond public lineage")));
	TestTrue(TEXT("Private reflection remains an existing optional memory choice, not an added model action"),
		Prompt.Contains(TEXT("new_memories")) && !Prompt.Contains(TEXT("arrangement_reflection action")));
	return true;
}
