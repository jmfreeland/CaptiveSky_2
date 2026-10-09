#include "Misc/AutomationTest.h"
#include "IslandRainBasin.h"
#include "AgentBrainComponent.h"
#include "IslandInteractionUtility.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandRainBasinTest, "CaptiveSky2.Agent.IslandRainBasin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandRainBasinTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Dry sky, dry night: nothing to gain or lose"), FIslandBasinState::Advance(0.f, 0.f, -0.5f, 600.f), 0.f);
	TestTrue(TEXT("Heavy rain fills it"), FIslandBasinState::Advance(0.f, 1.f, -0.5f, 120.f) > 0.4f);
	TestEqual(TEXT("Rain cannot overfill it"), FIslandBasinState::Advance(0.9f, 1.f, 0.f, 600.f), 1.f);
	TestTrue(TEXT("A sprinkle gathers nothing"), FIslandBasinState::Advance(0.f, 0.03f, 0.f, 600.f) <= 0.f);
	TestTrue(TEXT("Heavy rain still fills the basin in a strong wind"), FIslandBasinState::Advance(0.f, 1.f, -0.5f, 120.f, 600.f) > 0.4f);
	const float Noon = FIslandBasinState::Advance(0.5f, 0.f, 1.f, 300.f);
	const float Night = FIslandBasinState::Advance(0.5f, 0.f, -1.f, 300.f);
	TestTrue(TEXT("Sun dries it faster than night"), Noon < Night && Night < 0.5f);
	const float CalmNight = FIslandBasinState::Advance(0.5f, 0.f, -1.f, 300.f, 0.f);
	const float BreezyNight = FIslandBasinState::Advance(0.5f, 0.f, -1.f, 300.f, 600.f);
	TestTrue(TEXT("Breezy night dries it faster than calm night"), BreezyNight < CalmNight);
	const float CalmNoon = FIslandBasinState::Advance(0.5f, 0.f, 1.f, 300.f, 0.f);
	const float BreezyNoon = FIslandBasinState::Advance(0.5f, 0.f, 1.f, 300.f, 600.f);
	TestTrue(TEXT("Breeze also speeds sunny-day drying"), BreezyNoon < CalmNoon);
	TestTrue(TEXT("It stays above zero while drying slowly"), FIslandBasinState::Advance(0.5f, 0.f, 1.f, 10.f) > 0.49f);
	TestEqual(TEXT("It never dries below zero"), FIslandBasinState::Advance(0.01f, 0.f, 1.f, 3600.f), 0.f);
	TestEqual(TEXT("No time passing changes nothing"), FIslandBasinState::Advance(0.4f, 1.f, 1.f, 0.f), 0.4f);
	TestEqual(TEXT("A non-finite wind sample leaves the water unchanged"), FIslandBasinState::Advance(0.4f, 0.f, 1.f, 10.f, NAN), 0.4f);
	const float Nonsense = FIslandBasinState::Advance(NAN, 5.f, -9.f, 50.f);
	TestTrue(TEXT("Non-finite water is treated as dry and stays in range"), Nonsense >= 0.f && Nonsense <= 1.f);

	FIslandBasinState State;
	State.bPlaced = true;
	State.Water = 0.1f;
	TestTrue(TEXT("Too little water to float a leaf"), State.FloatLeaf(TEXT("a"), 3, 11) == EIslandBasinFloat::TooDry);
	TestEqual(TEXT("Nothing was added when dry"), State.Leaves.Num(), 0);
	State.Water = 0.7f;
	TestTrue(TEXT("A leaf floats on a good pool"), State.FloatLeaf(TEXT("a"), 3, 11) == EIslandBasinFloat::Floated);
	TestTrue(TEXT("Same resident, same day: no second leaf"), State.FloatLeaf(TEXT("a"), 3, 12) == EIslandBasinFloat::AlreadyToday);
	TestTrue(TEXT("Same resident, next day: another leaf"), State.FloatLeaf(TEXT("a"), 4, 13) == EIslandBasinFloat::Floated);
	TestTrue(TEXT("Another resident the same day: a leaf"), State.FloatLeaf(TEXT("b"), 4, 14) == EIslandBasinFloat::Floated);
	TestEqual(TEXT("Leaves are counted per resident"), State.LeavesFrom(TEXT("a")), 2);
	for (int32 Day = 5; Day < 5 + (FIslandBasinState::MaxLeaves - 3); ++Day) State.FloatLeaf(TEXT("c"), Day, Day);
	TestEqual(TEXT("The basin holds a bounded number of leaves"), State.Leaves.Num(), FIslandBasinState::MaxLeaves);
	TestTrue(TEXT("A crowded basin drops its oldest leaf"), State.FloatLeaf(TEXT("d"), 99, 5) == EIslandBasinFloat::FloatedReplacingOldest);
	TestEqual(TEXT("The oldest leaf (resident a, day 3) is gone"), State.LeavesFrom(TEXT("a")), 1);

	FIslandBasinState Loaded;
	TestTrue(TEXT("Saved state loads back"), Loaded.FromJson(State.ToJson()));
	TestEqual(TEXT("Leaves survive a round trip"), Loaded.Leaves.Num(), State.Leaves.Num());
	TestEqual(TEXT("Water survives a round trip"), Loaded.Water, State.Water);
	TestTrue(TEXT("Place survives a round trip"), Loaded.bPlaced);
	if (Loaded.Leaves.Num() > 0) TestEqual(TEXT("A leaf keeps its author"), Loaded.Leaves.Last().AgentId, State.Leaves.Last().AgentId);
	TestFalse(TEXT("Garbage is rejected"), Loaded.FromJson(TEXT("not json")));
	TestFalse(TEXT("A future version is rejected"), Loaded.FromJson(TEXT("{\"version\":99}")));

	TestTrue(TEXT("Water is deeper when fuller"), AIslandRainBasin::WaterDepth(0.9f) > AIslandRainBasin::WaterDepth(0.2f));
	TestTrue(TEXT("Water stays below the rim"), AIslandRainBasin::WaterDepth(9.f) < AIslandRainBasin::RimHeight);
	TestTrue(TEXT("Leaves brown with age"), AIslandRainBasin::LeafColor(0).G > AIslandRainBasin::LeafColor(8).G);

	TestTrue(TEXT("A dry basin says so"), UIslandRainBasinSubsystem::DescribeWater(0.f, 0).Contains(TEXT("dry")));
	TestTrue(TEXT("A full basin says so"), UIslandRainBasinSubsystem::DescribeWater(1.f, 0).Contains(TEXT("full")));
	TestTrue(TEXT("One leaf is singular"), UIslandRainBasinSubsystem::DescribeWater(0.5f, 1).Contains(TEXT("1 leaf drifts")));
	TestTrue(TEXT("Several leaves are plural"), UIslandRainBasinSubsystem::DescribeWater(0.5f, 3).Contains(TEXT("3 leaves drift")));
	TestTrue(TEXT("Leaves on a dry basin lie on the stone"), UIslandRainBasinSubsystem::DescribeWater(0.f, 2).Contains(TEXT("dried leaves lie")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandRainBasinWorldTest, "CaptiveSky2.Agent.IslandRainBasinWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandRainBasinWorldTest::RunTest(const FString& Parameters)
{
	const FString Scratch = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandRainBasin"));
	IFileManager::Get().MakeDirectory(*Scratch, true);
	const FString StateFile = Scratch / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
	auto CleanupFiles = [&StateFile]()
	{
		IFileManager::Get().Delete(*StateFile, false, true, true);
		IFileManager::Get().Delete(*(StateFile + TEXT(".tmp")), false, true, true);
	};
	auto CreateWorld = [&StateFile](bool bWithPlacementFixture)
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
			.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		if (!World) return static_cast<UWorld*>(nullptr);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		if (UIslandRainBasinSubsystem* Basin = World->GetSubsystem<UIslandRainBasinSubsystem>()) Basin->StorageFileOverride = StateFile;
		if (bWithPlacementFixture)
		{
			AActor* Stones = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
			if (Stones) Stones->Tags = { TEXT("IslandLandmark"), TEXT("ListeningStones") };
			AActor* Ground = World->SpawnActor<AActor>();
			if (Ground)
			{
				UBoxComponent* Box = NewObject<UBoxComponent>(Ground);
				Ground->SetRootComponent(Box);
				Box->SetBoxExtent(FVector(5000.f, 5000.f, 20.f));
				Box->SetCollisionProfileName(TEXT("BlockAll"));
				Box->RegisterComponent();
				Ground->SetActorLocation(FVector(0.f, 0.f, -20.f));
			}
		}
		return World;
	};
	auto DestroyWorld = [](UWorld* World)
	{
		if (!World) return;
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	UWorld* World = CreateWorld(true);
	if (!TestNotNull(TEXT("Synthetic basin placement world created"), World))
	{
		CleanupFiles();
		return false;
	}
	World->BeginPlay();
	UIslandRainBasinSubsystem* Basin = World->GetSubsystem<UIslandRainBasinSubsystem>();
	TestNotNull(TEXT("Synthetic world owns the basin subsystem"), Basin);
	if (!Basin)
	{
		DestroyWorld(World);
		CleanupFiles();
		return false;
	}
	TestTrue(TEXT("The tagged Listening Stones cause deterministic basin placement"), Basin->GetState().bPlaced);
	AIslandRainBasin* BasinActor = nullptr;
	for (TActorIterator<AIslandRainBasin> It(World); It; ++It) { BasinActor = *It; break; }
	TestNotNull(TEXT("Placed basin has a visible interaction actor"), BasinActor);
	TestTrue(TEXT("The basin resolves through the ordinary interaction target tag"),
		BasinActor && IslandInteractionUtility::GetTargetTag(BasinActor) == FName(TEXT("RainBasin")));
	TestTrue(TEXT("The basin actor occupies its saved ground position plus its inspection lift"),
		BasinActor && BasinActor->GetActorLocation().Equals(Basin->GetState().Location + FVector(0.f, 0.f, AIslandRainBasin::OriginLift), 0.1f));
	if (BasinActor)
	{
		const FString Nearby = Basin->DescribeNearby(BasinActor->GetActorLocation());
		TestTrue(TEXT("A resident nearby receives the basin interaction affordance"), Nearby.Contains(TEXT("RainBasin")) && Nearby.Contains(TEXT("interact")));
		TestTrue(TEXT("A resident outside the notice radius receives no basin prompt"), Basin->DescribeNearby(BasinActor->GetActorLocation() + FVector(0.f, 0.f, UIslandRainBasinSubsystem::NoticeRadius + 100.f)).IsEmpty());
	}
	FIslandBasinState PlacedState;
	FString Json;
	TestTrue(TEXT("Initial placement is persisted to the isolated scratch file"), FFileHelper::LoadFileToString(Json, *StateFile) && PlacedState.FromJson(Json) && PlacedState.bPlaced);

	if (BasinActor)
	{
		Basin->ForceWater(0.7f);
		AActor* Resident = World->SpawnActor<AActor>();
		if (Resident)
		{
			USceneComponent* Root = NewObject<USceneComponent>(Resident);
			Resident->SetRootComponent(Root);
			Root->RegisterComponent();
			Resident->SetActorLocation(BasinActor->GetActorLocation() + FVector(50.f, 0.f, 40.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		TestNotNull(TEXT("Synthetic resident observer created beside the basin"), Resident);
		if (Resident)
		{
			UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Resident);
			Resident->AddInstanceComponent(Brain);
			Brain->RegisterComponent();
			const FString Situation = Brain->BuildSituationSummary(FAgentConversationContext());
			TestTrue(TEXT("Resident situation names the nearby basin and its exact interaction target"),
				Situation.Contains(TEXT("rain basin")) && Situation.Contains(TEXT("RainBasin")) && Situation.Contains(TEXT("rainwater")));

			const bool bCanInteract = IslandInteractionUtility::CanInteract(Resident, BasinActor, 250.f);
			TestTrue(*FString::Printf(TEXT("Resident can perceive the basin at %.1f cm (hidden=%d)"),
				FVector::Dist(Resident->GetActorLocation(), BasinActor->GetActorLocation()), BasinActor->IsHidden()), bCanInteract);
			AActor* FoundTarget = IslandInteractionUtility::FindNearestVisibleTarget(Resident, World, 250.f);
			TestTrue(*FString::Printf(TEXT("Ordinary nearest-visible-target discovery finds the basin (found=%s)"),
				FoundTarget ? *IslandInteractionUtility::GetTargetTag(FoundTarget).ToString() : TEXT("none")), FoundTarget == BasinActor);
			FString Fact;
			TestTrue(TEXT("Resident inspection uses the ordinary interaction path"), IslandInteractionUtility::Perform(Resident, BasinActor, Fact));
			TestTrue(TEXT("The resident's first inspection sets a leaf afloat"), Fact.Contains(TEXT("set it on the water")) && Basin->GetState().Leaves.Num() == 1);
			const int32 LeafCount = Basin->GetState().Leaves.Num();
			TestTrue(TEXT("A repeat inspection reports today's leaf without changing the basin"),
				IslandInteractionUtility::Perform(Resident, BasinActor, Fact) && Fact.Contains(TEXT("earlier today")) && Basin->GetState().Leaves.Num() == LeafCount);
			TestTrue(TEXT("Water and the resident's leaf are durably saved"),
				FFileHelper::LoadFileToString(Json, *StateFile) && PlacedState.FromJson(Json) &&
				FMath::IsNearlyEqual(PlacedState.Water, Basin->GetState().Water) && PlacedState.Leaves.Num() == 1 &&
				PlacedState.Leaves[0].AgentId == Resident->GetName());
		}
	}
	const FVector ExpectedLocation = Basin->GetState().Location;
	DestroyWorld(World);

	UWorld* ReloadedWorld = CreateWorld(false);
	if (!TestNotNull(TEXT("Second synthetic world created for persistence reload"), ReloadedWorld))
	{
		CleanupFiles();
		return false;
	}
	ReloadedWorld->BeginPlay();
	UIslandRainBasinSubsystem* ReloadedBasin = ReloadedWorld->GetSubsystem<UIslandRainBasinSubsystem>();
	TestTrue(TEXT("Saved placement reloads without needing Listening Stones in the new world"),
		ReloadedBasin && ReloadedBasin->GetState().bPlaced && ReloadedBasin->GetState().Location.Equals(ExpectedLocation, 0.1f));
	TestTrue(TEXT("Saved water and the resident's leaf survive subsystem reload"),
		ReloadedBasin && ReloadedBasin->GetState().Leaves.Num() == PlacedState.Leaves.Num() && PlacedState.Leaves.Num() == 1 &&
		ReloadedBasin->GetState().Leaves[0].Day == PlacedState.Leaves[0].Day &&
		ReloadedBasin->GetState().Leaves[0].AgentId == PlacedState.Leaves[0].AgentId &&
		FMath::IsNearlyEqual(ReloadedBasin->GetState().Water, PlacedState.Water));
	AIslandRainBasin* ReloadedActor = nullptr;
	for (TActorIterator<AIslandRainBasin> It(ReloadedWorld); It; ++It) { ReloadedActor = *It; break; }
	TestTrue(TEXT("Reload materializes the basin actor at its saved ground position"),
		ReloadedActor && ReloadedActor->GetActorLocation().Equals(ExpectedLocation + FVector(0.f, 0.f, AIslandRainBasin::OriginLift), 0.1f));
	DestroyWorld(ReloadedWorld);
	CleanupFiles();
	return true;
}
