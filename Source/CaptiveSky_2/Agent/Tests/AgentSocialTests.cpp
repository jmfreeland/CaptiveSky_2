#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "AgentDataPaths.h"
#include "AgentMemoryComponent.h"
#include "AgentMemoryTypes.h"
#include "AgentSocialComponent.h"
#include "AutonomousAgentCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "IslandSocialTestCharacter.h"
#include "IslandWorldStateSubsystem.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentSocialPacingTest, "CaptiveSky2.Agent.SocialPacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentSocialPacingTest::RunTest(const FString& Parameters)
{
	const UAgentSocialComponent* Defaults = GetDefault<UAgentSocialComponent>();
	if (!TestNotNull(TEXT("Social component defaults exist"), Defaults)) return false;

	TestEqual(TEXT("Autonomous conversations remain bounded to four spoken lines"), Defaults->MaximumConversationTurns, 4);
	TestEqual(TEXT("Pair cooldown gives residents five minutes between automatic exchanges"), Defaults->ConversationCooldownSeconds, 300.f);
	TestEqual(TEXT("Automatic replies pause at least fifteen seconds between spoken lines"), Defaults->ConversationReplyPauseSeconds, 15.f);
	TestEqual(TEXT("A short Blueprint reply pause cannot restore rapid-fire conversation"),
		UAgentSocialComponent::EffectiveConversationReplyPauseSeconds(3.f), 15.f);
	TestEqual(TEXT("Longer designer-authored reply pauses remain configurable"),
		UAgentSocialComponent::EffectiveConversationReplyPauseSeconds(30.f), 30.f);
	TestEqual(TEXT("Non-finite reply-pause overrides fall back to fifteen seconds"),
		UAgentSocialComponent::EffectiveConversationReplyPauseSeconds(std::numeric_limits<float>::quiet_NaN()), 15.f);
	TestEqual(TEXT("An old short Blueprint cooldown cannot restore rapid exchanges"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(45.f), 300.f);
	TestEqual(TEXT("Zero cannot disable the autonomous social pause"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(0.f), 300.f);
	TestEqual(TEXT("Longer designer-authored cooldowns remain configurable"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(420.f), 420.f);
	TestEqual(TEXT("Non-finite overrides fall back to the safe five-minute pause"),
		UAgentSocialComponent::EffectiveConversationCooldownSeconds(std::numeric_limits<float>::quiet_NaN()), 300.f);

	UAgentSocialComponent* Social = NewObject<UAgentSocialComponent>();
	TestEqual(TEXT("A resident with no previous exchange has no pair cooldown"),
		Social->GetConversationCooldownRemainingWith(TEXT("aster")), 0.f);
	Social->CooldownUntilByAgentId.Add(TEXT("aster"), FPlatformTime::Seconds() + 90.0);
	const float Remaining = Social->GetConversationCooldownRemainingWith(TEXT("aster"));
	TestTrue(TEXT("The pair cooldown exposes remaining real time to the resident's situation summary"),
		Remaining > 89.f && Remaining <= 90.f);
	Social->CooldownUntilByAgentId.Add(TEXT("aster"), FPlatformTime::Seconds() - 1.0);
	TestEqual(TEXT("An elapsed pair cooldown is reported as zero"),
		Social->GetConversationCooldownRemainingWith(TEXT("aster")), 0.f);
	Social->ConversationReplyNotBeforeSeconds = FPlatformTime::Seconds() + 15.0;
	TestTrue(TEXT("A pending conversational reply waits for its real-time pause"),
		Social->IsConversationReplyPauseActive());
	Social->ConversationReplyNotBeforeSeconds = FPlatformTime::Seconds() - 1.0;
	TestFalse(TEXT("A conversational reply resumes when its pause elapses"),
		Social->IsConversationReplyPauseActive());

	UAgentSocialComponent* RavenSocial = NewObject<UAgentSocialComponent>();
	UAgentSocialComponent* InnkeeperSocial = NewObject<UAgentSocialComponent>();
	const FString ConversationId = TEXT("social-pacing-fixture");
	TestTrue(TEXT("A spontaneous exchange reserves both residents before delivering its opener"),
		Social->TryReserveAutomaticConversation(RavenSocial, TEXT("aster"), TEXT("raven"), ConversationId));
	TestTrue(TEXT("An engaged resident exposes its busy state to the autonomous-thought scheduler"),
		Social->IsAutomaticConversationActive());
	TestTrue(TEXT("The counterpart sees the same active exchange reservation"),
		RavenSocial->IsReservedFor(TEXT("aster"), ConversationId));
	TestFalse(TEXT("The pair cannot open a simultaneous reverse exchange"),
		RavenSocial->TryReserveAutomaticConversation(Social, TEXT("raven"), TEXT("aster"), TEXT("reverse")));
	TestFalse(TEXT("A resident already speaking cannot begin a second exchange with another resident"),
		Social->TryReserveAutomaticConversation(InnkeeperSocial, TEXT("aster"), TEXT("innkeeper"), TEXT("overlap")));

	Social->ReleaseAutomaticConversation(RavenSocial, ConversationId);
	TestFalse(TEXT("Ending an exchange releases the initiator reservation"),
		Social->IsReservedFor(TEXT("raven"), ConversationId));
	TestFalse(TEXT("Ending an exchange releases the counterpart reservation"),
		RavenSocial->IsReservedFor(TEXT("aster"), ConversationId));
	TestFalse(TEXT("Ending an exchange makes the resident eligible for later independent thoughts"),
		Social->IsAutomaticConversationActive());

	TestTrue(TEXT("A fresh pair can begin after its prior reservation is released"),
		Social->TryReserveAutomaticConversation(RavenSocial, TEXT("aster"), TEXT("raven"), ConversationId));
	Social->CooldownUntilByAgentId.Add(TEXT("raven"), FPlatformTime::Seconds() + 300.0);
	RavenSocial->CooldownUntilByAgentId.Add(TEXT("aster"), FPlatformTime::Seconds() + 300.0);
	Social->ReleaseAutomaticConversation(RavenSocial, ConversationId);
	TestFalse(TEXT("The mutual cooldown blocks an immediate repeat exchange"),
		Social->TryReserveAutomaticConversation(RavenSocial, TEXT("aster"), TEXT("raven"), TEXT("repeat")));

	Social->MaximumConversationTurns = 4;
	TestFalse(TEXT("The third line may receive a reply within the four-line limit"),
		Social->IsConversationAtTurnLimit(3));
	TestTrue(TEXT("The fourth spoken line ends the exchange without requesting a fifth line"),
		Social->IsConversationAtTurnLimit(4));
	Social->MaximumConversationTurns = 1;
	TestTrue(TEXT("A one-line configured exchange does not queue a reply to its opener"),
		Social->IsConversationAtTurnLimit(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentCraftTeachingTest, "CaptiveSky2.Agent.CraftTeaching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentCraftTeachingTest::RunTest(const FString& Parameters)
{
	const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TeacherId = TEXT("CraftTeacher_") + Suffix;
	const FString LearnerId = TEXT("CraftLearner_") + Suffix;
	const FString UnknownLearnerId = TEXT("CraftUnknownLearner_") + Suffix;
	const FString DistantLearnerId = TEXT("CraftDistantLearner_") + Suffix;
	const FString StateFile = FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("CraftTeaching") / (Suffix + TEXT(".json"));
	const FString ConversationId = TEXT("craft-teaching-") + Suffix;
	const FString UnknownConversationId = TEXT("craft-unknown-") + Suffix;
	const FString DistantConversationId = TEXT("craft-distant-") + Suffix;

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Craft teaching fixture world is created"), World)) return false;
	if (!TestNotNull(TEXT("Engine exists for the social fixture"), GEngine))
	{
		World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	UIslandWorldStateSubsystem* WorldState = World->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!TestNotNull(TEXT("Persistent world state is available in the fixture"), WorldState))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	WorldState->StorageFileOverride = StateFile;

	AIslandSocialTestCharacter* Teacher = World->SpawnActor<AIslandSocialTestCharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
	AIslandSocialTestCharacter* Learner = World->SpawnActor<AIslandSocialTestCharacter>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
	AIslandSocialTestCharacter* UnknownLearner = World->SpawnActor<AIslandSocialTestCharacter>(FVector(120.f, 0.f, 0.f), FRotator::ZeroRotator);
	AIslandSocialTestCharacter* DistantLearner = World->SpawnActor<AIslandSocialTestCharacter>(FVector(5000.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The teacher body is spawned"), Teacher) || !TestNotNull(TEXT("The nearby learner body is spawned"), Learner) ||
		!TestNotNull(TEXT("The second learner body is spawned"), UnknownLearner) || !TestNotNull(TEXT("The distant learner body is spawned"), DistantLearner))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		IFileManager::Get().Delete(*StateFile, false, true, true);
		return false;
	}
	Teacher->Memory->AgentId = TeacherId;
	Learner->Memory->AgentId = LearnerId;
	UnknownLearner->Memory->AgentId = UnknownLearnerId;
	DistantLearner->Memory->AgentId = DistantLearnerId;

	FIslandCraftParameter Moisture;
	Moisture.Id = TEXT("moisture");
	Moisture.Value = 0.64f;
	TestTrue(TEXT("A real first practice establishes a tradition before anyone can teach it"),
		WorldState->RecordCraftAttempt(TEXT("HearthLoaf"), TEXT("A hearth loaf"), TeacherId, 1,
			TEXT("A remembered family recipe"), { Moisture }, EIslandCraftOutcome::Promising));
	TestFalse(TEXT("The learner begins without preordained craft knowledge"), WorldState->HasCraftKnowledge(TEXT("HearthLoaf"), LearnerId));
	const FString TeacherSituation = Teacher->Brain->BuildSituationSummary(FAgentConversationContext());
	const FString LearnerSituation = Learner->Brain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Only a resident who knows the tradition sees its exact optional teaching ID"),
		TeacherSituation.Contains(TEXT("teach_craft ID: HearthLoaf")) && !LearnerSituation.Contains(TEXT("teach_craft ID: HearthLoaf")));
	TArray<FAgentMemoryRecord> NoMemories;
	TestTrue(TEXT("The one scheduled decision prompt describes optional resident-to-resident craft teaching"),
		Teacher->Brain->BuildSystemPrompt(NoMemories).Contains(TEXT("teach_craft")));
	const FString Response = TEXT("{\"thought\":\"share a remembered method\",\"action\":{\"type\":\"speak\",\"target\":\"CraftLearner\",\"speech\":\"I remember a slow rise for this loaf.\",\"teach_craft\":\"HearthLoaf\"},\"new_memories\":[]}");
	const FAgentDecision Decision = UAgentBrainComponent::ParseDecisionAndStoreMemories(Response, nullptr);
	TestTrue(TEXT("The decision parser preserves the optional craft lineage ID"),
		Decision.bValid && Decision.ActionType == EAgentActionType::Speak && Decision.TeachCraft == TEXT("HearthLoaf"));

	const bool bReserved = Teacher->Social->TryReserveAutomaticConversation(Learner->Social, TeacherId, LearnerId, ConversationId);
	TestTrue(TEXT("The teacher and learner enter the existing bounded conversation path"), bReserved);
	if (bReserved)
		Teacher->Social->DeliverSpeech(Learner, Decision.Speech, ConversationId, 1, FString(), Decision.TeachCraft);
	const FIslandCraftTraditionRecord* Tradition = WorldState->FindCraftTradition(TEXT("HearthLoaf"));
	TestTrue(TEXT("A delivered nearby resident line grants exactly attributed craft knowledge"),
		WorldState->HasCraftKnowledge(TEXT("HearthLoaf"), LearnerId) && Tradition && Tradition->Lessons.Num() == 1 &&
		Tradition->Lessons[0].TeacherAgentId == TeacherId && Tradition->Lessons[0].LearnerAgentId == LearnerId);
	TestEqual(TEXT("Teaching queues the ordinary reply but starts no additional model request"),
		Learner->Brain->bRequestInFlight, false);

	Teacher->Social->ReleaseAutomaticConversation(Learner->Social, ConversationId);
	TestTrue(TEXT("An unrelated nearby conversation can be reserved afterward"),
		Teacher->Social->TryReserveAutomaticConversation(UnknownLearner->Social, TeacherId, UnknownLearnerId, UnknownConversationId));
	Teacher->Social->DeliverSpeech(UnknownLearner, TEXT("I describe a practice I do not know."), UnknownConversationId, 1,
		FString(), TEXT("InventedTradition"));
	TestFalse(TEXT("An invented tradition ID cannot grant craft knowledge"),
		WorldState->HasCraftKnowledge(TEXT("InventedTradition"), UnknownLearnerId));
	Teacher->Social->ReleaseAutomaticConversation(UnknownLearner->Social, UnknownConversationId);

	TestTrue(TEXT("The distant pair can reserve independently"),
		Teacher->Social->TryReserveAutomaticConversation(DistantLearner->Social, TeacherId, DistantLearnerId, DistantConversationId));
	Teacher->Social->DeliverSpeech(DistantLearner, TEXT("This line is out of range."), DistantConversationId, 1,
		FString(), TEXT("HearthLoaf"));
	TestFalse(TEXT("A line outside speaking range teaches nothing"), WorldState->HasCraftKnowledge(TEXT("HearthLoaf"), DistantLearnerId));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	for (const FString& AgentId : { TeacherId, LearnerId, UnknownLearnerId, DistantLearnerId })
		IFileManager::Get().DeleteDirectory(*CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / AgentId), false, true);
	return true;
}
