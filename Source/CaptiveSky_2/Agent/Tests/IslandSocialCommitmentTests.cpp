#include "Misc/AutomationTest.h"
#include "IslandWorldStateSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandSocialCommitmentTest, "CaptiveSky2.Agent.IslandSocialCommitment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandSocialCommitmentTest::RunTest(const FString& Parameters)
{
	const FString Scratch = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandSocialCommitment"));
	IFileManager::Get().MakeDirectory(*Scratch, true);
	const FString StateFile = Scratch / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
	auto MakeCommitment = [](FName Id, const FString& Proposer, const FString& Recipient, int32 CreatedDay, int32 DueDay)
	{
		FIslandSocialCommitmentRecord Record;
		Record.Id = Id;
		Record.ProposerAgentId = Proposer;
		Record.RecipientAgentId = Recipient;
		Record.ObjectId = TEXT("BlueJar");
		Record.TargetPlaceId = TEXT("InnCounter");
		Record.SourceConversationId = TEXT("conversation-17");
		Record.CreatedDay = CreatedDay;
		Record.DueDay = DueDay;
		return Record;
	};

	UIslandWorldStateSubsystem* State = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("An isolated world-state object is created"), State)) return false;
	State->StorageFileOverride = StateFile;
	TestTrue(TEXT("Aster can make a bounded promise to the Innkeeper"),
		State->CreateBringObjectCommitment(MakeCommitment(TEXT("JarPromise1"), TEXT("Aster"), TEXT("Innkeeper"), 1, 3)));
	TestFalse(TEXT("A commitment ID cannot be reused"),
		State->CreateBringObjectCommitment(MakeCommitment(TEXT("JarPromise1"), TEXT("Aster"), TEXT("Innkeeper"), 1, 3)));
	TestFalse(TEXT("A resident cannot promise themself an action"),
		State->CreateBringObjectCommitment(MakeCommitment(TEXT("SelfPromise"), TEXT("Aster"), TEXT("Aster"), 1, 3)));
	TestFalse(TEXT("A deadline cannot exceed the small supported horizon"),
		State->CreateBringObjectCommitment(MakeCommitment(TEXT("DistantPromise"), TEXT("Aster"), TEXT("Innkeeper"), 1, 32)));

	const bool bAdvancedBeforeDue = State->AdvanceSocialCommitments(2);
	const FIslandSocialCommitmentRecord* BeforeDue = State->FindSocialCommitment(TEXT("JarPromise1"));
	TestTrue(TEXT("The promise stays pending before its deadline"), bAdvancedBeforeDue && BeforeDue && BeforeDue->Status == EIslandSocialCommitmentStatus::Pending);
	const bool bReachedDueDay = State->SaveClock(12.f, 3);
	const FIslandSocialCommitmentRecord* Due = State->FindSocialCommitment(TEXT("JarPromise1"));
	TestTrue(TEXT("The Island clock deterministically makes it due on day three"), bReachedDueDay && Due && Due->Status == EIslandSocialCommitmentStatus::Due);
	TestFalse(TEXT("Only the proposer can fulfill the promise"),
		State->RecordSocialCommitmentFulfilled(TEXT("JarPromise1"), TEXT("Innkeeper"), 3, TEXT("Innkeeper"), TEXT("The blue jar is on the counter.")));
	TestFalse(TEXT("A promise cannot be fulfilled without recipient witness evidence"),
		State->RecordSocialCommitmentFulfilled(TEXT("JarPromise1"), TEXT("Aster"), 3, TEXT("Aster"), TEXT("The blue jar is on the counter.")));
	TestTrue(TEXT("The recipient can witness an on-time delivery"),
		State->RecordSocialCommitmentFulfilled(TEXT("JarPromise1"), TEXT("Aster"), 3, TEXT("Innkeeper"), TEXT("The blue jar is on the counter.")));
	TestTrue(TEXT("The witnessed delivery is recorded as fulfilled"),
		State->FindSocialCommitment(TEXT("JarPromise1"))->Status == EIslandSocialCommitmentStatus::Fulfilled);

	TestTrue(TEXT("A second promise is persisted"),
		State->CreateBringObjectCommitment(MakeCommitment(TEXT("JarPromise2"), TEXT("Aster"), TEXT("Innkeeper"), 3, 5)));
	const bool bSecondDue = State->SaveClock(12.f, 5);
	const FIslandSocialCommitmentRecord* SecondDue = State->FindSocialCommitment(TEXT("JarPromise2"));
	TestTrue(TEXT("An unanswered promise becomes due on its named day"), bSecondDue && SecondDue && SecondDue->Status == EIslandSocialCommitmentStatus::Due);
	const bool bSecondExpired = State->SaveClock(12.f, 6);
	const FIslandSocialCommitmentRecord* SecondExpired = State->FindSocialCommitment(TEXT("JarPromise2"));
	TestTrue(TEXT("A due promise expires after its deadline"), bSecondExpired && SecondExpired && SecondExpired->Status == EIslandSocialCommitmentStatus::Expired);

	UIslandWorldStateSubsystem* Reloaded = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
	const bool bReloaded = Reloaded && Reloaded->ReadStateFile(StateFile);
	const FIslandSocialCommitmentRecord* ReloadedFulfilled = Reloaded ? Reloaded->FindSocialCommitment(TEXT("JarPromise1")) : nullptr;
	const FIslandSocialCommitmentRecord* ReloadedExpired = Reloaded ? Reloaded->FindSocialCommitment(TEXT("JarPromise2")) : nullptr;
	TestTrue(TEXT("Fulfillment and expiry survive a world-state reload"), bReloaded &&
		Reloaded->GetSocialCommitments().Num() == 2 && ReloadedFulfilled && ReloadedExpired &&
		ReloadedFulfilled->OutcomeWitnessAgentId == TEXT("Innkeeper") && ReloadedExpired->ExpiredDay == 6);
	FString SavedJson;
	TestTrue(TEXT("New state writes schema version three and the commitment ledger"),
		FFileHelper::LoadFileToString(SavedJson, *StateFile) &&
		(SavedJson.Contains(TEXT("\"version\": 3")) || SavedJson.Contains(TEXT("\"version\":3"))) &&
		SavedJson.Contains(TEXT("\"social_commitments\"")));

	const FString LegacyFile = Scratch / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
	TestTrue(TEXT("An older world-state file remains readable"),
		FFileHelper::SaveStringToFile(TEXT("{\"version\":2,\"nests\":[],\"curios\":[]}"), *LegacyFile));
	UIslandWorldStateSubsystem* Legacy = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
	TestTrue(TEXT("Version two data loads with no commitments"), Legacy && Legacy->ReadStateFile(LegacyFile) && Legacy->GetSocialCommitments().IsEmpty());

	IFileManager::Get().Delete(*StateFile, false, true, true);
	IFileManager::Get().Delete(*LegacyFile, false, true, true);
	return true;
}
