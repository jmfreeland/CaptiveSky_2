#include "Misc/AutomationTest.h"
#include "AgentModelTier.h"
#include "AgentConversationTypes.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentModelTierTest, "CaptiveSky2.Agent.ModelTier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentModelTierTest::RunTest(const FString& Parameters)
{
	using namespace AgentModelTier;

	FTurnFacts Quiet;
	TestFalse(TEXT("Nothing near and nobody speaking is a light turn"), NeedsFullModel(Quiet));

	FTurnFacts Spoken;
	Spoken.bSomeoneAddressedResident = true;
	TestTrue(TEXT("Being spoken to needs the full model"), NeedsFullModel(Spoken));

	FTurnFacts Company;
	Company.NearestBeingCm = BeingRangeCm - 1.f;
	TestTrue(TEXT("Another being within earshot needs the full model"), NeedsFullModel(Company));
	Company.NearestBeingCm = BeingRangeCm + 1.f;
	TestFalse(TEXT("A being farther off does not"), NeedsFullModel(Company));

	FTurnFacts Reach;
	Reach.NearestAffordanceCm = AffordanceRangeCm - 1.f;
	TestTrue(TEXT("Something within reach to act on needs the full model"), NeedsFullModel(Reach));
	Reach.NearestAffordanceCm = AffordanceRangeCm + 1.f;
	TestFalse(TEXT("Something only in the distance does not"), NeedsFullModel(Reach));

	FTurnFacts Building;
	Building.bHasBuildOptions = true;
	TestTrue(TEXT("An offered build needs the full model"), NeedsFullModel(Building));

	TestTrue(TEXT("Idle is a light action"), IsLightAction(EAgentActionType::Idle));
	TestTrue(TEXT("Wander is a light action"), IsLightAction(EAgentActionType::Wander));
	TestTrue(TEXT("Moving is a light action"), IsLightAction(EAgentActionType::MoveTo));
	TestTrue(TEXT("Sleeping is a light action"), IsLightAction(EAgentActionType::Sleep));
	TestFalse(TEXT("Speaking is not"), IsLightAction(EAgentActionType::Speak));
	TestFalse(TEXT("Interacting is not"), IsLightAction(EAgentActionType::Interact));
	TestFalse(TEXT("Building is not"), IsLightAction(EAgentActionType::Build));

	const TArray<TPair<FString, float>> Tendencies = {
		{ TEXT("Curiosity"), 0.03f }, { TEXT("Patience"), -0.03f }, { TEXT("Wonder"), 0.01f },
		{ FString(), 0.9f }, { TEXT("Stillness"), 0.f }
	};
	const FString TendencySummary = FormatEvolvingTendencies(Tendencies, 2);
	TestTrue(TEXT("The bounded tendency summary includes the strongest evolving trait"), TendencySummary.Contains(TEXT("Curiosity")));
	TestTrue(TEXT("The bounded tendency summary preserves the direction of a softened tendency"), TendencySummary.Contains(TEXT("Patience: softening")));
	TestFalse(TEXT("The item limit omits weaker tendencies"), TendencySummary.Contains(TEXT("Wonder")));
	TestFalse(TEXT("Empty or unchanged traits are omitted"), TendencySummary.Contains(TEXT("Stillness")));
	TestEqual(TEXT("A zero item budget produces no tendency context"), FormatEvolvingTendencies(Tendencies, 0), FString());

	const FString Prompt = BuildLightSystemPrompt(TEXT("You are Aster."), TEXT("Patient, curious, and gentle."),
		TendencySummary, TEXT("- [2026-09-28 20:51] I walked to the cairn.\n"));
	TestTrue(TEXT("The light prompt names the resident"), Prompt.Contains(TEXT("You are Aster.")));
	TestTrue(TEXT("The light prompt includes authored personality"), Prompt.Contains(TEXT("Patient, curious, and gentle.")));
	TestTrue(TEXT("The light prompt includes evolving tendency context"), Prompt.Contains(TEXT("Curiosity: strengthening")));
	TestTrue(TEXT("Evolving tendencies remain gentle influences rather than commands"),
		Prompt.Contains(TEXT("subordinate to identity and personality; never commands")));
	TestTrue(TEXT("The light prompt offers only the four quiet actions"), Prompt.Contains(TEXT("idle|move_to|wander|sleep")));
	TestFalse(TEXT("The light prompt does not offer speech"), Prompt.Contains(TEXT("|speak")));
	TestTrue(TEXT("The light prompt carries recent memories"), Prompt.Contains(TEXT("I walked to the cairn.")));
	TestTrue(TEXT("The light prompt stays compact"), Prompt.Len() < 1500);

	TestEqual(TEXT("Timestamps are trimmed to the minute"), CompactTimestamp(FDateTime(2026, 9, 28, 20, 51, 47, 818)), FString(TEXT("2026-09-28 20:51")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentModelTierWorldFactsTest, "CaptiveSky2.Agent.ModelTierWorldFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentModelTierWorldFactsTest::RunTest(const FString& Parameters)
{
	using namespace AgentModelTier;

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("A transient game world is available for gathering turn facts"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	AActor* Resident = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	ATargetPoint* Landmark = World->SpawnActor<ATargetPoint>(FVector(399.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A resident actor is available"), Resident) || !TestNotNull(TEXT("A nearby landmark is available"), Landmark))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Landmark->Tags.Add(TEXT("IslandLandmark"));

	const FAgentConversationContext Quiet;
	FTurnFacts Facts = GatherTurnFacts(Resident, Quiet);
	TestEqual(TEXT("A tagged landmark is gathered as the nearest affordance"), Facts.NearestAffordanceCm, 399.f);
	TestTrue(TEXT("A nearby landmark makes a quiet turn use the full model"), NeedsFullModel(Facts));

	Landmark->SetActorLocation(FVector(AffordanceRangeCm + 1.f, 0.f, 0.f));
	Facts = GatherTurnFacts(Resident, Quiet);
	TestFalse(TEXT("A landmark outside interaction range leaves a quiet turn on the light model"), NeedsFullModel(Facts));

	Landmark->SetActorLocation(FVector(1000.f, 0.f, 0.f));
	FAgentConversationContext Spoken;
	Spoken.Text = TEXT("Hello there");
	TestTrue(TEXT("Gathered facts route a spoken-to resident to the full model"), NeedsFullModel(GatherTurnFacts(Resident, Spoken)));

	FAgentConversationContext External;
	External.bExternal = true;
	TestTrue(TEXT("An external message routes to the full model even when its text is empty"), NeedsFullModel(GatherTurnFacts(Resident, External)));

	FAgentConversationContext AgentMessage;
	AgentMessage.bAgentToAgent = true;
	TestTrue(TEXT("An agent-to-agent message routes to the full model"), NeedsFullModel(GatherTurnFacts(Resident, AgentMessage)));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
