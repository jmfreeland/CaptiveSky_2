#include "Misc/AutomationTest.h"
#include "IslandWeather.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Sound/SoundWaveProcedural.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherTest, "CaptiveSky2.Agent.IslandWeather",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherTest::RunTest(const FString& Parameters)
{
	const AIslandWeather* Weather = GetDefault<AIslandWeather>();
	float MinimumRain = 2.f;
	float MaximumRain = -1.f;
	double MinimumRainTime = 0.0;
	double MaximumRainTime = 0.0;
	for (int32 I = 0; I < 1000; ++I)
	{
		const FVector Position(I * 50.0, -I * 30.0, 700.0);
		const FVector Wind = Weather->SampleWind(Position, I);
		TestTrue(TEXT("Wind remains finite and bounded"), !Wind.ContainsNaN() && Wind.Size() <= Weather->MaximumWindSpeed + 0.01f);
		const float Cloud = Weather->SampleCloudCover(I);
		TestTrue(TEXT("Cloud cover is normalized"), Cloud >= 0.f && Cloud <= 1.f);
		TestTrue(TEXT("Sampling is repeatable"), Wind.Equals(Weather->SampleWind(Position, I)));
		const float Rain = Weather->SampleRainIntensity(I);
		TestTrue(TEXT("Rain intensity is normalized"), Rain >= 0.f && Rain <= 1.f);
		TestTrue(TEXT("Rain intensity is repeatable"), FMath::IsNearlyEqual(Rain, Weather->SampleRainIntensity(I)));
	}
	for (int32 Seconds = 0; Seconds <= 14400; Seconds += 15)
	{
		const float Rain = Weather->SampleRainIntensity(Seconds);
		if (Rain < MinimumRain) { MinimumRain = Rain; MinimumRainTime = Seconds; }
		if (Rain > MaximumRain) { MaximumRain = Rain; MaximumRainTime = Seconds; }
	}
	TestTrue(TEXT("Independent rain-front cycle includes dry periods"), MinimumRain < 0.01f);
	TestTrue(TEXT("Cloud-gated rain-front cycle includes gentle showers"), MaximumRain > 0.45f);
	TestTrue(TEXT("Rain-free sample remains stable"), FMath::IsNearlyEqual(Weather->SampleRainIntensity(MinimumRainTime), MinimumRain));
	TestTrue(TEXT("Shower sample remains stable"), FMath::IsNearlyEqual(Weather->SampleRainIntensity(MaximumRainTime), MaximumRain));
	TestFalse(TEXT("Weather changes over time"), Weather->SampleWind(FVector::ZeroVector, 0).Equals(Weather->SampleWind(FVector::ZeroVector, 100)));
	TestFalse(TEXT("Currents vary across the Island"), Weather->SampleWind(FVector::ZeroVector, 0).Equals(Weather->SampleWind(FVector(1000, 1000, 0), 0)));
	const FVector2D CalmGains = AIslandWeather::CalculateAmbienceGains(0.f, 0.f);
	const FVector2D WindGains = AIslandWeather::CalculateAmbienceGains(120.f, 0.f);
	const FVector2D RainGains = AIslandWeather::CalculateAmbienceGains(0.f, 0.9f);
	const FVector2D StormGains = AIslandWeather::CalculateAmbienceGains(300.f, 1.f);
	TestTrue(TEXT("Dry calm weather stays silent"), CalmGains.IsNearlyZero());
	TestTrue(TEXT("Wind ambience grows with measured wind and has a quiet ceiling"), WindGains.X > 0.f && WindGains.X <= 0.055f && WindGains.Y == 0.f);
	TestTrue(TEXT("Rain ambience follows rain independently of wind"), RainGains.Y > 0.f && RainGains.Y <= 0.035f && RainGains.X == 0.f);
	TestTrue(TEXT("Combined storm ambience remains strictly bounded"), StormGains.X <= 0.055f && StormGains.Y <= 0.035f);

	FIslandTransientGust Gust;
	Gust.Center = FVector::ZeroVector;
	Gust.Direction = FVector::ForwardVector;
	Gust.PeakSpeed = 200.f;
	Gust.Radius = 1000.f;
	Gust.StartedAt = 10.0;
	Gust.ExpiresAt = 20.0;
	TestTrue(TEXT("Gust begins at full strength at its center"), AIslandWeather::EvaluateTransientGust(Gust, FVector::ZeroVector, 10.0).Equals(FVector(200.f, 0.f, 0.f)));
	TestTrue(TEXT("Gust fades with distance and time"), AIslandWeather::EvaluateTransientGust(Gust, FVector(500.f, 0.f, 0.f), 15.0).Equals(FVector(50.f, 0.f, 0.f)));
	TestTrue(TEXT("Gust has no effect beyond its radius"), AIslandWeather::EvaluateTransientGust(Gust, FVector(1001.f, 0.f, 0.f), 15.0).IsNearlyZero());
	TestTrue(TEXT("Gust expires cleanly"), AIslandWeather::EvaluateTransientGust(Gust, FVector::ZeroVector, 20.0).IsNearlyZero());

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Shelter-trace fixture world created"), World) || !TestNotNull(TEXT("Engine is available for shelter fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AIslandWeather* WorldWeather = World->SpawnActor<AIslandWeather>();
	if (!TestNotNull(TEXT("Weather actor spawned for shelter trace"), WorldWeather))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();
	// This minimal fixture doesn't initialize actor components through the full gameplay start path.
	WorldWeather->InitializeWeatherAmbience();
	constexpr int32 OneSecondStereoPcmBytes = 24000 * 2 * sizeof(int16);
	TestEqual(TEXT("Wind ambience prepares exactly one second of procedural stereo PCM"), WorldWeather->WindAmbienceWave ? WorldWeather->WindAmbienceWave->GetAvailableAudioByteCount() : 0, OneSecondStereoPcmBytes);
	TestEqual(TEXT("Rain ambience prepares exactly one second of procedural stereo PCM"), WorldWeather->RainAmbienceWave ? WorldWeather->RainAmbienceWave->GetAvailableAudioByteCount() : 0, OneSecondStereoPcmBytes);
	TestTrue(TEXT("Ambient components begin silent until a player listener is present"), WorldWeather->WindAmbienceAudio && WorldWeather->WindAmbienceAudio->VolumeMultiplier == 0.f && WorldWeather->RainAmbienceAudio && WorldWeather->RainAmbienceAudio->VolumeMultiplier == 0.f);
	const FVector ShelterPoint(10000.f, 10000.f, 1000.f);
	const FVector AmbientWind = WorldWeather->SampleWind(ShelterPoint, World->GetTimeSeconds());
	TestTrue(TEXT("Shelter fixture has enough ambient wind to assess"), AmbientWind.Size() >= 35.f);
	TestTrue(TEXT("Open point is reported as exposed to current wind"), WorldWeather->DescribeWindShelterAt(ShelterPoint).Contains(TEXT("exposed to the present horizontal wind")));
	AActor* WindBlocker = World->SpawnActor<AActor>();
	UBoxComponent* BlockerBox = NewObject<UBoxComponent>(WindBlocker);
	WindBlocker->SetRootComponent(BlockerBox);
	BlockerBox->SetBoxExtent(FVector(120.f, 120.f, 150.f));
	BlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
	BlockerBox->RegisterComponent();
	WindBlocker->SetActorLocation(ShelterPoint - AmbientWind.GetSafeNormal() * 300.f);
	const FString ShelteredReport = WorldWeather->DescribeWindShelterAt(ShelterPoint);
	TestTrue(TEXT("Solid upwind obstruction is reported as wind shelter"), ShelteredReport.Contains(TEXT("blocks the upwind visibility trace")));
	TestTrue(TEXT("Sheltered description does not claim roof cover or safe support"), ShelteredReport.Contains(TEXT("does not establish overhead rain cover or safe perch support")));
	TestTrue(TEXT("Same upwind obstruction attenuates the actual local-wind signal"), WorldWeather->GetLocalWind(ShelterPoint).Size() < AmbientWind.Size() * 0.2f);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
