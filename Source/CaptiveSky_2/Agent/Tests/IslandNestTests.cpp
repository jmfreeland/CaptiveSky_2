#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandArrangement.h"
#include "IslandNest.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
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

	AIslandArrangement* FindArrangementActor(UWorld* World, FName SiteId)
	{
		for (TActorIterator<AIslandArrangement> It(World); It; ++It)
			if (It->GetSiteId() == SiteId) return *It;
		return nullptr;
	}
}

bool FIslandNestTest::RunTest(const FString& Parameters)
{
	// No gateway or model requests. The test memory component only receives sleep data if the test explicitly completes consolidation.
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandNest") / TEXT("WorldState.json"));
	IFileManager::Get().Delete(*StateFile, false, true, true);
	const FName Site(TEXT("TestNestRoost"));

	UWorld* World = CreateNestWorld(StateFile);
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!TestNotNull(TEXT("Play worlds own a world-state subsystem"), State)) { DestroyNestWorld(World); return false; }
	TestEqual(TEXT("Fixture writes only to its scratch state file"), State->GetStorageFilePath(), StateFile);

	ACharacter* Raven = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	UAgentConsolidationComponent* RavenRest = NewObject<UAgentConsolidationComponent>(Raven);
	Raven->AddInstanceComponent(RavenRest);
	RavenRest->RegisterComponent();
	RavenRest->DefaultRestDurationSeconds = 120.f;
	UAgentMemoryComponent* RavenMemory = NewObject<UAgentMemoryComponent>(Raven);
	RavenMemory->AgentId = TEXT("Codex_IslandNestSleepTest");
	Raven->AddInstanceComponent(RavenMemory);
	RavenMemory->RegisterComponent();
	AActor* Ground = World->SpawnActor<AActor>();
	UBoxComponent* GroundBox = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(GroundBox);
	GroundBox->SetBoxExtent(FVector(5000.f, 5000.f, 20.f));
	GroundBox->SetCollisionProfileName(TEXT("BlockAll"));
	GroundBox->RegisterComponent();
	Ground->SetActorLocation(FVector(0.f, 0.f, -20.f));
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
	FIslandArrangementSite StartForageSite;
	StartForageSite.Id = TEXT("ArrangingGround_TestStart");
	StartForageSite.Location = FVector::ZeroVector;
	State->ArrangementSites.Add(StartForageSite);
	AIslandArrangement* StartForagePatch = World->SpawnActor<AIslandArrangement>(StartForageSite.Location, FRotator::ZeroRotator);
	if (StartForagePatch)
	{
		State->ArrangementActors.Add(StartForageSite.Id, StartForagePatch);
		StartForagePatch->ShowSite(StartForageSite, 1);
	}
	FIslandArrangementSite OccludedForageSite;
	OccludedForageSite.Id = TEXT("ArrangingGround_TestOccluded");
	OccludedForageSite.Location = FVector(200.f, 0.f, 0.f);
	State->ArrangementSites.Add(OccludedForageSite);
	AIslandArrangement* OccludedForagePatch = World->SpawnActor<AIslandArrangement>(OccludedForageSite.Location, FRotator::ZeroRotator);
	if (OccludedForagePatch)
	{
		State->ArrangementActors.Add(OccludedForageSite.Id, OccludedForagePatch);
		OccludedForagePatch->ShowSite(OccludedForageSite, 1);
	}
	AActor* ForageOccluder = World->SpawnActor<AActor>();
	UBoxComponent* ForageOccluderBox = NewObject<UBoxComponent>(ForageOccluder);
	ForageOccluder->SetRootComponent(ForageOccluderBox);
	ForageOccluderBox->SetBoxExtent(FVector(20.f, 40.f, 50.f));
	ForageOccluderBox->SetCollisionProfileName(TEXT("BlockAll"));
	ForageOccluderBox->RegisterComponent();
	ForageOccluder->SetActorLocation(FVector(100.f, 0.f, 55.f));
	Controller->Possess(Raven);
	TestEqual(TEXT("No lasting state exists before anything is built"), State->GetNests().Num(), 0);

	auto Decide = [&Controller](const FString& Target)
	{
		FAgentDecision Decision;
		Decision.bValid = true;
		Decision.ActionType = EAgentActionType::Build;
		Decision.ActionTarget = Target;
		Controller->ActOnDecision(Decision);
	};

	// Gathering is embodied: only on the ground and beside a visible, consumable resource.
	TestTrue(TEXT("An empty arranging ground visibly holds one small twig bundle"), StartForagePatch &&
		StartForagePatch->HasForageableTwigs() && StartForagePatch->GetVisibleForageTwigCount() == 7);
	TestTrue(TEXT("Grounded raven is offered gathering only beside the visible twig pile"), Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")));
	Decide(Site.ToString());
	TestTrue(TEXT("Weaving away from the perch is refused"), Controller->DescribeActionState().Contains(TEXT("Weaving is only possible while perched")));
	Decide(TEXT("GatherTwigs"));
	TestTrue(TEXT("Twigs gathered on the ground"), Controller->bCarryingTwigs && StartForagePatch && !StartForagePatch->HasForageableTwigs() &&
		StartForagePatch->GetVisibleForageTwigCount() == 0);
	Controller->bCarryingTwigs = false;
	TestFalse(TEXT("Gathering again cannot create a bundle after the visible pile is depleted"), Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")));
	Decide(TEXT("GatherTwigs"));
	TestFalse(TEXT("A depleted patch refuses another bundle"), Controller->bCarryingTwigs);
	TestTrue(TEXT("The refusal says no visible twigs remain nearby"), Controller->DescribeActionState().Contains(TEXT("no available fallen twigs")));
	TestTrue(TEXT("An occluded nearby pile is not offered or consumed"), OccludedForagePatch && OccludedForagePatch->HasForageableTwigs() &&
		!Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")));
	ForageOccluderBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestTrue(TEXT("A clear view reveals the nearby pile without changing it"), OccludedForagePatch &&
		Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")) && OccludedForagePatch->HasForageableTwigs());
	Decide(TEXT("GatherTwigs"));
	TestTrue(TEXT("The newly visible second pile can be gathered once"), Controller->bCarryingTwigs && !OccludedForagePatch->HasForageableTwigs());
	Controller->bCarryingTwigs = false;
	TestFalse(TEXT("All nearby depleted piles remove the gathering option"), Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")));
	Controller->bCarryingTwigs = true;
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
	FAgentDecision SleepAtNest;
	SleepAtNest.bValid = true;
	SleepAtNest.ActionType = EAgentActionType::Sleep;
	SleepAtNest.ActionTarget = Site.ToString();
	Controller->ActOnDecision(SleepAtNest);
	TestFalse(TEXT("A raven can choose to sleep at its persistent woven nest"), RavenRest->IsAwake());
	TestTrue(TEXT("A completed sleep queues only the grounded nest and layer facts for later memory"),
		RavenRest->PendingSleepExperience.Contains(TEXT("my woven nest at TestNestRoost")) &&
		RavenRest->PendingSleepExperience.Contains(TEXT("1 of 5 layers")) &&
		RavenRest->PendingSleepExperienceTags.Contains(TEXT("nest")) &&
		RavenRest->PendingSleepExperienceTags.Contains(TEXT("roost")) &&
		!RavenRest->PendingSleepExperienceTags.Contains(TEXT("inn")));
	RavenRest->WakeUp();
	TestTrue(TEXT("Waking before the sleep interval finishes cancels the nest memory"),
		RavenRest->IsAwake() && RavenRest->PendingSleepExperience.IsEmpty() && RavenRest->PendingSleepExperienceTags.IsEmpty());

	Decide(Site.ToString());
	TestTrue(TEXT("Weaving without twigs changes nothing"), Controller->DescribeActionState().Contains(TEXT("no twigs")) && State->FindNest(Site)->Layers == 1);
	Controller->bCarryingTwigs = true;
	Decide(Site.ToString());
	TestTrue(TEXT("A fresh layer must settle before the next"), Controller->DescribeActionState().Contains(TEXT("still settling")) && State->FindNest(Site)->Layers == 1);
	Controller->WovenUntil.Reset();
	Decide(Site.ToString());
	TestEqual(TEXT("Second weave adds a layer"), State->FindNest(Site)->Layers, 2);
	for (TActorIterator<AIslandNest> It(World); It; ++It)
	{
		UInstancedStaticMeshComponent* TwigInstances = Cast<UInstancedStaticMeshComponent>(It->GetRootComponent());
		TestNotNull(TEXT("The visible nest exposes its instance weave for inspection"), TwigInstances);
		if (!TwigInstances || TwigInstances->GetInstanceCount() != 2 * AIslandNest::TwigsPerLayer) continue;
		for (int32 Layer = 0; Layer < 2; ++Layer)
		{
			for (int32 Twig = 0; Twig < AIslandNest::TwigsPerLayer; ++Twig)
			{
				FTransform TwigTransform;
				const int32 InstanceIndex = Layer * AIslandNest::TwigsPerLayer + Twig;
				if (!TwigInstances->GetInstanceTransform(InstanceIndex, TwigTransform, false))
				{
					AddError(FString::Printf(TEXT("Could not inspect nest twig %d"), InstanceIndex));
					continue;
				}
				const FVector Radial = TwigTransform.GetLocation().GetSafeNormal2D();
				const FVector TwigAxis = TwigTransform.GetRotation().RotateVector(FVector::UpVector).GetSafeNormal2D();
				const float RadialAlignment = FMath::Abs(FVector::DotProduct(Radial, TwigAxis));
				const float ExpectedAlignment = Layer % 2 == 0 ? 0.f : 1.f;
				TestTrue(FString::Printf(TEXT("Nest layer %d twig %d alternates between tangent and radial weave courses"), Layer, Twig),
					FMath::Abs(RadialAlignment - ExpectedAlignment) < 0.2f);
			}
		}
	}

	// Bystanders perceive the nest but not who made it.
	ACharacter* Bystander = World->SpawnActor<ACharacter>(FVector(0.f, 400.f, 302.f), FRotator::ZeroRotator);
	UAgentBrainComponent* BystanderBrain = NewObject<UAgentBrainComponent>(Bystander);
	Bystander->AddInstanceComponent(BystanderBrain);
	BystanderBrain->RegisterComponent();
	const FString BystanderView = BystanderBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A nearby resident can come across the nest"), BystanderView.Contains(TEXT("A small nest of woven twigs")) && BystanderView.Contains(TEXT("2 of 5 layers")));
	TestTrue(TEXT("The maker is not revealed to someone who did not see it"), BystanderView.Contains(TEXT("did not see who made it")) && !BystanderView.Contains(Raven->GetName()));
	TestFalse(TEXT("A non-raven is not offered the Raven-only forage or nest targets"),
		BystanderView.Contains(TEXT("build target: GatherTwigs")) || BystanderView.Contains(TEXT("build target: TestNestRoost")));
	Controller->UnPossess();
	DestroyNestWorld(World);

	// A new session restores the same nest from disk.
	World = CreateNestWorld(StateFile);
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	Raven = World->SpawnActor<ACharacter>(FVector(600.f, 0.f, 302.f), FRotator::ZeroRotator);
	Controller = World->SpawnActor<ARavenAgentAIController>();
	AActor* ForageGround = World->SpawnActor<AActor>();
	UBoxComponent* ForageGroundBox = NewObject<UBoxComponent>(ForageGround);
	ForageGround->SetRootComponent(ForageGroundBox);
	ForageGroundBox->SetBoxExtent(FVector(5000.f, 5000.f, 20.f));
	ForageGroundBox->SetCollisionProfileName(TEXT("BlockAll"));
	ForageGroundBox->RegisterComponent();
	ForageGround->SetActorLocation(FVector(0.f, 0.f, -20.f));
	Controller->Possess(Raven);
	Controller->LocomotionState = ERavenLocomotionState::Perched;
	Raven->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	UAgentBrainComponent* RavenBrain = NewObject<UAgentBrainComponent>(Raven);
	Raven->AddInstanceComponent(RavenBrain);
	RavenBrain->RegisterComponent();
	Nest = State->FindNest(Site);
	TestTrue(TEXT("Nest persists across sessions"), Nest && Nest->Layers == 2 && Nest->Builders.Num() == 1);
	TestEqual(TEXT("Persisted nest is visible again"), CountNestActors(World, Site), 1);
	const AIslandArrangement* RestoredStartForage = FindArrangementActor(World, StartForageSite.Id);
	const AIslandArrangement* RestoredOccludedForage = FindArrangementActor(World, OccludedForageSite.Id);
	TestTrue(TEXT("Gathered bundles remain depleted after reopening the same Island day"),
		RestoredStartForage && !RestoredStartForage->HasForageableTwigs() &&
		RestoredOccludedForage && !RestoredOccludedForage->HasForageableTwigs());
	for (const FName TestSiteId : { StartForageSite.Id, OccludedForageSite.Id })
	{
		if (TWeakObjectPtr<AIslandArrangement>* Actor = State->ArrangementActors.Find(TestSiteId))
			if (Actor->IsValid()) Actor->Get()->Destroy();
		State->ArrangementActors.Remove(TestSiteId);
	}
	TestEqual(TEXT("Temporary forage fixtures are removed before the nest continuation"),
		State->ArrangementSites.RemoveAll([&StartForageSite, &OccludedForageSite](const FIslandArrangementSite& Entry)
			{ return Entry.Id == StartForageSite.Id || Entry.Id == OccludedForageSite.Id; }), 2);
	TestTrue(TEXT("Fixture cleanup preserves the remaining nest state"), State->Save());
	TestEqual(TEXT("Third layer"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), 3);
	TestEqual(TEXT("Later contributors are recorded alongside the first"), State->FindNest(Site)->Builders.Num(), 2);
	TestTrue(TEXT("Original location is kept when others add to it"), !State->FindNest(Site)->Location.IsNearlyZero());
	State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other"));
	TestEqual(TEXT("Fifth layer completes the nest"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), UIslandWorldStateSubsystem::MaxNestLayers);
	TestEqual(TEXT("A complete nest accepts no more layers"), State->AddNestLayer(Site, FVector::ZeroVector, TEXT("Other")), 0);
	TestTrue(TEXT("Developer removal reverses the change"), State->RemoveNest(Site) && !State->FindNest(Site) && CountNestActors(World, Site) == 0);

	FIslandArrangementSite ForageSite;
	ForageSite.Id = TEXT("ArrangingGround_TestForage");
	ForageSite.Location = FVector(1000.f, 0.f, 0.f);
	State->ArrangementSites.Add(ForageSite);
	FIslandArrangementSite HiddenSite;
	HiddenSite.Id = TEXT("ArrangingGround_Hidden");
	HiddenSite.Location = FVector(600.f, 1000.f, 0.f);
	State->ArrangementSites.Add(HiddenSite);
	FIslandArrangementSite FarForageSite;
	FarForageSite.Id = TEXT("ArrangingGround_FarForage");
	FarForageSite.Location = FVector(2600.f, 0.f, 0.f);
	State->ArrangementSites.Add(FarForageSite);
	AIslandArrangement* FarForagePatch = World->SpawnActor<AIslandArrangement>(FarForageSite.Location, FRotator::ZeroRotator);
	if (FarForagePatch)
	{
		State->ArrangementActors.Add(FarForageSite.Id, FarForagePatch);
		FarForagePatch->ShowSite(FarForageSite, 1);
	}
	AActor* Occluder = World->SpawnActor<AActor>();
	UBoxComponent* OccluderBox = NewObject<UBoxComponent>(Occluder);
	Occluder->SetRootComponent(OccluderBox);
	OccluderBox->SetBoxExtent(FVector(100.f, 100.f, 300.f));
	OccluderBox->SetCollisionProfileName(TEXT("BlockAll"));
	OccluderBox->RegisterComponent();
	Occluder->SetActorLocation(FVector(600.f, 500.f, 300.f));
	const FString ForageSummary = RavenBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(*FString::Printf(TEXT("Open-ground forage site is offered (summary: %s)"), *ForageSummary),
		ForageSummary.Contains(TEXT("move_to/land target: ArrangingGround_TestForage")));
	TestTrue(*FString::Printf(TEXT("A clearly visible forage site 20 metres away is also offered as an exact landing target (summary: %s)"), *ForageSummary),
		ForageSummary.Contains(TEXT("move_to/land target: ArrangingGround_FarForage")));
	FAgentDecision HiddenLanding;
	HiddenLanding.bValid = true;
	HiddenLanding.ActionType = EAgentActionType::Land;
	HiddenLanding.ActionTarget = HiddenSite.Id.ToString();
	Controller->ActOnDecision(HiddenLanding);
	TestFalse(TEXT("An in-range but occluded landing site is refused"), Controller->bHasMovementTarget);
	TestTrue(TEXT("Visibility refusal keeps the raven perched"), Controller->LocomotionState == ERavenLocomotionState::Perched);
	TestTrue(*FString::Printf(TEXT("Visibility refusal explains why the target was unavailable (%s)"), *Controller->DescribeActionState()),
		Controller->DescribeActionState().Contains(TEXT("not currently visible")));
	FAgentDecision InvalidLanding;
	InvalidLanding.bValid = true;
	InvalidLanding.ActionType = EAgentActionType::Land;
	InvalidLanding.ActionTarget = Site.ToString();
	Controller->ActOnDecision(InvalidLanding);
	TestFalse(TEXT("A nest perch cannot be used as an open-ground landing target"), Controller->bHasMovementTarget);
	TestTrue(*FString::Printf(TEXT("Invalid landing target is explained without movement (%s)"), *Controller->DescribeActionState()),
		Controller->DescribeActionState().Contains(TEXT("exact listed ArrangingGround")));
	FAgentDecision Land;
	Land.bValid = true;
	Land.ActionType = EAgentActionType::Land;
	Land.ActionTarget = FarForageSite.Id.ToString();
	Controller->ActOnDecision(Land);
	for (int32 Frame = 0; Frame < 1800 && Controller->bHasMovementTarget; ++Frame) Controller->Tick(1.f / 60.f);
	TestTrue(TEXT("Raven can fly from its perch and descend onto a verified open-ground site"), Controller->LocomotionState == ERavenLocomotionState::Grounded && !Controller->bHasMovementTarget);
	TestTrue(TEXT("Landing reports the visible grounded forage affordance"), Controller->DescribeActionState().Contains(TEXT("visible bundle of fallen twigs is within reach")));
	TestEqual(TEXT("Landing alone creates no lasting nest"), State->GetNests().Num(), 0);
	TestTrue(TEXT("GatherTwigs is offered after ground arrival"), Controller->DescribeBuildOptions().Contains(TEXT("build target: GatherTwigs")));
	Decide(TEXT("GatherTwigs"));
	TestTrue(*FString::Printf(TEXT("The raven gathers the material after choosing to land (%s)"), *Controller->DescribeActionState()),
		Controller->bCarryingTwigs && FarForagePatch && !FarForagePatch->HasForageableTwigs() && FarForagePatch->GetVisibleForageTwigCount() == 0);
	const FIslandArrangementSite* GatheredFarSite = State->FindArrangementSite(FarForageSite.Id);
	TestNotNull(TEXT("The gathered source site remains in canonical world state"), GatheredFarSite);
	if (GatheredFarSite) TestEqual(TEXT("Gathering records the Island day at the source site"), GatheredFarSite->ForageGatheredDay, 1);
	TestFalse(TEXT("The same site cannot be gathered twice on the same Island day"), State->GatherArrangementTwigs(FarForageSite.Id, TEXT("Raven"), 1));
	TestTrue(TEXT("The next Island day renews forage at empty sites"), State->SaveClock(12.f, 2) && FarForagePatch->HasForageableTwigs());
	TestTrue(TEXT("A renewed bundle can be gathered on the new Island day"), State->GatherArrangementTwigs(FarForageSite.Id, TEXT("Raven"), 2));
	TestFalse(TEXT("The renewed site still yields only one bundle that day"), State->GatherArrangementTwigs(FarForageSite.Id, TEXT("Raven"), 2));
	DestroyNestWorld(World);

	// Forage depletion survives a restart, then renews when the saved Island day advances.
	World = CreateNestWorld(StateFile);
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	AIslandArrangement* ReloadedForage = FindArrangementActor(World, FarForageSite.Id);
	TestNotNull(TEXT("The saved forage site is visible after restart"), ReloadedForage);
	if (ReloadedForage)
	{
		TestFalse(TEXT("The day-two bundle remains depleted after restart"), ReloadedForage->HasForageableTwigs());
		TestTrue(TEXT("A further Island day restores the fallen-twig bundle"), State->SaveClock(12.f, 3) && ReloadedForage->HasForageableTwigs());
	}
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
