#include "Misc/AutomationTest.h"
#include "Agent/IslandWindArchPresentation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Agent/IslandWeather.h"
#include "Agent/IslandWindMoteEffect.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWindArchPresentationTest, "CaptiveSky2.Agent.WindArchPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWindArchPresentationTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Calm wind does not produce ambient Wind Arch motes"),
		AWindArchStonework::ShouldEmitNaturalWindMotes(104.9f, 0.f));
	TestTrue(TEXT("A stronger natural wind can produce an ambient mote pass"),
		AWindArchStonework::ShouldEmitNaturalWindMotes(105.f, 0.f));
	TestFalse(TEXT("The ambient effect remains rate-limited during its cooldown"),
		AWindArchStonework::ShouldEmitNaturalWindMotes(180.f, 0.1f));
	TestFalse(TEXT("An invalid negative wind sample cannot produce motes"),
		AWindArchStonework::ShouldEmitNaturalWindMotes(-1.f, 0.f));

	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Candidate = Context.World();
		if (Context.WorldType == EWorldType::Editor && Candidate && Candidate->GetMapName() == TEXT("Island"))
		{
			Island = Candidate;
			break;
		}
	}
	if (Island)
	{
		AActor* Marker = nullptr;
		TArray<AStaticMeshActor*> Pillars;
		AStaticMeshActor* Beam = nullptr;
		TestTrue(TEXT("The saved Island resolves its tagged Wind Arch and three Engine-cube proxies"),
			UIslandWindArchPresentationSubsystem::FindWindArchProxies(Island, Marker, Pillars, Beam));
		TestEqual(TEXT("The saved arch has two pillar proxies"), Pillars.Num(), 2);
		TestNotNull(TEXT("The saved arch has its lintel proxy"), Beam);
		if (Marker && Pillars.Num() == 2 && Beam)
		{
			for (const AStaticMeshActor* Pillar : Pillars)
				AddInfo(FString::Printf(TEXT("Saved Wind Arch pillar %s at %s, extent %s"), *Pillar->GetActorLabel(),
					*Pillar->GetActorLocation().ToCompactString(), *Pillar->GetStaticMeshComponent()->Bounds.BoxExtent.ToCompactString()));
			AddInfo(FString::Printf(TEXT("Saved Wind Arch lintel %s at %s, extent %s"), *Beam->GetActorLabel(),
				*Beam->GetActorLocation().ToCompactString(), *Beam->GetStaticMeshComponent()->Bounds.BoxExtent.ToCompactString()));
		}
	}
	else AddInfo(TEXT("The editor Island is not loaded; saved-map proxy recognition was not exercised."));

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Wind Arch fixture world is created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	ATargetPoint* Marker = World->SpawnActor<ATargetPoint>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Synthetic Wind Arch marker spawns"), Marker))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Marker->Tags.Add(TEXT("WindArch"));
	Marker->Tags.Add(TEXT("IslandLandmark"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Rock = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
	TestNotNull(TEXT("Engine cube proxy mesh resolves"), Cube);
	TestNotNull(TEXT("Existing Starter Content rock mesh resolves"), Rock);
	AStaticMeshActor* PillarA = World->SpawnActor<AStaticMeshActor>(FVector(-350.f, 0.f, 17.f), FRotator::ZeroRotator);
	AStaticMeshActor* PillarB = World->SpawnActor<AStaticMeshActor>(FVector(350.f, 0.f, 50.f), FRotator::ZeroRotator);
	AStaticMeshActor* Beam = World->SpawnActor<AStaticMeshActor>(FVector(0.f, 0.f, 380.f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Left cube proxy spawns"), PillarA) || !TestNotNull(TEXT("Right cube proxy spawns"), PillarB) ||
		!TestNotNull(TEXT("Cube lintel proxy spawns"), Beam) || !Cube || !Rock)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	PillarA->GetStaticMeshComponent()->SetStaticMesh(Cube);
	PillarB->GetStaticMeshComponent()->SetStaticMesh(Cube);
	Beam->GetStaticMeshComponent()->SetStaticMesh(Cube);
	PillarA->SetActorScale3D(FVector(0.6f, 0.6f, 6.65f));
	PillarB->SetActorScale3D(FVector(0.6f, 0.6f, 5.99f));
	Beam->SetActorScale3D(FVector(7.6f, 0.6f, 0.6f));
	PillarB->SetActorHiddenInGame(true);
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Synthetic Island weather actor spawns"), Weather))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Weather->MaximumWindSpeed = 300.f;
	Weather->SetActorTickEnabled(false);
	double StrongWindOffset = -1.0;
	for (int32 CandidateSeconds = 0; CandidateSeconds <= 600; CandidateSeconds += 5)
	{
		Weather->WeatherTimeOffset = CandidateSeconds;
		if (Weather->SampleWind(Marker->GetActorLocation(), World->GetTimeSeconds()).Size2D() >= 105.f)
		{
			StrongWindOffset = CandidateSeconds;
			break;
		}
	}
	TestTrue(TEXT("A strong natural-wind sample exists within one configured weather cycle"), StrongWindOffset >= 0.0);
	World->BeginPlay();

	UIslandWindArchPresentationSubsystem* PresentationSubsystem = World->GetSubsystem<UIslandWindArchPresentationSubsystem>();
	TestNotNull(TEXT("Game world has the transient landmark presentation subsystem"), PresentationSubsystem);
	AWindArchStonework* Stonework = nullptr;
	for (TActorIterator<AWindArchStonework> It(World); It; ++It) { Stonework = *It; break; }
	TestNotNull(TEXT("Game start replaces the three cube visuals with transient rock forms"), Stonework);
	TestEqual(TEXT("The arch is built from ten rough pillar stones and five lintel blocks"),
		Stonework ? Stonework->GetStoneCount() : 0, 15);
	TestTrue(TEXT("All three original map proxies are hidden only for presentation"),
		PillarA->IsHidden() && PillarB->IsHidden() && Beam->IsHidden());
	if (Stonework && Stonework->Stones)
	{
		TestTrue(TEXT("Replacement stones remain collisionless"), Stonework->Stones->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Replacement stones do not alter navigation"), Stonework->Stones->CanEverAffectNavigation());
		TestTrue(TEXT("The replacement uses the existing rough rock mesh"), Stonework->Stones->GetStaticMesh() == Rock);
		UMaterialInterface* ArchSurface = Stonework->Stones->GetMaterial(0);
		TestTrue(TEXT("The replacement uses a dedicated subdued material instead of Starter Content's stark mottling"),
			ArchSurface && ArchSurface->IsA<UMaterialInstanceDynamic>() && ArchSurface != Rock->GetMaterial(0));
		for (int32 Index = 0; Index < 10; ++Index)
		{
			FTransform Instance;
			const bool bGotInstance = Stonework->Stones->GetInstanceTransform(Index, Instance, false);
			TestTrue(FString::Printf(TEXT("Pillar stone %d has a transform"), Index), bGotInstance);
			if (!bGotInstance) continue;
			const AStaticMeshActor* Proxy = Index < 5 ? PillarA : PillarB;
			const FVector ProxyCenter = Marker->GetActorTransform().InverseTransformPosition(Proxy->GetStaticMeshComponent()->Bounds.Origin);
			const float ProxyHalfHeight = Proxy->GetStaticMeshComponent()->Bounds.BoxExtent.Z;
			TestTrue(FString::Printf(TEXT("Pillar stone %d is fitted to its own proxy height"), Index),
				Instance.GetLocation().Z >= ProxyCenter.Z - ProxyHalfHeight && Instance.GetLocation().Z <= ProxyCenter.Z + ProxyHalfHeight);
			TestTrue(FString::Printf(TEXT("Pillar stone %d keeps the art-directed narrower cross-section"), Index),
				Instance.GetScale3D().X <= 0.44f && Instance.GetScale3D().Y <= 0.29f);
		}
	}
	if (Stonework && StrongWindOffset >= 0.0)
	{
		Weather->WeatherTimeOffset = StrongWindOffset;
		TestTrue(TEXT("The fixture's current natural wind is above the emission threshold"),
			Weather->SampleWind(Stonework->GetActorLocation(), World->GetTimeSeconds()).Size2D() >= 105.f);
		Stonework->CheckForAmbientWind();
		int32 MoteCount = 0;
		AIslandWindMoteEffect* SpawnedMotes = nullptr;
		for (TActorIterator<AIslandWindMoteEffect> It(World); It; ++It)
		{
			++MoteCount;
			SpawnedMotes = *It;
		}
		TestEqual(TEXT("A strong sampled natural wind spawns one mote effect"), MoteCount, 1);
		TestNotNull(TEXT("The wind response has a transient mote actor"), SpawnedMotes);
		if (SpawnedMotes)
		{
			TestTrue(TEXT("The mote actor is transient and will not alter saved world state"), SpawnedMotes->HasAnyFlags(RF_Transient));
			TestTrue(TEXT("The mote actor is centered on the Wind Arch"),
				FVector::Dist2D(SpawnedMotes->GetActorLocation(), Stonework->GetActorLocation()) <= 1.f);
		}
		TestTrue(TEXT("Emission starts the full ambient cooldown"),
			FMath::IsNearlyEqual(Stonework->AmbientMoteCooldownRemaining, 30.f));

		Stonework->CheckForAmbientWind();
		int32 MotesAfterSecondSample = 0;
		for (TActorIterator<AIslandWindMoteEffect> It(World); It; ++It) ++MotesAfterSecondSample;
		TestEqual(TEXT("A second strong sample does not duplicate a live mote pass"), MotesAfterSecondSample, 1);

		Stonework->AmbientMotes.Reset();
		Stonework->AmbientMoteCooldownRemaining = 0.f;
		Stonework->CheckForAmbientWind();
		int32 MotesAfterOverlapAttempt = 0;
		for (TActorIterator<AIslandWindMoteEffect> It(World); It; ++It) ++MotesAfterOverlapAttempt;
		TestEqual(TEXT("An existing nearby mote pass suppresses an overlapping ambient pass"), MotesAfterOverlapAttempt, 1);
		TestTrue(TEXT("Overlap suppression retries only after a short delay"),
			FMath::IsNearlyEqual(Stonework->AmbientMoteCooldownRemaining, 3.f));
	}
	if (PresentationSubsystem) PresentationSubsystem->RestorePresentation();
	TestFalse(TEXT("A previously visible proxy is restored visible"), PillarA->IsHidden());
	TestTrue(TEXT("A proxy that was already hidden stays hidden after restoration"), PillarB->IsHidden());
	TestFalse(TEXT("The original beam is restored visible"), Beam->IsHidden());
	bool bStoneworkStillPresent = false;
	for (TActorIterator<AWindArchStonework> It(World); It; ++It) bStoneworkStillPresent = true;
	TestFalse(TEXT("The transient stonework actor is removed on restore"), bStoneworkStillPresent);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
