#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "IslandWeather.h"
#include "IslandPoolRippleEffect.h"
#include "IslandLightning.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "DynamicWindData.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundWaveProcedural.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherTest, "CaptiveSky2.Agent.IslandWeather",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherTest::RunTest(const FString& Parameters)
{
	const AIslandWeather* Weather = GetDefault<AIslandWeather>();
	TestTrue(TEXT("The player-controlled character retains local plant response even if its mesh is hidden"),
		AIslandWeather::ShouldIncludeResidentInSwayFocus(true, false));
	TestTrue(TEXT("A recently rendered resident retains local plant response"),
		AIslandWeather::ShouldIncludeResidentInSwayFocus(false, true));
	TestFalse(TEXT("An offscreen non-player resident does not create an invisible plant-sway neighborhood"),
		AIslandWeather::ShouldIncludeResidentInSwayFocus(false, false));
	TestTrue(TEXT("Meadow habitat retains lush-community coverage"),
		FMath::IsNearlyEqual(AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(0.f, 0.f), 0.64f));
	TestTrue(TEXT("Meadow habitat keeps a light baseline in open communities"),
		FMath::IsNearlyEqual(AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(1.f, 1.f), 0.40f));
	TestTrue(TEXT("Invalid meadow fields resolve to a bounded neutral density"),
		FMath::IsFinite(AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(
			std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity())));
	const FVector TideglassWorldCenter(-99500.f, 100300.f, 2890.f);
	const FVector TideglassWindArch(-101650.f, 100200.f, 2950.f);
	const FVector TideglassListeningStones(-101650.f, 101600.f, 2650.f);
	const FVector TideglassInn(-100085.85f, 103179.61f, 2784.24f);
	const FVector TideglassInnDoorLantern(-100232.05f, 103623.63f, 2929.24f);
	const FVector DistantLandmark(-106000.f, 100300.f, 0.f);
	TestEqual(TEXT("The Tideglass source mesh clearance still protects the waterline"),
		AIslandWeather::GetTideglassPollinatorFlowerLandmarkClearance(
			TideglassWorldCenter, TideglassWorldCenter, 306.f, 2600.f), 650.f);
	for (const FVector& NearbyLandmark : {TideglassWindArch, TideglassListeningStones, TideglassInn, TideglassInnDoorLantern})
	{
		const float Clearance = AIslandWeather::GetTideglassPollinatorFlowerLandmarkClearance(
			NearbyLandmark, TideglassWorldCenter, 306.f, 2600.f);
		TestEqual(TEXT("The small Tideglass flower pocket uses a defined nearby-landmark buffer"),
			Clearance, AIslandWeather::TideglassPollinatorFlowerNearbyLandmarkClearance);
		TestTrue(TEXT("The nearest point of the Tideglass ring remains outside that landmark buffer"),
			FVector::Dist2D(NearbyLandmark, TideglassWorldCenter) - AIslandWeather::TideglassPollinatorFlowerOuterRadius > Clearance);
	}
	TestEqual(TEXT("Distant landmarks retain the full meadow flower clearance"),
		AIslandWeather::GetTideglassPollinatorFlowerLandmarkClearance(
			DistantLandmark, TideglassWorldCenter, 306.f, 2600.f), 2600.f);
	for (const int32 Seed : {-17, 0, 42, 71, 2026})
	{
		for (int32 SiteIndex = 0; SiteIndex < AIslandWeather::TideglassPollinatorFlowerSiteCount; ++SiteIndex)
		{
			const FVector Candidate = TideglassWorldCenter +
				AIslandWeather::SelectTideglassPollinatorFlowerSiteOffset(Seed, SiteIndex);
			for (const FVector& NearbyLandmark : {TideglassWindArch, TideglassListeningStones, TideglassInn, TideglassInnDoorLantern})
			{
				const float Clearance = AIslandWeather::GetTideglassPollinatorFlowerLandmarkClearance(
					NearbyLandmark, TideglassWorldCenter, 306.f, 2600.f);
				TestTrue(TEXT("All deterministic pool-edge flower anchors clear nearby landmarks"),
					FVector::Dist2D(Candidate, NearbyLandmark) >= Clearance);
			}
		}
	}
	float AverageMeadowAcceptance = 0.f;
	constexpr int32 MeadowFieldSamples = 100;
	for (int32 HabitatIndex = 0; HabitatIndex <= MeadowFieldSamples; ++HabitatIndex)
		for (int32 CommunityIndex = 0; CommunityIndex <= MeadowFieldSamples; ++CommunityIndex)
			AverageMeadowAcceptance += AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(
				static_cast<float>(HabitatIndex) / MeadowFieldSamples,
				static_cast<float>(CommunityIndex) / MeadowFieldSamples);
	AverageMeadowAcceptance /= FMath::Square(MeadowFieldSamples + 1);
	TestTrue(TEXT("Soft habitat gradients preserve the former approximate 46% candidate acceptance"),
		FMath::IsNearlyEqual(AverageMeadowAcceptance, 0.46f, 0.001f));
	float MinimumRain = 2.f;
	float MaximumRain = -1.f;
	double MinimumRainTime = 0.0;
	double MaximumRainTime = 0.0;
	for (int32 I = 0; I < 1000; ++I)
	{
		const FVector Position(I * 50.0, -I * 30.0, 700.0);
		const FVector Wind = Weather->SampleWind(Position, I);
		TestTrue(TEXT("Wind remains finite and bounded, even in a storm"), !Wind.ContainsNaN() && Wind.Size() <= Weather->MaximumWindSpeed * (1.f + AIslandWeather::StormWindBoost) + 0.01f);
		const float Cloud = Weather->SampleCloudCover(I);
		TestTrue(TEXT("Cloud cover is normalized"), Cloud >= 0.f && Cloud <= 1.f);
		TestTrue(TEXT("Sampling is repeatable"), Wind.Equals(Weather->SampleWind(Position, I)));
		const float Rain = Weather->SampleRainIntensity(I);
		TestTrue(TEXT("Rain intensity is normalized"), Rain >= 0.f && Rain <= 1.f);
		TestTrue(TEXT("Rain intensity is repeatable"), FMath::IsNearlyEqual(Rain, Weather->SampleRainIntensity(I)));
	}
	// Long enough to cross several multi-day spells.
	float DriestSpell = 2.f, WettestSpell = -1.f, StrongestStorm = 0.f;
	double StormTime = 0.0;
	int32 StormSamples = 0, Samples = 0;
	for (int32 Seconds = 0; Seconds <= 120000; Seconds += 15)
	{
		const float Rain = Weather->SampleRainIntensity(Seconds);
		if (Rain < MinimumRain) { MinimumRain = Rain; MinimumRainTime = Seconds; }
		if (Rain > MaximumRain) { MaximumRain = Rain; MaximumRainTime = Seconds; }
		const float Spell = Weather->SampleSpell(Seconds);
		DriestSpell = FMath::Min(DriestSpell, Spell);
		WettestSpell = FMath::Max(WettestSpell, Spell);
		const float Storm = Weather->SampleStormIntensity(Seconds);
		TestTrue(TEXT("Storm intensity is normalized"), Storm >= 0.f && Storm <= 1.f);
		if (Storm > StrongestStorm) { StrongestStorm = Storm; StormTime = Seconds; }
		if (Storm > 0.35f) ++StormSamples;
		++Samples;
	}
	TestTrue(TEXT("The weather runs in both dry and wet spells"), DriestSpell < 0.25f && WettestSpell > 0.75f);
	TestTrue(TEXT("Full storms happen"), StrongestStorm > 0.8f);
	TestTrue(TEXT("Storms are rare"), StormSamples > 0 && StormSamples < Samples / 12);
	TestTrue(TEXT("A storm always brings heavy rain"), Weather->SampleRainIntensity(StormTime) >= StrongestStorm - 0.001f);
	TestTrue(TEXT("Storms only come in wet spells"), Weather->SampleSpell(StormTime) > 0.74f);
	TestTrue(TEXT("A storm's wind is stronger than the same sky without it"),
		Weather->SampleWind(FVector::ZeroVector, StormTime).Size() > Weather->MaximumWindSpeed * 0.25f);

	// Weather carries on across sessions: an offset continues the same timeline.
	AIslandWeather* Continuing = NewObject<AIslandWeather>();
	Continuing->WeatherTimeOffset = 5000.0;
	TestTrue(TEXT("Saved weather time continues the same weather"),
		FMath::IsNearlyEqual(Continuing->SampleRainIntensity(120.0), Weather->SampleRainIntensity(5120.0)) &&
		FMath::IsNearlyEqual(Continuing->SampleCloudCover(120.0), Weather->SampleCloudCover(5120.0)) &&
		Continuing->SampleWind(FVector(100, 200, 0), 120.0).Equals(Weather->SampleWind(FVector(100, 200, 0), 5120.0), 0.01f));
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
	const FVector2D IndoorGains = AIslandWeather::CalculateAmbienceGains(120.f, 0.9f, true);
	const FVector2D OutdoorGains = AIslandWeather::CalculateAmbienceGains(120.f, 0.9f);
	const float RisingGain = AIslandWeather::InterpolateAmbienceGain(0.f, 0.055f, 0.1f);
	const float FallingGain = AIslandWeather::InterpolateAmbienceGain(0.055f, 0.f, 0.1f);
	float SettledGain = RisingGain;
	for (int32 Step = 0; Step < 19; ++Step)
		SettledGain = AIslandWeather::InterpolateAmbienceGain(SettledGain, 0.055f, 0.1f);
	TestTrue(TEXT("Dry calm weather stays silent"), CalmGains.IsNearlyZero());
	TestTrue(TEXT("Wind ambience grows with measured wind and has a quiet ceiling"), WindGains.X > 0.f && WindGains.X <= 0.055f && WindGains.Y == 0.f);
	TestTrue(TEXT("Rain ambience follows rain independently of wind"), RainGains.Y > 0.f && RainGains.Y <= 0.035f && RainGains.X == 0.f);
	TestTrue(TEXT("Combined storm ambience remains strictly bounded"), StormGains.X <= 0.055f && StormGains.Y <= 0.035f);
	TestTrue(TEXT("A verified indoor listener hears both weather beds at one fifth their outdoor level"),
		FMath::IsNearlyEqual(IndoorGains.X, OutdoorGains.X * 0.2f) && FMath::IsNearlyEqual(IndoorGains.Y, OutdoorGains.Y * 0.2f));
	TestTrue(TEXT("Ambience fades toward a changing target without a 4 Hz gain step"), RisingGain > 0.f && RisingGain < 0.055f && FallingGain > 0.f && FallingGain < 0.055f);
	TestTrue(TEXT("Repeated ambience updates settle close to their target"), SettledGain > RisingGain && SettledGain < 0.055f && FMath::IsNearlyEqual(SettledGain, 0.055f, 0.00002f));
	TestEqual(TEXT("Ambience gain interpolation does not overshoot a target"),
		AIslandWeather::InterpolateAmbienceGain(0.f, 0.055f, 10.f), 0.055f);
	TestEqual(TEXT("Negative frame deltas cannot change ambience gain"),
		AIslandWeather::InterpolateAmbienceGain(0.02f, 0.055f, -1.f), 0.02f);
	TestEqual(TEXT("Calm air creates no wind-driven pool ripple"), AIslandPoolRippleEffect::WindRippleActivity(0.f), 0.f);
	TestTrue(TEXT("A strong breeze gives Tideglass a measurable but bounded ripple"),
		AIslandPoolRippleEffect::WindRippleActivity(90.f) > 0.f && AIslandPoolRippleEffect::WindRippleActivity(300.f) == 1.f);
	TestEqual(TEXT("Invalid wind cannot create a water response"),
		AIslandPoolRippleEffect::WindRippleActivity(std::numeric_limits<float>::quiet_NaN()), 0.f);

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
	if (!WorldWeather->HasActorBegunPlay()) WorldWeather->DispatchBeginPlay();
	WorldWeather->InitializeGroundCover();
	const TArray<UInstancedSkinnedMeshComponent*> MixedTreeComponents = {
		WorldWeather->IslandNorwayMaples, WorldWeather->IslandEuropeanAspens, WorldWeather->IslandEuropeanBeeches };
	const TCHAR* MixedTreeNames[] = { TEXT("Norway maple"), TEXT("European aspen"), TEXT("European beech") };
	for (int32 TreeIndex = 0; TreeIndex < MixedTreeComponents.Num(); ++TreeIndex)
	{
		const UInstancedSkinnedMeshComponent* TreeComponent = MixedTreeComponents[TreeIndex];
		const FString Label = FString::Printf(TEXT("%s foliage batch"), MixedTreeNames[TreeIndex]);
		TestNotNull(*FString::Printf(TEXT("%s is created"), *Label), TreeComponent);
		if (!TreeComponent) continue;
		TestNotNull(*FString::Printf(TEXT("%s resolves its Megaplant mesh"), *Label), TreeComponent->GetSkinnedAsset());
		TestTrue(*FString::Printf(TEXT("%s uses the UE DynamicWind transform provider"), *Label),
			TreeComponent->GetTransformProvider() && TreeComponent->GetTransformProvider()->IsA<UDynamicWindData>());
		TestEqual(*FString::Printf(TEXT("%s is non-colliding"), *Label), TreeComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(*FString::Printf(TEXT("%s does not affect navigation"), *Label), TreeComponent->CanEverAffectNavigation());
		int32 CullStart = 0;
		int32 CullEnd = 0;
		TreeComponent->GetCullDistances(CullStart, CullEnd);
		TestEqual(*FString::Printf(TEXT("%s starts fading at 80 m"), *Label), CullStart, 8000);
		TestEqual(*FString::Printf(TEXT("%s is culled at 125 m"), *Label), CullEnd, 12500);
	}
	const UHierarchicalInstancedStaticMeshComponent* BirchCoppices = WorldWeather->IslandBirchCoppices;
	TestNotNull(TEXT("A separate birch-coppice understorey batch is created"), BirchCoppices);
	if (BirchCoppices)
	{
		TestTrue(TEXT("Birch coppice resolves the generated low-poly mesh"), BirchCoppices->GetStaticMesh() &&
			BirchCoppices->GetStaticMesh()->GetPathName().Contains(TEXT("SM_BirchCoppice_01")));
		TestEqual(TEXT("Birch coppice is non-colliding"), BirchCoppices->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Birch coppice does not affect navigation"), BirchCoppices->CanEverAffectNavigation());
		int32 BirchCullStart = 0;
		int32 BirchCullEnd = 0;
		BirchCoppices->GetCullDistances(BirchCullStart, BirchCullEnd);
		TestEqual(TEXT("Birch coppice starts fading at 80 m"), BirchCullStart, 8000);
		TestEqual(TEXT("Birch coppice is culled at 125 m"), BirchCullEnd, 12500);
	}
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
	AActor* Pool = World->SpawnActor<AActor>(FVector(-10000.f, -10000.f, 1000.f), FRotator::ZeroRotator);
	Pool->Tags.Add(TEXT("TideglassPool"));
	FIslandTransientGust PoolGust;
	PoolGust.Center = Pool->GetActorLocation();
	PoolGust.Direction = FVector::ForwardVector;
	PoolGust.PeakSpeed = 120.f;
	PoolGust.Radius = 800.f;
	PoolGust.StartedAt = World->GetTimeSeconds();
	PoolGust.ExpiresAt = PoolGust.StartedAt + 20.0;
	WorldWeather->TransientGusts.Add(PoolGust);
	WorldWeather->CurrentRainIntensity = 0.f;
	WorldWeather->UpdateWindPoolResponse();
	TestTrue(TEXT("A measured dry-weather local gust creates a Tideglass wind ripple"), WorldWeather->WindPoolRipple.IsValid());
	int32 WindRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
		if (It->ActorHasTag(TEXT("WindImpact")))
		{
			++WindRippleCount;
			TestTrue(TEXT("Ambient wind ripple glints stay soft"), It->PeakLightIntensity <= 1.35f);
			TestTrue(TEXT("Wind ripple lifetime is bounded"), It->DurationSeconds > 0.f && It->DurationSeconds <= 2.2f);
		}
	TestEqual(TEXT("One local gust creates one finite pool response"), WindRippleCount, 1);
	if (AIslandPoolRippleEffect* RainRippleProbe = World->SpawnActor<AIslandPoolRippleEffect>(Pool->GetActorLocation(), FRotator::ZeroRotator))
		RainRippleProbe->ConfigureAsRainImpact();
	ACharacter* Observer = World->SpawnActor<ACharacter>(Pool->GetActorLocation() + FVector(400.f, 0.f, 100.f), FRotator::ZeroRotator);
	UAgentBrainComponent* ObserverBrain = NewObject<UAgentBrainComponent>(Observer);
	Observer->AddInstanceComponent(ObserverBrain);
	ObserverBrain->RegisterComponent();
	const FString VisibleRippleObservation = ObserverBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A nearby unobstructed resident notices the transient wind-made pool ripple"),
		VisibleRippleObservation.Contains(TEXT("stirred by the local wind")) && VisibleRippleObservation.Contains(TEXT("not a discovery")));
	TestTrue(TEXT("A nearby resident also notices faint weather-made rain rings"),
		VisibleRippleObservation.Contains(TEXT("Faint rain rings")));
	AActor* RippleOccluder = World->SpawnActor<AActor>(Pool->GetActorLocation() + FVector(220.f, 0.f, 50.f), FRotator::ZeroRotator);
	UBoxComponent* RippleOccluderBox = NewObject<UBoxComponent>(RippleOccluder);
	RippleOccluder->SetRootComponent(RippleOccluderBox);
	RippleOccluderBox->SetBoxExtent(FVector(80.f, 80.f, 80.f));
	RippleOccluderBox->SetCollisionProfileName(TEXT("BlockAll"));
	RippleOccluderBox->RegisterComponent();
	const FString OccludedRippleObservation = ObserverBrain->BuildSituationSummary(FAgentConversationContext());
	TestFalse(TEXT("A resident does not claim to see a ripple hidden behind solid geometry"),
		OccludedRippleObservation.Contains(TEXT("stirred by the local wind")));
	RippleOccluder->Destroy();
	Observer->SetActorLocation(Pool->GetActorLocation() + FVector(2200.f, 0.f, 100.f));
	const FString DistantRippleObservation = ObserverBrain->BuildSituationSummary(FAgentConversationContext());
	TestFalse(TEXT("A resident outside the short weather-effect radius does not notice the ripple"),
		DistantRippleObservation.Contains(TEXT("stirred by the local wind")));
	WorldWeather->UpdateWindPoolResponse();
	WindRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) if (It->ActorHasTag(TEXT("WindImpact"))) ++WindRippleCount;
	TestEqual(TEXT("Repeated weather ticks do not stack wind ripples"), WindRippleCount, 1);
	WorldWeather->CurrentRainIntensity = 0.8f;
	WorldWeather->WindPoolRipple->Destroy();
	WorldWeather->WindPoolRipple.Reset();
	WorldWeather->NextWindPoolRippleTime = 0.0;
	WorldWeather->UpdateWindPoolResponse();
	TestFalse(TEXT("Strong rain remains the pool's water response instead of stacking wind ripples"), WorldWeather->WindPoolRipple.IsValid());

	// Storms bring lightning: flashes, a bolt when near, and thunder after the time sound takes to arrive.
	TestTrue(TEXT("Lightning flashes in quick pulses and then goes dark"), AIslandLightning::FlashAt(0.15f) > 0.8f && AIslandLightning::FlashAt(1.f) == 0.f);
	const double StormStart = World->GetTimeSeconds();
	WorldWeather->UpdateStorm(StormStart);
	TestEqual(TEXT("Calm weather brings no lightning"), WorldWeather->StrikeCount, 0);
	WorldWeather->ForcedStormUntil = StormStart + 300.0;
	TestEqual(TEXT("A forced storm is a full storm"), WorldWeather->SampleStormIntensity(StormStart), 1.f);
	for (double Now = StormStart; Now < StormStart + 120.0; Now += 0.25) WorldWeather->UpdateStorm(Now);
	TestTrue(TEXT("A full storm strikes several times in two minutes, but not constantly"), WorldWeather->StrikeCount >= 3 && WorldWeather->StrikeCount <= 25);
	AIslandLightning* Strike = WorldWeather->LastStrike.Get();
	if (TestNotNull(TEXT("A strike leaves a lightning effect"), Strike))
	{
		const float Distance = FVector::Dist2D(WorldWeather->LastStrikeGround, WorldWeather->GetActorLocation());
		TestTrue(TEXT("Strikes land away from the viewer"), Distance >= 30000.f && Distance <= 720000.f);
		TestTrue(TEXT("Thunder waits for the sound to travel"), FMath::IsNearlyEqual(Strike->GetThunderDelay(), Distance / AIslandLightning::SoundSpeed, 0.01f));
		TestEqual(TEXT("A bolt is drawn only for strikes near enough to see"), Strike->HasBolt(), Distance <= AIslandLightning::BoltVisibleWithin);
		Strike->Tick(0.15f);
		TestTrue(TEXT("The flash is published while it lasts"), WorldWeather->GetLightningFlash() > 0.5f);
		TestFalse(TEXT("Thunder has not arrived during the flash"), Strike->HasThundered());
		Strike->Tick(Strike->GetThunderDelay());
		TestTrue(TEXT("Thunder follows"), Strike->HasThundered());
	}
	WorldWeather->LastStrikeTime = World->GetTimeSeconds();
	TestTrue(TEXT("Residents hear about the lightning"), WorldWeather->DescribeAt(WorldWeather->GetActorLocation()).Contains(TEXT("Lightning flashed")));
	TestTrue(TEXT("Residents are told a storm is overhead"), WorldWeather->DescribeAt(WorldWeather->GetActorLocation()).Contains(TEXT("a storm")));
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandNearbyVegetationPerceptionTest,
	"CaptiveSky2.Agent.NearbyVegetationPerception",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandNearbyVegetationPerceptionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Transient plant-perception fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for plant-perception fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	ACharacter* Observer = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Weather actor spawned"), Weather) || !TestNotNull(TEXT("Observer spawned"), Observer) ||
		!TestNotNull(TEXT("Engine cube mesh loaded for one foliage instance"), Cube)) return false;

	UHierarchicalInstancedStaticMeshComponent* Foliage = nullptr;
	TArray<UHierarchicalInstancedStaticMeshComponent*> FoliageComponents;
	Weather->GetComponents<UHierarchicalInstancedStaticMeshComponent>(FoliageComponents);
	if (FoliageComponents.Num() > 0) Foliage = FoliageComponents[0];
	if (!TestNotNull(TEXT("Weather actor exposes a HISM foliage component"), Foliage)) return false;
	Foliage->SetStaticMesh(Cube);
	Foliage->SetVisibility(true);
	Foliage->SetHiddenInGame(false);
	Foliage->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Foliage->AddInstance(FTransform(FRotator::ZeroRotator, FVector(300.f, 0.f, 0.f)));
	UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Observer);
	Observer->AddInstanceComponent(Brain);
	Brain->RegisterComponent();

	const FString VisibleSituation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A nearby visible HISM plant is described for a possible upgrade request"),
		VisibleSituation.Contains(TEXT("patch of Cube vegetation")) && VisibleSituation.Contains(TEXT("request_upgrade")));
	TestTrue(TEXT("A plant description never creates a movement or interaction target"),
		VisibleSituation.Contains(TEXT("no movement or interaction target")) && !VisibleSituation.Contains(TEXT("move_to target: Cube")));

	UStaticMesh* FlowerMesh = DuplicateObject<UStaticMesh>(Cube, GetTransientPackage(), TEXT("flower_04_01"));
	TestNotNull(TEXT("A transient flower-named mesh can exercise the Island's asset-label rule"), FlowerMesh);
	Foliage->SetStaticMesh(FlowerMesh);
	const FString FlowerSituation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Numbered flower asset IDs are described as wildflowers, not exposed as technical names"),
		FlowerSituation.Contains(TEXT("patch of wildflowers")) && !FlowerSituation.Contains(TEXT("flower 04 01")));

	UStaticMesh* GroundCoverMesh = DuplicateObject<UStaticMesh>(Cube, GetTransientPackage(), TEXT("ground_05_01"));
	TestNotNull(TEXT("A transient ground-cover mesh can exercise the Island's asset-label rule"), GroundCoverMesh);
	Foliage->SetStaticMesh(GroundCoverMesh);
	const FString GroundCoverSituation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Numbered ground-cover asset IDs are described perceptually"),
		GroundCoverSituation.Contains(TEXT("patch of low groundcover")) && !GroundCoverSituation.Contains(TEXT("ground 05 01")));

	struct FInstalledPlantLabelCase
	{
		const TCHAR* AssetPath;
		const TCHAR* ExpectedDescription;
		const TCHAR* CaseName;
	};
	const FInstalledPlantLabelCase InstalledPlantLabels[] =
	{
		{ TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_04_01.flower_04_01"), TEXT("patch of wildflowers"), TEXT("numbered Fab flower") },
		{ TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_05_01.ground_05_01"), TEXT("patch of low groundcover"), TEXT("numbered Fab ground cover") },
		{ TEXT("/Game/PN_interactiveSpruceForest/Meshes/half/high/spruce_half_01.spruce_half_01"), TEXT("patch of spruce trees"), TEXT("interactive spruce") },
		{ TEXT("/Game/Plants/Meshes/Festuca_gautieri_LD.Festuca_gautieri_LD"), TEXT("patch of grasses"), TEXT("Festuca") },
		{ TEXT("/Game/Plants/Meshes/Phalaris_arundinacea_LD.Phalaris_arundinacea_LD"), TEXT("patch of grasses"), TEXT("Phalaris") }
	};
	for (const FInstalledPlantLabelCase& LabelCase : InstalledPlantLabels)
	{
		UStaticMesh* InstalledPlant = LoadObject<UStaticMesh>(nullptr, LabelCase.AssetPath);
		if (!InstalledPlant)
		{
			AddInfo(FString::Printf(TEXT("Optional %s asset is not installed; its resident-label check was skipped."), LabelCase.CaseName));
			continue;
		}
		Foliage->SetStaticMesh(InstalledPlant);
		const FString InstalledPlantSituation = Brain->BuildSituationSummary(FAgentConversationContext());
		TestTrue(FString::Printf(TEXT("Installed %s mesh gets a perceptual resident label"), LabelCase.CaseName),
			InstalledPlantSituation.Contains(LabelCase.ExpectedDescription) && !InstalledPlantSituation.Contains(InstalledPlant->GetName()));
	}
	Foliage->SetStaticMesh(Cube);

	AActor* Occluder = World->SpawnActor<AActor>(FVector(150.f, 0.f, 25.f), FRotator::ZeroRotator, Spawn);
	UBoxComponent* OccluderBox = NewObject<UBoxComponent>(Occluder);
	Occluder->SetRootComponent(OccluderBox);
	OccluderBox->SetBoxExtent(FVector(35.f, 100.f, 100.f));
	OccluderBox->SetCollisionProfileName(TEXT("BlockAll"));
	OccluderBox->RegisterComponent();
	const FString OccludedSituation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestFalse(TEXT("A plant hidden behind solid geometry is not described as visible"),
		OccludedSituation.Contains(TEXT("patch of Cube vegetation")));
	Occluder->Destroy();

	Observer->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	const FString DistantSituation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestFalse(TEXT("HISM vegetation beyond the short perception radius is omitted"),
		DistantSituation.Contains(TEXT("patch of Cube vegetation")));
	return true;
}
