#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandCurio.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCurioTest, "CaptiveSky2.Agent.IslandCurio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	constexpr float GroundTop = 0.f;

	/** Flat open ground with a ListeningStones landmark, its lasting state in a scratch file. */
	UWorld* CreateCurioWorld(const FString& StateFile, bool bWithLandmark)
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->GetSubsystem<UIslandWorldStateSubsystem>()->StorageFileOverride = StateFile;
		AActor* Ground = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Ground);
		Ground->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(8000, 8000, 50));
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Ground->SetActorLocation(FVector(0, 0, GroundTop - 50));
		if (bWithLandmark)
		{
			ATargetPoint* Stones = World->SpawnActor<ATargetPoint>(FVector(0, 0, 150), FRotator::ZeroRotator);
			Stones->Tags = {TEXT("ListeningStones"), TEXT("IslandLandmark")};
		}
		return World;
	}

	void DestroyCurioWorld(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	AIslandCurio* FindCurioActor(UWorld* World, FName Id)
	{
		for (TActorIterator<AIslandCurio> It(World); It; ++It)
			if (It->ActorHasTag(Id)) return *It;
		return nullptr;
	}
}

bool FIslandCurioTest::RunTest(const FString& Parameters)
{
	// No gateway, model requests, or autobiographical memory in this fixture.
	// What a brain-only resident standing at Where would be told (a lambda so it shares this test's access).
	auto DescribeFrom = [](UWorld* World, const FVector& Where, const FString& AgentId = FString())
	{
		ACharacter* Body = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Body);
		Body->AddInstanceComponent(Brain);
		Brain->RegisterComponent();
		if (!AgentId.IsEmpty())
		{
			UAgentMemoryComponent* Memory = NewObject<UAgentMemoryComponent>(Body);
			Memory->AgentId = AgentId;
			Body->AddInstanceComponent(Memory);
			Memory->RegisterComponent();
		}
		const FString View = Brain->BuildSituationSummary(FAgentConversationContext());
		Body->Destroy();
		return View;
	};
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandCurio") / TEXT("WorldState.json"));
	IFileManager::Get().Delete(*StateFile, false, true, true);

	UWorld* World = CreateCurioWorld(StateFile, false);
	World->BeginPlay();
	TestEqual(TEXT("Levels without the Island landmarks get no curios"), World->GetSubsystem<UIslandWorldStateSubsystem>()->GetCurios().Num(), 0);
	DestroyCurioWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);

	World = CreateCurioWorld(StateFile, true);
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	TestEqual(TEXT("Six trail stones, a pod, and a cairn are placed on first play"), State->GetCurios().Num(), 8);
	TestTrue(TEXT("Placement is saved immediately"), FPaths::FileExists(StateFile));
	TArray<FVector> Placed;
	for (const FIslandCurioRecord& Curio : State->GetCurios())
	{
		Placed.Add(Curio.Location);
		TestTrue(*FString::Printf(TEXT("%s rests on the ground"), *Curio.Id.ToString()), FMath::IsNearlyEqual(Curio.Location.Z, GroundTop, 1.f));
		TestNotNull(*FString::Printf(TEXT("%s is visible"), *Curio.Id.ToString()), FindCurioActor(World, Curio.Id));
	}
	const FIslandCurioRecord* First = State->FindCurio(TEXT("PaleStone_1"));
	const FIslandCurioRecord* Last = State->FindCurio(TEXT("PaleStone_6"));
	const FIslandCurioRecord* Pod = State->FindCurio(TEXT("Seedpod"));
	if (!TestTrue(TEXT("Trail and pod exist"), First && Last && Pod)) { DestroyCurioWorld(World); return false; }
	TestTrue(TEXT("The trail leads away from the ListeningStones"), FVector::Dist2D(First->Location, FVector::ZeroVector) < FVector::Dist2D(Last->Location, FVector::ZeroVector));
	TestTrue(TEXT("The pod lies just past the last stone"), FVector::Dist2D(Pod->Location, Last->Location) < 900.f && FVector::Dist2D(Pod->Location, FVector::ZeroVector) > 3000.f);

	// Hidden: nothing is mentioned from the landmark itself; each stone reveals only the next.
	const FString AtLandmark = DescribeFrom(World, FVector(0, 0, 100));
	TestFalse(TEXT("From the landmark, at most the first stone can be noticed"), AtLandmark.Contains(TEXT("target: PaleStone_3")) || AtLandmark.Contains(TEXT("Seedpod")));
	const FString AtFirstStone = DescribeFrom(World, First->Location + FVector(150, 0, 100));
	TestTrue(TEXT("Close to the first stone, it is noticed"), AtFirstStone.Contains(TEXT("target: PaleStone_1")));
	TestTrue(TEXT("The first stone hints at the second"), AtFirstStone.Contains(TEXT("move_to target: PaleStone_2")));
	TestFalse(TEXT("The pod is not given away at the start of the trail"), AtFirstStone.Contains(TEXT("Seedpod")));
	const FIslandCurioRecord* Second = State->FindCurio(TEXT("PaleStone_2"));
	if (TestNotNull(TEXT("Second trail stone exists for the visibility check"), Second))
	{
		const FVector TrailObserver = First->Location + (Second->Location - First->Location).GetSafeNormal() * 150.f + FVector(0.f, 0.f, 100.f);
		const FVector NextView = Second->Location + FVector(0.f, 0.f, AIslandCurio::GroundClearance);
		AActor* TrailBlocker = World->SpawnActor<AActor>();
		if (!TestNotNull(TEXT("Occlusion blocker spawned"), TrailBlocker))
		{
			DestroyCurioWorld(World);
			IFileManager::Get().Delete(*StateFile, false, true, true);
			return false;
		}
		UBoxComponent* TrailBlockerBox = NewObject<UBoxComponent>(TrailBlocker);
		TrailBlocker->SetRootComponent(TrailBlockerBox);
		TrailBlockerBox->SetBoxExtent(FVector(40.f, 240.f, 150.f));
		TrailBlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
		TrailBlockerBox->RegisterComponent();
		TrailBlocker->SetActorLocation(FMath::Lerp(TrailObserver, NextView, 0.55f));
		TrailBlocker->SetActorRotation(FRotator(0.f, (NextView - TrailObserver).Rotation().Yaw, 0.f));
		FCollisionQueryParams VisibilityCheck(SCENE_QUERY_STAT(IslandCurioOcclusionFixture), false);
		FHitResult BlockedView;
		TestTrue(TEXT("The fixture wall blocks the view to the next stone"), World->LineTraceSingleByChannel(BlockedView, TrailObserver, NextView, ECC_Visibility, VisibilityCheck));
		const FString BehindWall = DescribeFrom(World, TrailObserver);
		TestTrue(TEXT("The first stone remains perceptible before the wall"), BehindWall.Contains(TEXT("target: PaleStone_1")));
		TestFalse(TEXT("An occluded next stone is not hinted to the resident"), BehindWall.Contains(TEXT("move_to target: PaleStone_2")));
		TrailBlocker->Destroy();
	}
	const FString AtLastStone = DescribeFrom(World, Last->Location + FVector(0, 0, 100));
	TestFalse(TEXT("The last stone points nowhere further"), AtLastStone.Contains(TEXT("PaleStone_7")));
	const FString AtPod = DescribeFrom(World, Pod->Location + FVector(200, 0, 100));
	TestTrue(TEXT("The closed pod is noticed up close"), AtPod.Contains(TEXT("closed pod")) && AtPod.Contains(TEXT("target: Seedpod")));

	// Pod: one step per visit on a new Island day, then it stays open.
	TestTrue(TEXT("Stones never change"), State->ExamineCurio(TEXT("PaleStone_3"), 1).Contains(TEXT("leave it as it was")));
	TestTrue(TEXT("Day 1 visit opens the pod a little"), State->ExamineCurio(TEXT("Seedpod"), 1).Contains(TEXT("peel back")) && State->FindCurio(TEXT("Seedpod"))->State == 1);
	TestTrue(TEXT("A second visit the same day changes nothing"), State->ExamineCurio(TEXT("Seedpod"), 1).Contains(TEXT("nothing about it has changed")) && State->FindCurio(TEXT("Seedpod"))->State == 1);
	TestTrue(TEXT("Perception says it already changed today"), DescribeFrom(World, Pod->Location + FVector(200, 0, 100)).Contains(TEXT("already changed once today")));
	State->ExamineCurio(TEXT("Seedpod"), 2);
	TestTrue(TEXT("Day 3 opens it fully"), State->ExamineCurio(TEXT("Seedpod"), 3).Contains(TEXT("glowing faintly")));
	AIslandCurio* PodActor = FindCurioActor(World, TEXT("Seedpod"));
	TestTrue(TEXT("The open pod visibly glows"), PodActor && PodActor->IsGlowing());
	TestTrue(TEXT("An open pod stays open"), State->ExamineCurio(TEXT("Seedpod"), 4).Contains(TEXT("stands open")) && State->FindCurio(TEXT("Seedpod"))->State == AIslandCurio::PodOpenState);

	// Cairn through the real Interact path: any resident, one stone per Island day.
	const FIslandCurioRecord* Cairn = State->FindCurio(TEXT("Cairn"));
	if (!TestNotNull(TEXT("Cairn exists"), Cairn)) { DestroyCurioWorld(World); return false; }
	ACharacter* Visitor = World->SpawnActor<ACharacter>(Cairn->Location + FVector(150, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	Controller->Possess(Visitor);
	FAgentDecision Examine;
	Examine.bValid = true;
	Examine.ActionType = EAgentActionType::Interact;
	Examine.ActionTarget = TEXT("Cairn");
	Controller->ActOnDecision(Examine);
	TestTrue(TEXT("A resident adds a stone"), Controller->DescribeActionState().Contains(TEXT("now stands 4 stones high")));
	TestEqual(TEXT("The cairn's new height is shown"), FindCurioActor(World, TEXT("Cairn"))->GetVisibleStoneCount(), 4);
	const FString CairnContributorId = Visitor->GetName();
	TestTrue(TEXT("The cairn records the resident who added the stone"), State->FindCurio(TEXT("Cairn"))->Contributors.Contains(CairnContributorId));
	const FString ContributorView = DescribeFrom(World, Cairn->Location + FVector(150, 0, 100), CairnContributorId);
	TestTrue(TEXT("A maker recognizes their own contribution in perception"), ContributorView.Contains(TEXT("including stones you set there")));
	TestTrue(TEXT("A maker is not told who placed the other stones"), ContributorView.Contains(TEXT("does not identify who placed the other stones")));
	TestTrue(TEXT("Only one stone per day, with no false contributor record"), State->ExamineCurio(TEXT("Cairn"), 1, TEXT("AnotherResident")).Contains(TEXT("another would topple it")) && State->FindCurio(TEXT("Cairn"))->State == 4 && !State->FindCurio(TEXT("Cairn"))->Contributors.Contains(TEXT("AnotherResident")));
	TestTrue(TEXT("A second resident can contribute on a later day"), State->ExamineCurio(TEXT("Cairn"), 2, TEXT("AnotherResident")).Contains(TEXT("now stands 5 stones high")) && State->FindCurio(TEXT("Cairn"))->Contributors.Contains(TEXT("AnotherResident")));
	const FString OtherMakerView = DescribeFrom(World, Cairn->Location + FVector(150, 0, 100), TEXT("AnotherResident"));
	TestTrue(TEXT("A second maker recognizes their own contribution"), OtherMakerView.Contains(TEXT("including stones you set there")));
	const FString UninvolvedView = DescribeFrom(World, Cairn->Location + FVector(150, 0, 100), TEXT("UninvolvedResident"));
	TestTrue(TEXT("An uninvolved resident receives no invented authorship"), UninvolvedView.Contains(TEXT("does not identify you as a contributor")));
	Controller->UnPossess();
	DestroyCurioWorld(World);

	// A later session finds everything where and as it was left.
	World = CreateCurioWorld(StateFile, true);
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	TestEqual(TEXT("Curios are not placed a second time"), State->GetCurios().Num(), 8);
	bool bSamePlaces = State->GetCurios().Num() == Placed.Num();
	for (int32 Index = 0; bSamePlaces && Index < Placed.Num(); ++Index) bSamePlaces = State->GetCurios()[Index].Location.Equals(Placed[Index], 0.5f);
	TestTrue(TEXT("Curios stay exactly where they were"), bSamePlaces);
	TestEqual(TEXT("The open pod stays open next session"), State->FindCurio(TEXT("Seedpod"))->State, AIslandCurio::PodOpenState);
	TestEqual(TEXT("The cairn keeps its added stones"), State->FindCurio(TEXT("Cairn"))->State, 5);
	TestTrue(TEXT("Cairn contributors remain recognizable after reload"), State->FindCurio(TEXT("Cairn"))->Contributors.Contains(CairnContributorId) && State->FindCurio(TEXT("Cairn"))->Contributors.Contains(TEXT("AnotherResident")));
	TestTrue(TEXT("The open pod is restored glowing"), FindCurioActor(World, TEXT("Seedpod")) && FindCurioActor(World, TEXT("Seedpod"))->IsGlowing());
	TestTrue(TEXT("Developer reset forgets the curios"), State->ForgetCurios() && State->GetCurios().Num() == 0 && !FindCurioActor(World, TEXT("Cairn")));
	DestroyCurioWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);

	// When Island is the open editor map, check the real layout against its terrain and navigation
	// without saving or spawning anything.
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Island = Context.World();
		if (Context.WorldType != EWorldType::Editor || !Island || Island->GetMapName() != TEXT("Island")) continue;
		TArray<FIslandCurioRecord> Layout;
		if (!TestTrue(TEXT("The saved Island has open walkable ground for every curio"), UIslandWorldStateSubsystem::BuildCurioLayout(Island, Layout))) continue;
		TestEqual(TEXT("Full Island layout"), Layout.Num(), 8);
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Island);
		FVector Start = FVector::ZeroVector;
		for (TActorIterator<AActor> It(Island); It; ++It)
			if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"))) Start = It->GetActorLocation();
		FNavLocation StartOnNav;
		const bool bStartWalkable = Navigation && Navigation->ProjectPointToNavigation(Start, StartOnNav, FVector(250, 250, 1000));
		TestTrue(TEXT("ListeningStones have walkable ground nearby"), bStartWalkable);
		for (const FIslandCurioRecord& Curio : Layout)
		{
			const UNavigationPath* Path = bStartWalkable ? Navigation->FindPathToLocationSynchronously(Island, StartOnNav.Location, Curio.Location) : nullptr;
			const bool bReachable = Path && Path->IsValid() && !Path->IsPartial();
			AddInfo(FString::Printf(TEXT("%s at %s, %.1f m from the ListeningStones, walk path %s"), *Curio.Id.ToString(), *Curio.Location.ToString(),
				FVector::Dist2D(Curio.Location, Start) / 100.f, bReachable ? *FString::Printf(TEXT("%.1f m"), Path->GetPathLength() / 100.f) : TEXT("missing")));
			TestTrue(*FString::Printf(TEXT("Grounded residents can walk to %s"), *Curio.Id.ToString()), bReachable);
		}
	}
	return true;
}
