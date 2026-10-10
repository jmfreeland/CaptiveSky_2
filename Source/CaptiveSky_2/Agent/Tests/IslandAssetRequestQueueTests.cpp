// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Agent/IslandAssetRequestQueue.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
