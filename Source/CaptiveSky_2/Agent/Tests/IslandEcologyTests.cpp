#include "Misc/AutomationTest.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "IslandPoolRippleEffect.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "IslandWindMoteEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Components/VolumetricCloudComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWaveProcedural.h"

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
	const int32 OriginalWeatherSeed = Weather->WeatherSeed;
	float PeakRain = -1.f;
	float LowestRain = 2.f;
	int32 StormSeed = 0;
	int32 DryWeatherSeed = 0;
	for (int32 Seed = -1000; Seed <= 1000; ++Seed)
	{
		Weather->WeatherSeed = Seed;
		const float Rain = Weather->SampleRainIntensity(World->GetTimeSeconds());
		if (Rain > PeakRain) { PeakRain = Rain; StormSeed = Seed; }
		if (Rain < LowestRain) { LowestRain = Rain; DryWeatherSeed = Seed; }
	}
	TestTrue(TEXT("Weather seed space includes heavy rain and dry conditions"), PeakRain > 0.55f && LowestRain < 0.01f);
	Weather->WeatherSeed = StormSeed;
	Weather->UpdateRainRendering();
	AIslandPoolRippleEffect* RainRipple = nullptr;
	int32 RainRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("RainImpact"))) continue;
		RainRipple = *It;
		++RainRippleCount;
	}
	TestEqual(TEXT("A strong shower creates one Tideglass water impact"), RainRippleCount, 1);
	if (RainRipple)
	{
		TestTrue(TEXT("Rain water response is subtler than a deliberate pool interaction"), RainRipple->PeakLightIntensity < 55.f && RainRipple->SurfaceRadius < 150.f && RainRipple->DurationSeconds < 1.6f);
		Weather->UpdateRainRendering();
		RainRippleCount = 0;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) if (It->ActorHasTag(TEXT("RainImpact"))) ++RainRippleCount;
		TestEqual(TEXT("Repeated weather updates do not stack pool impacts"), RainRippleCount, 1);
	}
	Weather->WeatherSeed = DryWeatherSeed;
	Weather->UpdateRainRendering();
	RainRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) if (It->ActorHasTag(TEXT("RainImpact"))) ++RainRippleCount;
	TestEqual(TEXT("Dry conditions do not create additional water impacts"), RainRippleCount, 1);
	Weather->WeatherSeed = OriginalWeatherSeed;
	
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
		TestNotNull(TEXT("Firefly has its segmented body mesh"), It->FindComponentByClass<UStaticMeshComponent>());
		TArray<UStaticMeshComponent*> BodyParts;
		It->GetComponents<UStaticMeshComponent>(BodyParts);
		bool bHasLeftWing = false;
		bool bHasRightWing = false;
		for (const UStaticMeshComponent* Part : BodyParts)
		{
			if (!Part) continue;
			bHasLeftWing |= Part->GetFName() == FName(TEXT("LeftWing"));
			bHasRightWing |= Part->GetFName() == FName(TEXT("RightWing"));
		}
		TestTrue(TEXT("Firefly has separate left and right wing meshes"), bHasLeftWing && bHasRightWing);
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

	const FVector TestPoolLocation(4000.f, 5000.f, 600.f);
	ACharacter* Observer = World->SpawnActor<ACharacter>(TestPoolLocation, FRotator::ZeroRotator, Spawn);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>(Spawn);
	ATargetPoint* PoolTarget = World->SpawnActor<ATargetPoint>(TestPoolLocation, FRotator::ZeroRotator, Spawn);
	ATargetPoint* WindTarget = World->SpawnActor<ATargetPoint>(TestPoolLocation, FRotator::ZeroRotator, Spawn);
	ATargetPoint* StonesTarget = World->SpawnActor<ATargetPoint>(TestPoolLocation, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Interaction observer spawned"), Observer) || !TestNotNull(TEXT("Interaction controller spawned"), Controller) || !TestNotNull(TEXT("Tideglass target spawned"), PoolTarget) || !TestNotNull(TEXT("WindArch target spawned"), WindTarget) || !TestNotNull(TEXT("ListeningStones target spawned"), StonesTarget))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Controller->Possess(Observer);
	PoolTarget->Tags = {TEXT("TideglassPool"), TEXT("IslandLandmark")};
	Controller->InspectTarget(TEXT("TideglassPool"));
	AIslandPoolRippleEffect* Ripple = nullptr;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) { if (!It->ActorHasTag(TEXT("RainImpact"))) { Ripple = *It; break; } }
	TestNotNull(TEXT("Interacting with Tideglass spawns a transient optical ripple"), Ripple);
	if (Ripple)
	{
		TestEqual(TEXT("Ripple uses eight overlapping light points"), Ripple->RippleLights.Num(), 8);
		Ripple->Tick(0.8f);
		TestTrue(TEXT("Ripple light ring expands across the pool midway through its life"), FMath::IsNearlyEqual(Ripple->RippleLights[0]->GetRelativeLocation().Size2D(), 81.f, 0.5f));
		Controller->InspectTarget(TEXT("TideglassPool"));
		int32 RippleCount = 0;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) if (!It->ActorHasTag(TEXT("RainImpact"))) ++RippleCount;
		TestEqual(TEXT("Inspection cooldown prevents stacking ripples"), RippleCount, 1);
		Ripple->Tick(0.9f);
		TestTrue(TEXT("Ripple destroys itself after fading"), Ripple->IsActorBeingDestroyed());
	}
	WindTarget->Tags = {TEXT("WindArch"), TEXT("IslandLandmark")};
	Controller->InspectTarget(TEXT("WindArch"));
	AIslandWindMoteEffect* Motes = nullptr;
	for (TActorIterator<AIslandWindMoteEffect> It(World); It; ++It) { Motes = *It; break; }
	TestNotNull(TEXT("Interacting with WindArch creates a transient visible airflow cue"), Motes);
	if (Motes)
	{
		TestEqual(TEXT("WindArch airflow cue uses three non-shadowing motes"), Motes->MoteLights.Num(), 3);
		TestTrue(TEXT("Wind motes follow the normalized simulated gust direction"), Motes->FlowDirection.IsNormalized());
		const FVector StartingPosition = Motes->MoteMeshes[0]->GetRelativeLocation();
		Motes->Tick(2.f);
		const FVector Displacement = Motes->MoteMeshes[0]->GetRelativeLocation() - StartingPosition;
		TestTrue(TEXT("Airflow motes advance along the actual gust direction"), FVector::DotProduct(Displacement, Motes->FlowDirection) > 0.f);
		Motes->Tick(17.f);
		TestTrue(TEXT("WindArch visual response disappears when the gust expires"), Motes->IsActorBeingDestroyed());
	}
	StonesTarget->Tags = {TEXT("ListeningStones"), TEXT("IslandLandmark")};
	Controller->InspectTarget(TEXT("ListeningStones"));
	AIslandListeningStonesChime* Chime = nullptr;
	for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) { Chime = *It; break; }
	TestNotNull(TEXT("ListeningStones interaction creates a transient chime actor"), Chime);
	if (Chime)
	{
		TestNotNull(TEXT("Chime uses a procedural sound wave without external assets"), Chime->ChimeWave.Get());
		if (Chime->ChimeWave)
		{
			TestEqual(TEXT("Chime uses mono audio"), Chime->ChimeWave->NumChannels, 1);
			TestTrue(TEXT("Chime declares a finite playback duration"), FMath::IsNearlyEqual(Chime->ChimeWave->GetDuration(), 2.8f));
			TestTrue(TEXT("Chime queues a finite PCM signal"), Chime->ChimeWave->GetAvailableAudioByteCount() >= 24000 * 2);
		}
		TestTrue(TEXT("Chime is spatially attenuated around the landmark"), Chime->AudioComponent->bOverrideAttenuation && Chime->AudioComponent->AttenuationOverrides.bSpatialize);
		Controller->InspectTarget(TEXT("ListeningStones"));
		int32 ChimeCount = 0;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) ++ChimeCount;
		TestEqual(TEXT("Inspection cooldown prevents stacking chimes"), ChimeCount, 1);
		Chime->Tick(2.9f);
		TestTrue(TEXT("Generated sound actor cleans itself up after playback"), Chime->IsActorBeingDestroyed());
	}
	AIslandFirefly* WatchableFirefly = World->SpawnActor<AIslandFirefly>(TestPoolLocation, FRotator::ZeroRotator, Spawn);
	TestNotNull(TEXT("Nearby wild firefly spawned for observation interaction"), WatchableFirefly);
	if (WatchableFirefly)
	{
		Controller->InspectTarget(TEXT("Firefly"));
		TestTrue(TEXT("Quiet observation triggers only a brief glow accent"), WatchableFirefly->ObservationPulseRemaining > 0.f && WatchableFirefly->ObservationPulseRemaining <= 3.f);
		WatchableFirefly->Tick(1.f);
		TestTrue(TEXT("Firefly returns naturally toward its usual pulse"), WatchableFirefly->ObservationPulseRemaining > 0.f && WatchableFirefly->ObservationPulseRemaining < 2.1f);
		WatchableFirefly->Tick(2.f);
		TestTrue(TEXT("Observation accent expires without persistent state"), FMath::IsNearlyZero(WatchableFirefly->ObservationPulseRemaining));
	}
	Controller->UnPossess();
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
		bool bHasFlattenedTideglassSurface = false;
		bool bHasVolumetricCloud = false;
		bool bHasCloudCoverageParameter = false;
		bool bHasCloudDensityParameter = false;
		bool bHasStormCloudsParameter = false;
		AActor* HabitatMarker = nullptr;
		AIslandWeather* MapWeather = nullptr;
		UVolumetricCloudComponent* MapCloud = nullptr;
		UMaterialInterface* AuthoredCloudMaterial = nullptr;
		for (TActorIterator<AIslandWeather> It(Island); It; ++It) bHasWeather = true;
		for (TActorIterator<AIslandDayNight> It(Island); It; ++It) bHasDayNight = true;
		for (TActorIterator<AActor> It(Island); It; ++It)
		{
			if (It->ActorHasTag(TEXT("TideglassPool"))) { bHasTideglassHabitat = true; HabitatMarker = *It; }
			if (AIslandWeather* SavedMapWeather = Cast<AIslandWeather>(*It)) MapWeather = SavedMapWeather;
			if (UVolumetricCloudComponent* Cloud = It->FindComponentByClass<UVolumetricCloudComponent>())
			{
				bHasVolumetricCloud = true;
				if (UMaterialInterface* Material = Cloud->GetMaterial())
				{
					MapCloud = Cloud;
					AuthoredCloudMaterial = Material;
					TArray<FMaterialParameterInfo> ScalarParameters;
					TArray<FGuid> ParameterIds;
					Material->GetAllScalarParameterInfo(ScalarParameters, ParameterIds);
					bHasCloudCoverageParameter = ScalarParameters.ContainsByPredicate([](const FMaterialParameterInfo& Parameter) { return Parameter.Name == FName(TEXT("Cloud_GlobalCoverage")); });
					bHasCloudDensityParameter = ScalarParameters.ContainsByPredicate([](const FMaterialParameterInfo& Parameter) { return Parameter.Name == FName(TEXT("Cloud_GlobalDensity")); });
					bHasStormCloudsParameter = ScalarParameters.ContainsByPredicate([](const FMaterialParameterInfo& Parameter) { return Parameter.Name == FName(TEXT("StormClouds")); });
				}
			}
		}
		TestTrue(TEXT("Saved Island contains the weather actor required by the ecology spawner"), bHasWeather);
		TestTrue(TEXT("Saved Island contains the day/night clock required by the ecology spawner"), bHasDayNight);
		TestTrue(TEXT("Saved Island contains the TideglassPool habitat tag"), bHasTideglassHabitat);
		TestTrue(TEXT("Saved Island contains its authored volumetric cloud layer"), bHasVolumetricCloud);
		TestTrue(TEXT("Saved Island cloud material exposes coverage and density controls"), bHasCloudCoverageParameter && bHasCloudDensityParameter);
		TestTrue(TEXT("Saved Island cloud material exposes its storm-cloud control"), bHasStormCloudsParameter);
		if (MapWeather && MapCloud && AuthoredCloudMaterial && bHasCloudCoverageParameter && bHasCloudDensityParameter && bHasStormCloudsParameter)
		{
			const int32 SavedWeatherSeed = MapWeather->WeatherSeed;
			float MinimumCover = 2.f;
			float MaximumCover = -1.f;
			int32 ClearSeed = 0;
			int32 OvercastSeed = 0;
			float MinimumRain = 2.f;
			float MaximumRain = -1.f;
			int32 DrySeed = 0;
			int32 RainySeed = 0;
			for (int32 Seed = -1000; Seed <= 1000; ++Seed)
			{
				MapWeather->WeatherSeed = Seed;
				const float Cover = MapWeather->SampleCloudCover(Island->GetTimeSeconds());
				const float Rain = MapWeather->SampleRainIntensity(Island->GetTimeSeconds());
				if (Cover < MinimumCover) { MinimumCover = Cover; ClearSeed = Seed; }
				if (Cover > MaximumCover) { MaximumCover = Cover; OvercastSeed = Seed; }
				if (Rain < MinimumRain) { MinimumRain = Rain; DrySeed = Seed; }
				if (Rain > MaximumRain) { MaximumRain = Rain; RainySeed = Seed; }
			}
			TestTrue(TEXT("Saved weather seed space includes both dry and rainy cloud states"), MinimumRain < 0.01f && MaximumRain > 0.25f);
			MapWeather->WeatherSeed = ClearSeed;
			MapWeather->UpdateCloudRendering();
			UMaterialInstanceDynamic* WeatherCloudMID = Cast<UMaterialInstanceDynamic>(MapCloud->GetMaterial());
			TestNotNull(TEXT("Weather creates a transient dynamic instance of the authored cloud material"), WeatherCloudMID);
			if (WeatherCloudMID)
			{
				const float ClearCoverage = WeatherCloudMID->K2_GetScalarParameterValue(TEXT("Cloud_GlobalCoverage"));
				const float ClearDensity = WeatherCloudMID->K2_GetScalarParameterValue(TEXT("Cloud_GlobalDensity"));
				MapWeather->WeatherSeed = DrySeed;
				MapWeather->UpdateCloudRendering();
				const float DryStorm = WeatherCloudMID->K2_GetScalarParameterValue(TEXT("StormClouds"));
				MapWeather->WeatherSeed = RainySeed;
				MapWeather->UpdateCloudRendering();
				TestTrue(TEXT("Passing rain front activates the authored storm-cloud material"), WeatherCloudMID->K2_GetScalarParameterValue(TEXT("StormClouds")) > DryStorm);
				MapWeather->UpdateRainRendering();
				TestNotNull(TEXT("Rain uses one instanced component rather than per-drop actors"), MapWeather->RainStreaks.Get());
				if (MapWeather->RainStreaks)
				{
					UMaterialInterface* RainMaterial = MapWeather->RainStreaks->GetMaterial(0);
					TestTrue(TEXT("Rain uses a translucent or additive material"), RainMaterial && (RainMaterial->GetBlendMode() == BLEND_Translucent || RainMaterial->GetBlendMode() == BLEND_Additive));
					TestEqual(TEXT("Rain visualization pool stays within its configured bound"), MapWeather->RainStreaks->GetInstanceCount(), FMath::Clamp(MapWeather->RainStreakCount, 16, 192));
					TestTrue(TEXT("Rain front makes a bounded set of streaks visible"), MapWeather->ActiveRainStreakCount > 0 && MapWeather->ActiveRainStreakCount <= MapWeather->RainStreakCount && MapWeather->RainStreaks->IsVisible());
					const int32 PoolCount = MapWeather->RainStreaks->GetInstanceCount();
					MapWeather->WeatherSeed = DrySeed;
					MapWeather->UpdateRainRendering();
					TestTrue(TEXT("Streak pool hides when the front passes"), MapWeather->ActiveRainStreakCount == 0 && !MapWeather->RainStreaks->IsVisible());
					TestEqual(TEXT("Weather reuses the same finite streak pool"), MapWeather->RainStreaks->GetInstanceCount(), PoolCount);
				}
				MapWeather->WeatherSeed = OvercastSeed;
				MapWeather->UpdateCloudRendering();
				TestTrue(TEXT("Overcast changes authored cloud coverage"), WeatherCloudMID->K2_GetScalarParameterValue(TEXT("Cloud_GlobalCoverage")) > ClearCoverage);
				TestTrue(TEXT("Overcast changes authored cloud density"), WeatherCloudMID->K2_GetScalarParameterValue(TEXT("Cloud_GlobalDensity")) > ClearDensity);
			}
			MapCloud->SetMaterial(AuthoredCloudMaterial);
			TestTrue(TEXT("Automation restores the authored cloud material after probing"), MapCloud->GetMaterial() == AuthoredCloudMaterial);
			MapWeather->CloudComponent.Reset();
			MapWeather->WeatherCloudMaterial = nullptr;
			MapWeather->OriginalCloudMaterial = nullptr;
			MapWeather->bHasCloudCoverageParameter = false;
			MapWeather->bHasCloudDensityParameter = false;
			MapWeather->bHasStormCloudsParameter = false;
			MapWeather->WeatherSeed = SavedWeatherSeed;
		}
		if (HabitatMarker)
		{
			for (TActorIterator<AActor> It(Island); It && !bHasFlattenedTideglassSurface; ++It)
			{
				if (FVector::Dist(It->GetActorLocation(), HabitatMarker->GetActorLocation()) > 25.f) continue;
				TArray<UStaticMeshComponent*> MeshComponents;
				It->GetComponents<UStaticMeshComponent>(MeshComponents);
				for (const UStaticMeshComponent* MeshComponent : MeshComponents)
				{
					if (!MeshComponent || !MeshComponent->GetStaticMesh()) continue;
					const FVector Scale = MeshComponent->GetComponentScale();
					if (MeshComponent->GetStaticMesh()->GetName() == TEXT("Sphere") && Scale.X > 2.f && Scale.Y > 2.f && Scale.Z < 0.25f)
					{
						bHasFlattenedTideglassSurface = true;
						break;
					}
				}
			}
		}
		TestTrue(TEXT("Saved Island TideglassPool landmark sits beside a flattened sphere prototype surface"), bHasFlattenedTideglassSurface);
		break;
	}
	TestTrue(TEXT("Editor automation opened the saved Island map"), bFoundIslandEditorWorld);
	return true;
}
