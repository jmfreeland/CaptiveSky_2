#include "Misc/AutomationTest.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "IslandWeather.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandNightEcologyTest, "CaptiveSky2.Agent.NightEcology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandNightEcologyTest::RunTest(const FString& Parameters)
{
	// Isolated world: no Island agents, brains, memory files, or model requests.
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Fixture world created"), World)) return false;
	if (!TestNotNull(TEXT("Engine is available for the fixture"), GEngine))
	{
		World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	AActor* Habitat = World->SpawnActor<AActor>(FVector(1000.f, 2000.f, 300.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Weather actor spawned"), Weather) || !TestNotNull(TEXT("Clock actor spawned"), Clock) || !TestNotNull(TEXT("Habitat marker spawned"), Habitat))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Habitat->Tags.Add(TEXT("TideglassPool"));
	
	Clock->CurrentHour = 12.f;
	Weather->RefreshNightEcology();
	int32 Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("No fireflies are active at midday"), Population, 0);
	Clock->CurrentHour = 18.f;
	Weather->RefreshNightEcology();
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Fireflies wait until nightfall"), Population, 0);

	Clock->CurrentHour = 20.f;
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It)
	{
		++Population;
		TestTrue(TEXT("Firefly advertises as ambient life"), It->ActorHasTag(TEXT("IslandLife")));
		TestTrue(TEXT("Firefly remains untargeted wildlife"), !It->ActorHasTag(TEXT("IslandLandmark")) && !It->ActorHasTag(TEXT("RavenNestSite")));
		TestTrue(TEXT("Firefly stays near its Tideglass habitat"), FVector::Dist2D(It->GetActorLocation(), Habitat->GetActorLocation()) < 700.f);
		TestNotNull(TEXT("Firefly has a fluctuating glow component"), It->FindComponentByClass<UPointLightComponent>());
	}
	TestEqual(TEXT("Night population is bounded at three"), Population, 3);
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Repeated night refresh does not duplicate the population"), Population, 3);

	Clock->CurrentHour = 12.f;
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Fireflies leave the habitat in daytime"), Population, 0);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
