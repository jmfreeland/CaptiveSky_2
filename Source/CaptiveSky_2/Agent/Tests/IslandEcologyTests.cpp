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
	TestTrue(TEXT("Local wind gently nudges the firefly drift"), AIslandFirefly::WindDisplacement(FVector(100.f, 0.f, 0.f)).Equals(FVector(12.f, 0.f, 0.f)));
	TestTrue(TEXT("Strong gust displacement stays bounded"), AIslandFirefly::WindDisplacement(FVector(1000.f, 0.f, 0.f)).Equals(FVector(30.f, 0.f, 0.f)));
	TestTrue(TEXT("Still air adds no wind displacement"), AIslandFirefly::WindDisplacement(FVector::ZeroVector).IsNearlyZero());

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

	bool bFoundIslandEditorWorld = false;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Island = Context.World();
		if (Context.WorldType != EWorldType::Editor || !Island || Island->GetMapName() != TEXT("Island")) continue;
		bFoundIslandEditorWorld = true;
		bool bHasWeather = false;
		bool bHasDayNight = false;
		bool bHasTideglassHabitat = false;
		for (TActorIterator<AIslandWeather> It(Island); It; ++It) bHasWeather = true;
		for (TActorIterator<AIslandDayNight> It(Island); It; ++It) bHasDayNight = true;
		for (TActorIterator<AActor> It(Island); It; ++It)
			if (It->ActorHasTag(TEXT("TideglassPool"))) bHasTideglassHabitat = true;
		TestTrue(TEXT("Saved Island contains the weather actor required by the ecology spawner"), bHasWeather);
		TestTrue(TEXT("Saved Island contains the day/night clock required by the ecology spawner"), bHasDayNight);
		TestTrue(TEXT("Saved Island contains the TideglassPool habitat tag"), bHasTideglassHabitat);
		break;
	}
	TestTrue(TEXT("Editor automation opened the saved Island map"), bFoundIslandEditorWorld);
	return true;
}
