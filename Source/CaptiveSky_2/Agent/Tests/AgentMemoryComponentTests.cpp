// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "AgentMemoryComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentMemoryComponentTest, "CaptiveSky2.Agent.MemoryComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentMemoryComponentTest::RunTest(const FString& Parameters)
{
	const FString TestAgentId = TEXT("AutomationTest_Memory");
	const FString TestDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Agents") / TestAgentId);

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

	// --- Token budget actually limits how many records come back ---
	{
		const TArray<FAgentMemoryRecord> Tiny = Reader->GetRelevantContext(1, TEXT("anything"));
		TestTrue(TEXT("A tiny token budget returns fewer records than the full set"), Tiny.Num() < 3);
		TestTrue(TEXT("A tiny token budget still returns at least one record"), Tiny.Num() >= 1);
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

	// Clean up after ourselves.
	IFileManager::Get().DeleteDirectory(*TestDir, false, true);

	return true;
}
