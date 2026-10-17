#include "Misc/AutomationTest.h"
#include "RatAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AutonomousAgentCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "IslandListeningStonesChime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRatAgentAIControllerTest, "CaptiveSky2.Agent.RatLocomotionAnimation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRatAgentAIControllerTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A quiet rat uses its idle animation"), ARatAgentAIController::SelectLocomotionMode(0.f, false) == ERatLocomotionMode::Idle);
	TestTrue(TEXT("Tiny animation noise does not trigger locomotion"), ARatAgentAIController::SelectLocomotionMode(8.f, false) == ERatLocomotionMode::Idle);
	TestTrue(TEXT("Ordinary travel uses the walk cycle"), ARatAgentAIController::SelectLocomotionMode(60.f, false) == ERatLocomotionMode::Walking);
	TestTrue(TEXT("A quick dash uses the run cycle"), ARatAgentAIController::SelectLocomotionMode(140.f, false) == ERatLocomotionMode::Running);
	TestTrue(TEXT("Sleep takes priority over residual capsule velocity"), ARatAgentAIController::SelectLocomotionMode(80.f, true) == ERatLocomotionMode::Sleeping);
	TestTrue(TEXT("Non-finite movement falls back to idle"), ARatAgentAIController::SelectLocomotionMode(std::numeric_limits<float>::quiet_NaN(), false) == ERatLocomotionMode::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRatRavenAttentionTest, "CaptiveSky2.Agent.RatRavenAttention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRatRavenAttentionTest::RunTest(const FString& Parameters)
{
	using Rat = ARatAgentAIController;
	TestTrue(TEXT("A quiet rat notices a nearby visible raven"),
		Rat::ShouldBeginRavenNotice(Rat::RavenNoticeRadius, true, false, false, false, false));
	TestFalse(TEXT("A wall or thicket prevents an unseen glimpse"),
		Rat::ShouldBeginRavenNotice(300.f, false, false, false, false, false));
	TestFalse(TEXT("A moving rat keeps following its chosen route"),
		Rat::ShouldBeginRavenNotice(300.f, true, true, false, false, false));
	TestFalse(TEXT("A resting rat is not pulled out of sleep"),
		Rat::ShouldBeginRavenNotice(300.f, true, false, true, false, false));
	TestFalse(TEXT("The rat does not disturb a resting raven"),
		Rat::ShouldBeginRavenNotice(300.f, true, false, false, true, false));
	TestFalse(TEXT("The cooldown prevents repetitive glances"),
		Rat::ShouldBeginRavenNotice(300.f, true, false, false, false, true));
	TestFalse(TEXT("The glimpse has a strict range"),
		Rat::ShouldBeginRavenNotice(Rat::RavenNoticeRadius + 1.f, true, false, false, false, false));
	TestFalse(TEXT("Invalid negative distance never starts an attention cue"),
		Rat::ShouldBeginRavenNotice(-1.f, true, false, false, false, false));
	TestFalse(TEXT("Invalid distances never start an attention cue"),
		Rat::ShouldBeginRavenNotice(std::numeric_limits<float>::quiet_NaN(), true, false, false, false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRatListeningStonesAttentionTest, "CaptiveSky2.Agent.RatListeningStonesAttention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRatListeningStonesAttentionTest::RunTest(const FString& Parameters)
{
	using Rat = ARatAgentAIController;
	TestTrue(TEXT("A quiet rat briefly notices a nearby audible stone chime"),
		Rat::ShouldBeginStoneChimeNotice(900.f, true, false, false, false));
	TestFalse(TEXT("A faded or inaudible tone is ignored"),
		Rat::ShouldBeginStoneChimeNotice(300.f, false, false, false, false));
	TestFalse(TEXT("An active rat action keeps its chosen focus"),
		Rat::ShouldBeginStoneChimeNotice(300.f, true, true, false, false));
	TestFalse(TEXT("A resting rat is not pulled out of sleep"),
		Rat::ShouldBeginStoneChimeNotice(300.f, true, false, true, false));
	TestFalse(TEXT("The rat does not repeatedly react during its attention cooldown"),
		Rat::ShouldBeginStoneChimeNotice(300.f, true, false, false, true));
	TestFalse(TEXT("A chime outside its audible radius cannot attract the rat"),
		Rat::ShouldBeginStoneChimeNotice(AIslandListeningStonesChime::AudibleRadius + 1.f, true, false, false, false));
	TestFalse(TEXT("Invalid negative distance never starts a sound cue"),
		Rat::ShouldBeginStoneChimeNotice(-1.f, true, false, false, false));
	TestFalse(TEXT("Invalid distances never start a sound cue"),
		Rat::ShouldBeginStoneChimeNotice(std::numeric_limits<float>::quiet_NaN(), true, false, false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRatAnimationAssetsTest, "CaptiveSky2.Agent.RatAnimationAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRatAnimationAssetsTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* RatMesh = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/Characters/Creatures/Rat/SK_Rat_01.SK_Rat_01"));
	if (!TestNotNull(TEXT("The imported Rat body is present"), RatMesh)) return false;
	UClass* RatBodyClass = LoadObject<UClass>(nullptr,
		TEXT("/Game/Agents/BP_Rat_Placeholder.BP_Rat_Placeholder_C"));
	if (!TestNotNull(TEXT("The Rat placeholder body class is present"), RatBodyClass)) return false;

	static const TCHAR* AnimationNames[] = {
		TEXT("ANIM_Fox_IdleBreathe"), TEXT("ANIM_Fox_IdleLookAround"), TEXT("ANIM_Fox_Walk"),
		TEXT("ANIM_Fox_Run"), TEXT("ANIM_Fox_Sleeping")
	};
	for (const TCHAR* Name : AnimationNames)
	{
		const FString Path = FString::Printf(TEXT("/Game/AnimalVarietyPack/Fox/Animations/%s.%s"), Name, Name);
		UAnimSequence* Animation = LoadObject<UAnimSequence>(nullptr, *Path);
		if (!TestNotNull(FString::Printf(TEXT("Fox animation %s is available"), Name), Animation)) continue;
		TestTrue(FString::Printf(TEXT("Fox animation %s uses the Rat's Fox skeleton"), Name),
			Animation->GetSkeleton() == RatMesh->GetSkeleton());
	}

	if (!TestNotNull(TEXT("Engine is available for the transient animation fixture"), GEngine)) return false;
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Transient Rat animation world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAutonomousAgentCharacter* Rat = World->SpawnActor<AAutonomousAgentCharacter>(RatBodyClass, FTransform::Identity, Spawn);
	ARatAgentAIController* Controller = World->SpawnActor<ARatAgentAIController>(ARatAgentAIController::StaticClass(),
		FTransform::Identity, Spawn);
	if (!TestNotNull(TEXT("Transient autonomous Rat body spawned"), Rat) ||
		!TestNotNull(TEXT("Transient Rat controller spawned"), Controller))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}

	Rat->GetMesh()->SetSkeletalMeshAsset(RatMesh);
	Controller->Possess(Rat);
	TestTrue(TEXT("Possession switches the Rat mesh to single-node animation mode"),
		Rat->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationSingleNode);
	TestTrue(TEXT("Possession starts the breathing idle clip"), Controller->CurrentAnimation == Controller->IdleAnimation &&
		Rat->GetMesh()->GetSingleNodeInstance() && Rat->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() == Controller->IdleAnimation);
	Controller->UpdateAnimationForMotion(0.f, 60.f, false);
	TestTrue(TEXT("Walking selects and plays the compatible walk clip"), Controller->CurrentAnimation == Controller->WalkAnimation &&
		Rat->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() == Controller->WalkAnimation);
	Controller->UpdateAnimationForMotion(0.f, 140.f, false);
	TestTrue(TEXT("Running selects and plays the compatible run clip"), Controller->CurrentAnimation == Controller->RunAnimation &&
		Rat->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() == Controller->RunAnimation);
	Controller->UpdateAnimationForMotion(0.f, 0.f, true);
	TestTrue(TEXT("Sleeping selects and plays the compatible sleep clip"), Controller->CurrentAnimation == Controller->SleepAnimation &&
		Rat->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() == Controller->SleepAnimation);
	Controller->UpdateAnimationForMotion(0.f, 0.f, false);
	TestTrue(TEXT("Waking returns to the breathing idle clip"), Controller->CurrentAnimation == Controller->IdleAnimation &&
		Rat->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() == Controller->IdleAnimation);
	TestTrue(TEXT("The transient fixture never dispatches an LLM request"), !Rat->Brain || !Rat->Brain->bRequestInFlight);

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRatPeriwinkleAttentionTest, "CaptiveSky2.Agent.RatPeriwinkleAttention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRatPeriwinkleAttentionTest::RunTest(const FString& Parameters)
{
	using Rat = ARatAgentAIController;
	TestTrue(TEXT("An idle rat notices a nearby visible periwinkle"),
		Rat::ShouldBeginPeriwinkleNotice(Rat::PeriwinkleNoticeRadius, true, false, false, false));
	TestFalse(TEXT("A wall or terrain fold hides the small snail"),
		Rat::ShouldBeginPeriwinkleNotice(200.f, false, false, false, false));
	TestFalse(TEXT("A rat following an action does not divert its attention"),
		Rat::ShouldBeginPeriwinkleNotice(200.f, true, true, false, false));
	TestFalse(TEXT("A resting rat remains asleep"),
		Rat::ShouldBeginPeriwinkleNotice(200.f, true, false, true, false));
	TestFalse(TEXT("The attention cooldown prevents repetitive glances"),
		Rat::ShouldBeginPeriwinkleNotice(200.f, true, false, false, true));
	TestFalse(TEXT("A snail outside close range is ignored"),
		Rat::ShouldBeginPeriwinkleNotice(Rat::PeriwinkleNoticeRadius + 1.f, true, false, false, false));
	TestFalse(TEXT("Invalid distance is ignored"),
		Rat::ShouldBeginPeriwinkleNotice(std::numeric_limits<float>::infinity(), true, false, false, false));
	return true;
}
