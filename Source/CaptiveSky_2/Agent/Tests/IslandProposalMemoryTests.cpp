// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Agent/AgentMemoryComponent.h"
#include "Agent/AutonomousAgentAIController.h"
#include "Agent/AgentBrainComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandProposalMemoryTest,
	"CaptiveSky2.Agent.IslandProposalMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandProposalMemoryTest::RunTest(const FString& Parameters)
{
	const FString TestRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") /
		TEXT("IslandProposalMemory") / FGuid::NewGuid().ToString(EGuidFormats::Digits));
	IFileManager::Get().MakeDirectory(*TestRoot, true);
	const FString OriginalCommandLine = FCommandLine::Get();
	FCommandLine::Set(*(OriginalCommandLine + TEXT(" -CaptiveSkyDataRoot=\"") + TestRoot + TEXT("\"")));

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Isolated proposal-memory world created"), World))
	{
		FCommandLine::Set(*OriginalCommandLine);
		IFileManager::Get().DeleteDirectory(*TestRoot, false, true);
		return false;
	}
	if (!TestNotNull(TEXT("Engine is available for the fixture"), GEngine))
	{
		World->DestroyWorld(false);
		FCommandLine::Set(*OriginalCommandLine);
		IFileManager::Get().DeleteDirectory(*TestRoot, false, true);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->BeginPlay();

	ACharacter* Resident = World->SpawnActor<ACharacter>();
	AAutonomousAgentAIController* Controller = World->SpawnActor<AAutonomousAgentAIController>();
	UAgentMemoryComponent* Memory = Resident ? NewObject<UAgentMemoryComponent>(Resident) : nullptr;
	UAgentBrainComponent* Brain = Resident ? NewObject<UAgentBrainComponent>(Resident) : nullptr;
	if (!TestNotNull(TEXT("Proposal resident spawned"), Resident) ||
		!TestNotNull(TEXT("Proposal controller spawned"), Controller) ||
		!TestNotNull(TEXT("Resident memory component created"), Memory) ||
		!TestNotNull(TEXT("Resident decision brain created"), Brain))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		FCommandLine::Set(*OriginalCommandLine);
		IFileManager::Get().DeleteDirectory(*TestRoot, false, true);
		return false;
	}
	Resident->AddInstanceComponent(Memory);
	Memory->AgentId = TEXT("ProposalResident_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Memory->RegisterComponent();
	Resident->AddInstanceComponent(Brain);
	Brain->RegisterComponent();
	Controller->Possess(Resident);

	FAgentDecision ObjectRequest;
	ObjectRequest.bValid = true;
	ObjectRequest.ActionType = EAgentActionType::RequestObject;
	ObjectRequest.ObjectRequest = TEXT("A small weatherproof shelter beside the garden wall");
	Brain->OnDecisionReady.Broadcast(ObjectRequest);
	TArray<FAgentMemoryRecord> Memories = Memory->GetMemoriesSince(FDateTime::MinValue());
	TestEqual(TEXT("A successfully queued object proposal is remembered immediately"), Memories.Num(), 1);
	if (Memories.Num() == 1)
	{
		TestTrue(TEXT("The object memory states the proposal is pending review and not yet made"),
			Memories[0].Text.Contains(ObjectRequest.ObjectRequest) && Memories[0].Text.Contains(TEXT("pending human review")) &&
			Memories[0].Text.Contains(TEXT("nothing has been made or placed")));
		TestTrue(TEXT("The object memory has a stable proposal ID tag"),
			Memories[0].Tags.Contains(TEXT("world-proposal")) && Memories[0].Tags.Contains(TEXT("proposal-pending")) &&
			Memories[0].Tags.ContainsByPredicate([](const FString& Tag) { return Tag.StartsWith(TEXT("proposal-id:")); }));
	}

	Brain->OnDecisionReady.Broadcast(ObjectRequest);
	TestEqual(TEXT("A duplicate rejected by the queue does not create another memory"),
		Memory->GetMemoriesSince(FDateTime::MinValue()).Num(), 1);

	FAgentDecision UpgradeRequest;
	UpgradeRequest.bValid = true;
	UpgradeRequest.ActionType = EAgentActionType::RequestUpgrade;
	UpgradeRequest.UpgradeTarget = TEXT("WindArch_UpperBeam");
	UpgradeRequest.UpgradeKind = TEXT("functionality");
	UpgradeRequest.UpgradeRequest = TEXT("Add a sheltered perch near the upper beam");
	Brain->OnDecisionReady.Broadcast(UpgradeRequest);
	Memories = Memory->GetMemoriesSince(FDateTime::MinValue());
	TestEqual(TEXT("A successfully queued upgrade proposal is remembered"), Memories.Num(), 2);
	if (Memories.Num() == 2)
	{
		const FAgentMemoryRecord& UpgradeMemory = Memories[1];
		TestTrue(TEXT("Upgrade memory preserves its target, category, and pending state"),
			UpgradeMemory.Text.Contains(UpgradeRequest.UpgradeTarget) && UpgradeMemory.Text.Contains(UpgradeRequest.UpgradeKind) &&
			UpgradeMemory.Text.Contains(UpgradeRequest.UpgradeRequest) && UpgradeMemory.Text.Contains(TEXT("pending human review")));
	}

	FAgentDecision InvalidRequest;
	InvalidRequest.bValid = true;
	InvalidRequest.ActionType = EAgentActionType::RequestObject;
	InvalidRequest.ObjectRequest = TEXT(" ");
	Brain->OnDecisionReady.Broadcast(InvalidRequest);
	TestEqual(TEXT("A request rejected before queueing does not create memory"),
		Memory->GetMemoriesSince(FDateTime::MinValue()).Num(), 2);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	FCommandLine::Set(*OriginalCommandLine);
	IFileManager::Get().DeleteDirectory(*TestRoot, false, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
