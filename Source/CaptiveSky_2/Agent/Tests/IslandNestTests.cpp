#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "IslandNest.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandNestTest, "CaptiveSky2.Agent.IslandNest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/** Fixture world with its lasting state pointed at a scratch file, never the real per-map file. */
	UWorld* CreateNestWorld(const FString& StateFile)
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		if (UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>()) State->StorageFileOverride = StateFile;
		return World;
	}

	void DestroyNestWorld(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	int32 CountNestActors(UWorld* World, FName SiteTag)
	{
		int32 Count = 0;
		for (TActorIterator<AIslandNest> It(World); It; ++It)
			if (It->ActorHasTag(FName(*(TEXT("Nest_") + SiteTag.ToString())))) ++Count;
		return Count;
	}
}

bool FIslandNestTest::RunTest(const FString& Parameters)
{
	// No gateway, model requests, or autobiographical memory in this fixture.
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandNest") / TEXT("WorldState.json"));
	IFileManager::Get().Delete(*StateFile, false, true, true);
	const FName Site(TEXT("TestNestRoost"));

	UWorld* World = CreateNestWorld(StateFile);
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!TestNotNull(TEXT("Play worlds own a world-state subsystem"), State)) { DestroyNestWorld(World); return false; }
	TestEqual(TEXT("Fixture writes only to its scratch state file"), State->GetStorageFilePath(), StateFile);

	ACharacter* Raven = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	ATargetPoint* Perch = World->SpawnActor<ATargetPoint>(FVector(600, 0, 302), FRotator::ZeroRotator);
	Perch->Tags = {Site, TEXT("RavenPerch"), TEXT("RavenNestSite")};
	AActor* Support = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Support);
	Support->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(150, 150, 20));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	const float SupportTop = 302 - Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 2;
	Support->SetActorLocation(FVector(600, 0, SupportTop - 20));
	World->BeginPlay();
	Controller->Possess(Raven);
	TestEqual(TEXT("No lasting state exists before anything is built"), State->GetNests().Num(), 0);

	auto Decide = [Controller](const FString& Target)
	{
		FAgentDecision Decision;
		Decision.bValid = true;
		Decision.ActionType = EAgentActionType::Build;
		Decision.ActionTarget = Target;
		Controller->ActOnDecision(Decision);
	};

	// Gathering is embodied: only on the ground, one bundle at a time.
	TestTrue(TEXT("Grounded raven is offered twig gathering"), Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")));
	Decide(Site.ToString());
	TestTrue(TEXT("Weaving away from the perch is refused"), Controller->DescribeActionState().Contains(TEXT("Weaving is only possible while perched")));
	Decide(TEXT("GatherTwigs"));
	TestTrue(TEXT("Twigs gathered on the ground"), Controller->bCarryingTwigs);
	Decide(TEXT("GatherTwigs"));
	TestTrue(TEXT("A second bundle cannot be carried"), Controller->DescribeActionState().Contains(TEXT("already carrying")));

	TestTrue(TEXT("Raven can fly to the nest site"), Controller->RequestPerch(Site));
	for (int32 I = 0; I < 600; ++I) Controller->Tick(1.f / 60.f);
	TestTrue(TEXT("Raven perched at the nest site"), Controller->LocomotionState == ERavenLocomotionState::Perched);
	TestTrue(TEXT("Perched raven carrying twigs is offered this site as a build target"), Controller->DescribeBuildOptions().Contains(TEXT("build target: TestNestRoost")));
	Decide(TEXT("GatherTwigs"));
	TestTrue(TEXT("Gathering while perched is refused and keeps the bundle"), Controller->bCarryingTwigs && Controller->DescribeActionState().Contains(TEXT("only be gathered while standing")));

	Decide(Site.ToString());
	const FIslandNestRecord* Nest = State->FindNest(Site);
	TestTrue(TEXT("First weave starts a one-layer nest"), Nest && Nest->Layers == 1);
	TestFalse(TEXT("Weaving uses up the carried twigs"), Controller->bCarryingTwigs);
	TestTrue(TEXT("Weave result says the change is lasting"), Controller->DescribeActionState().Contains(TEXT("stays in the world after this session")));
	TestTrue(TEXT("The change was saved"), FPaths::FileExists(StateFile));
	TestEqual(TEXT("One visible nest actor"), CountNestActors(World, Site), 1);
	for (TActorIterator<AIslandNest> It(World); It; ++It)
	{
		TestEqual(TEXT("One layer of twigs is visible"), It->GetVisibleTwigCount(), AIslandNest::TwigsPerLayer);
		TestTrue(TEXT("Nest rests on the support surface, not the marker in the air"), FMath::IsNearlyEqual(It->GetActorLocation().Z, SupportTop, 4.f));
		TestTrue(TEXT("Nest is visual only and cannot alter perch support"), It->GetActorEnableCollision() == false || It->GetRootComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	TestTrue(TEXT("Raven still counts as perched on real support after weaving"), Controller->LocomotionState == ERavenLocomotionState::Perched);

	Decide(Site.ToString());
	TestTrue(TEXT("Weaving without twigs changes nothing"), Controller->DescribeActionState().Contains(TEXT("no twigs")) && State->FindNest(Site)->Layers == 1);
	Controller->bCarryingTwigs = true;
	Decide(Site.ToString());
	TestTrue(TEXT("A fresh layer must settle before the next"), Controller->DescribeActionState().Contains(TEXT("still settling")) && State->FindNest(Site)->Layers == 1);
	Controller->WovenUntil.Reset();
	Decide(Site.ToString());
	TestEqual(TEXT("Second weave adds a layer"), State->FindNest(Site)->Layers, 2);

	// Bystanders perceive the nest but not who made it.
	ACharacter* Bystander = World->SpawnActor<ACharacter>(FVector(0.f, 400.f, 302.f), FRotator::ZeroRotator);
	UAgentBrainComponent* BystanderBrain = NewObject<UAgentBrainComponent>(Bystander);
	Bystander->AddInstanceComponent(BystanderBrain);
	BystanderBrain->RegisterComponent();
	const FString BystanderView = BystanderBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A nearby resident can come across the nest"), BystanderView.Contains(TEXT("A small nest of woven twigs")) && BystanderView.Contains(TEXT("2 of 5 layers")));
	TestTrue(TEXT("The maker is not revealed to someone who did not see it"), BystanderView.Contains(TEXT("did not see who made it")) && !BystanderView.Contains(Raven->GetName()));
	TestFalse(TEXT("Build targets are not offered to a body that cannot use them"), BystanderView.Contains(TEXT("build target")));
	Controller->UnPossess();
	DestroyNestWorld(World);

	// A new session restores the same nest from disk.
	World = CreateNestWorld(StateFile);
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	Nest = State->FindNest(Site);
	TestTrue(TEXT("Nest persists across sessions"), Nest && Nest->Layers == 2 && Nest->Builders.Num() == 1);
	TestEqual(TEXT("Persisted nest is visible again"), CountNestActors(World, Site), 1);
	TestEqual(TEXT("Third layer"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), 3);
	TestEqual(TEXT("Later contributors are recorded alongside the first"), State->FindNest(Site)->Builders.Num(), 2);
	TestTrue(TEXT("Original location is kept when others add to it"), !State->FindNest(Site)->Location.IsNearlyZero());
	State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other"));
	TestEqual(TEXT("Fifth layer completes the nest"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), UIslandWorldStateSubsystem::MaxNestLayers);
	TestEqual(TEXT("A complete nest accepts no more layers"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), 0);
	TestTrue(TEXT("Developer removal reverses the change"), State->RemoveNest(Site) && !State->FindNest(Site) && CountNestActors(World, Site) == 0);
	DestroyNestWorld(World);

	// An unreadable file is never overwritten by a later save.
	const FString Corrupt = TEXT("{ not json");
	FFileHelper::SaveStringToFile(Corrupt, *StateFile);
	AddExpectedError(TEXT("Could not read world state"), EAutomationExpectedErrorFlags::Contains, 1);
	World = CreateNestWorld(StateFile);
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	TestEqual(TEXT("Unreadable state refuses new lasting changes"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), 0);
	FString AfterRefusal;
	FFileHelper::LoadFileToString(AfterRefusal, *StateFile);
	TestEqual(TEXT("Unreadable state file is left untouched"), AfterRefusal, Corrupt);
	DestroyNestWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	return true;
}
