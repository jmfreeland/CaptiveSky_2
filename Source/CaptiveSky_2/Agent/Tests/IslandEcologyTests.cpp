#include "Misc/AutomationTest.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "IslandPoolRippleEffect.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "IslandWindMoteEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInterface.h"
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
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) { Ripple = *It; break; }
	TestNotNull(TEXT("Interacting with Tideglass spawns a transient optical ripple"), Ripple);
	if (Ripple)
	{
		TestEqual(TEXT("Ripple uses eight overlapping light points"), Ripple->RippleLights.Num(), 8);
		Ripple->Tick(0.8f);
		TestTrue(TEXT("Ripple light ring expands across the pool midway through its life"), FMath::IsNearlyEqual(Ripple->RippleLights[0]->GetRelativeLocation().Size2D(), 81.f, 0.5f));
		Controller->InspectTarget(TEXT("TideglassPool"));
		int32 RippleCount = 0;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) ++RippleCount;
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
		AActor* HabitatMarker = nullptr;
		for (TActorIterator<AIslandWeather> It(Island); It; ++It) bHasWeather = true;
		for (TActorIterator<AIslandDayNight> It(Island); It; ++It) bHasDayNight = true;
		for (TActorIterator<AActor> It(Island); It; ++It)
			if (It->ActorHasTag(TEXT("TideglassPool"))) { bHasTideglassHabitat = true; HabitatMarker = *It; }
		TestTrue(TEXT("Saved Island contains the weather actor required by the ecology spawner"), bHasWeather);
		TestTrue(TEXT("Saved Island contains the day/night clock required by the ecology spawner"), bHasDayNight);
		TestTrue(TEXT("Saved Island contains the TideglassPool habitat tag"), bHasTideglassHabitat);
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
