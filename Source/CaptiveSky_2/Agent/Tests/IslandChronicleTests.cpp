#include "Misc/AutomationTest.h"
#include "IslandChronicle.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandChronicleTest, "CaptiveSky2.Agent.IslandChronicle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandChronicleTest::RunTest(const FString& Parameters)
{
	auto Parse = [](const FString& Line) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Object;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Line), Object);
		return Object;
	};

	const FDateTime Now(2026, 9, 29, 8, 30, 15);
	const FString Line = UIslandChronicleSubsystem::FormatEntry(Now, 4, TEXT("07:45"), TEXT("decision"), TEXT("Agent_Aster_01"),
		TEXT("Hello \"friend\"\nsecond line"), { { TEXT("action"), TEXT("speak") }, { TEXT("type"), TEXT("overridden") }, { TEXT("text"), TEXT("overridden") } });
	TestFalse(TEXT("An entry is a single line"), Line.Contains(TEXT("\n")) || Line.Contains(TEXT("\r")));
	const TSharedPtr<FJsonObject> Entry = Parse(Line);
	if (!TestTrue(TEXT("An entry is valid JSON"), Entry.IsValid())) return false;
	TestEqual(TEXT("Type is kept"), Entry->GetStringField(TEXT("type")), FString(TEXT("decision")));
	TestEqual(TEXT("Text survives quotes and newlines"), Entry->GetStringField(TEXT("text")), FString(TEXT("Hello \"friend\"\nsecond line")));
	TestEqual(TEXT("Agent is kept"), Entry->GetStringField(TEXT("agent")), FString(TEXT("Agent_Aster_01")));
	TestEqual(TEXT("Island day is kept"), static_cast<int32>(Entry->GetNumberField(TEXT("day"))), 4);
	TestEqual(TEXT("Island clock is kept"), Entry->GetStringField(TEXT("clock")), FString(TEXT("07:45")));
	TestEqual(TEXT("Extra fields are kept"), Entry->GetStringField(TEXT("action")), FString(TEXT("speak")));
	TestEqual(TEXT("Extra fields cannot replace the fixed ones"), Entry->GetStringField(TEXT("type")), FString(TEXT("decision")));
	TestTrue(TEXT("Time is UTC ISO 8601"), Entry->GetStringField(TEXT("t")).StartsWith(TEXT("2026-09-29T08:30:15")));

	const TSharedPtr<FJsonObject> Anonymous = Parse(UIslandChronicleSubsystem::FormatEntry(Now, 1, TEXT("09:00"), TEXT("weather"), FString(), TEXT("A storm arrives."), {}));
	if (TestTrue(TEXT("An entry without an agent is valid"), Anonymous.IsValid()))
		TestFalse(TEXT("It carries no agent field"), Anonymous->HasField(TEXT("agent")));

	UIslandChronicleSubsystem* Chronicle = NewObject<UIslandChronicleSubsystem>(GetTransientPackage());
	TestTrue(TEXT("Without a world or a file there is nowhere to write"), Chronicle->GetChroniclePath().IsEmpty());
	TestFalse(TEXT("So nothing is written"), Chronicle->AppendLine(TEXT("{}")));
	Chronicle->RecordEntry(TEXT("guest_book"), TEXT("A"), TEXT("ignored"), {});

	const FString File = FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("Chronicle") / TEXT("chronicle-test.jsonl");
	IFileManager::Get().Delete(*File, false, true, true);
	Chronicle->ChronicleFileOverride = File;
	Chronicle->RecordEntry(TEXT("guest_book"), TEXT("Agent_A"), TEXT("The tide was kind."), { { TEXT("k"), TEXT("v") } });
	Chronicle->RecordEntry(TEXT("nest"), TEXT("Agent_B"), TEXT("wove a layer."), {});
	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *File);
	if (TestEqual(TEXT("Each entry adds one line"), Lines.Num(), 2))
	{
		const TSharedPtr<FJsonObject> First = Parse(Lines[0]);
		const TSharedPtr<FJsonObject> Second = Parse(Lines[1]);
		if (TestTrue(TEXT("Both lines parse"), First.IsValid() && Second.IsValid()))
		{
			TestEqual(TEXT("Entries stay in order"), First->GetStringField(TEXT("type")), FString(TEXT("guest_book")));
			TestEqual(TEXT("Second entry follows"), Second->GetStringField(TEXT("type")), FString(TEXT("nest")));
		}
	}
	IFileManager::Get().Delete(*File, false, true, true);
	return true;
}
