// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "AgentDataPaths.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentMemoryComponentTest, "CaptiveSky2.Agent.MemoryComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentMemoryComponentTest::RunTest(const FString& Parameters)
{
	const FString TestAgentId = TEXT("AutomationTest_Memory");
	const FString TestDir = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / TestAgentId);

	// Start from a clean slate so re-running the test is deterministic.
	IFileManager::Get().DeleteDirectory(*TestDir, false, true);

	// --- Persistence + reload round trip ---
	UAgentMemoryComponent* Writer = NewObject<UAgentMemoryComponent>(GetTransientPackage());
	Writer->AgentId = TestAgentId;

	Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Observation, TEXT("Saw the player near the old oak tree."), 0.3f, { TEXT("player"), TEXT("tree") }));
	Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Conversation, TEXT("The player said hello and asked about the weather."), 0.5f, { TEXT("player"), TEXT("dialogue") }));
	Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Reflection, TEXT("I should keep watch over the nest at dawn."), 0.9f, { TEXT("nest"), TEXT("goal") }));

	TestEqual(TEXT("Writer cache has 3 records after appending"), Writer->GetMemoryCount(), 3);
	TestTrue(TEXT("memory.jsonl was created on disk"), FPaths::FileExists(Writer->GetMemoryFilePath()));

	UAgentMemoryComponent* Reader = NewObject<UAgentMemoryComponent>(GetTransientPackage());
	Reader->AgentId = TestAgentId;
	TestEqual(TEXT("A fresh component reloads all 3 records from disk"), Reader->GetMemoryCount(), 3);
	TestEqual(TEXT("A minimum timestamp returns the full chronological consolidation window"),
		Reader->GetMemoriesSince(FDateTime::MinValue()).Num(), 3);
	TestEqual(TEXT("A future timestamp returns no consolidation memories"),
		Reader->GetMemoriesSince(FDateTime::UtcNow() + FTimespan::FromMinutes(1)).Num(), 0);

	// --- Relevance scoring: importance should win when there is no keyword overlap ---
	{
		const TArray<FAgentMemoryRecord> Results = Reader->GetRelevantContext(10000, TEXT("random unrelated situation"));
		if (TestEqual(TEXT("All 3 records returned with a large token budget"), Results.Num(), 3))
		{
			TestEqual(TEXT("Highest-importance record (the nest reflection) ranks first"), Results[0].Text, FString(TEXT("I should keep watch over the nest at dawn.")));
		}
	}

	// --- Relevance scoring: keyword overlap should be able to promote a lower-importance record ---
	{
		const TArray<FAgentMemoryRecord> Results = Reader->GetRelevantContext(10000, TEXT("player said hello dialogue"));
		if (TestTrue(TEXT("At least one record returned for a keyword-matching query"), Results.Num() > 0))
		{
			TestEqual(TEXT("The dialogue record ranks first when the query matches its keywords"), Results[0].Text,
				FString(TEXT("The player said hello and asked about the weather.")));
		}
	}

	// --- Durable topic recall: an old, distinct wish should survive newer unrelated dialogue ---
	{
		const FString WishId = TEXT("AutomationTest_MemoryDurableWish");
		const FString WishDir = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / WishId);
		IFileManager::Get().DeleteDirectory(*WishDir, false, true);
		UAgentMemoryComponent* WishMemory = NewObject<UAgentMemoryComponent>(GetTransientPackage());
		WishMemory->AgentId = WishId;
		FAgentMemoryRecord OldWish = WishMemory->MakeMemory(EAgentMemoryType::Conversation,
			TEXT("I replied to CaptiveSky via discord: \"A few: give the Island changing weather, varied wind currents, and quiet places where a raven can nest undisturbed. Let objects and paths sometimes move, vanish, or appear for reasons that are not immediately explained. I’d also like other living things with their own habits—not merely decorations—and the freedom to approach, leave, and discover at my own pace. Most importantly, keep some corners genuinely wild and unfinished.\""),
			0.5f, { TEXT("conversation"), TEXT("external"), TEXT("discord"), TEXT("speech") });
		OldWish.Timestamp = FDateTime::UtcNow() - FTimespan::FromDays(27.0);
		WishMemory->AppendMemory(OldWish);
		FAgentMemoryRecord SingleWordMemory = WishMemory->MakeMemory(EAgentMemoryType::Conversation,
			TEXT("I once saw a raven fly over the sea."), 0.5f, { TEXT("conversation"), TEXT("external") });
		SingleWordMemory.Timestamp = OldWish.Timestamp;
		WishMemory->AppendMemory(SingleWordMemory);
		const TArray<FString> RecentObservations = {
			TEXT("I watched pale moonlight reveal a hidden waterfall beneath the old bridge."),
			TEXT("I followed a moth around a copper lantern near the western shore."),
			TEXT("Wild thyme smelled sharp after rain beside the steep green meadow."),
			TEXT("A fox called from beyond the sleeping village while I listened."),
			TEXT("I found a blue shell buried near the northern tide pools."),
			TEXT("Warm stone rested underfoot in the late afternoon sun.")
		};
		for (const FString& Observation : RecentObservations)
		{
			FAgentMemoryRecord Recent = WishMemory->MakeMemory(EAgentMemoryType::Observation, Observation, 0.65f, { TEXT("action-result") });
			Recent.Timestamp = FDateTime::UtcNow();
			WishMemory->AppendMemory(Recent);
		}
		const TArray<FString> RecentDialogue = {
			TEXT("I spoke with Aster about silver light crossing the western stones."),
			TEXT("I told the Innkeeper that warm tea was pleasant after rain."),
			TEXT("Aster asked whether the eastern path was quiet before sunrise."),
			TEXT("The Innkeeper described clouds gathering above the distant hills.")
		};
		for (const FString& Line : RecentDialogue)
		{
			FAgentMemoryRecord Recent = WishMemory->MakeMemory(EAgentMemoryType::Conversation, Line, 0.5f, { TEXT("conversation"), TEXT("agent-to-agent") });
			Recent.Timestamp = FDateTime::UtcNow();
			WishMemory->AppendMemory(Recent);
		}

		const FString RoostSituation = TEXT("At a nearby roost, the raven can weave a nest from fallen twigs gathered from the ground. The changing weather and varied wind currents shape this perch; gathering and weaving are optional, and you may explore elsewhere.");
		const TArray<FAgentMemoryRecord> RoostContext = WishMemory->GetRelevantContext(500, RoostSituation);
		TestTrue(TEXT("A 27-day-old nest wish is recalled in a nearby-roost situation despite newer unrelated dialogue"),
			RoostContext.ContainsByPredicate([&OldWish](const FAgentMemoryRecord& Record) { return Record.Id == OldWish.Id; }));
		TestFalse(TEXT("A single shared word does not promote a stale, unrelated raven memory"),
			RoostContext.ContainsByPredicate([&SingleWordMemory](const FAgentMemoryRecord& Record) { return Record.Id == SingleWordMemory.Id; }));
		IFileManager::Get().DeleteDirectory(*WishDir, false, true);
	}

	// --- Token budget actually limits how many records come back ---
	{
		const TArray<FAgentMemoryRecord> Tiny = Reader->GetRelevantContext(1, TEXT("anything"));
		TestTrue(TEXT("A tiny token budget returns fewer records than the full set"), Tiny.Num() < 3);
		TestTrue(TEXT("A tiny token budget still returns at least one record"), Tiny.Num() >= 1);
	}

	// --- Retrieval keeps dialogue from crowding out varied lived experience ---
	{
		const TArray<FString> ConversationLines = {
			TEXT("Aster told the raven about a copper leaf drifting beside the western path."),
			TEXT("The raven asked whether moonlight reaches the north ridge after midnight."),
			TEXT("They remembered low thunder rolling across the sea shortly before dawn."),
			TEXT("Aster spoke of warm stone beneath the sunlit arch and salt on the wind."),
			TEXT("The raven shared a quiet thought about shadows crossing the pale dunes."),
			TEXT("They wondered where small boats travel when the far horizon turns grey.")
		};
		const TArray<FString> ObservationLines = {
			TEXT("I found a green feather near the old stone bridge."),
			TEXT("A red flower opened beside the eastern path at midday."),
			TEXT("Rain left a silver trail across the northern window."),
			TEXT("The western shore held a smooth hooked piece of driftwood."),
			TEXT("Two sandpipers ran over the black rocks near sunset."),
			TEXT("Warm air rose from the shallow pool when sunlight touched the ridge.")
		};
		for (const FString& Line : ConversationLines)
			Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Conversation, Line, 0.5f, { TEXT("conversation"), TEXT("agent-to-agent") }));
		for (const FString& Line : ObservationLines)
			Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Observation, Line, 0.65f, { TEXT("action-result") }));

		const FString NearDuplicateA = TEXT("I watched the small silver raven circle the shallow tidepool just before sunrise.");
		const FString NearDuplicateB = TEXT("I watched a small silver raven circle the shallow tidepool before sunrise.");
		Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Observation, NearDuplicateA, 0.8f, { TEXT("raven"), TEXT("tidepool") }));
		Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Observation, NearDuplicateB, 0.9f, { TEXT("raven"), TEXT("tidepool") }));

		const TArray<FAgentMemoryRecord> Diverse = Writer->GetRelevantContext(10000, TEXT("unrelated afternoon weather"));
		int32 ConversationCount = 0;
		int32 NearDuplicateObservationCount = 0;
		for (const FAgentMemoryRecord& Record : Diverse)
		{
			if (Record.Type == EAgentMemoryType::Conversation) ++ConversationCount;
			if (Record.Text.Contains(TEXT("silver raven circle"))) ++NearDuplicateObservationCount;
		}
		TestTrue(TEXT("A large query budget returns substantial context"), Diverse.Num() >= 12);
		TestTrue(TEXT("Dialogue uses no more than about one third of the selected records"), ConversationCount * 3 <= Diverse.Num());
		TestEqual(TEXT("Near-duplicate observations contribute only one recalled record"), NearDuplicateObservationCount, 1);
	}
	{
		FAgentConversationContext InWorldConversation;
		InWorldConversation.bAgentToAgent = true;
		TestEqual(TEXT("In-world small talk receives lower importance than distinctive experience"),
			UAgentBrainComponent::GetConversationMemoryImportance(InWorldConversation), 0.3f);
		TestEqual(TEXT("Visitor and external conversation importance remains unchanged"),
			UAgentBrainComponent::GetConversationMemoryImportance(FAgentConversationContext()), 0.5f);
	}
	{
		const FString ConversationOnlyId = TEXT("AutomationTest_MemoryConversationOnly");
		const FString ConversationOnlyDir = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / ConversationOnlyId);
		IFileManager::Get().DeleteDirectory(*ConversationOnlyDir, false, true);
		UAgentMemoryComponent* ConversationOnly = NewObject<UAgentMemoryComponent>(GetTransientPackage());
		ConversationOnly->AgentId = ConversationOnlyId;
		ConversationOnly->AppendMemory(ConversationOnly->MakeMemory(EAgentMemoryType::Conversation, TEXT("Aster told the raven about the quiet western shore."), 0.5f, {}));
		ConversationOnly->AppendMemory(ConversationOnly->MakeMemory(EAgentMemoryType::Conversation, TEXT("The raven asked about the distant moonlit ridge."), 0.5f, {}));
		const TArray<FAgentMemoryRecord> ConversationFallback = ConversationOnly->GetRelevantContext(1000, TEXT("silent evening"));
		TestTrue(TEXT("A conversation-only history retains one usable memory"), ConversationFallback.Num() == 1 && ConversationFallback[0].Type == EAgentMemoryType::Conversation);
		IFileManager::Get().DeleteDirectory(*ConversationOnlyDir, false, true);
	}

	// --- Near-duplicate reflections are suppressed at write time without deleting history ---
	{
		const FString ExistingReflection = TEXT("I watched the small silver raven circle the shallow tidepool just before sunrise.");
		const FString DuplicateReflection = TEXT("I watched a small silver raven circle the shallow tidepool before sunrise.");
		FAgentMemoryRecord OldReflection = Writer->MakeMemory(EAgentMemoryType::Reflection, ExistingReflection, 0.8f, { TEXT("raven"), TEXT("tidepool") });
		OldReflection.Timestamp = FDateTime::UtcNow() - FTimespan::FromHours(5.0);
		Writer->AppendMemory(OldReflection);
		TestFalse(TEXT("A similar reflection outside the four-hour window does not suppress new reflection"),
			Writer->HasSimilarMemorySince(EAgentMemoryType::Reflection, DuplicateReflection, FDateTime::UtcNow() - FTimespan::FromHours(4.0), 0.6f));
		Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Reflection, ExistingReflection, 0.8f, { TEXT("raven"), TEXT("tidepool") }));
		TestTrue(TEXT("The memory component detects recent near-duplicate reflections"),
			Writer->HasSimilarMemorySince(EAgentMemoryType::Reflection, DuplicateReflection, FDateTime::UtcNow() - FTimespan::FromHours(4.0), 0.6f));
		TestFalse(TEXT("The duplicate detector does not match the same words across unrelated memory kinds"),
			Writer->HasSimilarMemorySince(EAgentMemoryType::Conversation, DuplicateReflection, FDateTime::UtcNow() - FTimespan::FromHours(4.0), 0.6f));

		const FString DuplicateResponse = TEXT("{\"thought\":\"I remember this\",\"action\":{\"type\":\"idle\"},\"new_memories\":[{\"text\":\"I watched a small silver raven circle the shallow tidepool before sunrise.\",\"importance\":0.8,\"tags\":[\"raven\",\"tidepool\"]}]}");
		const int32 CountBeforeDuplicate = Writer->GetMemoryCount();
		const FAgentDecision DuplicateDecision = UAgentBrainComponent::ParseDecisionAndStoreMemories(DuplicateResponse, Writer);
		TestTrue(TEXT("The duplicate-reflection response still yields a valid decision"), DuplicateDecision.bValid);
		TestEqual(TEXT("A repeated reflection is not appended again"), Writer->GetMemoryCount(), CountBeforeDuplicate);

		const FString DistinctResponse = TEXT("{\"thought\":\"A new day\",\"action\":{\"type\":\"idle\"},\"new_memories\":[{\"text\":\"Today I met a fox by the old stone bridge during the storm.\",\"importance\":0.7,\"tags\":[\"fox\",\"bridge\"]}]}");
		UAgentBrainComponent::ParseDecisionAndStoreMemories(DistinctResponse, Writer);
		TestEqual(TEXT("A distinct reflection remains eligible for storage"), Writer->GetMemoryCount(), CountBeforeDuplicate + 1);
	}

	// --- JSON line round trip is exact for the fields that matter ---
	{
		FAgentMemoryRecord Original;
		Original.Id = TEXT("test-id-1234");
		Original.Timestamp = FDateTime(2026, 1, 15, 10, 30, 0);
		Original.Type = EAgentMemoryType::Decision;
		Original.Text = TEXT("Decided to fly toward the barn.");
		Original.Tags = { TEXT("flight"), TEXT("barn") };
		Original.Importance = 0.75f;
		Original.Location = FVector(100.f, 200.f, 300.f);

		const FString Line = Original.ToJsonLine();
		FAgentMemoryRecord RoundTripped;
		if (TestTrue(TEXT("FromJsonLine parses a line produced by ToJsonLine"), FAgentMemoryRecord::FromJsonLine(Line, RoundTripped)))
		{
			TestEqual(TEXT("Id round-trips"), RoundTripped.Id, Original.Id);
			TestEqual(TEXT("Text round-trips"), RoundTripped.Text, Original.Text);
			TestEqual(TEXT("Type round-trips"), static_cast<uint8>(RoundTripped.Type), static_cast<uint8>(Original.Type));
			TestEqual(TEXT("Importance round-trips"), RoundTripped.Importance, Original.Importance);
			TestEqual(TEXT("Tag count round-trips"), RoundTripped.Tags.Num(), Original.Tags.Num());
			TestEqual(TEXT("Location round-trips"), RoundTripped.Location, Original.Location);
		}
	}

	// Mixed ASCII and Unicode appends must remain a single UTF-8 JSONL stream.
	const FString UnicodeText = TEXT("I\u2019m here \u2014 caf\u00e9 \u98a8");
	const int32 CountBeforeUnicode = Writer->GetMemoryCount();
	Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Conversation, UnicodeText, 0.5f, {}));
	Writer->AppendMemory(Writer->MakeMemory(EAgentMemoryType::Observation, TEXT("ASCII after Unicode"), 0.4f, {}));
	UAgentMemoryComponent* UnicodeReader = NewObject<UAgentMemoryComponent>(GetTransientPackage());
	UnicodeReader->AgentId = TestAgentId;
	const auto UnicodeRecords = UnicodeReader->GetMemoriesSince(FDateTime::MinValue());
	if (TestEqual(TEXT("Mixed character sets reload every record"), UnicodeRecords.Num(), CountBeforeUnicode + 2))
	{
		// Appends can share a serialized timestamp; retrieval tie order is not a file-order contract.
		TestTrue(TEXT("Unicode text preserved exactly"), UnicodeRecords.ContainsByPredicate([&](const FAgentMemoryRecord& Record) { return Record.Text == UnicodeText; }));
		TestTrue(TEXT("ASCII after Unicode preserved"), UnicodeRecords.ContainsByPredicate([](const FAgentMemoryRecord& Record) { return Record.Text == TEXT("ASCII after Unicode"); }));
	}
	UAgentBrainComponent* PromptBrain = NewObject<UAgentBrainComponent>(GetTransientPackage());
	const FString SystemPrompt = PromptBrain->BuildSystemPrompt({});
	TestTrue(TEXT("Action targets must copy identifiers shown in the current situation"),
		SystemPrompt.Contains(TEXT("copy the exact target identifier shown in the current situation")));
	TestTrue(TEXT("The prompt discourages invented target names"), SystemPrompt.Contains(TEXT("Do not invent or paraphrase a target")));
	TestTrue(TEXT("The prompt gives wander or idle as the alternative to a guessed target"),
		SystemPrompt.Contains(TEXT("choose wander or idle instead of guessing")));
	TestTrue(TEXT("Remembered places are optional returns, not claims of changed world state"),
		SystemPrompt.Contains(TEXT("optional return destinations, not evidence that anything has changed")) &&
		SystemPrompt.Contains(TEXT("return only if you are curious")));
	TestTrue(TEXT("The prompt restricts land to Raven and listed open-ground sites"),
		SystemPrompt.Contains(TEXT("The land action is for the raven only")) && SystemPrompt.Contains(TEXT("ArrangingGround target")));
	TestTrue(TEXT("The response schema lists the human-reviewed object request action"),
		SystemPrompt.Contains(TEXT("request_object")) && SystemPrompt.Contains(TEXT("object_request")));
	TestTrue(TEXT("Object requests are proposals and do not create assets automatically"),
		SystemPrompt.Contains(TEXT("human caretaker")) && SystemPrompt.Contains(TEXT("does not create, import, or place anything")));
	TestTrue(TEXT("Upgrade proposals cover any existing world element and the three requested improvement kinds"),
		SystemPrompt.Contains(TEXT("any existing object or feature in the landscape or level")) &&
		SystemPrompt.Contains(TEXT("aesthetic")) && SystemPrompt.Contains(TEXT("variation")) && SystemPrompt.Contains(TEXT("functionality")));
	TestTrue(TEXT("Upgrade proposals remain human-reviewed suggestions that change nothing automatically"),
		SystemPrompt.Contains(TEXT("request_upgrade")) && SystemPrompt.Contains(TEXT("not permission to change the world")));
	const FAgentDecision LandDecision = UAgentBrainComponent::ParseDecisionAndStoreMemories(
		TEXT("{\"thought\":\"A quiet descent\",\"action\":{\"type\":\"land\",\"target\":\"ArrangingGround_1\"}}"), Writer);
	TestTrue(TEXT("The land action parses with its exact site target"), LandDecision.bValid &&
		LandDecision.ActionType == EAgentActionType::Land && LandDecision.ActionTarget == TEXT("ArrangingGround_1"));
	const FAgentDecision ObjectRequestDecision = UAgentBrainComponent::ParseDecisionAndStoreMemories(
		TEXT("{\"thought\":\"A sheltered place might help.\",\"action\":{\"type\":\"request_object\",\"object_request\":\"A small weatherproof bird shelter\"}}"), Writer);
	TestTrue(TEXT("The request_object action and description parse"), ObjectRequestDecision.bValid &&
		ObjectRequestDecision.ActionType == EAgentActionType::RequestObject &&
		ObjectRequestDecision.ObjectRequest == TEXT("A small weatherproof bird shelter"));
	const FAgentDecision UpgradeDecision = UAgentBrainComponent::ParseDecisionAndStoreMemories(
		TEXT("{\"thought\":\"The sign could be clearer.\",\"action\":{\"type\":\"request_upgrade\",\"upgrade_target\":\"InnNoticeBoard\",\"upgrade_kind\":\"functionality\",\"upgrade_request\":\"Add a small weatherproof shelf for messages\"}}"), Writer);
	TestTrue(TEXT("The request_upgrade action preserves its target, kind, and proposal"), UpgradeDecision.bValid &&
		UpgradeDecision.ActionType == EAgentActionType::RequestUpgrade &&
		UpgradeDecision.UpgradeTarget == TEXT("InnNoticeBoard") &&
		UpgradeDecision.UpgradeKind == TEXT("functionality") &&
		UpgradeDecision.UpgradeRequest == TEXT("Add a small weatherproof shelf for messages"));
	IFileManager::Get().DeleteDirectory(*TestDir, false, true);

	return true;
}
