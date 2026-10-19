#include "Agent/IslandCommonFrog.h"
#include "Agent/IslandInteractionUtility.h"
#include "CaptiveSky_2PlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCommonFrogInteractionTest,
	"CaptiveSky2.Agent.CommonFrogVisitorInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandCommonFrogInteractionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic visitor world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	// Keep the fixture away from actors that world subsystems may create at the origin.
	const FVector TestOrigin(10000.f, 0.f, 0.f);
	AActor* Visitor = World->SpawnActor<AActor>(TestOrigin + FVector(160.f, 0.f, 40.f), FRotator::ZeroRotator);
	AIslandCommonFrog* Frog = World->SpawnActor<AIslandCommonFrog>(TestOrigin, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Visitor spawned"), Visitor) || !TestNotNull(TEXT("Common frog spawned"), Frog))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	USceneComponent* VisitorRoot = NewObject<USceneComponent>(Visitor);
	Visitor->SetRootComponent(VisitorRoot);
	VisitorRoot->RegisterComponent();
	Visitor->SetActorLocation(TestOrigin + FVector(160.f, 0.f, 40.f));

	TestEqual(TEXT("The frog has a visitor-facing interaction target"),
		IslandInteractionUtility::GetTargetTag(Frog), FName(TEXT("TideglassFrog")));
	const ACaptiveSky_2PlayerController* VisitorController =
		ACaptiveSky_2PlayerController::StaticClass()->GetDefaultObject<ACaptiveSky_2PlayerController>();
	TestTrue(TEXT("The player proximity hint describes quiet frog observation naturally"),
		VisitorController && VisitorController->DescribeWorldInteractionHint(Frog).Contains(TEXT("quietly watch a wild common frog")));
	TestFalse(TEXT("Wild frogs remain non-movement targets"), IslandInteractionUtility::IsMovementTargetAllowed(Frog));
	TestTrue(TEXT("A nearby visitor has clear interaction range"), IslandInteractionUtility::CanInteract(Visitor, Frog));
	AActor* NearestTarget = IslandInteractionUtility::FindNearestVisibleTarget(Visitor, World);
	TestTrue(FString::Printf(TEXT("Nearest-visible interaction discovery can find the frog (selected %s with tag %s)"),
		NearestTarget ? *NearestTarget->GetName() : TEXT("none"),
		NearestTarget ? *IslandInteractionUtility::GetTargetTag(NearestTarget).ToString() : TEXT("none")), NearestTarget == Frog);
	struct FWildlifeHintCase
	{
		const TCHAR* Tag;
		const TCHAR* Phrase;
	};
	const FWildlifeHintCase WildlifeHints[] = {
		{ TEXT("WoodlandFox"), TEXT("quietly watch a wild fox") },
		{ TEXT("WoodlandDeer"), TEXT("quietly watch a wild stag") },
		{ TEXT("TidepoolPeriwinkle"), TEXT("quietly watch a small wild periwinkle") },
		{ TEXT("TideglassDragonfly"), TEXT("quietly watch a Tideglass dragonfly") }
	};
	for (const FWildlifeHintCase& HintCase : WildlifeHints)
	{
		AActor* Creature = World->SpawnActor<AActor>();
		if (!TestNotNull(TEXT("Synthetic wild creature spawned for hint review"), Creature)) continue;
		Creature->Tags.AddUnique(TEXT("IslandLife"));
		Creature->Tags.AddUnique(HintCase.Tag);
		const FString Hint = VisitorController ? VisitorController->DescribeWorldInteractionHint(Creature) : FString();
		TestTrue(FString::Printf(TEXT("%s receives a natural visitor hint"), HintCase.Tag), Hint.Contains(HintCase.Phrase));
	}
	AActor* Ground = World->SpawnActor<AActor>();
	if (Ground)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(Ground);
		Ground->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(5000.f, 5000.f, 20.f));
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Ground->SetActorLocation(TestOrigin + FVector(0.f, 0.f, -20.f));
	}
	if (!TestNotNull(TEXT("Synthetic ground supports the frog hop"), Ground))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();

	FString Fact;
	TestTrue(TEXT("Quiet observation is accepted through the ordinary interaction path"),
		IslandInteractionUtility::Perform(Visitor, Frog, Fact));
	TestTrue(TEXT("The visitor interaction starts a real, short frog hop"), Frog->IsHopping());
	TestTrue(TEXT("The response explains the frog remains wild and unclaimed"),
		Fact.Contains(TEXT("wild")) && Fact.Contains(TEXT("did not touch")));
	const FVector HopStart = Frog->GetActorLocation();
	FString RepeatedFact;
	IslandInteractionUtility::Perform(Visitor, Frog, RepeatedFact);
	TestTrue(TEXT("A repeated observation does not interrupt the same hop"),
		Frog->IsHopping() && Frog->GetActorLocation().Equals(HopStart));
	TestTrue(TEXT("A repeated observation explains that nothing persistent changed"),
		RepeatedFact.Contains(TEXT("nothing persistent changed")));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
