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
	TestFalse(TEXT("An empty description is rejected"), IslandAssetRequestQueue::AppendRequest(
		TEXT("Agent_Aster_01"), TEXT("  "), RequestId, Error, InboxPath));

	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
