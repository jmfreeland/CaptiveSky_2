#include "Misc/AutomationTest.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandClockTest, "CaptiveSky2.Agent.DayNight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIslandClockTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Midnight wraps"), AIslandDayNight::WrapHour(24), 0.f);
	TestEqual(TEXT("Negative preview wraps"), AIslandDayNight::WrapHour(-1), 23.f);
	TestTrue(TEXT("Sun rises at six"), FMath::IsNearlyZero(AIslandDayNight::SunHeight(6)));
	TestTrue(TEXT("Sun overhead at noon"), FMath::IsNearlyEqual(AIslandDayNight::SunHeight(12), 1.f));
	TestTrue(TEXT("Sun below horizon at midnight"), FMath::IsNearlyEqual(AIslandDayNight::SunHeight(0), -1.f));
	TestTrue(TEXT("Clear weather leaves daylight unchanged"), FMath::IsNearlyEqual(AIslandDayNight::CloudSunlightTransmission(0.f), 1.f) && FMath::IsNearlyEqual(AIslandDayNight::CloudSkylightTransmission(0.f), 1.f));
	TestTrue(TEXT("Overcast modestly softens direct sunlight"), FMath::IsNearlyEqual(AIslandDayNight::CloudSunlightTransmission(1.f), 0.76f));
	TestTrue(TEXT("Overcast gently softens ambient skylight"), FMath::IsNearlyEqual(AIslandDayNight::CloudSkylightTransmission(1.f), 0.70f));
	TestTrue(TEXT("Cloud illumination remains bounded for out-of-range input"), AIslandDayNight::CloudSunlightTransmission(2.f) == AIslandDayNight::CloudSunlightTransmission(1.f));

	if (!TestNotNull(TEXT("Engine is available for the lighting fixture"), GEngine)) return false;
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Lighting fixture world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(Spawn);
	ASkyLight* Sky = World->SpawnActor<ASkyLight>(Spawn);
	if (!TestNotNull(TEXT("Weather spawned in lighting fixture"), Weather) || !TestNotNull(TEXT("Clock spawned in lighting fixture"), Clock) || !TestNotNull(TEXT("Sun spawned in lighting fixture"), Sun) || !TestNotNull(TEXT("Sky spawned in lighting fixture"), Sky))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Clock->Sun = Sun;
	Clock->Sky = Sky;
	Clock->CurrentHour = 12.f;
	Clock->DaySunIntensity = 10.f;
	Clock->MoonIntensity = 0.f;
	float MinimumCover = 2.f;
	float MaximumCover = -1.f;
	int32 ClearSeed = 0;
	int32 OvercastSeed = 0;
	for (int32 Seed = -1000; Seed <= 1000; ++Seed)
	{
		Weather->WeatherSeed = Seed;
		const float Cover = Weather->SampleCloudCover(World->GetTimeSeconds());
		if (Cover < MinimumCover) { MinimumCover = Cover; ClearSeed = Seed; }
		if (Cover > MaximumCover) { MaximumCover = Cover; OvercastSeed = Seed; }
	}
	Weather->WeatherSeed = ClearSeed;
	Clock->UpdateLighting();
	const float ClearSun = Sun->GetLightComponent()->Intensity;
	const float ClearSky = Sky->GetLightComponent()->Intensity;
	Weather->WeatherSeed = OvercastSeed;
	Clock->UpdateLighting();
	TestTrue(TEXT("Overcast cloud signal dims the actual directional sunlight"), Sun->GetLightComponent()->Intensity < ClearSun * 0.8f);
	TestTrue(TEXT("Overcast cloud signal dims the actual skylight"), Sky->GetLightComponent()->Intensity < ClearSky * 0.75f);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
