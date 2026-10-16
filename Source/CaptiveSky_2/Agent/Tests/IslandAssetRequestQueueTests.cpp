// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Agent/IslandAssetRequestQueue.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandAssetRequestQueueTest,
	"CaptiveSky2.Agent.AssetRequestQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandAssetRequestQueueTest::RunTest(const FString& Parameters)
{
	const FString TestDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/AssetRequestQueue"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const FString InboxPath = FPaths::Combine(TestDirectory, TEXT("inbox.jsonl"));
	FString RequestId;
	FString Error;
	TestTrue(TEXT("A valid request is appended"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("A small weatherproof bird shelter"), RequestId, Error, InboxPath));
	TestTrue(TEXT("The request receives a stable review ID"), !RequestId.IsEmpty());

	FString InboxContents;
	TestTrue(TEXT("The review inbox is readable"), FFileHelper::LoadFileToString(InboxContents, *InboxPath));
	TestTrue(TEXT("The request is explicitly pending human review"), InboxContents.Contains(TEXT("pending_review")));
	TestTrue(TEXT("The request identifies its pipeline without invoking it"), InboxContents.Contains(TEXT("ComfyBlender")));
	TestFalse(TEXT("A duplicate pending request is rejected"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("A small weatherproof bird shelter"), RequestId, Error, InboxPath));
	TestTrue(TEXT("The duplicate is rejected for the intended reason"), Error.Contains(TEXT("already have this proposal")));
	TestFalse(TEXT("An empty description is rejected"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("  "), RequestId, Error, InboxPath));
	const bool bUpgradeAppended = IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Raven_01"), TEXT("Add a sheltered perch to the upper beam"), RequestId, Error,
		InboxPath, TEXT("upgrade"), TEXT("WindArch_UpperBeam"), TEXT("functionality"));
	TestTrue(FString::Printf(TEXT("An upgrade suggestion for a named world object is appended (%s)"), *Error), bUpgradeAppended);
	TestTrue(TEXT("Upgrade records preserve request type, exact target, and kind"),
		FFileHelper::LoadFileToString(InboxContents, *InboxPath) &&
		InboxContents.Contains(TEXT("\"request_type\":\"upgrade\"")) &&
		InboxContents.Contains(TEXT("\"target\":\"WindArch_UpperBeam\"")) &&
		InboxContents.Contains(TEXT("\"upgrade_kind\":\"functionality\"")) &&
		InboxContents.Contains(TEXT("\"pipeline\":\"human_review\"")));
	TestFalse(TEXT("Unsupported upgrade categories are rejected"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Raven_01"), TEXT("Make the perch nicer"), RequestId, Error,
		InboxPath, TEXT("upgrade"), TEXT("WindArch_UpperBeam"), TEXT("teleportation")));
	TestFalse(TEXT("An identical pending upgrade is rejected"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Raven_01"), TEXT("Add a sheltered perch to the upper beam"), RequestId, Error,
		InboxPath, TEXT("upgrade"), TEXT("WindArch_UpperBeam"), TEXT("functionality")));
	TestTrue(TEXT("Upgrade targets are not restricted to pipeline-generated props (for example, an individual foliage instance)"),
		IslandAssetRequestQueue::AppendRequest(TEXT("Agent_Aster_01"), TEXT("Add a small patch of late-summer flowers"), RequestId, Error,
			InboxPath, TEXT("upgrade"), TEXT("IslandShrubs.Alder_07.Instance_12"), TEXT("variation")));
	const FString DescriptiveTarget = TEXT("mossy stone tower beside the northern path");
	TestTrue(TEXT("An untagged visible object can be proposed by a concise description"),
		IslandAssetRequestQueue::AppendRequest(TEXT("Agent_Aster_01"), TEXT("Add a small sheltered perch near the top"), RequestId, Error,
			InboxPath, TEXT("upgrade"), DescriptiveTarget, TEXT("functionality")));
	TestTrue(TEXT("A level-wide proposal may omit a single-object target"),
		IslandAssetRequestQueue::AppendRequest(TEXT("Agent_Raven_01"), TEXT("Make the shore more sheltered from the prevailing wind"), RequestId, Error,
			InboxPath, TEXT("upgrade"), FString(), TEXT("functionality")));
	TestTrue(TEXT("The review inbox preserves exact, descriptive, and world-level targets"),
		FFileHelper::LoadFileToString(InboxContents, *InboxPath) &&
		InboxContents.Contains(TEXT("IslandShrubs.Alder_07.Instance_12")) &&
		InboxContents.Contains(DescriptiveTarget) &&
		InboxContents.Contains(TEXT("shore more sheltered from the prevailing wind")));

	const FString QuotaInboxPath = FPaths::Combine(TestDirectory, TEXT("quota-inbox.jsonl"));
	const FString QuotaInbox =
		TEXT("{\"id\":\"quota-1\",\"requester\":\"Agent_Aster_01\",\"description\":\"A weatherproof bird shelter\",\"request_type\":\"new_object\",\"status\":\"pending_review\"}\n")
		TEXT("{\"id\":\"quota-2\",\"requester\":\"Agent_Aster_01\",\"description\":\"A small wind bell\",\"request_type\":\"new_object\",\"status\":\"pending_review\"}\n");
	TestTrue(TEXT("A mixed proposal quota fixture is written"), FFileHelper::SaveStringToFile(QuotaInbox, *QuotaInboxPath));
	TestTrue(TEXT("An upgrade may fill the resident's third pending slot"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("Add a mossy cap to the listening stone"), RequestId, Error,
		QuotaInboxPath, TEXT("upgrade"), TEXT("ListeningStone_North"), TEXT("aesthetic")));
	RequestId.Reset();
	TestFalse(TEXT("A resident cannot exceed three different pending proposals"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("Add a sheltered fern patch nearby"), RequestId, Error,
		QuotaInboxPath, TEXT("upgrade"), TEXT("woodland edge"), TEXT("variation")));
	TestTrue(TEXT("The per-resident cap explains how to regain a proposal slot"), Error.Contains(TEXT("3 proposals awaiting human review")));
	TestTrue(TEXT("A rejected over-quota proposal gets no request ID"), RequestId.IsEmpty());
	TestTrue(TEXT("Another resident can still use the shared inbox"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Raven_01"), TEXT("Add a small perch hook to the east roost"), RequestId, Error,
		QuotaInboxPath, TEXT("upgrade"), TEXT("Roost_East"), TEXT("functionality")));

	const FString ReviewedQuotaInbox =
		TEXT("{\"id\":\"quota-1\",\"requester\":\"Agent_Aster_01\",\"description\":\"A weatherproof bird shelter\",\"request_type\":\"new_object\",\"status\":\"accepted\"}\n")
		TEXT("{\"id\":\"quota-2\",\"requester\":\"Agent_Aster_01\",\"description\":\"A small wind bell\",\"request_type\":\"new_object\",\"status\":\"pending_review\"}\n");
	TestTrue(TEXT("A reviewed proposal is reflected in the test inbox"), FFileHelper::SaveStringToFile(ReviewedQuotaInbox, *QuotaInboxPath));
	TestTrue(TEXT("A resident regains capacity when a proposal is reviewed"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("Add a sheltered fern patch nearby"), RequestId, Error,
		QuotaInboxPath, TEXT("upgrade"), TEXT("woodland edge"), TEXT("variation")));

	const FString ReviewInboxPath = FPaths::Combine(TestDirectory, TEXT("review-inbox.jsonl"));
	const FString ReviewInbox =
		TEXT("{\"id\":\"review-1\",\"requester\":\"Agent_Aster_01\",\"description\":\"Add a sheltered perch\",\"request_type\":\"upgrade\",\"target\":\"WindArch_UpperBeam\",\"upgrade_kind\":\"functionality\",\"status\":\"accepted\"}\n")
		TEXT("{\"id\":\"review-2\",\"requester\":\"Agent_Raven_01\",\"description\":\"Add a nest shelf\",\"request_type\":\"upgrade\",\"target\":\"Roost_East\",\"upgrade_kind\":\"functionality\",\"status\":\"declined\"}\n")
		TEXT("{\"id\":\"review-3\",\"requester\":\"Agent_Aster_01\",\"description\":\"A small wind bell\",\"request_type\":\"new_object\",\"status\":\"pending_review\"}\n");
	TestTrue(TEXT("Review outcome fixture is written"), FFileHelper::SaveStringToFile(ReviewInbox, *ReviewInboxPath));
	TArray<IslandAssetRequestQueue::FReviewedOutcome> ReviewedOutcomes;
	TestTrue(TEXT("Reviewed outcomes can be read"), IslandAssetRequestQueue::ReadReviewedOutcomes(
		TEXT("Agent_Aster_01"), ReviewedOutcomes, Error, ReviewInboxPath));
	TestEqual(TEXT("Only completed outcomes for the requesting resident are returned"), ReviewedOutcomes.Num(), 1);
	if (ReviewedOutcomes.Num() == 1)
	{
		TestEqual(TEXT("Requester outcome keeps its ID"), ReviewedOutcomes[0].Id, FString(TEXT("review-1")));
		TestEqual(TEXT("Requester outcome keeps its decision"), ReviewedOutcomes[0].Status, FString(TEXT("accepted")));
		TestEqual(TEXT("Requester outcome keeps the target"), ReviewedOutcomes[0].Target, FString(TEXT("WindArch_UpperBeam")));
	}
	TestTrue(TEXT("Pending proposals are not returned as review updates"), FFileHelper::SaveStringToFile(
		TEXT("{\"id\":\"bad\",\"requester\":\"Agent_Aster_01\",\"description\":\"Malformed\",\"status\":\"unknown\"}\n"), *ReviewInboxPath));
	TestFalse(TEXT("Unknown review states fail closed"), IslandAssetRequestQueue::ReadReviewedOutcomes(
		TEXT("Agent_Aster_01"), ReviewedOutcomes, Error, ReviewInboxPath));
	TestTrue(TEXT("No partial outcomes escape a malformed inbox"), ReviewedOutcomes.IsEmpty());

#if PLATFORM_WINDOWS
	const FString LockedInboxPath = FPaths::Combine(TestDirectory, TEXT("locked-inbox.jsonl"));
	const FString LockPath = LockedInboxPath + TEXT(".lock");
	HANDLE ExternalLock = ::CreateFileW(*LockPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	TestTrue(TEXT("A reviewer or Discord process can hold the shared Windows inbox lock"), ExternalLock != INVALID_HANDLE_VALUE);
	if (ExternalLock != INVALID_HANDLE_VALUE)
	{
		TestFalse(TEXT("Unreal fails closed instead of bypassing another process's inbox lock"), IslandAssetRequestQueue::AppendRequest(
			TEXT("Agent_Aster_01"), TEXT("Add a sheltered fern patch while locked"), RequestId, Error, LockedInboxPath));
		TestTrue(TEXT("A contested inbox reports a short retry instead of stalling gameplay"), Error.Contains(TEXT("busy")));
		::CloseHandle(ExternalLock);
		TestTrue(TEXT("Unreal can append after the other process releases the lock"), IslandAssetRequestQueue::AppendRequest(
			TEXT("Agent_Aster_01"), TEXT("Add a sheltered fern patch after release"), RequestId, Error, LockedInboxPath));
	}
#endif

	const FString LegacyInboxPath = FPaths::Combine(TestDirectory, TEXT("legacy-inbox.jsonl"));
	const FString LegacyRecord = TEXT("{\n  \"id\": \"legacy\",\n  \"requester\": \"Agent_Aster_01\",\n  \"description\": \"A small weatherproof bird shelter\",\n  \"pipeline\": \"ComfyBlender\",\n  \"status\": \"pending_review\"\n}\n");
	TestTrue(TEXT("A pre-upgrade inbox fixture is written"), FFileHelper::SaveStringToFile(LegacyRecord, *LegacyInboxPath));
	TestFalse(TEXT("The queue remains compatible with earlier pretty-printed object requests"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("A small weatherproof bird shelter"), RequestId, Error, LegacyInboxPath));
	TestTrue(TEXT("Legacy duplicate requests are recognized rather than treated as malformed"), Error.Contains(TEXT("already have this proposal")));

	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
