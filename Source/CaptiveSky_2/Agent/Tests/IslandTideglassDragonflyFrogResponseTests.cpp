#include "Agent/IslandTideglassDragonfly.h"
#include "Agent/IslandCommonFrog.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassDragonflyFrogResponseTests,
	"CaptiveSky2.Agent.Ecology.DragonflyFrogResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassDragonflyFrogResponseTests::RunTest(const FString& Parameters)
{
	using Dragonfly = AIslandTideglassDragonfly;
	TestTrue(TEXT("A nearby hopping frog can startle the dragonfly"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 180.f, 110.f));
	TestTrue(TEXT("The startle radius is inclusive"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 230.f, 0.f));
	TestFalse(TEXT("A settled frog is not a startle cue"),
		Dragonfly::ShouldScatterFromHoppingFrog(false, 100.f, 0.f));
	TestFalse(TEXT("A distant hop is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 231.f, 0.f));
	TestFalse(TEXT("A frog at a much higher elevation is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 100.f, 251.f));
	TestFalse(TEXT("Invalid distance is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, std::numeric_limits<float>::quiet_NaN(), 0.f));
	TestFalse(TEXT("Negative distance is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, -1.f, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassDragonflyFrogResponseWorldTest,
	"CaptiveSky2.Agent.Ecology.DragonflyFrogResponseWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassDragonflyFrogResponseWorldTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic ecology world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

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
	AIslandCommonFrog* Frog = World->SpawnActor<AIslandCommonFrog>(FVector::ZeroVector, FRotator::ZeroRotator);
	AIslandTideglassDragonfly* Dragonfly = World->SpawnActor<AIslandTideglassDragonfly>(
		FVector(150.f, 0.f, 110.f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Synthetic ground supports the frog hop"), Ground) ||
		!TestNotNull(TEXT("Frog actor spawned"), Frog) || !TestNotNull(TEXT("Dragonfly actor spawned"), Dragonfly))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	World->BeginPlay();
	TestTrue(TEXT("The nearby frog begins a real transient hop"), Frog->RespondToQuietObservation(FVector(100.f, 0.f, 0.f)));
	Dragonfly->CheckForNearbyHoppingFrog();
	TestTrue(TEXT("The hopping frog starts the dragonfly's brief scatter"), Dragonfly->ScatterRemaining > 0.f);
	TestTrue(TEXT("The dragonfly scatters away from the frog"), Dragonfly->ScatterDirection.X > 0.f);
	TestTrue(TEXT("The frog is latched to prevent repeat startles"), Dragonfly->FrogPresenceLatch.Get() == Frog);

	Dragonfly->ScatterRemaining = 0.f;
	Dragonfly->FrogScatterCooldownRemaining = 0.f;
	Dragonfly->CheckForNearbyHoppingFrog();
	TestTrue(TEXT("The same ongoing hop does not restart the startle"), FMath::IsNearlyZero(Dragonfly->ScatterRemaining));

	Dragonfly->FrogPresenceLatch.Reset();
	Dragonfly->FlowerInterestRemaining = 1.f;
	Dragonfly->CheckForNearbyHoppingFrog();
	TestTrue(TEXT("A deliberate flower visit is not interrupted by the frog cue"),
		FMath::IsNearlyZero(Dragonfly->ScatterRemaining) && FMath::IsNearlyEqual(Dragonfly->FlowerInterestRemaining, 1.f));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
