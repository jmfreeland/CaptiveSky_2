#include "Agent/IslandTideglassDragonfly.h"
#include "Agent/IslandWeather.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandDragonflyMeadowFlowerResponseTest,
	"CaptiveSky2.Agent.Ecology.DragonflySeasonalFlowerVisit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandDragonflyMeadowFlowerResponseTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic ecology world created"), World) ||
		!TestNotNull(TEXT("Engine is available for the ecology fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>();
	AIslandTideglassDragonfly* Dragonfly = World->SpawnActor<AIslandTideglassDragonfly>(
		FVector(500.f, 0.f, 140.f), FRotator::ZeroRotator);
	UStaticMesh* FlowerMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!TestNotNull(TEXT("Weather actor spawned"), Weather) ||
		!TestNotNull(TEXT("Dragonfly actor spawned"), Dragonfly) ||
		!TestNotNull(TEXT("Engine fixture flower mesh loaded"), FlowerMesh))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	constexpr int32 SpeciesCount = AIslandWeather::MeadowFlowerSpeciesCount;
	Weather->IslandMeadowFlowers.SetNumZeroed(SpeciesCount);
	Weather->IslandMeadowFlowerBaseTransforms.SetNum(SpeciesCount);
	Weather->IslandMeadowFlowerBloomScales.SetNum(SpeciesCount);
	UHierarchicalInstancedStaticMeshComponent* Bluebells = NewObject<UHierarchicalInstancedStaticMeshComponent>(
		Weather, TEXT("SeasonalBluebellFixture"));
	Bluebells->SetupAttachment(Weather->GetRootComponent());
	Bluebells->SetStaticMesh(FlowerMesh);
	Bluebells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bluebells->RegisterComponent();
	const FVector FlowerLocation(0.f, 0.f, 0.f);
	const FTransform FlowerTransform(FRotator::ZeroRotator, FlowerLocation, FVector(0.5f));
	Bluebells->AddInstance(FlowerTransform, true);
	Weather->IslandMeadowFlowers[AIslandWeather::WoodlandBluebellSpeciesIndex] = Bluebells;
	Weather->IslandMeadowFlowerBaseTransforms[AIslandWeather::WoodlandBluebellSpeciesIndex].Add(FlowerTransform);
	Dragonfly->Weather = Weather;
	Dragonfly->HomeLocation = Dragonfly->GetActorLocation();

	Weather->UpdateSeasonalFlowerVisibility(60.0);
	FVector FoundFlower = FVector::ZeroVector;
	TestTrue(TEXT("A spring bluebell is visible to the pollinator query"),
		Weather->FindNearestMeadowFlower(Dragonfly->GetActorLocation(), 900.f, FoundFlower));
	TestTrue(TEXT("The seasonal pollinator query returns the planted bloom location"),
		FoundFlower.Equals(FlowerLocation, 1.f));
	FVector HoverStations[3];
	for (int32 Variant = 0; Variant < UE_ARRAY_COUNT(HoverStations); ++Variant)
	{
		Dragonfly->SetColorVariant(Variant);
		Dragonfly->FlowerInterestRemaining = 0.f;
		Dragonfly->FlowerInterestCooldownRemaining = 0.f;
		TestTrue(FString::Printf(TEXT("Color morph %d accepts a nearby seasonal bloom"), Variant),
			Dragonfly->RespondToMeadowFlower(FoundFlower));
		HoverStations[Variant] = Dragonfly->FlowerInterestLocation;
		TestTrue(FString::Printf(TEXT("Color morph %d hovers above the bloom at the shared radius"), Variant),
			FMath::IsNearlyEqual(HoverStations[Variant].Z, FlowerLocation.Z + 70.f) &&
			FMath::IsNearlyEqual(HoverStations[Variant].Size2D(), 32.f, 0.01f));
	}
	TestTrue(TEXT("Three dragonfly color morphs choose separate stations around the flower"),
		FVector::DistSquared2D(HoverStations[0], HoverStations[1]) > FMath::Square(50.f) &&
		FVector::DistSquared2D(HoverStations[1], HoverStations[2]) > FMath::Square(50.f) &&
		FVector::DistSquared2D(HoverStations[2], HoverStations[0]) > FMath::Square(50.f));
	TestTrue(TEXT("The visit is brief and has a separate re-visit cooldown"),
		FMath::IsNearlyEqual(Dragonfly->FlowerInterestRemaining, 8.f) &&
		FMath::IsNearlyEqual(Dragonfly->FlowerInterestCooldownRemaining, 16.f));

	const FVector BeforeVisit = Dragonfly->GetActorLocation();
	Dragonfly->Tick(0.6f);
	TestTrue(TEXT("The dragonfly bends its short flight toward the active spring flower"),
		FVector::Dist(Dragonfly->GetActorLocation(), Dragonfly->FlowerInterestLocation) <
		FVector::Dist(BeforeVisit, Dragonfly->FlowerInterestLocation));

	Weather->UpdateSeasonalFlowerVisibility(150.0);
	TestFalse(TEXT("The same bluebell is unavailable to pollinators out of season"),
		Weather->FindNearestMeadowFlower(Dragonfly->GetActorLocation(), 900.f, FoundFlower));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
