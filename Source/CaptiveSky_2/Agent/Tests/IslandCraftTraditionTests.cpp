#include "Misc/AutomationTest.h"
#include "IslandWorldStateSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCraftTraditionTest, "CaptiveSky2.Agent.IslandCraftTradition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandCraftTraditionTest::RunTest(const FString& Parameters)
{
	const FString Scratch = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandCraftTradition"));
	IFileManager::Get().MakeDirectory(*Scratch, true);
	const FString StateFile = Scratch / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
	auto MakeParameter = [](FName Id, float Value)
	{
		FIslandCraftParameter Parameter;
		Parameter.Id = Id;
		Parameter.Value = Value;
		return Parameter;
	};
	auto Attempt = [&MakeParameter](UIslandWorldStateSubsystem* State, const FString& Agent, int32 Day, float Moisture,
		EIslandCraftOutcome Outcome = EIslandCraftOutcome::Promising)
	{
		return State->RecordCraftAttempt(TEXT("HearthLoaf"), TEXT("Aster's hearth loaf"), Agent, Day,
			TEXT("a remembered family recipe"), { MakeParameter(TEXT("moisture"), Moisture), MakeParameter(TEXT("crust"), 0.45f) }, Outcome);
	};

	UIslandWorldStateSubsystem* State = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("An isolated world-state object is created"), State)) return false;
	State->StorageFileOverride = StateFile;

	TestTrue(TEXT("Aster can establish a first attempt with two bounded traits"), Attempt(State, TEXT("Aster"), 1, 0.64f));
	const FIslandCraftTraditionRecord* Tradition = State->FindCraftTradition(TEXT("HearthLoaf"));
	TestTrue(TEXT("The first maker becomes the origin and knows the tradition"), Tradition && Tradition->OriginAgentId == TEXT("Aster") &&
		State->HasCraftKnowledge(TEXT("HearthLoaf"), TEXT("Aster")));
	TestEqual(TEXT("The first attempt preserves its two meaningful parameters"),
		Tradition && !Tradition->Attempts.IsEmpty() ? Tradition->Attempts[0].Parameters.Num() : 0, 2);
	TestFalse(TEXT("Tradition IDs stay short and slug-safe for exact conversational references"),
		State->RecordCraftAttempt(TEXT("Hearth Loaf"), TEXT("Unsafe ID"), TEXT("Aster"), 2,
			TEXT("invalid identifier"), { MakeParameter(TEXT("moisture"), 0.6f) }, EIslandCraftOutcome::Promising));
	TestFalse(TEXT("Out-of-range craft traits cannot enter the persistent history"),
		State->RecordCraftAttempt(TEXT("HearthLoaf"), TEXT("Aster's hearth loaf"), TEXT("Aster"), 2,
			TEXT("invalid attempt"), { MakeParameter(TEXT("moisture"), 1.2f) }, EIslandCraftOutcome::Promising));
	TestFalse(TEXT("The same resident cannot duplicate a craft attempt on the same Island day"), Attempt(State, TEXT("Aster"), 1, 0.72f));
	TestFalse(TEXT("A resident cannot practice a tradition they have not learned"), Attempt(State, TEXT("Innkeeper"), 2, 0.61f));

	TestTrue(TEXT("A later attempt records Aster's refinement"), Attempt(State, TEXT("Aster"), 2, 0.70f, EIslandCraftOutcome::Successful));
	TestTrue(TEXT("Aster can explicitly teach the practice to the Innkeeper"),
		State->RecordCraftTeaching(TEXT("HearthLoaf"), TEXT("Aster"), TEXT("Innkeeper"), 2));
	TestTrue(TEXT("The Innkeeper gains knowledge through the attributed lesson"), State->HasCraftKnowledge(TEXT("HearthLoaf"), TEXT("Innkeeper")));
	TestFalse(TEXT("The same resident cannot receive the same lesson twice"),
		State->RecordCraftTeaching(TEXT("HearthLoaf"), TEXT("Aster"), TEXT("Innkeeper"), 2));
	TestFalse(TEXT("A learner cannot teach before acquiring the practice"),
		State->RecordCraftTeaching(TEXT("HearthLoaf"), TEXT("Visitor"), TEXT("Rowan"), 2));
	TestTrue(TEXT("The Innkeeper can pass the lineage on to Rowan"),
		State->RecordCraftTeaching(TEXT("HearthLoaf"), TEXT("Innkeeper"), TEXT("Rowan"), 3));
	TestTrue(TEXT("Rowan can practice after receiving the lesson"), Attempt(State, TEXT("Rowan"), 3, 0.58f));
	TestFalse(TEXT("The history cannot be backdated over later attempts"), Attempt(State, TEXT("Aster"), 2, 0.68f));

	for (int32 Day = 4; Day < 4 + UIslandWorldStateSubsystem::MaxCraftAttemptsPerTradition + 2; ++Day)
		TestTrue(FString::Printf(TEXT("Aster can continue practicing on a later day (%d)"), Day), Attempt(State, TEXT("Aster"), Day, 0.50f + 0.01f * (Day % 20)));
	Tradition = State->FindCraftTradition(TEXT("HearthLoaf"));
	TestEqual(TEXT("Practice history stays within its fixed cap"),
		Tradition ? Tradition->Attempts.Num() : 0, UIslandWorldStateSubsystem::MaxCraftAttemptsPerTradition);
	TestTrue(TEXT("The oldest attempt rolls off once the bounded history fills"), Tradition &&
		!Tradition->Attempts.ContainsByPredicate([](const FIslandCraftAttemptRecord& Existing) { return Existing.Day == 1; }));

	UIslandWorldStateSubsystem* PersistenceProbe = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
	TestTrue(TEXT("Craft knowledge and practice survive a state-file reload"), PersistenceProbe && PersistenceProbe->ReadStateFile(StateFile) &&
		PersistenceProbe->GetCraftTraditions().Num() == 1 &&
		PersistenceProbe->HasCraftKnowledge(TEXT("HearthLoaf"), TEXT("Rowan")) &&
		PersistenceProbe->FindCraftTradition(TEXT("HearthLoaf"))->Lessons.Num() == 2 &&
		PersistenceProbe->FindCraftTradition(TEXT("HearthLoaf"))->Attempts.Num() == UIslandWorldStateSubsystem::MaxCraftAttemptsPerTradition);
	FString SavedJson;
	TestTrue(TEXT("The saved schema is advanced to version 2 and names the new record array"),
		FFileHelper::LoadFileToString(SavedJson, *StateFile) &&
		(SavedJson.Contains(TEXT("\"version\": 2")) || SavedJson.Contains(TEXT("\"version\":2"))) &&
		SavedJson.Contains(TEXT("\"craft_traditions\"")));

	const FString LegacyFile = Scratch / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
	TestTrue(TEXT("A version-1 world-state file without craft data can still be written"),
		FFileHelper::SaveStringToFile(TEXT("{\"version\":1,\"nests\":[],\"curios\":[]}"), *LegacyFile));
	UIslandWorldStateSubsystem* LegacyProbe = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
	TestTrue(TEXT("Legacy files load with an empty craft ledger"), LegacyProbe && LegacyProbe->ReadStateFile(LegacyFile) && LegacyProbe->GetCraftTraditions().IsEmpty());

	IFileManager::Get().Delete(*StateFile, false, true, true);
	IFileManager::Get().Delete(*LegacyFile, false, true, true);
	return true;
}
