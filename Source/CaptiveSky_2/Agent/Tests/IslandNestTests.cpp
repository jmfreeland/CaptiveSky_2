#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandArrangement.h"
#include "IslandNest.h"
#include "IslandRatSubsystem.h"
#include "IslandWildlifeHome.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
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
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandNestTest, "CaptiveSky2.Agent.IslandNest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWildlifeHomeRecordTest, "CaptiveSky2.Agent.WildlifeHomeRecord",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandRatHomeSelectionTest, "CaptiveSky2.Agent.RatHomeSelection",
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

	int32 CountWildlifeHomeActors(UWorld* World, FName HomeId)
	{
		int32 Count = 0;
		for (TActorIterator<AIslandWildlifeHome> It(World); It; ++It)
			if (It->GetHomeId() == HomeId) ++Count;
		return Count;
	}

	AIslandArrangement* FindNestForageArrangementActor(UWorld* World, FName SiteId)
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
	TestTrue(TEXT("The refusal explains that no fresh driftwood remains within reach"),
		Controller->DescribeActionState().Contains(TEXT("No fresh driftwood lies within reach")) &&
		Controller->DescribeActionState().Contains(TEXT("no twigs were gathered")));
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
		TestEqual(TEXT("A shallow woven base sits beneath the first twig layer"), It->GetVisibleTwigCount(),
			AIslandNest::FoundationTwigCount + AIslandNest::TwigsPerLayer);
		TestEqual(TEXT("Every woven twig has two visual-only tapered ends"), It->GetVisibleTwigTipCount(),
			2 * It->GetVisibleTwigCount());
		TestTrue(TEXT("Nest rests on the support surface, not the marker in the air"), FMath::IsNearlyEqual(It->GetActorLocation().Z, SupportTop, 4.f));
		TestTrue(TEXT("Nest is visual only and cannot alter perch support"), It->GetActorEnableCollision() == false || It->GetRootComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

		UInstancedStaticMeshComponent* TwigInstances = Cast<UInstancedStaticMeshComponent>(It->GetRootComponent());
		TestNotNull(TEXT("The visible nest exposes its foundation instances for inspection"), TwigInstances);
		if (!TwigInstances) continue;
		UInstancedStaticMeshComponent* TipInstances = It->TwigTips;
		TestNotNull(TEXT("Tapered ends use a separate batched visual component"), TipInstances);
		if (TipInstances)
		{
			TestTrue(TEXT("Twig tips use Unreal's built-in cone primitive"), TipInstances->GetStaticMesh() &&
				TipInstances->GetStaticMesh()->GetName().Contains(TEXT("Cone")));
			TestTrue(TEXT("Tapered ends stay collisionless, shadowless, and off navigation"),
				TipInstances->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
				!TipInstances->CastShadow && !TipInstances->CanEverAffectNavigation());
			TestTrue(TEXT("Woven bodies and storm debris do not cast hard shadows onto the tiny roost"),
				!TwigInstances->CastShadow && It->FallenTwigs && !It->FallenTwigs->CastShadow);
		}
		TArray<FTransform> SeededTipTransforms;
		if (TipInstances)
		{
			for (int32 Index = 0; Index < 2; ++Index)
			{
				FTransform TipTransform;
				TestTrue(FString::Printf(TEXT("Tapered end %d has a visible instance transform"), Index),
					TipInstances->GetInstanceTransform(Index, TipTransform, false));
				SeededTipTransforms.Add(TipTransform);
			}
			FTransform BodyTransform, PositiveTip, NegativeTip;
			if (TwigInstances->GetInstanceTransform(0, BodyTransform, false) &&
				TipInstances->GetInstanceTransform(0, PositiveTip, false) &&
				TipInstances->GetInstanceTransform(1, NegativeTip, false))
			{
				const FVector BodyAxis = BodyTransform.GetRotation().RotateVector(FVector::UpVector).GetSafeNormal();
				const FVector PositiveAxis = PositiveTip.GetRotation().RotateVector(FVector::UpVector).GetSafeNormal();
				const FVector NegativeAxis = NegativeTip.GetRotation().RotateVector(FVector::UpVector).GetSafeNormal();
				TestTrue(TEXT("The first cone points out along one end of the twig"), FVector::DotProduct(BodyAxis, PositiveAxis) > 0.98f);
				TestTrue(TEXT("The second cone points out along the opposite end of the twig"), FVector::DotProduct(BodyAxis, NegativeAxis) < -0.98f);
				const float HalfTwigLength = 50.f * BodyTransform.GetScale3D().Z;
				const float PositiveTipEnd = FVector::DotProduct(PositiveTip.GetLocation() - BodyTransform.GetLocation(), BodyAxis) +
					50.f * PositiveTip.GetScale3D().Z;
				const float NegativeTipEnd = -FVector::DotProduct(NegativeTip.GetLocation() - BodyTransform.GetLocation(), BodyAxis) +
					50.f * NegativeTip.GetScale3D().Z;
				TestTrue(TEXT("The positive tip exposes a short, visible taper beyond the twig body"), PositiveTipEnd > HalfTwigLength * 1.2f);
				TestTrue(TEXT("The negative tip exposes the same short taper beyond the twig body"), NegativeTipEnd > HalfTwigLength * 1.2f);
			}
		}
		TArray<FTransform> FoundationTransforms;
		for (int32 Index = 0; Index < AIslandNest::FoundationTwigCount; ++Index)
		{
			FTransform TwigTransform;
			TestTrue(FString::Printf(TEXT("Foundation twig %d has a visible instance transform"), Index),
				TwigInstances->GetInstanceTransform(Index, TwigTransform, false));
			if (TwigInstances->GetInstanceTransform(Index, TwigTransform, false))
			{
				const FVector TwigAxis = TwigTransform.GetRotation().RotateVector(FVector::UpVector).GetSafeNormal();
				TestTrue(FString::Printf(TEXT("Foundation twig %d lies nearly flat"), Index), FMath::Abs(TwigAxis.Z) < 0.1f);
				TestTrue(FString::Printf(TEXT("Foundation twig %d sits at the floor of the nest"), Index),
					FMath::Abs(TwigTransform.GetLocation().Z - 1.5f) < 0.3f);
				FoundationTransforms.Add(TwigTransform);
			}
		}
		It->SetWoven(Site, 1);
		TestEqual(TEXT("Rebuilding the same weave retains both tapered ends per twig"), It->GetVisibleTwigTipCount(),
			2 * It->GetVisibleTwigCount());
		if (TipInstances)
		{
			for (int32 Index = 0; Index < SeededTipTransforms.Num(); ++Index)
			{
				FTransform RebuiltTip;
				const bool bFoundTip = TipInstances->GetInstanceTransform(Index, RebuiltTip, false);
				TestTrue(FString::Printf(TEXT("Tapered end %d remains after repeating the same weave"), Index), bFoundTip);
				if (bFoundTip)
					TestTrue(FString::Printf(TEXT("Tapered end %d remains deterministic for its site"), Index),
						SeededTipTransforms[Index].Equals(RebuiltTip, 0.01f));
			}
		}
		for (int32 Index = 0; Index < FoundationTransforms.Num(); ++Index)
		{
			FTransform RebuiltTransform;
			const bool bFoundTransform = TwigInstances->GetInstanceTransform(Index, RebuiltTransform, false);
			TestTrue(FString::Printf(TEXT("Foundation twig %d remains after repeating the same weave"), Index), bFoundTransform);
			if (bFoundTransform)
			{
				TestTrue(FString::Printf(TEXT("Foundation twig %d is deterministic for its site"), Index),
					FoundationTransforms[Index].Equals(RebuiltTransform, 0.01f));
			}
		}
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
		if (!TwigInstances || TwigInstances->GetInstanceCount() != AIslandNest::FoundationTwigCount + 2 * AIslandNest::TwigsPerLayer) continue;
		TestEqual(TEXT("Both woven courses keep matching tapered ends"), It->GetVisibleTwigTipCount(), 2 * TwigInstances->GetInstanceCount());
		for (int32 Layer = 0; Layer < 2; ++Layer)
		{
			for (int32 Twig = 0; Twig < AIslandNest::TwigsPerLayer; ++Twig)
			{
				FTransform TwigTransform;
				const int32 InstanceIndex = AIslandNest::FoundationTwigCount + Layer * AIslandNest::TwigsPerLayer + Twig;
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
	AIslandNest* WovenNest = nullptr;
	for (TActorIterator<AIslandNest> It(World); It; ++It) { WovenNest = *It; break; }
	TArray<FString> StormMarks;
	TestTrue(TEXT("A storm can leave marks on lasting nest work"), State->ApplyStormMarks(3600.0, 1, StormMarks));
	Nest = State->FindNest(Site);
	TestTrue(TEXT("The storm removes one outer layer and records when it happened"),
		Nest && Nest->Layers == 1 && Nest->StormDamagedDay == 1 && StormMarks.ContainsByPredicate([](const FString& Mark)
			{ return Mark.Contains(TEXT("nest at TestNestRoost")); }));
	TestEqual(TEXT("A fresh storm tear renders fallen twigs below the nest"),
		WovenNest ? WovenNest->GetVisibleFallenTwigCount() : 0, AIslandNest::StormDebrisTwigCount);
	TestEqual(TEXT("Storm debris receives the same paired tapered ends"),
		WovenNest ? WovenNest->GetVisibleTwigTipCount() : 0,
		WovenNest ? 2 * (WovenNest->GetVisibleTwigCount() + WovenNest->GetVisibleFallenTwigCount()) : 0);

	// Bystanders perceive the nest but not who made it.
	ACharacter* Bystander = World->SpawnActor<ACharacter>(FVector(0.f, 400.f, 302.f), FRotator::ZeroRotator);
	UAgentBrainComponent* BystanderBrain = NewObject<UAgentBrainComponent>(Bystander);
	Bystander->AddInstanceComponent(BystanderBrain);
	BystanderBrain->RegisterComponent();
	const FString DamagedNestView = BystanderBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Nearby residents are told about recent storm damage and loose twigs"),
		DamagedNestView.Contains(TEXT("outer layer was torn loose in a recent storm")) && DamagedNestView.Contains(TEXT("twigs lie scattered beneath it")));
	Controller->WovenUntil.Reset();
	Controller->bCarryingTwigs = true;
	Decide(Site.ToString());
	Nest = State->FindNest(Site);
	TestTrue(TEXT("Re-weaving repairs the nest and clears its storm-damage mark"), Nest && Nest->Layers == 2 && Nest->StormDamagedDay == -1);
	TestEqual(TEXT("Repair removes the visible fallen-twig debris"),
		WovenNest ? WovenNest->GetVisibleFallenTwigCount() : -1, 0);
	TestEqual(TEXT("Repair also removes every loose debris tip"),
		WovenNest ? WovenNest->GetVisibleTwigTipCount() : -1, WovenNest ? 2 * WovenNest->GetVisibleTwigCount() : -1);
	const FString BystanderView = BystanderBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A nearby resident can come across the repaired nest"), BystanderView.Contains(TEXT("A small nest of woven twigs")) && BystanderView.Contains(TEXT("2 of 5 layers")));
	TestTrue(TEXT("The maker is not revealed to someone who did not see it"), BystanderView.Contains(TEXT("did not see who made it")) && !BystanderView.Contains(Raven->GetName()));
	TestFalse(TEXT("The repaired nest no longer reads as storm-torn"), BystanderView.Contains(TEXT("outer layer was torn loose in a recent storm")));
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
	const AIslandArrangement* RestoredStartForage = FindNestForageArrangementActor(World, StartForageSite.Id);
	const AIslandArrangement* RestoredOccludedForage = FindNestForageArrangementActor(World, OccludedForageSite.Id);
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
	AIslandArrangement* ReloadedForage = FindNestForageArrangementActor(World, FarForageSite.Id);
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

bool FIslandRatHomeSelectionTest::RunTest(const FString& Parameters)
{
	TArray<FIslandWildlifeHomeRecord> Homes;
	FVector OutLocation(1.f, 2.f, 3.f);
	TestFalse(TEXT("An empty record set does not invent a Rat home"), UIslandRatSubsystem::FindRatHomeLocation(Homes, OutLocation));
	TestTrue(TEXT("A missing home leaves the caller's fallback location untouched"), OutLocation.Equals(FVector(1.f, 2.f, 3.f)));

	FIslandWildlifeHomeRecord WrongSpecies;
	WrongSpecies.Id = TEXT("FoxHome");
	WrongSpecies.Species = TEXT("Fox");
	WrongSpecies.OccupantAgentId = UIslandRatSubsystem::GetAgentId();
	WrongSpecies.EntranceLocation = FVector(100.f, 0.f, 0.f);
	WrongSpecies.ShelterState = EIslandWildlifeShelterState::Settled;
	WrongSpecies.ShelterQuality = 5;
	Homes.Add(WrongSpecies);

	FIslandWildlifeHomeRecord WrongOccupant = WrongSpecies;
	WrongOccupant.Id = TEXT("OtherRatHome");
	WrongOccupant.Species = TEXT("Rat");
	WrongOccupant.OccupantAgentId = TEXT("Agent_Other");
	WrongOccupant.EntranceLocation = FVector(200.f, 0.f, 0.f);
	Homes.Add(WrongOccupant);

	FIslandWildlifeHomeRecord Disturbed = WrongOccupant;
	Disturbed.Id = TEXT("DisturbedRatHome");
	Disturbed.OccupantAgentId = UIslandRatSubsystem::GetAgentId();
	Disturbed.EntranceLocation = FVector(300.f, 0.f, 0.f);
	Disturbed.ShelterState = EIslandWildlifeShelterState::Disturbed;
	Homes.Add(Disturbed);

	FIslandWildlifeHomeRecord Temporary = Disturbed;
	Temporary.Id = TEXT("TemporaryRatHome");
	Temporary.ShelterState = EIslandWildlifeShelterState::Temporary;
	Temporary.ShelterQuality = 5;
	Temporary.EntranceLocation = FVector(400.f, 0.f, 0.f);
	Homes.Add(Temporary);

	FIslandWildlifeHomeRecord Settled = Temporary;
	Settled.Id = TEXT("SettledRatHome");
	Settled.ShelterState = EIslandWildlifeShelterState::Settled;
	Settled.ShelterQuality = 2;
	Settled.EntranceLocation = FVector(500.f, 0.f, 0.f);
	Homes.Add(Settled);

	FIslandWildlifeHomeRecord BestSettled = Settled;
	BestSettled.Id = TEXT("BestSettledRatHome");
	BestSettled.ShelterQuality = 4;
	BestSettled.EntranceLocation = FVector(600.f, 0.f, 0.f);
	Homes.Add(BestSettled);

	TestTrue(TEXT("A supported canonical Rat home is found"), UIslandRatSubsystem::FindRatHomeLocation(Homes, OutLocation));
	TestTrue(TEXT("A settled home outranks a higher-quality temporary shelter"), OutLocation.Equals(BestSettled.EntranceLocation));
	return true;
}

bool FIslandWildlifeHomeRecordTest::RunTest(const FString& Parameters)
{
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("WildlifeHomeRecord") / TEXT("WorldState.json"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(StateFile), true);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	const FString LegacyState = TEXT("{\"version\":1,\"nests\":[{\"site\":\"Roost_Legacy\",\"layers\":2,\"location\":[10,20,30],\"builders\":[]}],\"curios\":[]}");
	TestTrue(TEXT("A pre-wildlife-home world-state fixture is written to the scratch file"),
		FFileHelper::SaveStringToFile(LegacyState, *StateFile));

	UWorld* World = CreateNestWorld(StateFile);
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!TestNotNull(TEXT("Fixture world has a world-state subsystem"), State))
	{
		DestroyNestWorld(World);
		return false;
	}
	TestTrue(TEXT("Legacy JSON without wildlife_homes remains readable"), State->ReadStateFile(StateFile));
	TestEqual(TEXT("Legacy nest records remain available"), State->GetNests().Num(), 1);
	TestEqual(TEXT("Legacy state starts with no wildlife homes"), State->GetWildlifeHomes().Num(), 0);
	TestEqual(TEXT("Legacy state does not invent or materialize a wildlife home"), CountWildlifeHomeActors(World, TEXT("Fenrus_FallenWood")), 0);

	FIslandWildlifeHomeRecord Home;
	Home.Id = TEXT("Fenrus_FallenWood");
	Home.Species = TEXT("Rat");
	Home.OccupantAgentId = TEXT("Agent_Rat_01");
	Home.EntranceLocation = FVector(-104500.f, 105000.f, 2337.5f);
	Home.EntranceRotation = FRotator(0.f, 180.f, 0.f);
	Home.ShelterQuality = 4;
	Home.ShelterState = EIslandWildlifeShelterState::Temporary;
	Home.LastUsedUtc = FDateTime::UtcNow();
	FIslandWildlifeHomeCache Cache;
	Cache.ItemId = TEXT("Hazelnut");
	Cache.Quantity = 3;
	Cache.LastChangedDay = 2;
	Home.CachedSupplies.Add(Cache);

	FIslandWildlifeHomeRecord Invalid = Home;
	Invalid.EntranceLocation.X = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Non-finite entrance transforms are rejected"), State->StoreWildlifeHome(Invalid));
	TestEqual(TEXT("Rejected home records do not mutate the state"), State->GetWildlifeHomes().Num(), 0);
	TestTrue(TEXT("A valid canonical wildlife home is saved"), State->StoreWildlifeHome(Home));
	const FIslandWildlifeHomeRecord* Stored = State->FindWildlifeHome(Home.Id);
	TestTrue(TEXT("The home can be found by stable place ID"), Stored && Stored->OccupantAgentId == TEXT("Agent_Rat_01"));
	AIslandWildlifeHome* Materialized = State->FindWildlifeHomeActor(Home.Id);
	TestNotNull(TEXT("A recorded supported Rat home gets a transient visual actor"), Materialized);
	if (Materialized)
	{
		TestTrue(TEXT("Materialization preserves the entrance anchor"), Materialized->GetActorLocation().Equals(Home.EntranceLocation, 0.01f));
		TestTrue(TEXT("The imported RatHollow mesh is assigned"), Materialized->GetShelterMesh() &&
			Materialized->GetShelterMesh()->GetStaticMesh() &&
			Materialized->GetShelterMesh()->GetStaticMesh()->GetPathName() == TEXT("/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01.SM_RatHollow_01"));
		TestEqual(TEXT("A wildlife shelter cannot block player or resident movement yet"),
			Materialized->GetShelterMesh()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(TEXT("The visual shelter cannot alter navigation"), Materialized->GetShelterMesh()->CanEverAffectNavigation());
		TestTrue(TEXT("The imported entrance is aligned to its saved anchor"),
			Materialized->GetShelterMesh()->GetRelativeLocation().Equals(FVector(AIslandWildlifeHome::RatHollowEntranceOffsetCm, 0.f, 0.f), 0.01f));
		UInstancedStaticMeshComponent* CacheDisplay = Materialized->GetCacheDisplayMesh();
		TestNotNull(TEXT("A transient cache display component is created"), CacheDisplay);
		if (CacheDisplay)
		{
			TestTrue(TEXT("The bounded hazelnut cache uses the imported Common Hazel nut branch"), CacheDisplay->GetStaticMesh() &&
				CacheDisplay->GetStaticMesh()->GetPathName() == TEXT("/Game/Megaplant_Library/Tree_Common_Hazel/Instances/Branch_Common_Hazel_Nut_01.Branch_Common_Hazel_Nut_01"));
			TestEqual(TEXT("Three cached hazelnuts appear as three visual items"), CacheDisplay->GetInstanceCount(), 3);
			TestEqual(TEXT("The cache remains non-colliding"), CacheDisplay->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestFalse(TEXT("The cache cannot alter navigation"), CacheDisplay->CanEverAffectNavigation());

			FIslandWildlifeHomeRecord DenseCache = Home;
			DenseCache.CachedSupplies[0].Quantity = UIslandWorldStateSubsystem::MaxWildlifeHomeCacheQuantity;
			TestTrue(TEXT("A full cache record can be reapplied"), Materialized->ApplyHomeRecord(DenseCache));
			TestEqual(TEXT("A full canonical cache stays visually bounded"), CacheDisplay->GetInstanceCount(), 4);
			FIslandWildlifeHomeRecord EmptyCache = Home;
			EmptyCache.CachedSupplies.Reset();
			TestTrue(TEXT("Removing cached supplies updates the materialized home"), Materialized->ApplyHomeRecord(EmptyCache));
			TestEqual(TEXT("Removed supplies leave no stale cache visuals"), CacheDisplay->GetInstanceCount(), 0);
			TestTrue(TEXT("The canonical home can restore its current cache"), Materialized->ApplyHomeRecord(Home));
		}
	}

	TestFalse(TEXT("An invalid day cannot change the Rat's home"), State->RecordWildlifeHomeRestStart(Home.Id, 0));
	TestTrue(TEXT("A physical rest start is recorded on the first Island day"), State->RecordWildlifeHomeRestStart(Home.Id, 1));
	TestTrue(TEXT("A second rest start on the same day is idempotent"), State->RecordWildlifeHomeRestStart(Home.Id, 1));
	const FIslandWildlifeHomeRecord* AfterFirstDay = State->FindWildlifeHome(Home.Id);
	TestTrue(TEXT("One day of use keeps a provisional home temporary"), AfterFirstDay &&
		AfterFirstDay->RestVisitDays == 1 && AfterFirstDay->ShelterState == EIslandWildlifeShelterState::Temporary);
	TestTrue(TEXT("A second distinct Island day is recorded"), State->RecordWildlifeHomeRestStart(Home.Id, 2));
	TestTrue(TEXT("The third distinct Island day settles the occupied home"), State->RecordWildlifeHomeRestStart(Home.Id, 3));
	const FIslandWildlifeHomeRecord* SettledHome = State->FindWildlifeHome(Home.Id);
	TestTrue(TEXT("Three observed rest-start days produce a settled canonical state"), SettledHome &&
		SettledHome->RestVisitDays == 3 && SettledHome->LastRestStartIslandDay == 3 &&
		SettledHome->ShelterState == EIslandWildlifeShelterState::Settled);

	TestTrue(TEXT("The updated file can be read again"), State->ReadStateFile(StateFile));
	const FIslandWildlifeHomeRecord* Loaded = State->FindWildlifeHome(Home.Id);
	TestNotNull(TEXT("The wildlife home survives a JSON round trip"), Loaded);
	if (Loaded)
	{
		TestTrue(TEXT("Species survives persistence"), Loaded->Species == FName(TEXT("Rat")));
		TestTrue(TEXT("Entrance anchor survives persistence"), Loaded->EntranceLocation.Equals(Home.EntranceLocation, 0.01f));
		TestTrue(TEXT("Shelter state survives persistence"), Loaded->ShelterState == EIslandWildlifeShelterState::Settled);
		TestEqual(TEXT("Distinct rest-visit days survive persistence"), Loaded->RestVisitDays, 3);
		TestEqual(TEXT("Last rest day survives persistence"), Loaded->LastRestStartIslandDay, 3);
		TestEqual(TEXT("Cached supplies survive persistence"), Loaded->CachedSupplies.Num(), 1);
		if (Loaded->CachedSupplies.Num() == 1)
			TestEqual(TEXT("Cache quantity survives persistence"), Loaded->CachedSupplies[0].Quantity, 3);
	}
	TestEqual(TEXT("Older nest data is preserved when the new field is written"), State->GetNests().Num(), 1);
	State->LoadAndSpawn();
	TestEqual(TEXT("Loading the saved record recreates exactly one transient RatHollow"), CountWildlifeHomeActors(World, Home.Id), 1);
	if (Loaded)
	{
		FIslandWildlifeHomeRecord DisturbedHome = *Loaded;
		DisturbedHome.ShelterState = EIslandWildlifeShelterState::Disturbed;
		TestTrue(TEXT("An explicit disturbance is stored"), State->StoreWildlifeHome(DisturbedHome));
		TestTrue(TEXT("A later rest visit is recorded without erasing disturbance"), State->RecordWildlifeHomeRestStart(Home.Id, 4));
		TestTrue(TEXT("Repeated future use remains recordable"), State->RecordWildlifeHomeRestStart(Home.Id, 5));
		TestTrue(TEXT("A third later rest visit is recorded"), State->RecordWildlifeHomeRestStart(Home.Id, 6));
		const FIslandWildlifeHomeRecord* StillDisturbed = State->FindWildlifeHome(Home.Id);
		TestTrue(TEXT("Repeated use does not silently rehabilitate a disturbed home"), StillDisturbed &&
			StillDisturbed->ShelterState == EIslandWildlifeShelterState::Disturbed &&
			StillDisturbed->RestVisitDays == 6);
	}
	TestTrue(TEXT("Developer removal reverses only the wildlife-home record"), State->RemoveWildlifeHome(Home.Id));
	TestEqual(TEXT("The legacy nest record remains after home removal"), State->GetNests().Num(), 1);
	TestFalse(TEXT("The removed home is no longer present"), State->FindWildlifeHome(Home.Id) != nullptr);
	TestEqual(TEXT("Removing the record also removes its transient presentation actor"), CountWildlifeHomeActors(World, Home.Id), 0);

	IFileManager::Get().Delete(*StateFile, false, true, true);
	DestroyNestWorld(World);
	return true;
}
