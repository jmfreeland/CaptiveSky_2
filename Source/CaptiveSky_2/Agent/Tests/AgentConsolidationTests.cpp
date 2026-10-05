// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "AgentDataPaths.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentConsolidationTest, "CaptiveSky2.Agent.PersonalityConsolidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentConsolidationTest::RunTest(const FString& Parameters)
{
	const FString TestAgentId = TEXT("Automation_Consolidation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TestDirectory = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / TestAgentId);
	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Personality consolidation fixture world created"), World)) return false;
	if (!TestNotNull(TEXT("Engine is available for the fixture"), GEngine))
	{
		World->DestroyWorld(false);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AActor* Resident = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Consolidating resident fixture spawned"), Resident))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	UAgentMemoryComponent* Memory = NewObject<UAgentMemoryComponent>(Resident);
	if (!TestNotNull(TEXT("Resident memory component created"), Memory))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	Resident->AddInstanceComponent(Memory);
	Memory->AgentId = TestAgentId;
	Memory->RegisterComponent();
	UAgentConsolidationComponent* Consolidation = NewObject<UAgentConsolidationComponent>(Resident);
	if (!TestNotNull(TEXT("Sleep consolidation component created"), Consolidation))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	Resident->AddInstanceComponent(Consolidation);
	Consolidation->RegisterComponent();

	FAgentMemoryRecord Evidence;
	Evidence.Id = TEXT("lived-memory-1");
	Evidence.Type = EAgentMemoryType::Observation;
	Evidence.Text = TEXT("I watched the rain clear and stayed to see what changed.");
	Evidence.Timestamp = FDateTime::UtcNow();
	Evidence.Importance = 0.7f;
	const TArray<FAgentMemoryRecord> Memories = { Evidence };
	FAgentMemoryRecord ConsolidationReflection = Evidence;
	ConsolidationReflection.Id = TEXT("derived-consolidation-reflection");
	ConsolidationReflection.Type = EAgentMemoryType::Reflection;
	ConsolidationReflection.Tags.Add(TEXT("consolidation"));
	ConsolidationReflection.Timestamp = Evidence.Timestamp + FTimespan::FromSeconds(30.0);
	FAgentMemoryRecord OrdinaryReflection = Evidence;
	OrdinaryReflection.Id = TEXT("ordinary-reflection");
	OrdinaryReflection.Type = EAgentMemoryType::Reflection;
	OrdinaryReflection.Tags.Reset();
	OrdinaryReflection.Timestamp = Evidence.Timestamp + FTimespan::FromSeconds(10.0);
	FAgentMemoryRecord RecentObservation = Evidence;
	RecentObservation.Id = TEXT("lived-memory-2");
	RecentObservation.Timestamp = Evidence.Timestamp + FTimespan::FromSeconds(20.0);
	const TArray<FAgentMemoryRecord> CandidateMemories = { Evidence, OrdinaryReflection, RecentObservation, ConsolidationReflection };
	const TArray<FAgentMemoryRecord> EligibleMemories = UAgentConsolidationComponent::FilterConsolidationEvidence(CandidateMemories, 40);
	TestEqual(TEXT("Consolidation excludes its own tagged reflection from future evidence"), EligibleMemories.Num(), 3);
	TestFalse(TEXT("The derived reflection is absent from eligible evidence"),
		EligibleMemories.ContainsByPredicate([](const FAgentMemoryRecord& Record)
			{ return Record.Id == TEXT("derived-consolidation-reflection"); }));
	TestTrue(TEXT("An ordinary reflection remains eligible autobiographical evidence"),
		EligibleMemories.ContainsByPredicate([](const FAgentMemoryRecord& Record)
			{ return Record.Id == TEXT("ordinary-reflection"); }));
	const TArray<FAgentMemoryRecord> CappedMemories = UAgentConsolidationComponent::FilterConsolidationEvidence(CandidateMemories, 2);
	TestTrue(TEXT("The memory cap retains the newest eligible evidence after filtering"),
		CappedMemories.Num() == 2 && CappedMemories[0].Id == OrdinaryReflection.Id && CappedMemories[1].Id == RecentObservation.Id);
	auto MakeAdjustment = [](const TCHAR* Trait, const TCHAR* Direction, const TCHAR* EvidenceId)
	{
		return FString::Printf(TEXT("{\"trait\":\"%s\",\"direction\":\"%s\",\"amount\":0.2,\"reason\":\"A lived observation supports this small tendency.\",\"evidence_memory_ids\":[\"%s\"]}"),
			Trait, Direction, EvidenceId);
	};
	const TArray<FString> Adjustments = {
		MakeAdjustment(TEXT("Curiosity"), TEXT("strengthen"), TEXT("lived-memory-1")),
		MakeAdjustment(TEXT("cURIOSITY"), TEXT("strengthen"), TEXT("lived-memory-1")), // Duplicate names are case-insensitive.
		MakeAdjustment(TEXT("Patience"), TEXT("soften"), TEXT("invented-memory-id")), // Unsupported evidence is ignored.
		MakeAdjustment(TEXT("Humor"), TEXT("strengthen"), TEXT("lived-memory-1")),
		MakeAdjustment(TEXT("Courage"), TEXT("soften"), TEXT("lived-memory-1")),
		MakeAdjustment(TEXT("Patience"), TEXT("strengthen"), TEXT("lived-memory-1")), // Fourth valid change exceeds the per-sleep cap.
		MakeAdjustment(TEXT("Wonder"), TEXT("strengthen"), TEXT("lived-memory-1"))
	};
	const FString Response = TEXT("{\"reflection\":\"I learned from watching the storm pass.\",\"personality_adjustments\":[") +
		FString::Join(Adjustments, TEXT(",")) + TEXT("]}");

	Consolidation->MaximumAdjustmentPerSleep = 0.5f; // Runtime overrides cannot exceed the design's hard gradual-change limit.
	TestTrue(TEXT("Evidence-backed consolidation response is accepted"), Consolidation->ApplyConsolidationResponse(Response, Memories));
	TestEqual(TEXT("A single sleep changes no more than three tendencies"), Consolidation->Tendencies.Num(),
		UAgentConsolidationComponent::MaximumAdjustmentsPerSleep);
	const FAgentPersonalityTendency* Curiosity = Consolidation->Tendencies.FindByPredicate(
		[](const FAgentPersonalityTendency& Tendency) { return Tendency.Name.Equals(TEXT("Curiosity"), ESearchCase::IgnoreCase); });
	const FAgentPersonalityTendency* Humor = Consolidation->Tendencies.FindByPredicate(
		[](const FAgentPersonalityTendency& Tendency) { return Tendency.Name.Equals(TEXT("Humor"), ESearchCase::IgnoreCase); });
	const FAgentPersonalityTendency* Courage = Consolidation->Tendencies.FindByPredicate(
		[](const FAgentPersonalityTendency& Tendency) { return Tendency.Name.Equals(TEXT("Courage"), ESearchCase::IgnoreCase); });
	TestTrue(TEXT("A requested amount larger than the configured limit remains a small change"),
		Curiosity && FMath::IsNearlyEqual(Curiosity->Strength, UAgentConsolidationComponent::MaximumAdjustmentMagnitude));
	TestTrue(TEXT("The same trait cannot be amplified twice in one sleep, regardless of case"),
		Curiosity && Curiosity->EvidenceMemoryIds.Num() == 1 && Curiosity->EvidenceMemoryIds[0] == Evidence.Id);
	TestTrue(TEXT("Subsequent unique supported tendencies can still evolve in the same sleep"),
		Humor && FMath::IsNearlyEqual(Humor->Strength, UAgentConsolidationComponent::MaximumAdjustmentMagnitude) &&
		Courage && FMath::IsNearlyEqual(Courage->Strength, -UAgentConsolidationComponent::MaximumAdjustmentMagnitude));
	TestFalse(TEXT("Unsupported evidence and adjustments past the per-sleep cap do not add tendencies"),
		Consolidation->Tendencies.ContainsByPredicate([](const FAgentPersonalityTendency& Tendency)
			{ return Tendency.Name.Equals(TEXT("Patience"), ESearchCase::IgnoreCase) || Tendency.Name.Equals(TEXT("Wonder"), ESearchCase::IgnoreCase); }));

	TArray<FString> HistoryLines;
	TestTrue(TEXT("Each accepted change is represented in reversible personality history"),
		FFileHelper::LoadFileToStringArray(HistoryLines, *Consolidation->GetPersonalityHistoryPath()));
	TestEqual(TEXT("History contains one event per accepted unique tendency"), HistoryLines.Num(),
		UAgentConsolidationComponent::MaximumAdjustmentsPerSleep);
	const TArray<FAgentMemoryRecord> SavedMemories = Memory->GetMemoriesSince(FDateTime::MinValue());
	TestTrue(TEXT("The sleep reflection is still saved as a separate lived memory"),
		SavedMemories.ContainsByPredicate([](const FAgentMemoryRecord& Record)
			{ return Record.Type == EAgentMemoryType::Reflection && Record.Text.Contains(TEXT("storm pass")); }));
	TestTrue(TEXT("The bounded derived personality state was saved atomically"), FPaths::FileExists(Consolidation->GetPersonalityStatePath()));
	UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Resident);
	Resident->AddInstanceComponent(Brain);
	Brain->RegisterComponent();
	const FString EvolvedPrompt = Brain->BuildSystemPrompt({});
	TestTrue(TEXT("The next full thought receives the persisted evolving tendency"),
		EvolvedPrompt.Contains(TEXT("Evolving evidence-bound tendencies")) && EvolvedPrompt.Contains(TEXT("Curiosity: strengthening")));
	TestFalse(TEXT("The prompt receives a concise tendency, not private history evidence IDs"),
		EvolvedPrompt.Contains(TEXT("evidence_memory_ids")));
	UAgentConsolidationComponent* Reloaded = NewObject<UAgentConsolidationComponent>(Resident);
	Resident->AddInstanceComponent(Reloaded);
	Reloaded->RegisterComponent();
	TestEqual(TEXT("A later awake instance reloads the bounded personality overlay"), Reloaded->GetEvolvingTendencies().Num(),
		UAgentConsolidationComponent::MaximumAdjustmentsPerSleep);

	FString StateBeforeFailedSave;
	FString HistoryBeforeFailedSave;
	TestTrue(TEXT("The committed personality overlay can be read before the failure probe"),
		FFileHelper::LoadFileToString(StateBeforeFailedSave, *Consolidation->GetPersonalityStatePath()));
	TestTrue(TEXT("The committed personality history can be read before the failure probe"),
		FFileHelper::LoadFileToString(HistoryBeforeFailedSave, *Consolidation->GetPersonalityHistoryPath()));
	const int32 RevisionBeforeFailedSave = Consolidation->Revision;
	const FDateTime WatermarkBeforeFailedSave = Consolidation->LastConsolidatedAt;
	const FAgentPersonalityTendency* CuriosityBeforeFailedSave = Consolidation->Tendencies.FindByPredicate(
		[](const FAgentPersonalityTendency& Tendency) { return Tendency.Name.Equals(TEXT("Curiosity"), ESearchCase::IgnoreCase); });
	TestNotNull(TEXT("The existing tendency is available before the failure probe"), CuriosityBeforeFailedSave);
	const FAgentPersonalityTendency CuriositySnapshot = CuriosityBeforeFailedSave ? *CuriosityBeforeFailedSave : FAgentPersonalityTendency();
	const FString FailedSaveBlocker = Consolidation->GetPersonalityStatePath() + TEXT(".tmp");
	TestTrue(TEXT("The persistence failure fixture creates its temporary-path blocker"), IFileManager::Get().MakeDirectory(*FailedSaveBlocker, true));
	const FString FailedResponse = FString(TEXT("{\"reflection\":\"The unsaved thought must not alter who I am.\",\"personality_adjustments\":[")) +
		TEXT("{\"trait\":\"Curiosity\",\"direction\":\"soften\",\"amount\":0.02,\"reason\":\"This reason must roll back.\",\"evidence_memory_ids\":[\"lived-memory-1\"]},") +
		MakeAdjustment(TEXT("Patience"), TEXT("strengthen"), TEXT("lived-memory-1")) + TEXT("]}");
	TestFalse(TEXT("A failed overlay save rejects the consolidation transaction"), Consolidation->ApplyConsolidationResponse(FailedResponse, Memories));
	TestEqual(TEXT("A failed save restores the prior in-memory tendency count"), Consolidation->Tendencies.Num(),
		UAgentConsolidationComponent::MaximumAdjustmentsPerSleep);
	TestFalse(TEXT("An unsaved tendency is absent from the in-memory personality"),
		Consolidation->Tendencies.ContainsByPredicate([](const FAgentPersonalityTendency& Tendency)
			{ return Tendency.Name.Equals(TEXT("Patience"), ESearchCase::IgnoreCase); }));
	const FAgentPersonalityTendency* CuriosityAfterFailedSave = Consolidation->Tendencies.FindByPredicate(
		[](const FAgentPersonalityTendency& Tendency) { return Tendency.Name.Equals(TEXT("Curiosity"), ESearchCase::IgnoreCase); });
	TestTrue(TEXT("A failed save restores an existing tendency's strength and evidence metadata"),
		CuriosityAfterFailedSave &&
		FMath::IsNearlyEqual(CuriosityAfterFailedSave->Strength, CuriositySnapshot.Strength) &&
		CuriosityAfterFailedSave->LastReason == CuriositySnapshot.LastReason &&
		CuriosityAfterFailedSave->EvidenceMemoryIds == CuriositySnapshot.EvidenceMemoryIds);
	TestEqual(TEXT("A failed save restores the prior personality revision"), Consolidation->Revision, RevisionBeforeFailedSave);
	TestTrue(TEXT("A failed save restores the previous consolidation watermark"), Consolidation->LastConsolidatedAt == WatermarkBeforeFailedSave);
	FString StateAfterFailedSave;
	FString HistoryAfterFailedSave;
	TestTrue(TEXT("The failed save preserves the previous overlay file"),
		FFileHelper::LoadFileToString(StateAfterFailedSave, *Consolidation->GetPersonalityStatePath()) && StateAfterFailedSave == StateBeforeFailedSave);
	TestTrue(TEXT("The failed save does not append a misleading personality history event"),
		FFileHelper::LoadFileToString(HistoryAfterFailedSave, *Consolidation->GetPersonalityHistoryPath()) && HistoryAfterFailedSave == HistoryBeforeFailedSave);
	TestEqual(TEXT("The failed save does not leave behind its sleep reflection"), Memory->GetMemoryCount(), 1);
	IFileManager::Get().DeleteDirectory(*FailedSaveBlocker, false, true);

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}
