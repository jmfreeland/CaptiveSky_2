#include "Misc/AutomationTest.h"
#include "CaptiveSky_2PlayerController.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "IslandInteractionTestPlayerController.h"
#include "IslandDayNight.h"
#include "IslandInteractionUtility.h"
#include "IslandFirefly.h"
#include "IslandTidepoolCrab.h"
#include "IslandTideglassSubsystem.h"
#include "IslandTideglassDragonfly.h"
#include "IslandPoolRippleEffect.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "IslandTrail.h"
#include "IslandWindMoteEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/InputComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWaveProcedural.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandNightEcologyTest, "CaptiveSky2.Agent.NightEcology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandGroundCoverTest, "CaptiveSky2.Agent.GroundCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGroundCoverTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Ground-cover fixture world created"), World)) return false;
	if (!TestNotNull(TEXT("Engine is available for the ground-cover fixture"), GEngine))
	{
		World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const IConsoleVariable* GlobalDistanceFields = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DistanceFields"));
	const int32 GlobalDistanceFieldsBefore = GlobalDistanceFields ? GlobalDistanceFields->GetInt() : -1;
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	ATargetPoint* Tideglass = World->SpawnActor<ATargetPoint>(FVector(0.f, 0.f, 300.f), FRotator::ZeroRotator, Spawn);
	ATargetPoint* ListeningStones = World->SpawnActor<ATargetPoint>(FVector(3500.f, 0.f, 300.f), FRotator::ZeroRotator, Spawn);
	ATargetPoint* InnDoorLantern = World->SpawnActor<ATargetPoint>(FVector(7000.f, 0.f, 300.f), FRotator::ZeroRotator, Spawn);
	ATargetPoint* WindArch = World->SpawnActor<ATargetPoint>(FVector(10500.f, 0.f, 300.f), FRotator::ZeroRotator, Spawn);
	AActor* TideglassGround = World->SpawnActor<AActor>(FVector(0.f, 0.f, -20.f), FRotator::ZeroRotator, Spawn);
	AActor* StonesGround = World->SpawnActor<AActor>(FVector(3500.f, 0.f, -20.f), FRotator::ZeroRotator, Spawn);
	AActor* InnGround = World->SpawnActor<AActor>(FVector(7000.f, 0.f, -20.f), FRotator::ZeroRotator, Spawn);
	AActor* InnRoof = World->SpawnActor<AActor>(FVector(7000.f, 0.f, 480.f), FRotator::ZeroRotator, Spawn);
	AActor* WindArchGround = World->SpawnActor<AActor>(FVector(10500.f, 0.f, -20.f), FRotator::ZeroRotator, Spawn);
	AActor* TideglassFootprint = World->SpawnActor<AActor>(FVector(0.f, 0.f, 300.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Ground-cover weather actor spawned"), Weather) ||
		!TestNotNull(TEXT("Tideglass ground-cover marker spawned"), Tideglass) ||
		!TestNotNull(TEXT("ListeningStones ground-cover marker spawned"), ListeningStones) ||
		!TestNotNull(TEXT("Inn-door lantern ground-cover marker spawned"), InnDoorLantern) ||
		!TestNotNull(TEXT("WindArch ground-cover marker spawned"), WindArch) ||
		!TestNotNull(TEXT("Tideglass collision fixture spawned"), TideglassGround) ||
		!TestNotNull(TEXT("ListeningStones collision fixture spawned"), StonesGround) ||
		!TestNotNull(TEXT("Inn approach ground fixture spawned"), InnGround) ||
		!TestNotNull(TEXT("Inn roof fixture spawned"), InnRoof) ||
		!TestNotNull(TEXT("WindArch approach ground fixture spawned"), WindArchGround) ||
		!TestNotNull(TEXT("Tideglass water-footprint fixture spawned"), TideglassFootprint))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	IConsoleVariable* SwayFocusRadius = IConsoleManager::Get().FindConsoleVariable(TEXT("CaptiveSky.Island.FoliageSwayFocusRadiusCm"));
	const float PreviousFocusRadius = SwayFocusRadius ? SwayFocusRadius->GetFloat() : 100.f;
	ON_SCOPE_EXIT
	{
		if (SwayFocusRadius) SwayFocusRadius->Set(PreviousFocusRadius, ECVF_SetByCode);
	};
	if (TestNotNull(TEXT("CPU foliage sway focus-radius CVar is registered"), SwayFocusRadius))
		// Existing fixture interactions rely on seeing distant local gust markers; keep that
		// expectation independent of the intentionally tighter runtime default.
		SwayFocusRadius->Set(3000.f, ECVF_SetByCode);
	IConsoleVariable* LandscapeDensity = IConsoleManager::Get().FindConsoleVariable(TEXT("CaptiveSky.Island.GroundCoverLandscapeDensity"));
	const float PreviousLandscapeDensity = LandscapeDensity ? LandscapeDensity->GetFloat() : 1.f;
	ON_SCOPE_EXIT
	{
		if (LandscapeDensity) LandscapeDensity->Set(PreviousLandscapeDensity, ECVF_SetByCode);
	};
	if (TestNotNull(TEXT("Broad meadow-density control is registered"), LandscapeDensity))
	{
		LandscapeDensity->Set(0.65f, ECVF_SetByCode);
		TestTrue(TEXT("Broad meadow density accepts a reversible reduced value"),
			FMath::IsNearlyEqual(LandscapeDensity->GetFloat(), 0.65f));
		LandscapeDensity->Set(1.f, ECVF_SetByCode);
		TestTrue(TEXT("Broad meadow density can restore the authored default"),
			FMath::IsNearlyEqual(LandscapeDensity->GetFloat(), 1.f));
	}
	IConsoleVariable* SwayUpdateInterval = IConsoleManager::Get().FindConsoleVariable(TEXT("CaptiveSky.Island.FoliageSwayUpdateIntervalSeconds"));
	if (TestNotNull(TEXT("CPU foliage sway update interval CVar is registered"), SwayUpdateInterval))
	{
		const float PreviousInterval = SwayUpdateInterval->GetFloat();
		Weather->GroundCoverSwayUpdateAccumulator = 0.f;
		SwayUpdateInterval->Set(1.f, ECVF_SetByCode);
		Weather->Tick(0.4f);
		TestTrue(TEXT("CPU foliage sway waits for its configured one-second interval"),
			FMath::IsNearlyEqual(Weather->GroundCoverSwayUpdateAccumulator, 0.4f));
		Weather->Tick(0.5f);
		TestTrue(TEXT("CPU foliage sway retains elapsed time below the configured interval"),
			FMath::IsNearlyEqual(Weather->GroundCoverSwayUpdateAccumulator, 0.9f));
		Weather->Tick(0.2f);
		TestTrue(TEXT("CPU foliage sway updates when the interval elapses and retains only its fractional remainder"),
			FMath::IsNearlyEqual(Weather->GroundCoverSwayUpdateAccumulator, 0.1f, 0.001f));
		SwayUpdateInterval->Set(PreviousInterval, ECVF_SetByCode);
		Weather->GroundCoverSwayUpdateAccumulator = 0.f;
	}
	TArray<UHierarchicalInstancedStaticMeshComponent*> WeatherFoliageComponents;
	Weather->GetComponents(WeatherFoliageComponents);
	TestEqual(TEXT("All current foliage component groups are covered by the rendering contract"), WeatherFoliageComponents.Num(), 22);
	const bool bExpectedFoliageDistanceFields = FParse::Param(FCommandLine::Get(), TEXT("IslandFoliageDistanceFields"));
	for (const UHierarchicalInstancedStaticMeshComponent* Component : WeatherFoliageComponents)
	{
		if (!TestNotNull(TEXT("Foliage rendering-contract component exists"), Component)) continue;
		TestEqual(FString::Printf(TEXT("%s uses targeted foliage distance-field policy"), *Component->GetName()),
			bool(Component->bAffectDistanceFieldLighting), bExpectedFoliageDistanceFields);
		TestFalse(FString::Printf(TEXT("%s remains shadowless"), *Component->GetName()), bool(Component->CastShadow));
		TestFalse(FString::Printf(TEXT("%s remains nonblocking for navigation"), *Component->GetName()), Component->CanEverAffectNavigation());
	}
	if (TestNotNull(TEXT("Global distance-field renderer setting exists"), GlobalDistanceFields))
		TestEqual(TEXT("Foliage initialization preserves global distance-field rendering"), GlobalDistanceFields->GetInt(), GlobalDistanceFieldsBefore);
	auto CheckCullRange = [this, &WeatherFoliageComponents](FName ComponentName, int32 ExpectedStart, int32 ExpectedEnd)
	{
		UHierarchicalInstancedStaticMeshComponent* Component = nullptr;
		for (UHierarchicalInstancedStaticMeshComponent* Candidate : WeatherFoliageComponents)
			if (Candidate && Candidate->GetFName() == ComponentName) { Component = Candidate; break; }
		if (!TestNotNull(FString::Printf(TEXT("Ground-cover component %s exists"), *ComponentName.ToString()), Component)) return;
		int32 ActualStart = 0;
		int32 ActualEnd = 0;
		Component->GetCullDistances(ActualStart, ActualEnd);
		TestEqual(FString::Printf(TEXT("%s foliage fade-start distance"), *ComponentName.ToString()), ActualStart, ExpectedStart);
		TestEqual(FString::Printf(TEXT("%s foliage cull-end distance"), *ComponentName.ToString()), ActualEnd, ExpectedEnd);
	};
	CheckCullRange(TEXT("ShoreGrassA"), 6000, 9000);
	CheckCullRange(TEXT("ShoreGroundPlantLowD"), 6000, 9000);
	CheckCullRange(TEXT("IslandMeadowFlowerA"), 6000, 9000);
	CheckCullRange(TEXT("IslandShrubs"), 8000, 14000);
	CheckCullRange(TEXT("IslandSpruce"), 35000, 55000);
	CheckCullRange(TEXT("IslandFestuca"), 6000, 9000);
	CheckCullRange(TEXT("IslandPhalaris"), 6000, 9000);
	CheckCullRange(TEXT("IslandCattails"), 6000, 9000);

	Tideglass->Tags = {TEXT("TideglassPool"), TEXT("IslandLandmark")};
	ListeningStones->Tags = {TEXT("ListeningStones"), TEXT("IslandLandmark")};
	WindArch->Tags = {TEXT("WindArch"), TEXT("IslandLandmark")};
	InnDoorLantern->Tags.Add(TEXT("InnDoorLantern"));
	TideglassFootprint->Tags.Add(TEXT("IslandLandmark"));
	UStaticMesh* TideglassSphereAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!TestNotNull(TEXT("Tideglass water-footprint sphere asset loaded"), TideglassSphereAsset))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	UStaticMeshComponent* TideglassSphere = NewObject<UStaticMeshComponent>(TideglassFootprint);
	TideglassSphere->SetStaticMesh(TideglassSphereAsset);
	TideglassFootprint->SetRootComponent(TideglassSphere);
	TideglassSphere->SetRelativeScale3D(FVector(6.f, 6.f, 0.01f));
	TideglassSphere->RegisterComponent();
	TideglassFootprint->SetActorLocation(Tideglass->GetActorLocation());
	TestTrue(TEXT("The shallow sphere fixture remains within the pool marker's 25 cm discovery radius"),
		FVector::Dist(TideglassFootprint->GetActorLocation(), Tideglass->GetActorLocation()) <= 25.f);
	auto AddGround = [](AActor* Actor, const FVector& Location, const FVector& Extent)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
	};
	AddGround(TideglassGround, FVector(0.f, 0.f, -20.f), FVector(1800.f, 1800.f, 10.f));
	AddGround(StonesGround, FVector(3500.f, 0.f, -20.f), FVector(1000.f, 1000.f, 10.f));
	AddGround(InnGround, FVector(7000.f, 0.f, -20.f), FVector(1500.f, 1500.f, 10.f));
	AddGround(InnRoof, FVector(7000.f, 0.f, 480.f), FVector(500.f, 500.f, 20.f));
	AddGround(WindArchGround, FVector(10500.f, 0.f, -20.f), FVector(1500.f, 1500.f, 10.f));
	int32 TideglassLandmarkCount = 0;
	int32 ListeningStonesLandmarkCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TideglassLandmarkCount += It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("TideglassPool")) ? 1 : 0;
		ListeningStonesLandmarkCount += It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("ListeningStones")) ? 1 : 0;
	}
	TestEqual(TEXT("Both intended ground-cover landmarks are visible to the world iterator"), TideglassLandmarkCount, 1);
	TestEqual(TEXT("ListeningStones ground-cover landmark is visible to the world iterator"), ListeningStonesLandmarkCount, 1);
	const TPair<const TCHAR*, AActor*> Samples[] = { { TEXT("Tideglass"), Tideglass }, { TEXT("ListeningStones"), ListeningStones } };
	for (const TPair<const TCHAR*, AActor*>& Sample : Samples)
	{
		const FVector Probe = Sample.Value->GetActorLocation() + FVector(500.f, 0.f, 1400.f);
		FHitResult Hit;
		const bool bFoundGround = World->LineTraceSingleByChannel(Hit, Probe, Probe - FVector(0.f, 0.f, 6400.f), ECC_WorldStatic);
		TestTrue(FString::Printf(TEXT("%s fixture ring has world-static ground beneath a sample point"), Sample.Key), bFoundGround);
		AddInfo(FString::Printf(TEXT("%s probe hit %s at %s."), Sample.Key,
			bFoundGround && Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("nothing"),
			bFoundGround ? *Hit.ImpactPoint.ToCompactString() : TEXT("no location")));
	}
	Weather->InitializeGroundCover();
	UHierarchicalInstancedStaticMeshComponent* GrassC = Weather->FindShoreGrassC();
	TestNotNull(TEXT("Third grass component is present"), GrassC);
	TestTrue(TEXT("PlantFactory Festuca mesh and all six imported materials resolve"),
		Weather->IslandFestuca->GetStaticMesh() && Weather->IslandFestuca->GetStaticMesh()->GetStaticMaterials().Num() == 6);
	TestTrue(TEXT("PlantFactory Phalaris mesh and all five imported materials resolve"),
		Weather->IslandPhalaris->GetStaticMesh() && Weather->IslandPhalaris->GetStaticMesh()->GetStaticMaterials().Num() == 5);
	TestEqual(TEXT("Eight collisionless HISM components carry the expanded Fab meadow flower mix"),
		Weather->IslandMeadowFlowers.Num(), AIslandWeather::MeadowFlowerSpeciesCount);
	const TCHAR* MeadowFlowerPaths[AIslandWeather::MeadowFlowerSpeciesCount] = {
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_01_01.flower_01_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_04_01.flower_04_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_17_01.flower_17_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_08_01.flower_08_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_13_01.flower_13_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_20_01.flower_20_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_02_01.flower_02_01"),
		TEXT("/Game/PN_FoliageCollection/Meshes/flowerMesh/flower_03_01.flower_03_01") };
	for (int32 SpeciesIndex = 0; SpeciesIndex < Weather->IslandMeadowFlowers.Num(); ++SpeciesIndex)
	{
		UStaticMesh* MeadowFlowerMesh = LoadObject<UStaticMesh>(nullptr, MeadowFlowerPaths[SpeciesIndex]);
		TestNotNull(FString::Printf(TEXT("Fab meadow flower mesh %d is available"), SpeciesIndex), MeadowFlowerMesh);
		TestNotNull(FString::Printf(TEXT("Fab meadow flower HISM %d is present"), SpeciesIndex), Weather->IslandMeadowFlowers[SpeciesIndex].Get());
		if (Weather->IslandMeadowFlowers[SpeciesIndex])
		{
			TestTrue(TEXT("Imported flowers stay nonblocking and outside navigation"),
				Weather->IslandMeadowFlowers[SpeciesIndex]->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
				!Weather->IslandMeadowFlowers[SpeciesIndex]->CanEverAffectNavigation());
			TestEqual(FString::Printf(TEXT("Fab meadow flower HISM %d uses its matching imported mesh"), SpeciesIndex),
				Weather->IslandMeadowFlowers[SpeciesIndex]->GetStaticMesh().Get(), MeadowFlowerMesh);
		}
	}
	const FBox SpruceLocalBounds = Weather->IslandSpruce->GetStaticMesh()->GetBoundingBox();
	const float SpruceMeshHeight = SpruceLocalBounds.GetSize().Z;
	TestTrue(TEXT("The fixture can inspect the loaded spruce crown bounds"), SpruceMeshHeight > KINDA_SMALL_NUMBER);
	if (SpruceMeshHeight > KINDA_SMALL_NUMBER)
	{
		auto MakeCrownProbe = [&SpruceLocalBounds](const FTransform& TreeTransform, TArray<FVector>& Starts, TArray<FVector>& Ends)
		{
			const float ProbeLocalZ = SpruceLocalBounds.Min.Z + SpruceLocalBounds.GetSize().Z * 0.62f;
			const FVector ProbeLocalXY(SpruceLocalBounds.GetCenter().X, SpruceLocalBounds.GetCenter().Y, ProbeLocalZ);
			Starts.Add(TreeTransform.TransformPosition(ProbeLocalXY));
			Ends.Add(TreeTransform.TransformPosition(ProbeLocalXY + FVector(0.f, 0.f, SpruceLocalBounds.GetSize().Z * 0.12f)));
		};
		const FTransform MatureTree(FRotator::ZeroRotator, FVector(14000.f, 0.f, 0.f), FVector(1200.f / SpruceMeshHeight));
		Weather->IslandSpruce->AddInstance(MatureTree, true);
		TArray<FVector> MatureProbeStarts, MatureProbeEnds;
		MakeCrownProbe(MatureTree, MatureProbeStarts, MatureProbeEnds);
		TestEqual(TEXT("The collisionless mature spruce HISM contributes a crown-cover clue"),
			Weather->CountSpruceCrownCoverProbes(MatureProbeStarts, MatureProbeEnds), 1);
		TestEqual(TEXT("Spruce remains non-colliding for the canopy query"),
			Weather->IslandSpruce->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		const FTransform Sapling(FRotator::ZeroRotator, FVector(17000.f, 0.f, 0.f), FVector(500.f / SpruceMeshHeight));
		Weather->IslandSpruce->AddInstance(Sapling, true);
		TArray<FVector> SaplingProbeStarts, SaplingProbeEnds;
		MakeCrownProbe(Sapling, SaplingProbeStarts, SaplingProbeEnds);
		TestEqual(TEXT("A short spruce sapling does not count as overhead shelter"),
			Weather->CountSpruceCrownCoverProbes(SaplingProbeStarts, SaplingProbeEnds), 0);
		Weather->IslandSpruce->ClearInstances();
	}
	const FVector SpeciesPatchProbe(-100500.f, 100500.f, 0.f);
	TestEqual(TEXT("Ground-cover species selection repeats for the same seed and world position"),
		AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe, 71), AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe, 71));
	TestEqual(TEXT("Nearby plants within one botanical patch share a species"),
		AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe, 71),
		AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe + FVector(200.f, -300.f, 0.f), 71));
	int32 SpeciesCounts[11] = {};
	int32 ExposedGrassCounts[3] = {};
	const TArray<FVector> ExposedAnchors = {FVector::ZeroVector};
	const TArray<FVector> NoOtherAnchors;
	TestEqual(TEXT("Exposure is absent without an exposed landmark"),
		AIslandWeather::CalculateGroundCoverExposure(FVector::ZeroVector, NoOtherAnchors, NoOtherAnchors), 0.f);
	TestEqual(TEXT("The 50 metre exposed core is fully grass dominated"),
		AIslandWeather::CalculateGroundCoverExposure(FVector(5000.f, 0.f, 900.f), ExposedAnchors, NoOtherAnchors), 1.f);
	TestEqual(TEXT("The habitat feathers halfway at 57.5 metres"),
		AIslandWeather::CalculateGroundCoverExposure(FVector(5750.f, 0.f, 0.f), ExposedAnchors, NoOtherAnchors), 0.5f);
	TestEqual(TEXT("The exposed habitat ends at 65 metres"),
		AIslandWeather::CalculateGroundCoverExposure(FVector(6500.f, 0.f, 0.f), ExposedAnchors, NoOtherAnchors), 0.f);
	const TArray<FVector> WetEdgeAnchors = {FVector(4000.f, 0.f, 0.f)};
	TestEqual(TEXT("A nearer wet or social landmark retains its original meadow cover"),
		AIslandWeather::CalculateGroundCoverExposure(FVector(3500.f, 0.f, 0.f), ExposedAnchors, WetEdgeAnchors), 0.f);
	TestEqual(TEXT("Equal-distance habitat ownership preserves the non-exposed habitat"),
		AIslandWeather::CalculateGroundCoverExposure(FVector(2000.f, 0.f, 0.f), ExposedAnchors, WetEdgeAnchors), 0.f);
	TestTrue(TEXT("The cairn keeps a small, human-scale open patch"),
		AIslandWeather::IsWithinCurioGroundCoverClearance(FVector(200.f, 0.f, 0.f), FVector::ZeroVector, EIslandCurioKind::Cairn));
	TestFalse(TEXT("The cairn clearance does not erase the surrounding meadow"),
		AIslandWeather::IsWithinCurioGroundCoverClearance(FVector(220.f, 0.f, 0.f), FVector::ZeroVector, EIslandCurioKind::Cairn));
	TestTrue(TEXT("An open seed pod remains visible above nearby grass"),
		AIslandWeather::IsWithinCurioGroundCoverClearance(FVector(160.f, 0.f, 0.f), FVector::ZeroVector, EIslandCurioKind::SeedPod));
	TestFalse(TEXT("Small trail stones clear only their immediate footprint"),
		AIslandWeather::IsWithinCurioGroundCoverClearance(FVector(70.f, 0.f, 0.f), FVector::ZeroVector, EIslandCurioKind::PaleStone));
	FIslandTrailLedger WornTrail;
	const FVector TrailCellPoint(75.f, 75.f, 0.f);
	for (int32 Step = 0; Step < UIslandTrailSubsystem::WearStartSteps; ++Step)
		WornTrail.AddStep(TrailCellPoint, FVector::UpVector);
	TestFalse(TEXT("A barely visible trail does not clear plants before wear begins"),
		AIslandWeather::IsGroundCoverWithinWornTrailClearance(WornTrail, TrailCellPoint));
	for (int32 Step = UIslandTrailSubsystem::WearStartSteps; Step < UIslandTrailSubsystem::WearFullSteps; ++Step)
		WornTrail.AddStep(TrailCellPoint, FVector::UpVector);
	TestTrue(TEXT("A well-worn trail clears ground cover underfoot"),
		AIslandWeather::IsGroundCoverWithinWornTrailClearance(WornTrail, TrailCellPoint));
	TestTrue(TEXT("The path clearance stays close to the worn trace"),
		AIslandWeather::IsGroundCoverWithinWornTrailClearance(WornTrail, TrailCellPoint + FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("The path leaves adjacent plants beyond its narrow edge intact"),
		AIslandWeather::IsGroundCoverWithinWornTrailClearance(WornTrail, TrailCellPoint + FVector(140.f, 0.f, 0.f)));
	TestFalse(TEXT("An unrelated unwalked cell retains its vegetation"),
		AIslandWeather::IsGroundCoverWithinWornTrailClearance(WornTrail, FVector(1000.f, 1000.f, 0.f)));
	WornTrail.LastDecayDay = 1;
	TestTrue(TEXT("A later Island day weathers the saved path ledger"), WornTrail.WeatherTo(31));
	TestEqual(TEXT("Thirty days of unused path weathering leave a faint 10-step trace"), WornTrail.StepsAt(TrailCellPoint), 10);
	TestFalse(TEXT("Regrown grass returns beyond the thinner aged trail"),
		AIslandWeather::IsGroundCoverWithinWornTrailClearance(WornTrail, TrailCellPoint + FVector(100.f, 0.f, 0.f)));
	const TArray<FVector> NoCompetingHabitats;
	TestEqual(TEXT("The pool margin begins at full wet-edge influence"),
		AIslandWeather::CalculateGroundCoverWetEdgeMoisture(FVector(500.f, 0.f, 0.f), FVector::ZeroVector,
			NoCompetingHabitats, 500.f, 1500.f), 1.f);
	TestEqual(TEXT("The wet-edge composition fades smoothly halfway into the meadow"),
		AIslandWeather::CalculateGroundCoverWetEdgeMoisture(FVector(1000.f, 0.f, 0.f), FVector::ZeroVector,
			NoCompetingHabitats, 500.f, 1500.f), 0.5f);
	TestEqual(TEXT("The wet-edge composition ends at its outer radius"),
		AIslandWeather::CalculateGroundCoverWetEdgeMoisture(FVector(1500.f, 0.f, 0.f), FVector::ZeroVector,
			NoCompetingHabitats, 500.f, 1500.f), 0.f);
	TestEqual(TEXT("A nearer or tied landmark keeps ownership of its own ground-cover patch"),
		AIslandWeather::CalculateGroundCoverWetEdgeMoisture(FVector(1000.f, 0.f, 0.f), FVector::ZeroVector,
			{FVector(2000.f, 0.f, 0.f)}, 500.f, 1500.f), 0.f);
	TestEqual(TEXT("An invalid zero-width wet-edge band has no influence"),
		AIslandWeather::CalculateGroundCoverWetEdgeMoisture(FVector(500.f, 0.f, 0.f), FVector::ZeroVector,
			NoCompetingHabitats, 500.f, 500.f), 0.f);
	int32 DryEdgeBroadleafPatches = 0;
	int32 WetEdgeBroadleafPatches = 0;
	for (int32 X = -8; X < 8; ++X)
		for (int32 Y = -8; Y < 8; ++Y)
		{
			const FVector Position(X * 700.f + 100.f, Y * 700.f + 100.f, 0.f);
			const int32 NormalSpecies = AIslandWeather::SelectGroundCoverVariant(Position, 71);
			const int32 DryEdgeSpecies = AIslandWeather::SelectGroundCoverVariant(Position, 71, 0.f, 0.f);
			const int32 WetEdgeSpecies = AIslandWeather::SelectGroundCoverVariant(Position, 71, 0.f, 1.f);
			TestEqual(TEXT("A dry Tideglass margin retains its original deterministic meadow choice"), DryEdgeSpecies, NormalSpecies);
			TestEqual(TEXT("Wetness never selects a species slot outside the existing foliage set"), WetEdgeSpecies >= 0 && WetEdgeSpecies <= 10, true);
			if (DryEdgeSpecies < 5) ++DryEdgeBroadleafPatches;
			if (WetEdgeSpecies < 5) ++WetEdgeBroadleafPatches;
			TestEqual(TEXT("Wet-edge species remain coherent within a 7 m botanical patch"), WetEdgeSpecies,
				AIslandWeather::SelectGroundCoverVariant(Position + FVector(200.f, 200.f, 0.f), 71, 0.f, 1.f));
			TestEqual(TEXT("Wet-edge influence clamps above one"), WetEdgeSpecies,
				AIslandWeather::SelectGroundCoverVariant(Position, 71, 0.f, 2.f));
		}
	TestTrue(TEXT("The wet edge measurably favours broadleaf cover without forcing a monoculture"),
		WetEdgeBroadleafPatches >= DryEdgeBroadleafPatches + 60 && WetEdgeBroadleafPatches < 220);
	for (int32 X = -8; X < 8; ++X)
		for (int32 Y = -8; Y < 8; ++Y)
		{
			const FVector Position(X * 700.f + 100.f, Y * 700.f + 100.f, 0.f);
			const int32 ExposedVariant = AIslandWeather::SelectGroundCoverVariant(Position, 71, 1.f);
			TestTrue(TEXT("Fully exposed botanical patches select only grass"), ExposedVariant >= 5 && ExposedVariant <= 10);
			++ExposedGrassCounts[(ExposedVariant - 5) / 2];
			TestEqual(TEXT("Exposure below zero preserves the original meadow"),
				AIslandWeather::SelectGroundCoverVariant(Position, 71, -1.f), AIslandWeather::SelectGroundCoverVariant(Position, 71));
			TestEqual(TEXT("Exposure above one is bounded"),
				AIslandWeather::SelectGroundCoverVariant(Position, 71, 2.f), ExposedVariant);
			const int32 TransitionVariant = AIslandWeather::SelectGroundCoverVariant(Position, 71, 0.5f);
			TestTrue(TEXT("Feathered exposure retains an existing species slot"), TransitionVariant >= 0 && TransitionVariant <= 10);
		}
	TestTrue(TEXT("Exposed habitat retains all three grass forms"),
		ExposedGrassCounts[0] > 0 && ExposedGrassCounts[1] > 0 && ExposedGrassCounts[2] > 0);
	for (int32 X = 0; X < 8; ++X)
		for (int32 Y = 0; Y < 8; ++Y)
			++SpeciesCounts[AIslandWeather::SelectGroundCoverVariant(FVector(X * 2500.f + 900.f, Y * 2500.f + 900.f, 0.f), 71)];
	int32 MinimumSpeciesCells = MAX_int32;
	int32 MaximumSpeciesCells = 0;
	for (int32 Count : SpeciesCounts)
	{
		MinimumSpeciesCells = FMath::Min(MinimumSpeciesCells, Count);
		MaximumSpeciesCells = FMath::Max(MaximumSpeciesCells, Count);
	}
	TestTrue(TEXT("Spatial patches use all eight foliage meshes with a reasonably balanced distribution"),
		MinimumSpeciesCells >= 2 && MaximumSpeciesCells <= 14);
	const FVector FlowerPatchProbe(-100500.f, 100500.f, 0.f);
	TestEqual(TEXT("A meadow flower patch selects the same species for the same seed and cell"),
		AIslandWeather::SelectMeadowFlowerVariant(FlowerPatchProbe, 71), AIslandWeather::SelectMeadowFlowerVariant(FlowerPatchProbe, 71));
	TestEqual(TEXT("Nearby meadow flowers share a botanical color patch"),
		AIslandWeather::SelectMeadowFlowerVariant(FlowerPatchProbe, 71),
		AIslandWeather::SelectMeadowFlowerVariant(FlowerPatchProbe + FVector(500.f, -400.f, 0.f), 71));
	const FVector FlowerPocketA = AIslandWeather::SelectMeadowFlowerPocketOffset(71, 0);
	const FVector FlowerPocketB = AIslandWeather::SelectMeadowFlowerPocketOffset(71, 1);
	TestTrue(TEXT("Flower pocket offsets are deterministic, bounded, and spaced for separate stems"),
		FlowerPocketA.Equals(AIslandWeather::SelectMeadowFlowerPocketOffset(71, 0)) &&
		FlowerPocketA.Size2D() >= 550.f && FlowerPocketA.Size2D() <= 600.f &&
		FlowerPocketB.Size2D() >= 825.f && FlowerPocketB.Size2D() <= 875.f &&
		FVector::Dist2D(FlowerPocketA, FlowerPocketB) >= 475.f);
	TestTrue(TEXT("Flower pockets have a strict three-instance-per-site ceiling"),
		AIslandWeather::MeadowFlowersPerPocket == 3);
	int32 MeadowFlowerSpeciesCounts[AIslandWeather::MeadowFlowerSpeciesCount] = {};
	for (int32 X = 0; X < 12; ++X)
		for (int32 Y = 0; Y < 12; ++Y)
			++MeadowFlowerSpeciesCounts[AIslandWeather::SelectMeadowFlowerVariant(FVector(X * 2000.f, Y * 2000.f, 0.f), 71)];
	int32 MinimumFlowerPatches = MAX_int32;
	int32 MaximumFlowerPatches = 0;
	for (int32 Count : MeadowFlowerSpeciesCounts)
	{
		MinimumFlowerPatches = FMath::Min(MinimumFlowerPatches, Count);
		MaximumFlowerPatches = FMath::Max(MaximumFlowerPatches, Count);
	}
	TestTrue(TEXT("All eight imported flower forms occupy balanced, spatially coherent meadow patches"),
		MinimumFlowerPatches >= 8 && MaximumFlowerPatches <= 28);
	TestTrue(TEXT("ListeningStones flower pockets stay outside the landmark clearing and within the 45 m flower fade range"),
		AIslandWeather::ListeningStonesFlowerInnerRadius > 2600.f &&
		AIslandWeather::ListeningStonesFlowerInnerRadius < AIslandWeather::ListeningStonesFlowerOuterRadius &&
		AIslandWeather::ListeningStonesFlowerOuterRadius < 4500.f);
	TestTrue(TEXT("The ListeningStones habitat receives a fixed, bounded share of the existing flower-site budget"),
		AIslandWeather::ListeningStonesFlowerSiteCount > 0 && AIslandWeather::ListeningStonesFlowerSiteCount < 512);
	TestTrue(TEXT("Pool clearance, the inn roof filter, and hillside patches preserve varied cover within the 17,068-instance budget"),
		Weather->GroundCoverInstanceCount > 96 && Weather->GroundCoverInstanceCount <= 17068);
	const int32 FixturePlantCount = Weather->ShoreGroundPlants->GetInstanceCount() + Weather->ShoreGroundPlantLowA->GetInstanceCount() +
		Weather->ShoreGroundPlantLowB->GetInstanceCount() + Weather->ShoreGroundPlantLowC->GetInstanceCount() + Weather->ShoreGroundPlantLowD->GetInstanceCount();
	TestTrue(TEXT("Mixed habitats retain broadleaf cover within the fixed vegetation budget"),
		FixturePlantCount > 0 && FixturePlantCount * 100 <= Weather->GroundCoverInstanceCount * 54);
	TestEqual(TEXT("The fixture has no landscape or sea plane, so no global meadow patches are generated"), Weather->GroundCoverMeadowInstanceCount, 0);
	for (UHierarchicalInstancedStaticMeshComponent* LowPlant : {Weather->ShoreGroundPlantLowA, Weather->ShoreGroundPlantLowB,
		Weather->ShoreGroundPlantLowC, Weather->ShoreGroundPlantLowD})
	{
		const float MeshDiameter = 2.f * FMath::Max(LowPlant->GetStaticMesh()->GetBounds().BoxExtent.X, LowPlant->GetStaticMesh()->GetBounds().BoxExtent.Y);
		for (int32 Index = 0; Index < LowPlant->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			if (LowPlant->GetInstanceTransform(Index, Transform, false))
				TestTrue(TEXT("Additional broad-leaf ground plants stay within their 75 cm jittered footprint bound"), MeshDiameter * Transform.GetScale3D().X <= 75.f);
		}
	}
	const FVector PoolScale = TideglassSphere->GetComponentScale();
	const FBoxSphereBounds PoolBounds = TideglassSphereAsset->GetBounds();
	const float PoolClearanceRadius = FVector::Dist2D(TideglassSphere->GetComponentLocation(), Tideglass->GetActorLocation()) +
		FMath::Max(PoolBounds.BoxExtent.X * PoolScale.X, PoolBounds.BoxExtent.Y * PoolScale.Y) * 1.13f + 125.f;
	AddInfo(FString::Printf(TEXT("Pool-edge fixture expects %.1f cm clearance; weather placed %d instances (%d grass A + %d grass B + %d grass C + %d broadleaf + %d low plant A + %d low plant B + %d rosette + %d leafy plant)."),
		PoolClearanceRadius, Weather->GroundCoverInstanceCount, Weather->ShoreGrassA->GetInstanceCount(), Weather->ShoreGrassB->GetInstanceCount(), GrassC->GetInstanceCount(),
		Weather->ShoreGroundPlants->GetInstanceCount(), Weather->ShoreGroundPlantLowA->GetInstanceCount(), Weather->ShoreGroundPlantLowB->GetInstanceCount(),
		Weather->ShoreGroundPlantLowC->GetInstanceCount(), Weather->ShoreGroundPlantLowD->GetInstanceCount()));
	int32 GrassInsidePoolClearance = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Grass : {Weather->ShoreGrassA.Get(), Weather->ShoreGrassB.Get(), GrassC, Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(), Weather->ShoreGroundPlantLowB.Get(), Weather->ShoreGroundPlantLowC.Get(), Weather->ShoreGroundPlantLowD.Get(), Weather->IslandFestuca.Get()})
	{
		for (int32 Index = 0; Index < Grass->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			if (Grass->GetInstanceTransform(Index, Transform, true) && FVector::Dist2D(Transform.GetLocation(), Tideglass->GetActorLocation()) < PoolClearanceRadius)
			{
				++GrassInsidePoolClearance;
				AddInfo(FString::Printf(TEXT("Pool-clearance violation: %s instance %d at %s (%.1f cm from marker)."),
					*Grass->GetName(), Index, *Transform.GetLocation().ToCompactString(),
					FVector::Dist2D(Transform.GetLocation(), Tideglass->GetActorLocation())));
			}
		}
	}
	TestEqual(TEXT("No ground-cover instance intrudes into the Tideglass water footprint or edge margin"), GrassInsidePoolClearance, 0);
	int32 InnApproachGrass = 0;
	int32 InnGrassUnderRoof = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Grass : {Weather->ShoreGrassA.Get(), Weather->ShoreGrassB.Get(), GrassC, Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(), Weather->ShoreGroundPlantLowB.Get(), Weather->ShoreGroundPlantLowC.Get(), Weather->ShoreGroundPlantLowD.Get()})
	{
		for (int32 Index = 0; Index < Grass->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			if (!Grass->GetInstanceTransform(Index, Transform, true)) continue;
			const FVector Location = Transform.GetLocation();
			if (FVector::Dist2D(Location, InnDoorLantern->GetActorLocation()) > 1250.f) continue;
			++InnApproachGrass;
			if (FMath::Abs(Location.X - 7000.f) <= 500.f && FMath::Abs(Location.Y) <= 500.f)
			{
				++InnGrassUnderRoof;
				AddInfo(FString::Printf(TEXT("Roof-overlap instance in %s at %s."), *Grass->GetName(), *Location.ToCompactString()));
			}
		}
	}
	TestTrue(TEXT("Transient foliage reaches the outdoor inn approach"), InnApproachGrass > 0 && InnApproachGrass <= 108);
	TestEqual(TEXT("No inn ground cover is placed beneath the roof footprint"), InnGrassUnderRoof, 0);
	int32 WindArchApproachGrass = 0;
	int32 WindArchCenterGrass = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Grass : {Weather->ShoreGrassA.Get(), Weather->ShoreGrassB.Get(), GrassC, Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(), Weather->ShoreGroundPlantLowB.Get(), Weather->ShoreGroundPlantLowC.Get(), Weather->ShoreGroundPlantLowD.Get()})
	{
		for (int32 Index = 0; Index < Grass->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			if (!Grass->GetInstanceTransform(Index, Transform, true)) continue;
			const float Distance = FVector::Dist2D(Transform.GetLocation(), WindArch->GetActorLocation());
			if (Distance >= 349.f && Distance <= 1201.f) ++WindArchApproachGrass;
			if (Distance < 349.f) ++WindArchCenterGrass;
		}
	}
	TestTrue(TEXT("Transient foliage populates the WindArch approach"), WindArchApproachGrass > 0 && WindArchApproachGrass <= 288);
	TestEqual(TEXT("WindArch landmark center remains open"), WindArchCenterGrass, 0);
	int32 WindArchBroadleafCount = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Plant : {Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(),
		Weather->ShoreGroundPlantLowB.Get(), Weather->ShoreGroundPlantLowC.Get(), Weather->ShoreGroundPlantLowD.Get()})
		for (int32 Index = 0; Index < Plant->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			if (Plant->GetInstanceTransform(Index, Transform, true) &&
				FVector::Dist2D(Transform.GetLocation(), WindArch->GetActorLocation()) < 1250.f) ++WindArchBroadleafCount;
		}
	TestEqual(TEXT("WindArch exposed verge materializes grasses rather than woodland broadleaf cover"), WindArchBroadleafCount, 0);
	const int32 MixedHabitatCount = Weather->GroundCoverInstanceCount - WindArchApproachGrass - Weather->IslandFestuca->GetInstanceCount();
	// Compare the other verges against the same fixture without the exposed landmark,
	// rather than prescribing a global ratio for three small, unevenly sampled patches.
	WindArch->Tags.Remove(TEXT("WindArch"));
	AIslandWeather* MixedHabitatBaseline = World->SpawnActor<AIslandWeather>(Spawn);
	if (MixedHabitatBaseline) MixedHabitatBaseline->InitializeGroundCover();
	WindArch->Tags.Add(TEXT("WindArch"));
	if (TestNotNull(TEXT("Non-exposed habitat baseline spawned"), MixedHabitatBaseline))
	{
		TestEqual(TEXT("Exposed habitat leaves the other landmark placement counts unchanged"),
			MixedHabitatCount, MixedHabitatBaseline->GroundCoverInstanceCount);
		const int32 BaselinePlants = MixedHabitatBaseline->ShoreGroundPlants->GetInstanceCount() +
			MixedHabitatBaseline->ShoreGroundPlantLowA->GetInstanceCount() + MixedHabitatBaseline->ShoreGroundPlantLowB->GetInstanceCount() +
			MixedHabitatBaseline->ShoreGroundPlantLowC->GetInstanceCount() + MixedHabitatBaseline->ShoreGroundPlantLowD->GetInstanceCount();
		TestEqual(TEXT("Exposed habitat leaves other landmark broadleaf counts unchanged"), FixturePlantCount, BaselinePlants);
		MixedHabitatBaseline->Destroy();
	}
	TestTrue(TEXT("Festuca replaces a bounded share of the WindArch approach placements"),
		Weather->IslandFestuca->GetInstanceCount() > 0 && Weather->IslandFestuca->GetInstanceCount() < WindArchApproachGrass);
	TestEqual(TEXT("Each wind-driven grass and ground-plant species retains an immutable baseline for every instance"),
		Weather->ShoreGrassABaseTransforms.Num() + Weather->ShoreGrassBBaseTransforms.Num() + Weather->ShoreGroundPlantBaseTransforms.Num() +
		Weather->ShoreGroundPlantLowABaseTransforms.Num() + Weather->ShoreGroundPlantLowBBaseTransforms.Num() +
		Weather->ShoreGroundPlantLowCBaseTransforms.Num() + Weather->ShoreGroundPlantLowDBaseTransforms.Num() +
		Weather->IslandFestucaBaseTransforms.Num(), Weather->GroundCoverInstanceCount);
	TestEqual(TEXT("HISM populations match the reported transient ground-cover population"),
		Weather->ShoreGrassA->GetInstanceCount() + Weather->ShoreGrassB->GetInstanceCount() + GrassC->GetInstanceCount() + Weather->ShoreGroundPlants->GetInstanceCount() +
		Weather->ShoreGroundPlantLowA->GetInstanceCount() + Weather->ShoreGroundPlantLowB->GetInstanceCount() +
		Weather->ShoreGroundPlantLowC->GetInstanceCount() + Weather->ShoreGroundPlantLowD->GetInstanceCount() +
		Weather->IslandFestuca->GetInstanceCount(), Weather->GroundCoverInstanceCount);
	TestTrue(TEXT("Ground cover stays nonblocking and off navigation"),
		Weather->ShoreGrassA->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGrassB->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		GrassC->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlants->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlantLowA->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlantLowB->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlantLowC->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlantLowD->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->IslandFestuca->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->IslandPhalaris->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		!Weather->ShoreGrassA->CanEverAffectNavigation() && !Weather->ShoreGrassB->CanEverAffectNavigation() && !GrassC->CanEverAffectNavigation() && !Weather->ShoreGroundPlants->CanEverAffectNavigation() &&
		!Weather->ShoreGroundPlantLowA->CanEverAffectNavigation() && !Weather->ShoreGroundPlantLowB->CanEverAffectNavigation() &&
		!Weather->ShoreGroundPlantLowC->CanEverAffectNavigation() && !Weather->ShoreGroundPlantLowD->CanEverAffectNavigation() &&
		!Weather->IslandFestuca->CanEverAffectNavigation() && !Weather->IslandPhalaris->CanEverAffectNavigation());
	TestTrue(TEXT("All grass and ground-plant variants are visible around the landmarks and inn approach"),
		Weather->ShoreGrassA->IsVisible() && Weather->ShoreGrassB->IsVisible() && GrassC->IsVisible() && Weather->ShoreGroundPlants->IsVisible() &&
		Weather->ShoreGrassA->GetInstanceCount() > 0 && Weather->ShoreGrassB->GetInstanceCount() > 0 && GrassC->GetInstanceCount() > 0 &&
		Weather->ShoreGroundPlantLowA->IsVisible() && Weather->ShoreGroundPlantLowB->IsVisible() &&
		Weather->ShoreGroundPlantLowC->IsVisible() && Weather->ShoreGroundPlantLowD->IsVisible() &&
		Weather->ShoreGroundPlants->GetInstanceCount() > 0 && Weather->ShoreGroundPlantLowA->GetInstanceCount() > 0 &&
		Weather->ShoreGroundPlantLowC->GetInstanceCount() > 0 &&
		Weather->ShoreGroundPlantLowD->GetInstanceCount() > 0);
	const FTransform GrassBase(FQuat(FVector::UpVector, FMath::DegreesToRadians(37.f)), FVector(120.f, 230.f, 8.f), FVector(1.7f));
	const FTransform CalmGrass = AIslandWeather::CalculateGroundCoverSway(GrassBase, FVector::ZeroVector, 2.0, 4, 71, 180.f);
	TestTrue(TEXT("Calm air preserves the exact authored grass transform"), CalmGrass.Equals(GrassBase));
	const FTransform WindyGrass = AIslandWeather::CalculateGroundCoverSway(GrassBase, FVector(180.f, 0.f, 0.f), 2.0, 4, 71, 180.f);
	TestTrue(TEXT("Wind bends a clump without translating its planted base"),
		WindyGrass.GetLocation().Equals(GrassBase.GetLocation()) && WindyGrass.GetScale3D().Equals(GrassBase.GetScale3D()) &&
		!WindyGrass.GetRotation().Equals(GrassBase.GetRotation()));
	TestTrue(TEXT("Wind sway stays below ten degrees even at the reference wind ceiling"),
		GrassBase.GetRotation().AngularDistance(WindyGrass.GetRotation()) <= FMath::DegreesToRadians(10.f));
	const FTransform SpruceBase(FQuat(FVector::UpVector, FMath::DegreesToRadians(19.f)), FVector(120.f, 230.f, 8.f), FVector(1.7f));
	const FVector SpruceMeshBottom(12.f, -7.f, -100.f);
	const FTransform CalmSpruce = AIslandWeather::CalculateSpruceSway(SpruceBase, SpruceMeshBottom, FVector::ZeroVector, 2.0, 4, 71, 180.f);
	TestTrue(TEXT("Calm air preserves the exact planted spruce transform"), CalmSpruce.Equals(SpruceBase));
	const FTransform WindySpruce = AIslandWeather::CalculateSpruceSway(SpruceBase, SpruceMeshBottom, FVector(180.f, 0.f, 0.f), 2.0, 4, 71, 180.f);
	TestTrue(TEXT("Wind moves a spruce crown while the mesh-bottom planting point stays fixed"),
		WindySpruce.TransformPosition(SpruceMeshBottom).Equals(SpruceBase.TransformPosition(SpruceMeshBottom), 0.01f) &&
		WindySpruce.GetScale3D().Equals(SpruceBase.GetScale3D()) &&
		!WindySpruce.GetRotation().Equals(SpruceBase.GetRotation()));
	TestTrue(TEXT("Spruce crown lean stays below 4.5 degrees at the reference wind ceiling"),
		SpruceBase.GetRotation().AngularDistance(WindySpruce.GetRotation()) <= FMath::DegreesToRadians(4.5f));
	Weather->AddTransientGust(Tideglass->GetActorLocation(), FVector::ForwardVector, 280.f, 900.f, 18.f);
	Weather->AddTransientGust(ListeningStones->GetActorLocation(), FVector::ForwardVector, 280.f, 900.f, 18.f);
	Weather->UpdateGroundCoverSway();
	AddInfo(FString::Printf(TEXT("Nearby ground-cover sway updated %d of %d instances in the landmark-gust fixture."),
		Weather->GroundCoverSwayLastUpdatedInstanceCount, Weather->GroundCoverInstanceCount));
	TestTrue(TEXT("Ground-cover sway updates only instances inside nearby view/gust regions"),
		Weather->GroundCoverSwayLastUpdatedInstanceCount > 0 && Weather->GroundCoverSwayLastUpdatedInstanceCount < Weather->GroundCoverInstanceCount);
	bool bAClumpRespondedToLocalWind = false;
	bool bDistantInstancesRemainAtBaseline = true;
	int32 DistantInstanceCount = 0;
	auto CheckWindResponse = [this, &bAClumpRespondedToLocalWind, &bDistantInstancesRemainAtBaseline, &DistantInstanceCount, Weather, Tideglass, ListeningStones](UHierarchicalInstancedStaticMeshComponent* Grass, const TArray<FTransform>& Baselines, int32 FirstBaseline = 0)
	{
		for (int32 Index = 0; Index < Grass->GetInstanceCount(); ++Index)
		{
			FTransform Current;
			if (!Grass->GetInstanceTransform(Index, Current, false)) continue;
			const FTransform& Base = Baselines[FirstBaseline + Index];
			const FVector WorldLocation = Weather->GetActorTransform().TransformPosition(Base.GetLocation());
			const bool bNearWindFocus = FVector::Dist2D(WorldLocation, Tideglass->GetActorLocation()) <= 3000.f ||
				FVector::Dist2D(WorldLocation, ListeningStones->GetActorLocation()) <= 3000.f;
			if (!bNearWindFocus)
			{
				++DistantInstanceCount;
				bDistantInstancesRemainAtBaseline &= Current.GetRotation().Equals(Base.GetRotation(), 0.001f);
			}
			TestTrue(TEXT("Wind leaves each clump's planted location and scale unchanged"),
				Current.GetLocation().Equals(Base.GetLocation(), 0.01f) && Current.GetScale3D().Equals(Base.GetScale3D(), 0.01f));
			bAClumpRespondedToLocalWind |= !Current.GetRotation().Equals(Base.GetRotation(), 0.001f);
		}
	};
	CheckWindResponse(Weather->ShoreGrassA, Weather->ShoreGrassABaseTransforms);
	CheckWindResponse(Weather->ShoreGrassB, Weather->ShoreGrassBBaseTransforms);
	CheckWindResponse(GrassC, Weather->ShoreGrassBBaseTransforms, Weather->ShoreGrassB->GetInstanceCount());
	CheckWindResponse(Weather->ShoreGroundPlants, Weather->ShoreGroundPlantBaseTransforms);
	CheckWindResponse(Weather->ShoreGroundPlantLowA, Weather->ShoreGroundPlantLowABaseTransforms);
	CheckWindResponse(Weather->ShoreGroundPlantLowB, Weather->ShoreGroundPlantLowBBaseTransforms);
	CheckWindResponse(Weather->ShoreGroundPlantLowC, Weather->ShoreGroundPlantLowCBaseTransforms);
	CheckWindResponse(Weather->ShoreGroundPlantLowD, Weather->ShoreGroundPlantLowDBaseTransforms);
	CheckWindResponse(Weather->IslandShrubs, Weather->IslandShrubBaseTransforms);
	TestTrue(TEXT("Distant plants stay at immutable baselines until they enter an active view or gust region"),
		DistantInstanceCount > 0 && bDistantInstancesRemainAtBaseline);
	TestTrue(TEXT("Transient local gusts visibly sway planted shore grass"), bAClumpRespondedToLocalWind);
	FTransform InteractionSample;
	FTransform UnoccupiedWindPose;
	ACharacter* Walker = nullptr;
	if (Weather->ShoreGrassA->GetInstanceCount() > 0 && Weather->ShoreGrassA->GetInstanceTransform(0, InteractionSample, true))
	{
		const FVector WalkerLocation = InteractionSample.GetLocation() + FVector(100.f, 0.f, 90.f);
		Walker = World->SpawnActor<ACharacter>(ACharacter::StaticClass(), WalkerLocation, FRotator::ZeroRotator);
	}
	TestNotNull(TEXT("Resident interaction fixture creates a walking character"), Walker);
	if (Walker)
	{
		Weather->ShoreGrassA->GetInstanceTransform(0, UnoccupiedWindPose, false);
		Weather->UpdateGroundCoverSway();
		FTransform BrushedAsidePose;
		Weather->ShoreGrassA->GetInstanceTransform(0, BrushedAsidePose, false);
		const FVector ResidentGroundPoint = Walker->GetActorLocation() - FVector(0.f, 0.f, Walker->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FVector AwayFromResident = (InteractionSample.GetLocation() - ResidentGroundPoint).GetSafeNormal2D();
		const FTransform GrassComponentTransform = Weather->ShoreGrassA->GetComponentTransform();
		const FVector WindOnlyUp = GrassComponentTransform.TransformVectorNoScale(UnoccupiedWindPose.GetRotation().RotateVector(FVector::UpVector)).GetSafeNormal();
		const FVector BrushedUp = GrassComponentTransform.TransformVectorNoScale(BrushedAsidePose.GetRotation().RotateVector(FVector::UpVector)).GetSafeNormal();
		TestTrue(TEXT("A nearby resident bends grass away without moving its planted base"),
			!BrushedAsidePose.GetRotation().Equals(UnoccupiedWindPose.GetRotation(), 0.001f) &&
			BrushedAsidePose.GetLocation().Equals(UnoccupiedWindPose.GetLocation(), 0.01f) &&
			BrushedAsidePose.GetScale3D().Equals(UnoccupiedWindPose.GetScale3D(), 0.01f));
		TestTrue(TEXT("The brushed grass leans away from the resident"), FVector::DotProduct(BrushedUp - WindOnlyUp, AwayFromResident) > 0.f);
		Walker->SetActorLocation(InteractionSample.GetLocation() + FVector(100.f, 0.f, 590.f));
		Weather->UpdateGroundCoverSway();
		const FTransform& GrassBaseTransform = Weather->ShoreGrassABaseTransforms[0];
		const FVector GrassWorldLocation = Weather->ShoreGrassA->GetComponentTransform().TransformPosition(GrassBaseTransform.GetLocation());
		const FVector GrassWorldWind = Weather->GetLocalWind(GrassWorldLocation, Weather);
		const FVector GrassComponentWind = Weather->ShoreGrassA->GetComponentTransform().InverseTransformVectorNoScale(GrassWorldWind);
		const FVector GrassLocalWind = GrassBaseTransform.GetRotation().UnrotateVector(GrassComponentWind);
		const FTransform WindOnlyOverheadPose = AIslandWeather::CalculateGroundCoverSway(GrassBaseTransform, GrassLocalWind,
			World->GetTimeSeconds(), 0, Weather->WeatherSeed, Weather->MaximumWindSpeed);
		FTransform OverheadPose;
		Weather->ShoreGrassA->GetInstanceTransform(0, OverheadPose, false);
		TestTrue(TEXT("A resident passing overhead does not bend ground cover"),
			OverheadPose.GetRotation().Equals(WindOnlyOverheadPose.GetRotation(), 0.001f));
		Walker->SetActorLocation(InteractionSample.GetLocation() + FVector(5000.f, 0.f, 90.f));
		Weather->UpdateGroundCoverSway();
		FTransform RecoveredPose;
		Weather->ShoreGrassA->GetInstanceTransform(0, RecoveredPose, false);
		TestTrue(TEXT("Grass recovers to the same wind-only pose after the resident passes"),
			RecoveredPose.GetRotation().Equals(UnoccupiedWindPose.GetRotation(), 0.001f));
		Walker->Destroy();
	}
	// Drop the destroyed walker's last proximity bend before sampling idempotence.
	Weather->UpdateGroundCoverSway();
	auto CaptureTransforms = [](UHierarchicalInstancedStaticMeshComponent* Grass, TArray<FTransform>& OutTransforms)
	{
		OutTransforms.Reset(Grass->GetInstanceCount());
		for (int32 Index = 0; Index < Grass->GetInstanceCount(); ++Index)
		{
			FTransform Current;
			if (Grass->GetInstanceTransform(Index, Current, false)) OutTransforms.Add(Current);
		}
	};
	TArray<FTransform> FirstSwayA;
	TArray<FTransform> FirstSwayB;
	TArray<FTransform> FirstSwayC;
	TArray<FTransform> FirstSwayPlants;
	TArray<FTransform> FirstSwayPlantsLowA;
	TArray<FTransform> FirstSwayPlantsLowB;
	TArray<FTransform> FirstSwayPlantsLowC;
	TArray<FTransform> FirstSwayPlantsLowD;
	TArray<FTransform> FirstSwayShrubs;
	CaptureTransforms(Weather->ShoreGrassA, FirstSwayA);
	CaptureTransforms(Weather->ShoreGrassB, FirstSwayB);
	CaptureTransforms(GrassC, FirstSwayC);
	CaptureTransforms(Weather->ShoreGroundPlants, FirstSwayPlants);
	CaptureTransforms(Weather->ShoreGroundPlantLowA, FirstSwayPlantsLowA);
	CaptureTransforms(Weather->ShoreGroundPlantLowB, FirstSwayPlantsLowB);
	CaptureTransforms(Weather->ShoreGroundPlantLowC, FirstSwayPlantsLowC);
	CaptureTransforms(Weather->ShoreGroundPlantLowD, FirstSwayPlantsLowD);
	CaptureTransforms(Weather->IslandShrubs, FirstSwayShrubs);
	Weather->UpdateGroundCoverSway();
	TArray<FTransform> SecondSwayA;
	TArray<FTransform> SecondSwayB;
	TArray<FTransform> SecondSwayC;
	TArray<FTransform> SecondSwayPlants;
	TArray<FTransform> SecondSwayPlantsLowA;
	TArray<FTransform> SecondSwayPlantsLowB;
	TArray<FTransform> SecondSwayPlantsLowC;
	TArray<FTransform> SecondSwayPlantsLowD;
	TArray<FTransform> SecondSwayShrubs;
	CaptureTransforms(Weather->ShoreGrassA, SecondSwayA);
	CaptureTransforms(Weather->ShoreGrassB, SecondSwayB);
	CaptureTransforms(GrassC, SecondSwayC);
	CaptureTransforms(Weather->ShoreGroundPlants, SecondSwayPlants);
	CaptureTransforms(Weather->ShoreGroundPlantLowA, SecondSwayPlantsLowA);
	CaptureTransforms(Weather->ShoreGroundPlantLowB, SecondSwayPlantsLowB);
	CaptureTransforms(Weather->ShoreGroundPlantLowC, SecondSwayPlantsLowC);
	CaptureTransforms(Weather->ShoreGroundPlantLowD, SecondSwayPlantsLowD);
	CaptureTransforms(Weather->IslandShrubs, SecondSwayShrubs);
	bool bRepeatedSwayIsStable = FirstSwayA.Num() == SecondSwayA.Num() && FirstSwayB.Num() == SecondSwayB.Num() && FirstSwayC.Num() == SecondSwayC.Num() &&
		FirstSwayPlants.Num() == SecondSwayPlants.Num() && FirstSwayPlantsLowA.Num() == SecondSwayPlantsLowA.Num() &&
		FirstSwayPlantsLowB.Num() == SecondSwayPlantsLowB.Num() && FirstSwayPlantsLowC.Num() == SecondSwayPlantsLowC.Num() &&
		FirstSwayPlantsLowD.Num() == SecondSwayPlantsLowD.Num() && FirstSwayShrubs.Num() == SecondSwayShrubs.Num();
	for (int32 Index = 0; Index < FMath::Min(FirstSwayA.Num(), SecondSwayA.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayA[Index].Equals(SecondSwayA[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayB.Num(), SecondSwayB.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayB[Index].Equals(SecondSwayB[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayC.Num(), SecondSwayC.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayC[Index].Equals(SecondSwayC[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayPlants.Num(), SecondSwayPlants.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayPlants[Index].Equals(SecondSwayPlants[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayPlantsLowA.Num(), SecondSwayPlantsLowA.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayPlantsLowA[Index].Equals(SecondSwayPlantsLowA[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayPlantsLowB.Num(), SecondSwayPlantsLowB.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayPlantsLowB[Index].Equals(SecondSwayPlantsLowB[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayPlantsLowC.Num(), SecondSwayPlantsLowC.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayPlantsLowC[Index].Equals(SecondSwayPlantsLowC[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayPlantsLowD.Num(), SecondSwayPlantsLowD.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayPlantsLowD[Index].Equals(SecondSwayPlantsLowD[Index], 0.001f);
	for (int32 Index = 0; Index < FMath::Min(FirstSwayShrubs.Num(), SecondSwayShrubs.Num()); ++Index)
		bRepeatedSwayIsStable &= FirstSwayShrubs[Index].Equals(SecondSwayShrubs[Index], 0.001f);
	if (!bRepeatedSwayIsStable)
	{
		auto LogFirstMismatch = [this](const TCHAR* Species, const TArray<FTransform>& Before, const TArray<FTransform>& After)
		{
			for (int32 Index = 0; Index < FMath::Min(Before.Num(), After.Num()); ++Index)
				if (!Before[Index].Equals(After[Index], 0.001f))
				{
					AddInfo(FString::Printf(TEXT("First sway mismatch in %s[%d]: translation %.4f cm, rotation %.4f degrees, scale %.5f."),
						Species, Index, FVector::Distance(Before[Index].GetLocation(), After[Index].GetLocation()),
						FMath::RadiansToDegrees(Before[Index].GetRotation().AngularDistance(After[Index].GetRotation())),
						FVector::Distance(Before[Index].GetScale3D(), After[Index].GetScale3D())));
					return;
				}
		};
		LogFirstMismatch(TEXT("grass A"), FirstSwayA, SecondSwayA);
		LogFirstMismatch(TEXT("grass B"), FirstSwayB, SecondSwayB);
		LogFirstMismatch(TEXT("grass C"), FirstSwayC, SecondSwayC);
		LogFirstMismatch(TEXT("broadleaf"), FirstSwayPlants, SecondSwayPlants);
		LogFirstMismatch(TEXT("low plant A"), FirstSwayPlantsLowA, SecondSwayPlantsLowA);
		LogFirstMismatch(TEXT("low plant B"), FirstSwayPlantsLowB, SecondSwayPlantsLowB);
		LogFirstMismatch(TEXT("rosette"), FirstSwayPlantsLowC, SecondSwayPlantsLowC);
		LogFirstMismatch(TEXT("leafy plant"), FirstSwayPlantsLowD, SecondSwayPlantsLowD);
		LogFirstMismatch(TEXT("understory shrub"), FirstSwayShrubs, SecondSwayShrubs);
	}
	TestTrue(TEXT("Repeating a weather update at the same time does not accumulate transform drift"), bRepeatedSwayIsStable);
	const int32 GroundCoverCountAfterFirstInitialization = Weather->GroundCoverInstanceCount;
	UHierarchicalInstancedStaticMeshComponent* TrailSampleComponent = nullptr;
	const TArray<UHierarchicalInstancedStaticMeshComponent*> GrassComponents = { Weather->ShoreGrassA, Weather->ShoreGrassB, GrassC,
		Weather->ShoreGroundPlants, Weather->ShoreGroundPlantLowA, Weather->ShoreGroundPlantLowB, Weather->ShoreGroundPlantLowC, Weather->ShoreGroundPlantLowD };
	FTransform TrailSampleTransform;
	for (UHierarchicalInstancedStaticMeshComponent* GrassComponent : GrassComponents)
		if (GrassComponent && GrassComponent->GetInstanceCount() > 0 && GrassComponent->GetInstanceTransform(0, TrailSampleTransform, false))
		{
			TrailSampleComponent = GrassComponent;
			break;
		}
	TestNotNull(TEXT("The fixture provides a real grass placement for the trail integration test"), TrailSampleComponent);
	Weather->InitializeGroundCover();
	TestEqual(TEXT("Repeated initialization does not duplicate the ground cover"), Weather->GroundCoverInstanceCount, GroundCoverCountAfterFirstInitialization);
	TestEqual(TEXT("The marker-only fixture has no landscape spruce groves"), Weather->GroundCoverTreeCount, 0);
	TestEqual(TEXT("The marker-only fixture has no landscape shrub understorey"), Weather->GroundCoverShrubCount, 0);
	Weather->ClearGroundCover();
	TestEqual(TEXT("Transient cleanup clears all grass instances"), Weather->GroundCoverInstanceCount, 0);
	TestEqual(TEXT("Transient cleanup clears the meadow-patch diagnostic count"), Weather->GroundCoverMeadowInstanceCount, 0);
	TestEqual(TEXT("Transient cleanup clears the spruce-tree count"), Weather->GroundCoverTreeCount, 0);
	TestEqual(TEXT("Transient cleanup clears the broadleaf-shrub count"), Weather->GroundCoverShrubCount, 0);
	TestEqual(TEXT("Transient cleanup releases the saved grass baselines"),
		Weather->ShoreGrassABaseTransforms.Num() + Weather->ShoreGrassBBaseTransforms.Num() + Weather->ShoreGroundPlantBaseTransforms.Num() +
		Weather->ShoreGroundPlantLowABaseTransforms.Num() + Weather->ShoreGroundPlantLowBBaseTransforms.Num() +
		Weather->ShoreGroundPlantLowCBaseTransforms.Num() + Weather->ShoreGroundPlantLowDBaseTransforms.Num() + Weather->IslandShrubBaseTransforms.Num(), 0);
	TestTrue(TEXT("Transient cleanup releases nearby-sway indices and spatial cells"),
		Weather->SwayedShoreGrassAIndices.IsEmpty() && Weather->SwayedShoreGrassBIndices.IsEmpty() && Weather->SwayedShoreGrassCIndices.IsEmpty() &&
		Weather->SwayedGroundPlantIndices.IsEmpty() && Weather->SwayedGroundPlantLowAIndices.IsEmpty() && Weather->SwayedGroundPlantLowBIndices.IsEmpty() &&
		Weather->SwayedGroundPlantLowCIndices.IsEmpty() && Weather->SwayedGroundPlantLowDIndices.IsEmpty() && Weather->SwayedShrubIndices.IsEmpty() &&
		Weather->ShoreGrassACells.IsEmpty() && Weather->ShoreGrassBCells.IsEmpty() && Weather->ShoreGrassCCells.IsEmpty() &&
		Weather->GroundPlantCells.IsEmpty() && Weather->GroundPlantLowACells.IsEmpty() && Weather->GroundPlantLowBCells.IsEmpty() &&
		Weather->GroundPlantLowCCells.IsEmpty() && Weather->GroundPlantLowDCells.IsEmpty() && Weather->ShrubCells.IsEmpty());
	TestTrue(TEXT("Cleared ground cover is hidden"), !Weather->ShoreGrassA->IsVisible() && !Weather->ShoreGrassB->IsVisible() && !GrassC->IsVisible() && !Weather->ShoreGroundPlants->IsVisible() &&
		!Weather->ShoreGroundPlantLowA->IsVisible() && !Weather->ShoreGroundPlantLowB->IsVisible() &&
		!Weather->ShoreGroundPlantLowC->IsVisible() && !Weather->ShoreGroundPlantLowD->IsVisible() && !Weather->IslandSpruce->IsVisible() && !Weather->IslandShrubs->IsVisible());
	if (TrailSampleComponent)
	{
		UIslandTrailSubsystem* Trails = World->GetSubsystem<UIslandTrailSubsystem>();
		TestNotNull(TEXT("The world fixture provides the persistent trail subsystem"), Trails);
		if (Trails)
		{
			Trails->bAllowStorage = false;
			FIslandTrailLedger& TrailLedger = Trails->GetLedgerMutable();
			TrailLedger.Cells.Reset();
			const FVector TrailPoint = TrailSampleComponent->GetComponentTransform().TransformPosition(TrailSampleTransform.GetLocation());
			for (int32 Step = 0; Step < UIslandTrailSubsystem::WearFullSteps; ++Step)
				TrailLedger.AddStep(TrailPoint, FVector::UpVector);
			Weather->InitializeGroundCover();
			TestTrue(TEXT("A well-worn persistent path suppresses real scatter candidates"), Weather->GroundCoverTrailClearedInstanceCount > 0);
			TestEqual(TEXT("Every skipped ground-cover candidate is reflected in the placement total"),
				Weather->GroundCoverInstanceCount + Weather->GroundCoverTrailClearedInstanceCount, GroundCoverCountAfterFirstInitialization);
			Weather->ClearGroundCover();
			TrailLedger.Cells.Reset();
		}
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

bool FIslandNightEcologyTest::RunTest(const FString& Parameters)
{
	// Isolated world: no Island agents, brains, memory files, or model requests.
	TArray<FTransform> GrassOffsetsA;
	TArray<FTransform> GrassOffsetsB;
	TArray<FTransform> GrassOffsetsOtherSeed;
	TArray<FTransform> InnGrassOffsets;
	TArray<FTransform> WindArchGrassOffsets;
	AIslandWeather::BuildGroundCoverOffsets(31415, GrassOffsetsA);
	AIslandWeather::BuildGroundCoverOffsets(31415, GrassOffsetsB);
	AIslandWeather::BuildGroundCoverOffsets(27182, GrassOffsetsOtherSeed);
	AIslandWeather::BuildGroundCoverOffsets(31415, 108, 350.f, 1200.f, InnGrassOffsets);
	AIslandWeather::BuildGroundCoverOffsets(31415, 288, 350.f, 1200.f, WindArchGrassOffsets);
	TestEqual(TEXT("Shore landmark scatter has a bounded 96-offset default template"), GrassOffsetsA.Num(), 96);
	TestEqual(TEXT("Inn approach scatter has a bounded 108-candidate instance budget"), InnGrassOffsets.Num(), 108);
	TestEqual(TEXT("WindArch verge has a bounded 288-candidate instance budget"), WindArchGrassOffsets.Num(), 288);
	bool bSameSeedMatches = GrassOffsetsA.Num() == GrassOffsetsB.Num();
	bool bOtherSeedDiffers = GrassOffsetsA.Num() == GrassOffsetsOtherSeed.Num();
	for (int32 Index = 0; Index < GrassOffsetsA.Num(); ++Index)
	{
		const FVector Offset = GrassOffsetsA[Index].GetLocation();
		const float Radius = Offset.Size2D();
		TestTrue(TEXT("Each grass clump stays in the 2.6–7.2 metre landmark ring"), Radius >= 259.f && Radius <= 721.f);
		TestTrue(TEXT("Grass scatter scale stays within its authored variation bounds"), GrassOffsetsA[Index].GetScale3D().X >= 0.8f && GrassOffsetsA[Index].GetScale3D().X <= 1.2f);
		bSameSeedMatches &= GrassOffsetsA[Index].GetLocation().Equals(GrassOffsetsB[Index].GetLocation(), 0.001f) &&
			GrassOffsetsA[Index].GetRotation().Equals(GrassOffsetsB[Index].GetRotation(), 0.001f);
		bOtherSeedDiffers &= !GrassOffsetsA[Index].GetLocation().Equals(GrassOffsetsOtherSeed[Index].GetLocation(), 0.001f);
	}
	for (const FTransform& OffsetTransform : InnGrassOffsets)
	{
		const float Radius = OffsetTransform.GetLocation().Size2D();
		TestTrue(TEXT("Inn approach clumps stay 3.5–12 metres from the door marker"), Radius >= 349.f && Radius <= 1201.f);
	}
	for (const FTransform& OffsetTransform : WindArchGrassOffsets)
	{
		const float Radius = OffsetTransform.GetLocation().Size2D();
		TestTrue(TEXT("WindArch verge clumps stay 3.5–12 metres from the landmark marker"), Radius >= 349.f && Radius <= 1201.f);
	}
	TestTrue(TEXT("The same weather seed reproduces identical grass placement"), bSameSeedMatches);
	TestTrue(TEXT("A different weather seed changes the grass placement"), bOtherSeedDiffers);
	TestNotNull(TEXT("First native shore grass mesh is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/grassMesh/grass_01_02_mesh.grass_01_02_mesh")));
	TestNotNull(TEXT("Second native shore grass mesh is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/grassMesh/grass_01_03_mesh.grass_01_03_mesh")));
	TestNotNull(TEXT("Third native shore grass mesh is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/grassMesh/grass_01_04_mesh.grass_01_04_mesh")));
	TestNotNull(TEXT("Existing broadleaf ground plant mesh is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_05_01.ground_05_01")));
	TestNotNull(TEXT("First additional ground-plant species is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_01_01.ground_01_01")));
	TestNotNull(TEXT("Second additional ground-plant species is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_01_02.ground_01_02")));
	TestNotNull(TEXT("Fab rosette ground-plant mesh is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_06_01.ground_06_01")));
	TestNotNull(TEXT("Fab leafy ground-plant mesh is available"), LoadObject<UStaticMesh>(nullptr, TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_12_01.ground_12_01")));
	TestTrue(TEXT("Local wind gently nudges the firefly drift"), AIslandFirefly::WindDisplacement(FVector(100.f, 0.f, 0.f)).Equals(FVector(12.f, 0.f, 0.f)));
	TestTrue(TEXT("Strong gust displacement stays bounded"), AIslandFirefly::WindDisplacement(FVector(1000.f, 0.f, 0.f)).Equals(FVector(30.f, 0.f, 0.f)));
	TestTrue(TEXT("Still air adds no wind displacement"), AIslandFirefly::WindDisplacement(FVector::ZeroVector).IsNearlyZero());
	TestTrue(TEXT("Still air adds no dragonfly drift"), AIslandTideglassDragonfly::WindDisplacement(FVector::ZeroVector).IsNearlyZero());
	TestTrue(TEXT("Dragonfly drift stays bounded in a strong gust"), AIslandTideglassDragonfly::WindDisplacement(FVector(1000.f, 0.f, 0.f)).Equals(FVector(54.f, 0.f, 0.f)));
	TestTrue(TEXT("Dry conditions leave dragonfly movement unchanged"), FMath::IsNearlyEqual(AIslandTideglassDragonfly::RainMovementScale(0.f), 1.f));
	TestTrue(TEXT("Heavy rain slows but does not stop dragonfly movement"), AIslandTideglassDragonfly::RainMovementScale(1.f) > 0.f && AIslandTideglassDragonfly::RainMovementScale(1.f) < 1.f);
	TestTrue(TEXT("Dragonfly rain response is smooth and monotonic"), AIslandTideglassDragonfly::RainMovementScale(0.8f) < AIslandTideglassDragonfly::RainMovementScale(0.45f));
	TestTrue(TEXT("Dry conditions leave crab roaming unchanged"), FMath::IsNearlyEqual(AIslandTidepoolCrab::RainMovementScale(0.f), 1.f));
	TestTrue(TEXT("Heavy rain reduces but does not stop crab roaming"), AIslandTidepoolCrab::RainMovementScale(1.f) > 0.f && AIslandTidepoolCrab::RainMovementScale(1.f) < 1.f);
	TestTrue(TEXT("Crab rain response changes smoothly and monotonically"), AIslandTidepoolCrab::RainMovementScale(0.75f) < AIslandTidepoolCrab::RainMovementScale(0.45f));
	const float LowTideActivity = AIslandTidepoolCrab::TideMovementScale(-UIslandTideglassSubsystem::MaximumTideOffsetCm);
	const float HighTideActivity = AIslandTidepoolCrab::TideMovementScale(UIslandTideglassSubsystem::MaximumTideOffsetCm);
	TestTrue(TEXT("No tidal displacement leaves crab activity at its ordinary baseline"),
		FMath::IsNearlyEqual(AIslandTidepoolCrab::TideMovementScale(0.f), 1.f));
	TestTrue(TEXT("Low water slightly broadens crab foraging drift"), LowTideActivity > 1.f && LowTideActivity <= 1.18f);
	TestTrue(TEXT("High water gently tucks crab drift without stopping it"), HighTideActivity < 1.f && HighTideActivity >= 0.82f);
	TestTrue(TEXT("Tidal crab activity is monotonic and bounded outside the tide range"),
		AIslandTidepoolCrab::TideMovementScale(-1000.f) == LowTideActivity &&
		AIslandTidepoolCrab::TideMovementScale(1000.f) == HighTideActivity);
	TestTrue(TEXT("Dry weather leaves firefly movement, glow, and wingbeats unchanged"),
		FMath::IsNearlyEqual(AIslandFirefly::RainMovementScale(0.f), 1.f) &&
		FMath::IsNearlyEqual(AIslandFirefly::RainGlowScale(0.f), 1.f) &&
		FMath::IsNearlyEqual(AIslandFirefly::RainWingBeatScale(0.f), 1.f));
	TestTrue(TEXT("Strong rain reduces but never removes firefly activity"),
		AIslandFirefly::RainMovementScale(1.f) > 0.f && AIslandFirefly::RainMovementScale(1.f) < 1.f &&
		AIslandFirefly::RainGlowScale(1.f) > 0.f && AIslandFirefly::RainGlowScale(1.f) < 1.f &&
		AIslandFirefly::RainWingBeatScale(1.f) > 0.f && AIslandFirefly::RainWingBeatScale(1.f) < 1.f);
	TestTrue(TEXT("Firefly rain response changes smoothly and monotonically through a shower"),
		AIslandFirefly::RainMovementScale(0.75f) < AIslandFirefly::RainMovementScale(0.45f) &&
		AIslandFirefly::RainGlowScale(0.75f) < AIslandFirefly::RainGlowScale(0.45f) &&
		AIslandFirefly::RainWingBeatScale(0.75f) < AIslandFirefly::RainWingBeatScale(0.45f));
	TestTrue(TEXT("New-moon fireflies keep their full natural glow"),
		FMath::IsNearlyEqual(AIslandFirefly::MoonlightGlowScale(0.f), 1.f));
	TestTrue(TEXT("Bright moonlight gently softens, but never removes, firefly glow"),
		AIslandFirefly::MoonlightGlowScale(1.f) >= 0.72f && AIslandFirefly::MoonlightGlowScale(1.f) < 1.f);
	TestTrue(TEXT("Firefly moonlight response changes smoothly through the phase"),
		AIslandFirefly::MoonlightGlowScale(1.f) < AIslandFirefly::MoonlightGlowScale(0.5f) &&
		AIslandFirefly::MoonlightGlowScale(0.5f) < AIslandFirefly::MoonlightGlowScale(0.f));

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
	AActor* FlightBlocker = World->SpawnActor<AActor>(FVector(2000.f, 2000.f, 250.f), FRotator::ZeroRotator, Spawn);
	AIslandFirefly* FlightTester = World->SpawnActor<AIslandFirefly>(FVector(1800.f, 2000.f, 250.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Firefly obstacle fixture spawned"), FlightBlocker) || !TestNotNull(TEXT("Firefly path tester spawned"), FlightTester))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	UBoxComponent* FlightBlockerBox = NewObject<UBoxComponent>(FlightBlocker);
	FlightBlocker->SetRootComponent(FlightBlockerBox);
	FlightBlockerBox->SetBoxExtent(FVector(10.f, 300.f, 300.f));
	FlightBlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
	FlightBlockerBox->RegisterComponent();
	FlightBlocker->SetActorLocation(FVector(2000.f, 2000.f, 250.f));
	const FVector DeflectedFlight = FlightTester->ResolveFlightPath(FVector(1800.f, 2000.f, 250.f), FVector(2200.f, 2300.f, 250.f));
	TestTrue(TEXT("Firefly flight never sweeps through a solid obstacle"), DeflectedFlight.X < 1990.f);
	TestTrue(TEXT("Firefly uses the open tangent path around the obstacle"), DeflectedFlight.Y > 2100.f && DeflectedFlight.Y <= 2300.f);
	AIslandTidepoolCrab* CrabPathTester = World->SpawnActor<AIslandTidepoolCrab>(FVector(1800.f, 2000.f, 250.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Shore crab obstacle tester spawned"), CrabPathTester))
	{
		const FVector DeflectedScurry = CrabPathTester->ResolveGroundPath(FVector(1800.f, 2000.f, 250.f), FVector(2200.f, 2300.f, 250.f));
		TestTrue(TEXT("Shore crab scurry never sweeps through solid geometry"), DeflectedScurry.X < 1990.f);
		TestTrue(TEXT("Shore crab tries the open tangent path around the obstacle"), DeflectedScurry.Y > 2100.f && DeflectedScurry.Y <= 2300.f);
		CrabPathTester->Destroy();
	}
	FlightTester->Destroy();
	FlightBlocker->Destroy();

	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	ATargetPoint* Habitat = World->SpawnActor<ATargetPoint>(FVector(1000.f, 2000.f, 300.f), FRotator::ZeroRotator, Spawn);
	ATargetPoint* NightStonesHabitat = World->SpawnActor<ATargetPoint>(FVector(3500.f, 2000.f, 300.f), FRotator::ZeroRotator, Spawn);
	AActor* Ground = World->SpawnActor<AActor>(Spawn);
	if (!TestNotNull(TEXT("Weather actor spawned"), Weather) || !TestNotNull(TEXT("Clock actor spawned"), Clock) ||
		!TestNotNull(TEXT("Habitat marker spawned"), Habitat) || !TestNotNull(TEXT("Nearby ListeningStones habitat marker spawned"), NightStonesHabitat))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	if (!TestNotNull(TEXT("Ground fixture actor spawned"), Ground))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	UBoxComponent* GroundBox = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(GroundBox);
	GroundBox->SetBoxExtent(FVector(1800.f, 2500.f, 10.f));
	GroundBox->SetCollisionProfileName(TEXT("BlockAll"));
	GroundBox->RegisterComponent();
	Ground->SetActorLocation(FVector(0.f, 0.f, -20.f));
	Habitat->Tags.Add(TEXT("TideglassPool"));
	NightStonesHabitat->Tags.Add(TEXT("ListeningStones"));
	NightStonesHabitat->Tags.Add(TEXT("IslandLandmark"));
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
	TestNotNull(TEXT("Ground rain effects use a bounded instanced component"), Weather->RainGroundImpactStreaks.Get());
	TestEqual(TEXT("Ground rain effect pool has exactly three reusable splash streaks"), Weather->RainGroundImpactStreaks->GetInstanceCount(), 3);
	TestEqual(TEXT("A strong shower creates one small ground splash from a blocking surface"), Weather->ActiveRainGroundImpactCount, 3);
	TestTrue(TEXT("The ground splash is placed on the collision surface"), FMath::IsNearlyEqual(Weather->LastRainGroundImpactLocation.Z, -10.f, 1.f));
	Weather->UpdateRainGroundResponse(Weather->GetActorLocation(), nullptr, World->GetTimeSeconds() + 0.2);
	FTransform SplashTransform;
	Weather->RainGroundImpactStreaks->GetInstanceTransform(0, SplashTransform, false);
	TestTrue(TEXT("Ground splash streaks animate above the collision surface during their brief lifetime"), Weather->RainGroundImpactStreaks->IsVisible() && SplashTransform.GetScale3D().Z > 0.f);
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
		TestTrue(TEXT("Automatic rain glints remain soft beside a deliberate pool interaction"), RainRipple->PeakLightIntensity <= 0.75f && RainRipple->SurfaceRadius < 150.f && RainRipple->DurationSeconds < 1.6f);
		int32 ActorsBeforeRepeat = 0;
		for (TActorIterator<AActor> It(World); It; ++It) ++ActorsBeforeRepeat;
		Weather->UpdateRainRendering();
		int32 ActorsAfterRepeat = 0;
		for (TActorIterator<AActor> It(World); It; ++It) ++ActorsAfterRepeat;
		TestEqual(TEXT("Repeated rain updates reuse the bounded instance pool without spawning per-drop actors"), ActorsAfterRepeat, ActorsBeforeRepeat);
		RainRippleCount = 0;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) if (It->ActorHasTag(TEXT("RainImpact"))) ++RainRippleCount;
		TestEqual(TEXT("Repeated weather updates do not stack pool impacts"), RainRippleCount, 1);
	}
	Weather->WeatherSeed = DryWeatherSeed;
	Weather->UpdateRainRendering();
	TestTrue(TEXT("Dry weather hides and clears the transient ground splash"), Weather->ActiveRainGroundImpactCount == 0 && !Weather->RainGroundImpactStreaks->IsVisible());
	TestEqual(TEXT("Rain reuses the same fixed-size ground splash pool"), Weather->RainGroundImpactStreaks->GetInstanceCount(), 3);
	RainRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It) if (It->ActorHasTag(TEXT("RainImpact"))) ++RainRippleCount;
	TestEqual(TEXT("Dry conditions do not create additional water impacts"), RainRippleCount, 1);
	Weather->WeatherSeed = OriginalWeatherSeed;

	// Camera-centred rain should not render at a point proven to be inside a tagged roofed room.
	const FVector WeatherHome = Weather->GetActorLocation();
	const FVector InnCentre(4000.f, 4000.f, 0.f);
	auto SpawnInnPart = [World, &Spawn, &InnCentre](const FVector& Relative, const FVector& Extent)
	{
		AActor* Part = World->SpawnActor<AActor>(InnCentre + Relative, FRotator::ZeroRotator, Spawn);
		if (!Part) return static_cast<AActor*>(nullptr);
		Part->Tags.Add(TEXT("IslandInn"));
		UBoxComponent* Box = NewObject<UBoxComponent>(Part);
		Part->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Part->SetActorLocation(InnCentre + Relative, false, nullptr, ETeleportType::TeleportPhysics);
		return Part;
	};
	SpawnInnPart(FVector(450.f, 0.f, 200.f), FVector(20.f, 500.f, 200.f));
	SpawnInnPart(FVector(-450.f, 0.f, 200.f), FVector(20.f, 500.f, 200.f));
	SpawnInnPart(FVector(0.f, 450.f, 200.f), FVector(500.f, 20.f, 200.f));
	SpawnInnPart(FVector(0.f, -450.f, 200.f), FVector(500.f, 20.f, 200.f));
	SpawnInnPart(FVector(0.f, 0.f, 500.f), FVector(500.f, 500.f, 20.f));
	Weather->SetActorLocation(InnCentre);
	Weather->WeatherSeed = StormSeed;
	if (Weather->RainPoolRipple.IsValid()) Weather->RainPoolRipple->Destroy();
	Weather->RainPoolRipple.Reset();
	Weather->NextRainPoolRippleTime = 0.0;
	Weather->UpdateRainRendering();
	TestTrue(TEXT("A heavy shower is still active under the inn roof"), Weather->CurrentRainIntensity > 0.55f);
	TestTrue(TEXT("A verified indoor point suppresses its camera-centred rain streaks"), Weather->ActiveRainStreakCount == 0 && !Weather->RainStreaks->IsVisible());
	TestTrue(TEXT("A verified indoor point suppresses roof-local ground splash visuals"), Weather->ActiveRainGroundImpactCount == 0 && !Weather->RainGroundImpactStreaks->IsVisible());
	TestTrue(TEXT("Island-wide rain still creates a Tideglass ripple while the listener is sheltered"),
		Weather->RainPoolRipple.IsValid() && FVector::Dist2D(Weather->RainPoolRipple->GetActorLocation(), Habitat->GetActorLocation()) < 60.f);
	Weather->SetActorLocation(WeatherHome);
	Weather->UpdateRainRendering();
	TestTrue(TEXT("Moving back outdoors restores the local rain field during the same shower"), Weather->ActiveRainStreakCount > 0 && Weather->RainStreaks->IsVisible());
	Weather->WeatherSeed = OriginalWeatherSeed;
	
	Clock->CurrentHour = 12.f;
	Weather->RefreshNightEcology();
	int32 Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("No fireflies are active at midday"), Population, 0);
	int32 CrabPopulation = 0;
	TArray<TWeakObjectPtr<AIslandTidepoolCrab>> DayCrabResidents;
	for (TActorIterator<AIslandTidepoolCrab> It(World); It; ++It)
	{
		// This lightweight fixture does not begin the whole world (which would
		// also scatter the full landscape); exercise each resident's real startup.
		if (!It->HasActorBegunPlay()) It->DispatchBeginPlay();
		++CrabPopulation;
		DayCrabResidents.Add(*It);
		TestTrue(TEXT("Tidepool crab advertises as untargeted ambient life"), It->ActorHasTag(TEXT("IslandLife")) && It->ActorHasTag(TEXT("TidepoolCrab")) && !It->ActorHasTag(TEXT("IslandLandmark")));
		TestTrue(TEXT("Tidepool crab remains at the shoreline of its habitat"), FVector::Dist2D(It->GetActorLocation(), Habitat->GetActorLocation()) < 1000.f);
		TestEqual(TEXT("Tidepool crab has six visible leg placeholders"), It->Legs.Num(), 6);
		TestEqual(TEXT("Tidepool crab has two claws and two eye stalks"), It->Claws.Num(), 2);
		TestTrue(TEXT("Tidepool crab carapace remains visibly flatter than it is wide"), It->Shell && It->Shell->GetRelativeScale3D().Z < It->Shell->GetRelativeScale3D().X * 0.5f);
		UMaterialInstanceDynamic* CrabShellTint = It->Shell ? Cast<UMaterialInstanceDynamic>(It->Shell->GetMaterial(0)) : nullptr;
		TestNotNull(TEXT("Tidepool crab shell uses its per-instance natural tint"), CrabShellTint);
		if (CrabShellTint)
		{
			const FLinearColor ShellColor = CrabShellTint->K2_GetVectorParameterValue(TEXT("Color"));
			TestTrue(TEXT("Tidepool crab shell keeps a warm dark rust palette"), ShellColor.R > ShellColor.G && ShellColor.G > ShellColor.B && ShellColor.R < 0.2f);
		}
		TestEqual(TEXT("Tidepool crab geometry cannot block the world"), It->Shell->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		if (DayCrabResidents.Num() == 1)
		{
			// Hold world time, weather, and the crab's idle phase constant while
			// sampling opposite extrema of the same Island clock's lunar tide.
			const FVector CrabHome = It->HomeLocation;
			It->ScurryRemaining = 0.f;
			It->RippleCheckRemaining = 1.f;
			It->RavenCheckRemaining = 1.f;
			Clock->DayNumber = 1;
			Clock->CurrentHour = UIslandTideglassSubsystem::TidalDayHours * 0.75f;
			It->SetActorLocation(CrabHome);
			It->Tick(0.f);
			const float EbbingDrift = FVector::Dist2D(CrabHome, It->GetActorLocation());
			Clock->CurrentHour = UIslandTideglassSubsystem::TidalDayHours * 0.25f;
			It->SetActorLocation(CrabHome);
			It->Tick(0.f);
			const float HighWaterDrift = FVector::Dist2D(CrabHome, It->GetActorLocation());
			TestTrue(TEXT("A crab forages more broadly at low tide than high tide"), EbbingDrift > HighWaterDrift + 1.f);
			TestTrue(TEXT("Low tide movement changes only idle drift, not the crab's shoreline home"), CrabHome.Equals(It->HomeLocation));
			Clock->CurrentHour = 12.f;
		}
	}
	TestEqual(TEXT("A small bounded crab population is active by day"), CrabPopulation, 2);
	int32 DragonflyPopulation = 0;
	TArray<FLinearColor> DragonflyColors;
	const FLinearColor ExpectedDragonflyWingColors[] = {
		FLinearColor(0.18f, 0.30f, 0.20f, 0.44f),
		FLinearColor(0.18f, 0.31f, 0.36f, 0.44f),
		FLinearColor(0.36f, 0.24f, 0.12f, 0.44f)
	};
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It)
	{
		++DragonflyPopulation;
		TestTrue(TEXT("Dragonfly is wild ambient life, not a landmark"), It->ActorHasTag(TEXT("IslandLife")) && It->ActorHasTag(TEXT("TideglassDragonfly")) && !It->ActorHasTag(TEXT("IslandLandmark")));
		TestTrue(TEXT("Day dragonfly patrol stays near and above Tideglass"), FVector::Dist2D(It->GetActorLocation(), Habitat->GetActorLocation()) < 700.f && It->GetActorLocation().Z > Habitat->GetActorLocation().Z + 100.f);
		TestEqual(TEXT("Dragonfly has a distinct four-wing silhouette"), It->Wings.Num(), 4);
		TestTrue(TEXT("Dragonfly has head, thorax, elongated abdomen, and paired eyes"),
			It->Head && It->Thorax && It->Abdomen && It->Eyes.Num() == 2);
		UMaterialInstanceDynamic* EyeMaterial = It->EyeMaterial.Get();
		TestNotNull(TEXT("Dragonfly compound eyes use a distinct dark material"), EyeMaterial);
		if (EyeMaterial)
		{
			const FLinearColor EyeColor = EyeMaterial->K2_GetVectorParameterValue(TEXT("Color"));
			TestTrue(TEXT("Compound eyes remain visibly darker than the natural body morph"),
				EyeColor.R < 0.04f && EyeColor.G < 0.05f && EyeColor.B < 0.04f);
		}
		for (UStaticMeshComponent* Eye : It->Eyes)
			TestTrue(TEXT("Each compound eye is cosmetic and cannot block movement or navigation"),
				Eye && Eye->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
				!Eye->CastShadow && !Eye->CanEverAffectNavigation());
		UMaterialInstanceDynamic* BodyMaterial = It->BodyMaterial.Get();
		TestNotNull(TEXT("Dragonfly has a per-instance natural body color"), BodyMaterial);
		if (BodyMaterial) DragonflyColors.Add(BodyMaterial->K2_GetVectorParameterValue(TEXT("Color")));
		const FLinearColor ExpectedWingColor = ExpectedDragonflyWingColors[
			FMath::Clamp(It->ColorVariant, 0, UE_ARRAY_COUNT(ExpectedDragonflyWingColors) - 1)];
		for (UProceduralMeshComponent* Wing : It->Wings)
		{
			const FProcMeshSection* Section = Wing ? Wing->GetProcMeshSection(0) : nullptr;
			TestTrue(TEXT("Each dragonfly wing is a generated two-sided surface"),
				Wing && Wing->GetNumSections() == 1 && Section &&
				Section->ProcVertexBuffer.Num() >= 100 && Section->ProcIndexBuffer.Num() >= 200);
			const FColor WingTint = Section && Section->ProcVertexBuffer.Num() > 0
				? Section->ProcVertexBuffer[0].Color : FColor::Black;
			TestTrue(TEXT("Wing surfaces retain a tinted translucent vertex color"),
				Wing && Wing->GetMaterial(0) && Wing->GetMaterial(0)->GetBlendMode() == BLEND_Translucent &&
				WingTint.R + WingTint.G + WingTint.B > 0 && WingTint.A < 128);
			UMaterialInstanceDynamic* WingMaterial = Wing
				? Cast<UMaterialInstanceDynamic>(Wing->GetMaterial(0)) : nullptr;
			TestTrue(TEXT("Each translucent wing material receives the dragonfly's natural color morph"),
				WingMaterial && WingMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(ExpectedWingColor, 0.01f));
			bool bFiniteWingGeometry = Section && !Section->SectionLocalBox.ContainsNaN();
			if (Section)
				for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
					bFiniteWingGeometry &= FMath::IsFinite(Vertex.Position.X) && FMath::IsFinite(Vertex.Position.Y) && FMath::IsFinite(Vertex.Position.Z);
			TestTrue(TEXT("Generated wing bounds and vertices remain finite at the tapered tips"), bFiniteWingGeometry);
			TestTrue(TEXT("Generated wings are collisionless, shadowless, and excluded from navigation"),
				Wing && Wing->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Wing->CastShadow &&
				!Wing->CanEverAffectNavigation());
		}
		const FProcMeshSection* ForewingSection = It->Wings.IsValidIndex(0) && It->Wings[0]
			? It->Wings[0]->GetProcMeshSection(0) : nullptr;
		const FProcMeshSection* HindwingSection = It->Wings.IsValidIndex(2) && It->Wings[2]
			? It->Wings[2]->GetProcMeshSection(0) : nullptr;
		TestTrue(TEXT("Hindwings have a broader, longer silhouette than forewings"),
			ForewingSection && HindwingSection &&
			HindwingSection->SectionLocalBox.GetSize().X > ForewingSection->SectionLocalBox.GetSize().X * 1.25f &&
			HindwingSection->SectionLocalBox.GetSize().Y > ForewingSection->SectionLocalBox.GetSize().Y * 1.1f);
	}
	TestEqual(TEXT("Three daytime dragonflies form a small bounded population"), DragonflyPopulation, 3);
	int32 DistinctDragonflyColorCount = 0;
	for (int32 Index = 0; Index < DragonflyColors.Num(); ++Index)
	{
		bool bColorSeen = false;
		for (int32 PriorIndex = 0; PriorIndex < Index; ++PriorIndex)
			if (DragonflyColors[Index].Equals(DragonflyColors[PriorIndex], 0.001f)) { bColorSeen = true; break; }
		if (!bColorSeen) ++DistinctDragonflyColorCount;
	}
	TestEqual(TEXT("Each dragonfly receives a distinct stable natural color morph"), DistinctDragonflyColorCount, 3);
	AIslandTidepoolCrab* RainSensitiveCrab = World->SpawnActor<AIslandTidepoolCrab>(FVector(3500.f, 5000.f, 600.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Shore crab weather-response tester spawned"), RainSensitiveCrab))
	{
		RainSensitiveCrab->Weather = Weather;
		RainSensitiveCrab->HomeLocation = RainSensitiveCrab->GetActorLocation();
		RainSensitiveCrab->Phase = 1.1f;
		const FVector CrabHome = RainSensitiveCrab->HomeLocation;
		const float CrabOriginalWindSpeed = Weather->MaximumWindSpeed;
		Weather->MaximumWindSpeed = 0.f;
		Weather->WeatherSeed = DryWeatherSeed;
		RainSensitiveCrab->Tick(0.f);
		const float DryRoamingDistance = FVector::Dist2D(CrabHome, RainSensitiveCrab->GetActorLocation());
		RainSensitiveCrab->SetActorLocation(CrabHome);
		Weather->WeatherSeed = StormSeed;
		RainSensitiveCrab->Tick(0.f);
		const float RainRoamingDistance = FVector::Dist2D(CrabHome, RainSensitiveCrab->GetActorLocation());
		TestTrue(TEXT("The same crab contracts its idle roaming during a strong shower"), RainRoamingDistance < DryRoamingDistance);
		Weather->MaximumWindSpeed = CrabOriginalWindSpeed;
		Weather->WeatherSeed = OriginalWeatherSeed;
		RainSensitiveCrab->Destroy();
	}
	Clock->CurrentHour = 18.f;
	Weather->RefreshNightEcology();
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Fireflies wait until nightfall"), Population, 0);
	CrabPopulation = 0;
	for (TActorIterator<AIslandTidepoolCrab> It(World); It; ++It) ++CrabPopulation;
	TestEqual(TEXT("Shore crabs remain active in the late afternoon"), CrabPopulation, 2);
	DragonflyPopulation = 0;
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It) ++DragonflyPopulation;
	TestEqual(TEXT("Dragonflies remain active in late afternoon"), DragonflyPopulation, 3);

	Clock->CurrentHour = 20.f;
	Weather->RefreshNightEcology();
	int32 StoneSideFireflies = 0;
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It)
	{
		++Population;
		TestTrue(TEXT("Firefly advertises as ambient life"), It->ActorHasTag(TEXT("IslandLife")));
		TestTrue(TEXT("Firefly remains untargeted wildlife"), !It->ActorHasTag(TEXT("IslandLandmark")) && !It->ActorHasTag(TEXT("RavenNestSite")));
		const float PoolDistance = FVector::Dist2D(It->GetActorLocation(), Habitat->GetActorLocation());
		const float StonesDistance = FVector::Dist(It->GetActorLocation(), NightStonesHabitat->GetActorLocation());
		AddInfo(FString::Printf(TEXT("Night firefly %s is %.1f cm from Tideglass and %.1f cm from ListeningStones"), *It->GetName(), PoolDistance, StonesDistance));
		TestTrue(TEXT("Night fireflies stay at Tideglass or within the stones' existing sound radius"),
			PoolDistance < 700.f || StonesDistance <= AIslandListeningStonesChime::AudibleRadius);
		if (StonesDistance <= AIslandListeningStonesChime::AudibleRadius) ++StoneSideFireflies;
		UPointLightComponent* FireflyGlow = It->FindComponentByClass<UPointLightComponent>();
		TestNotNull(TEXT("Firefly has a fluctuating glow component"), FireflyGlow);
		TestTrue(TEXT("The visible body is large enough to read at the ListeningStones viewpoint"),
			It->GlowingBody && It->GlowingBody->GetRelativeScale3D().X >= 0.07f);
		TestNotNull(TEXT("Firefly has a per-instance emissive material"), It->GlowMaterial.Get());
		if (It->GlowMaterial)
		{
			const FLinearColor Emission = It->GlowMaterial->K2_GetVectorParameterValue(TEXT("Color"));
			TestTrue(TEXT("The emissive body keeps the characteristic yellow-green firefly hue"),
				Emission.G > Emission.R * 2.f && Emission.G > Emission.B * 2.f);
		}
		TestTrue(TEXT("The firefly glow reaches nearby ground without changing the night sky fill"),
			It->GlowIntensity >= 50.f && FireflyGlow && FireflyGlow->AttenuationRadius >= 400.f);
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
	DragonflyPopulation = 0;
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It) ++DragonflyPopulation;
	TestEqual(TEXT("The daytime dragonfly population leaves at dusk"), DragonflyPopulation, 0);
	TestEqual(TEXT("One independently spawned night firefly inhabits the route within chime range"), StoneSideFireflies, 1);
	NightStonesHabitat->Tags.Remove(TEXT("ListeningStones"));
	NightStonesHabitat->Tags.Remove(TEXT("IslandLandmark"));
	CrabPopulation = 0;
	for (TActorIterator<AIslandTidepoolCrab> It(World); It; ++It)
	{
		++CrabPopulation;
		TestTrue(TEXT("Night-sheltered crabs remain as hidden residents"), It->IsSheltered() && It->IsHidden());
		TestFalse(TEXT("Night-sheltered crabs pause their local routine"), It->IsActorTickEnabled());
	}
	TestEqual(TEXT("The same two shore-crab residents persist through nightfall"), CrabPopulation, 2);
	if (DayCrabResidents.Num() > 0 && DayCrabResidents[0].IsValid())
	{
		ACharacter* NightObserver = World->SpawnActor<ACharacter>(DayCrabResidents[0]->GetActorLocation() + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
		ARavenAgentAIController* NightController = World->SpawnActor<ARavenAgentAIController>(Spawn);
		if (TestNotNull(TEXT("Night observer spawned to check concealed wildlife"), NightObserver) && TestNotNull(TEXT("Night inspection controller spawned"), NightController))
		{
			NightController->Possess(NightObserver);
			NightController->InspectTarget(TEXT("TidepoolCrab"));
			TestTrue(TEXT("Residents cannot target a sheltered crab"), NightController->DescribeActionState().Contains(TEXT("No shore crab is close enough")));
			NightController->UnPossess();
		}
		if (NightObserver) NightObserver->Destroy();
		if (NightController) NightController->Destroy();
	}
	if (Population > 0)
	{
		AIslandFirefly* RainSensitiveFirefly = nullptr;
		for (TActorIterator<AIslandFirefly> It(World); It; ++It) { RainSensitiveFirefly = *It; break; }
		if (RainSensitiveFirefly)
		{
			// This isolated fixture calls ecology refresh directly without starting world play.
			RainSensitiveFirefly->Weather = Weather;
			RainSensitiveFirefly->DayNight = Clock;
			RainSensitiveFirefly->HomeLocation = RainSensitiveFirefly->GetActorLocation();
			RainSensitiveFirefly->Phase = 1.1f;
			const float OriginalWindSpeed = Weather->MaximumWindSpeed;
			const int32 RainTestWeatherSeed = Weather->WeatherSeed;
			Weather->MaximumWindSpeed = 0.f;
			Weather->WeatherSeed = DryWeatherSeed;
			RainSensitiveFirefly->Tick(0.f);
			const float DryWanderRadius = FVector::Dist2D(RainSensitiveFirefly->HomeLocation, RainSensitiveFirefly->GetActorLocation());
			Weather->WeatherSeed = StormSeed;
			RainSensitiveFirefly->Tick(0.f);
			const float RainWanderRadius = FVector::Dist2D(RainSensitiveFirefly->HomeLocation, RainSensitiveFirefly->GetActorLocation());
			TestTrue(TEXT("A lived firefly contracts its lateral drift during a strong shower"), RainWanderRadius < DryWanderRadius);
			Clock->CurrentHour = 20.f;
			Clock->DayNumber = 1;
			RainSensitiveFirefly->UpdateGlow(1.37, 0.f);
			const float NewMoonGlow = RainSensitiveFirefly->Glow->Intensity;
			Clock->DayNumber = 15;
			RainSensitiveFirefly->UpdateGlow(1.37, 0.f);
			TestTrue(TEXT("The same firefly softens its pulse under a full moon"), RainSensitiveFirefly->Glow->Intensity < NewMoonGlow);
			Clock->DayNumber = 1;
			RainSensitiveFirefly->UpdateGlow(1.37, 0.f);
			const float ClearGlowAtFixedPhase = RainSensitiveFirefly->Glow->Intensity;
			RainSensitiveFirefly->UpdateGlow(1.37, 1.f);
			TestTrue(TEXT("The same natural pulse is visibly dimmer in heavy rain"), RainSensitiveFirefly->Glow->Intensity < ClearGlowAtFixedPhase);
			RainSensitiveFirefly->UpdateGlow(World->GetTimeSeconds(), Weather->SampleRainIntensity(World->GetTimeSeconds()));
			Weather->MaximumWindSpeed = OriginalWindSpeed;
			Weather->WeatherSeed = RainTestWeatherSeed;
		}
	}
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Repeated night refresh does not duplicate the population"), Population, 3);

	Clock->CurrentHour = 12.f;
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Fireflies leave the habitat in daytime"), Population, 0);
	CrabPopulation = 0;
	for (TActorIterator<AIslandTidepoolCrab> It(World); It; ++It)
	{
		++CrabPopulation;
		TestFalse(TEXT("Daylight reveals each resident from shelter"), It->IsSheltered() || It->IsHidden());
		TestTrue(TEXT("Daylight resumes each resident's local routine"), It->IsActorTickEnabled());
	}
	TestEqual(TEXT("The same shore-crab residents re-emerge at midday"), CrabPopulation, 2);
	DragonflyPopulation = 0;
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It) ++DragonflyPopulation;
	TestEqual(TEXT("Three dragonflies return when daytime resumes"), DragonflyPopulation, 3);
	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabResidents)
		TestTrue(TEXT("Each original crab actor survives and resumes in place"), Crab.IsValid() && !Crab->IsSheltered());

	Clock->CurrentHour = 5.5f;
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Fireflies leave during the twilight transition"), Population, 0);
	DragonflyPopulation = 0;
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It) ++DragonflyPopulation;
	TestEqual(TEXT("Dragonflies do not appear before dawn"), DragonflyPopulation, 0);
	for (TActorIterator<AIslandTidepoolCrab> It(World); It; ++It)
		TestTrue(TEXT("Crabs remain concealed before their 06:00 emergence"), It->IsSheltered());
	Clock->CurrentHour = 6.f;
	Weather->RefreshNightEcology();
	DragonflyPopulation = 0;
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It) ++DragonflyPopulation;
	TestEqual(TEXT("Three dragonflies return at the 06:00 daylight boundary"), DragonflyPopulation, 3);
	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabResidents)
		TestTrue(TEXT("Crabs re-emerge at 06:00 as their original residents"), Crab.IsValid() && !Crab->IsSheltered());

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
		TestEqual(TEXT("Deliberate inspection keeps its stronger expressive ripple"), Ripple->PeakLightIntensity, 55.f);
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
		TestEqual(TEXT("Each WindArch mote has its own emissive presentation material"), Motes->MoteMaterials.Num(), 3);
		for (int32 Index = 0; Index < Motes->MoteMeshes.Num(); ++Index)
		{
			const UStaticMeshComponent* Mesh = Motes->MoteMeshes[Index];
			const UPointLightComponent* Light = Motes->MoteLights.IsValidIndex(Index) ? Motes->MoteLights[Index] : nullptr;
			TestTrue(FString::Printf(TEXT("Wind mote %d is scaled for overlook readability"), Index),
				Mesh && Mesh->GetRelativeScale3D().GetMin() >= 0.11f);
			TestTrue(FString::Printf(TEXT("Wind mote %d uses an emissive material"), Index),
				Mesh && Mesh->GetMaterial(0) && Mesh->GetMaterial(0)->IsA<UMaterialInstanceDynamic>());
			TestTrue(FString::Printf(TEXT("Wind mote %d has a broad, shadow-free local glow"), Index),
				Light && !Light->CastShadows && Light->AttenuationRadius >= 500.f && Light->Intensity > 100.f);
		}
		TestTrue(TEXT("Wind motes follow the normalized simulated gust direction"), Motes->FlowDirection.IsNormalized());
		const FVector StartingPosition = Motes->MoteMeshes[0]->GetRelativeLocation();
		Motes->Tick(2.f);
		const FVector Displacement = Motes->MoteMeshes[0]->GetRelativeLocation() - StartingPosition;
		TestTrue(TEXT("Airflow motes advance along the actual gust direction"), FVector::DotProduct(Displacement, Motes->FlowDirection) > 0.f);
		Motes->Tick(17.f);
		TestTrue(TEXT("WindArch visual response disappears when the gust expires"), Motes->IsActorBeingDestroyed());
	}
	StonesTarget->Tags = {TEXT("ListeningStones"), TEXT("IslandLandmark")};
	TestEqual(TEXT("Calm air keeps the original stone pitch"), AIslandListeningStonesChime::CalculateWindPitchRatio(0.f), 1.f);
	TestTrue(TEXT("A light wind subtly lifts the stone pitch"), AIslandListeningStonesChime::CalculateWindPitchRatio(90.f) > 1.f);
	TestTrue(TEXT("Stronger wind raises the pitch more than lighter wind"), AIslandListeningStonesChime::CalculateWindPitchRatio(240.f) > AIslandListeningStonesChime::CalculateWindPitchRatio(90.f));
	TestTrue(TEXT("Even maximum wind keeps the resonance within one and a half semitones"), AIslandListeningStonesChime::CalculateWindPitchRatio(900.f) <= FMath::Pow(2.f, 1.5f / 12.f));
	TestEqual(TEXT("Non-finite wind falls back to the calm-air pitch"), AIslandListeningStonesChime::CalculateWindPitchRatio(std::numeric_limits<float>::quiet_NaN()), 1.f);
	AIslandFirefly* VisibleStoneFirefly = World->SpawnActor<AIslandFirefly>(TestPoolLocation + FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	AIslandFirefly* HiddenStoneFirefly = World->SpawnActor<AIslandFirefly>(TestPoolLocation + FVector(650.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	TestNotNull(TEXT("Nearby night-life fixture spawned within the chime radius"), VisibleStoneFirefly);
	TestNotNull(TEXT("Occluded night-life fixture spawned within the chime radius"), HiddenStoneFirefly);
	if (HiddenStoneFirefly) HiddenStoneFirefly->SetActorHiddenInGame(true);
	Controller->InspectTarget(TEXT("ListeningStones"));
	TestTrue(TEXT("The inspecting resident is told when a nearby firefly's response was actually visible"),
		Controller->LastActionOutcome.Contains(TEXT("firefly answered with a small glow lift")));
	TestTrue(TEXT("A nearby visible firefly responds to the controller-triggered chime"),
		VisibleStoneFirefly && VisibleStoneFirefly->ChimeResponseRemaining > 0.f);
	TestTrue(TEXT("An occluded firefly still hears the local tone without revealing itself"),
		HiddenStoneFirefly && HiddenStoneFirefly->ChimeResponseRemaining > 0.f && HiddenStoneFirefly->IsHidden());
	AIslandListeningStonesChime* Chime = nullptr;
	for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) { Chime = *It; break; }
	AIslandFirefly* ChimeListener = nullptr;
	TestNotNull(TEXT("ListeningStones interaction creates a transient chime actor"), Chime);
	if (Chime)
	{
		TestNotNull(TEXT("Chime uses a procedural sound wave without external assets"), Chime->ChimeWave.Get());
		const float ExpectedWindSpeed = Weather->GetLocalWind(StonesTarget->GetActorLocation(), StonesTarget).Size2D();
		TestTrue(TEXT("Active fixture weather supplies a changing local wind sample"), ExpectedWindSpeed > 0.f);
		TestEqual(TEXT("Chime uses wind sampled at the ListeningStones"), Chime->SampledWindSpeed, ExpectedWindSpeed);
		TestEqual(TEXT("Chime waveform matches the local wind pitch sample"), Chime->AppliedPitchRatio,
			AIslandListeningStonesChime::CalculateWindPitchRatio(Chime->SampledWindSpeed));
		if (Chime->ChimeWave)
		{
			TestEqual(TEXT("Chime uses mono audio"), Chime->ChimeWave->NumChannels, 1);
			TestTrue(TEXT("Chime declares a finite playback duration"), FMath::IsNearlyEqual(Chime->ChimeWave->GetDuration(), 2.8f));
			TestTrue(TEXT("Chime queues a finite PCM signal"), Chime->ChimeWave->GetAvailableAudioByteCount() >= 24000 * 2);
		}
		TestTrue(TEXT("Chime is spatially attenuated around the landmark"), Chime->AudioComponent->bOverrideAttenuation && Chime->AudioComponent->AttenuationOverrides.bSpatialize);
		TestEqual(TEXT("The firefly reaction uses the same inner-plus-falloff range as the chime audio"),
			AIslandListeningStonesChime::AudibleRadius,
			AIslandListeningStonesChime::AttenuationInnerRadius + AIslandListeningStonesChime::AttenuationFalloffDistance);
		ChimeListener = World->SpawnActor<AIslandFirefly>(
			StonesTarget->GetActorLocation() + FVector(AIslandListeningStonesChime::AudibleRadius + 100.f, 0.f, 0.f),
			FRotator::ZeroRotator, Spawn);
		TestNotNull(TEXT("A wild firefly fixture can sense the transient stone resonance"), ChimeListener);
		if (ChimeListener)
		{
			ChimeListener->CheckForNearbyStoneChime();
			TestTrue(TEXT("A firefly outside the chime's attenuation range does not react"),
				FMath::IsNearlyZero(ChimeListener->ChimeResponseRemaining));
			ChimeListener->SetActorLocation(StonesTarget->GetActorLocation() + FVector(800.f, 0.f, 0.f));
			ChimeListener->CheckForNearbyStoneChime();
			TestTrue(TEXT("A firefly inside the audible range receives one brief pulse"),
				ChimeListener->ChimeResponseRemaining > 0.f && ChimeListener->ChimeResponseRemaining <= 1.2f);
			TestTrue(TEXT("The firefly records only which transient sound it already answered"),
				ChimeListener->RespondedChimes.Contains(TWeakObjectPtr<AIslandListeningStonesChime>(Chime)));
			ChimeListener->Phase = 0.f;
			ChimeListener->GlowIntensity = 100.f;
			ChimeListener->ChimeResponseRemaining = 0.f;
			ChimeListener->UpdateGlow(0.0, 0.f);
			const float NaturalGlow = ChimeListener->Glow->Intensity;
			ChimeListener->ChimeResponseRemaining = 1.2f;
			ChimeListener->UpdateGlow(0.0, 0.f);
			TestTrue(TEXT("The chime response gives a dim phase a small visible glow lift"),
				ChimeListener->Glow->Intensity > NaturalGlow);
			ChimeListener->ChimeResponseRemaining = 0.f;
			ChimeListener->CheckForNearbyStoneChime();
			TestTrue(TEXT("One chime cannot repeatedly retrigger the same firefly"),
				FMath::IsNearlyZero(ChimeListener->ChimeResponseRemaining));
			ChimeListener->Tick(1.3f);
			TestTrue(TEXT("The glow response fades without changing persistent state"),
				FMath::IsNearlyZero(ChimeListener->ChimeResponseRemaining));
		}
		Controller->InspectTarget(TEXT("ListeningStones"));
		int32 ChimeCount = 0;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) ++ChimeCount;
		TestEqual(TEXT("Inspection cooldown prevents stacking chimes"), ChimeCount, 1);
		Chime->Tick(2.9f);
		TestTrue(TEXT("Generated sound actor stops ticking after playback while the resident context remains"),
			!Chime->IsActorTickEnabled() && !Chime->IsAudibleAt(Chime->GetActorLocation()) &&
			!Chime->DescribeForListener(Chime->GetActorLocation()).IsEmpty());
		if (ChimeListener)
		{
			ChimeListener->CheckForNearbyStoneChime();
			TestTrue(TEXT("Finished sound actors are pruned from the firefly's temporary response history"),
				ChimeListener->RespondedChimes.IsEmpty());
		}
	}
	if (VisibleStoneFirefly) VisibleStoneFirefly->SetActorHiddenInGame(true);
	if (ChimeListener) ChimeListener->SetActorHiddenInGame(true);
	AActor* OtherListener = World->SpawnActor<AActor>(TestPoolLocation + FVector(50.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	FString OccludedChimeFact;
	TestTrue(TEXT("A second nearby listener can ring the stones"),
		OtherListener && IslandInteractionUtility::Perform(OtherListener, StonesTarget, OccludedChimeFact));
	TestTrue(TEXT("A listening firefly reacts acoustically even behind visual cover"),
		HiddenStoneFirefly && HiddenStoneFirefly->ChimeResponseRemaining > 0.f);
	TestFalse(TEXT("The interaction result never claims an occluded firefly was seen"),
		OccludedChimeFact.Contains(TEXT("firefly answered with a small glow lift")));
	Controller->InspectTarget(TEXT("ListeningStones"));
	int32 ChimeCountAfterRepeatedControllerAction = 0;
	for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) ++ChimeCountAfterRepeatedControllerAction;
	TestEqual(TEXT("The controller's repeat cooldown prevents a third chime while two finite contexts coexist"),
		ChimeCountAfterRepeatedControllerAction, 2);
	AIslandFirefly* DistantFirefly = World->SpawnActor<AIslandFirefly>(TestPoolLocation + FVector(700.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	AIslandFirefly* OccludedFirefly = World->SpawnActor<AIslandFirefly>(TestPoolLocation + FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	AIslandFirefly* WatchableFirefly = World->SpawnActor<AIslandFirefly>(TestPoolLocation, FRotator::ZeroRotator, Spawn);
	AActor* WildlifeBlocker = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Spawn);
	UBoxComponent* WildlifeBlockerBox = WildlifeBlocker ? NewObject<UBoxComponent>(WildlifeBlocker) : nullptr;
	if (WildlifeBlockerBox)
	{
		WildlifeBlocker->SetRootComponent(WildlifeBlockerBox);
		WildlifeBlockerBox->SetBoxExtent(FVector(15.f, 100.f, 100.f));
		WildlifeBlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
		WildlifeBlockerBox->RegisterComponent();
		WildlifeBlocker->SetActorLocation(TestPoolLocation + FVector(100.f, 0.f, 0.f));
	}
	TestNotNull(TEXT("Distant firefly candidate spawned before close wildlife"), DistantFirefly);
	TestNotNull(TEXT("Occluded nearby firefly candidate spawned before the visible one"), OccludedFirefly);
	TestNotNull(TEXT("Nearby wild firefly spawned for observation interaction"), WatchableFirefly);
	TestNotNull(TEXT("Wildlife visibility blocker registered"), WildlifeBlockerBox);
	if (Observer && OccludedFirefly && WatchableFirefly)
	{
		TestFalse(TEXT("The first close firefly is genuinely occluded"), IslandInteractionUtility::CanInspect(Observer, OccludedFirefly));
		TestTrue(TEXT("A second close firefly has a clear observation line"), IslandInteractionUtility::CanInspect(Observer, WatchableFirefly));
	}
	if (WatchableFirefly)
	{
		Controller->InspectTarget(TEXT("Firefly"));
		TestTrue(TEXT("Quiet observation triggers only a brief glow accent"), WatchableFirefly->ObservationPulseRemaining > 0.f && WatchableFirefly->ObservationPulseRemaining <= 3.f);
		TestTrue(TEXT("A visible close firefly is selected past earlier distant and occluded individuals"),
			(!DistantFirefly || FMath::IsNearlyZero(DistantFirefly->ObservationPulseRemaining)) &&
			(!OccludedFirefly || FMath::IsNearlyZero(OccludedFirefly->ObservationPulseRemaining)));
		WatchableFirefly->Tick(1.f);
		TestTrue(TEXT("Firefly returns naturally toward its usual pulse"), WatchableFirefly->ObservationPulseRemaining > 0.f && WatchableFirefly->ObservationPulseRemaining < 2.1f);
		WatchableFirefly->Tick(2.f);
		TestTrue(TEXT("Observation accent expires without persistent state"), FMath::IsNearlyZero(WatchableFirefly->ObservationPulseRemaining));
	}
	if (WildlifeBlocker) WildlifeBlocker->Destroy();
	const FVector CrabStart = TestPoolLocation + FVector(100.f, 0.f, 0.f);
	AIslandTidepoolCrab* WatchableCrab = World->SpawnActor<AIslandTidepoolCrab>(CrabStart, FRotator::ZeroRotator, Spawn);
	TestNotNull(TEXT("Nearby independent shore crab spawned for a quiet observation"), WatchableCrab);
	if (WatchableCrab)
	{
		WatchableCrab->HomeLocation = CrabStart;
		// Keep the inspection-only scurry separate from the settled-presence cue
		// exercised below; the Raven remains at the observer location.
		Controller->LocomotionState = ERavenLocomotionState::Flying;
		Controller->InspectTarget(TEXT("TidepoolCrab"));
		TestTrue(TEXT("Quiet observation prompts a brief scurry, not capture"), WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
		TestTrue(*FString::Printf(TEXT("Crab scurry direction is set (%s)"), *WatchableCrab->ScurryDirection.ToString()), !WatchableCrab->ScurryDirection.IsNearlyZero());
		WatchableCrab->Tick(0.7f);
		const float ScurryDistance = FVector::Dist2D(WatchableCrab->GetActorLocation(), CrabStart);
		TestTrue(*FString::Printf(TEXT("Crab moves only a short distance toward cover (%.1f cm at %s)"), ScurryDistance, *WatchableCrab->GetActorLocation().ToString()), ScurryDistance > 1.f && ScurryDistance < 140.f);
		WatchableCrab->Tick(2.f);
		TestTrue(TEXT("Crab resumes its local idle path without a persistent state change"), FMath::IsNearlyZero(WatchableCrab->ScurryRemaining) && FVector::Dist2D(WatchableCrab->GetActorLocation(), CrabStart) < 100.f);

		Controller->LocomotionState = ERavenLocomotionState::Flying;
		const FVector LowPassLocation = CrabStart + FVector(200.f, 0.f, 260.f);
		Observer->SetActorLocation(LowPassLocation);
		WatchableCrab->SetActorLocation(CrabStart);
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A nearby low raven pass briefly startles the shore crab"),
			WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
		const FVector AwayFromRaven = (CrabStart - LowPassLocation).GetSafeNormal2D();
		TestTrue(TEXT("The shore crab scurries away from the passing raven"),
			FVector::DotProduct(WatchableCrab->ScurryDirection, AwayFromRaven) > 0.95f);
		TestEqual(TEXT("One low pass starts a short response cooldown"), WatchableCrab->RavenFlybyCooldownRemaining, 8.f);

		WatchableCrab->ScurryRemaining = 0.f;
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A hovering raven cannot retrigger the response during cooldown"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		WatchableCrab->RavenFlybyCooldownRemaining = 0.f;
		Observer->SetActorLocation(CrabStart + FVector(0.f, 0.f, 900.f));
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A high raven flight does not startle a shore crab"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		Observer->SetActorLocation(CrabStart + FVector(800.f, 0.f, 260.f));
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A low but distant flight does not startle a shore crab"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));

		Controller->LocomotionState = ERavenLocomotionState::Perched;
		const FVector PerchedRavenLocation = CrabStart + FVector(-200.f, 0.f, 180.f);
		Observer->SetActorLocation(PerchedRavenLocation);
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A nearby perched raven makes the shore crab scurry briefly"),
			WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
		TestTrue(TEXT("The shore crab moves away from a settled raven"),
			FVector::DotProduct(WatchableCrab->ScurryDirection, (CrabStart - PerchedRavenLocation).GetSafeNormal2D()) > 0.95f);
		WatchableCrab->ScurryRemaining = 0.f;
		WatchableCrab->RavenFlybyCooldownRemaining = 0.f;
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A lingering perched raven does not repeatedly startle the same crab"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		Controller->LocomotionState = ERavenLocomotionState::Flying;
		Observer->SetActorLocation(CrabStart + FVector(0.f, 0.f, 900.f));
		WatchableCrab->CheckForNearbyRavenDisturbance();
		Controller->LocomotionState = ERavenLocomotionState::Perched;
		Observer->SetActorLocation(PerchedRavenLocation);
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("Changing flight state without leaving the wider ring does not rearm settled presence"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		Observer->SetActorLocation(CrabStart + FVector(800.f, 0.f, 180.f));
		WatchableCrab->CheckForNearbyRavenDisturbance();
		Observer->SetActorLocation(PerchedRavenLocation);
		WatchableCrab->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("Leaving and approaching again rearms the brief Raven response"),
			WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
		WatchableCrab->ScurryRemaining = 0.f;
		WatchableCrab->RavenFlybyCooldownRemaining = 0.f;

		WatchableCrab->RippleResponseCooldownRemaining = 0.f;
		AIslandPoolRippleEffect* ShoreRipple = World->SpawnActor<AIslandPoolRippleEffect>(
			CrabStart + FVector(160.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
		TestNotNull(TEXT("A nearby untagged pool ripple is available for filtering"), ShoreRipple);
		if (ShoreRipple)
		{
			WatchableCrab->CheckForNearbyNaturalRipple();
			TestTrue(TEXT("Untyped visitor ripples do not startle shore crabs"),
				FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
			ShoreRipple->ConfigureAsWindImpact(110.f);
			WatchableCrab->CheckForNearbyNaturalRipple();
			const FVector AwayFromRipple = (CrabStart - ShoreRipple->GetActorLocation()).GetSafeNormal2D();
			TestTrue(TEXT("A nearby natural wind ripple prompts a brief shoreward scurry"),
				WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
			TestTrue(TEXT("The crab moves away from the natural ripple"),
				FVector::DotProduct(WatchableCrab->ScurryDirection, AwayFromRipple) > 0.95f);
			TestEqual(TEXT("One natural ripple starts a bounded eight-second response cooldown"),
				WatchableCrab->RippleResponseCooldownRemaining, 8.f);
			WatchableCrab->ScurryRemaining = 0.f;
			WatchableCrab->CheckForNearbyNaturalRipple();
			TestTrue(TEXT("The same nearby ripple cannot retrigger during cooldown"),
				FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
			ShoreRipple->Destroy();
		}

		WatchableCrab->RippleResponseCooldownRemaining = 0.f;
		AIslandPoolRippleEffect* MinnowSplash = World->SpawnActor<AIslandPoolRippleEffect>(
			CrabStart + FVector(160.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
		TestNotNull(TEXT("A nearby minnow breach is available for the shore-crab response"), MinnowSplash);
		if (MinnowSplash)
		{
			MinnowSplash->ConfigureAsMinnowImpact();
			WatchableCrab->CheckForNearbyNaturalRipple();
			const FVector TowardMinnowSplash = (MinnowSplash->GetActorLocation() - CrabStart).GetSafeNormal2D();
			TestTrue(TEXT("A nearby minnow splash draws a brief cautious crab step toward the water"),
				WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 1.8f &&
				FVector::DotProduct(WatchableCrab->ScurryDirection, TowardMinnowSplash) > 0.95f);
			TestEqual(TEXT("A minnow splash shares the bounded natural-cue cooldown"),
				WatchableCrab->RippleResponseCooldownRemaining, 8.f);
			WatchableCrab->ScurryRemaining = 0.f;
			WatchableCrab->CheckForNearbyNaturalRipple();
			TestTrue(TEXT("The same minnow splash cannot repeatedly pull the crab during cooldown"),
				FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
			MinnowSplash->Destroy();
		}

		WatchableCrab->RippleResponseCooldownRemaining = 0.f;
		AIslandPoolRippleEffect* DistantShoreRipple = World->SpawnActor<AIslandPoolRippleEffect>(
			CrabStart + FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
		TestNotNull(TEXT("A distant natural ripple is available for radius filtering"), DistantShoreRipple);
		if (DistantShoreRipple)
		{
			DistantShoreRipple->ConfigureAsRainImpact();
			WatchableCrab->CheckForNearbyNaturalRipple();
			TestTrue(TEXT("A distant rain ripple does not startle the shore crab"),
				FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
			DistantShoreRipple->SetActorLocation(CrabStart + FVector(180.f, 0.f, 0.f));
			WatchableCrab->RippleCheckRemaining = 0.36f;
			WatchableCrab->Tick(0.36f);
			TestTrue(TEXT("A nearby rain ripple also prompts a brief temporary response"),
				WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
			DistantShoreRipple->Destroy();
		}
		Controller->LocomotionState = ERavenLocomotionState::Grounded;
	}
	AIslandTideglassDragonfly* WatchableDragonfly = World->SpawnActor<AIslandTideglassDragonfly>(TestPoolLocation + FVector(100.f, 0.f, 160.f), FRotator::ZeroRotator, Spawn);
	TestNotNull(TEXT("A nearby wild dragonfly spawns for a quiet observation"), WatchableDragonfly);
	if (WatchableDragonfly)
	{
		WatchableDragonfly->HomeLocation = WatchableDragonfly->GetActorLocation();
		Observer->SetActorLocation(TestPoolLocation);
		Controller->InspectTarget(TEXT("TideglassDragonfly"));
		TestTrue(TEXT("Quiet inspection makes the dragonfly briefly dart away"), WatchableDragonfly->ScatterRemaining > 0.f && WatchableDragonfly->ScatterRemaining <= 1.6f);
		TestTrue(TEXT("The interaction target resolver exposes dragonflies to visitors and residents"), IslandInteractionUtility::GetTargetTag(WatchableDragonfly) == FName(TEXT("TideglassDragonfly")));
		FString DragonflyFact;
		TestTrue(TEXT("A direct quiet observation is accepted without an agent-only capability"), IslandInteractionUtility::Perform(Observer, WatchableDragonfly, DragonflyFact));
		TestTrue(TEXT("Observation preserves the dragonfly's wild, nonpersistent status"), DragonflyFact.Contains(TEXT("wild and independent")) && DragonflyFact.Contains(TEXT("nothing persistent changed")));
		TestTrue(TEXT("Repeated quiet attention only refreshes the short natural startle response"), WatchableDragonfly->ScatterRemaining > 0.f && WatchableDragonfly->ScatterRemaining <= 1.6f);
		WatchableDragonfly->Tick(2.f);
		TestTrue(TEXT("The dragonfly resumes its ordinary pool-side flight after the brief response"), FMath::IsNearlyZero(WatchableDragonfly->ScatterRemaining));

		const FVector DragonflyRippleTestHome = TestPoolLocation + FVector(12000.f, 0.f, 800.f);
		WatchableDragonfly->SetActorLocation(DragonflyRippleTestHome);
		WatchableDragonfly->HomeLocation = DragonflyRippleTestHome;
		AIslandPoolRippleEffect* VisitorRipple = World->SpawnActor<AIslandPoolRippleEffect>(
			WatchableDragonfly->GetActorLocation() + FVector(100.f, 0.f, -160.f), FRotator::ZeroRotator, Spawn);
		TestNotNull(TEXT("A nearby untagged pool ripple is available for filtering"), VisitorRipple);
		if (VisitorRipple)
		{
			WatchableDragonfly->CheckForNearbyNaturalSurfaceRipple();
			TestTrue(TEXT("Visitor ripples do not trigger the dragonfly's natural response"),
				FMath::IsNearlyZero(WatchableDragonfly->RippleInterestRemaining));
			VisitorRipple->ConfigureAsMinnowImpact();
			WatchableDragonfly->CheckForNearbyNaturalSurfaceRipple();
			TestTrue(TEXT("A nearby fish surface break draws a brief, temporary dragonfly interest response"),
				WatchableDragonfly->RippleInterestRemaining > 0.f && WatchableDragonfly->RippleInterestRemaining <= 1.8f);
			TestTrue(TEXT("The dragonfly aims just above the water surface"),
				FMath::IsNearlyEqual(WatchableDragonfly->RippleInterestLocation.Z, VisitorRipple->GetActorLocation().Z + 110.f));
			TestTrue(TEXT("The dragonfly hovers on a small station beside the exact ripple point"),
				FMath::IsNearlyEqual(FVector::Dist2D(WatchableDragonfly->RippleInterestLocation, VisitorRipple->GetActorLocation()), 42.f, 0.1f));
			TestEqual(TEXT("One natural ripple starts a bounded seven-second response cooldown"),
				WatchableDragonfly->RippleInterestCooldownRemaining, 7.f);
			TArray<AIslandTideglassDragonfly*> OtherMorphs;
			for (int32 Variant = 1; Variant < 3; ++Variant)
			{
				AIslandTideglassDragonfly* OtherDragonfly = World->SpawnActor<AIslandTideglassDragonfly>(
					WatchableDragonfly->GetActorLocation(), FRotator::ZeroRotator, Spawn);
				TestNotNull(TEXT("A second daytime morph is available to check non-stacking interest"), OtherDragonfly);
				if (!OtherDragonfly) continue;
				OtherDragonfly->SetColorVariant(Variant);
				TestTrue(TEXT("Another color morph can investigate the same nearby ripple"),
					OtherDragonfly->RespondToSurfaceRipple(VisitorRipple->GetActorLocation()));
				TestTrue(TEXT("Different dragonfly morphs keep separate hover stations around one ripple"),
					FVector::DistSquared2D(OtherDragonfly->RippleInterestLocation, WatchableDragonfly->RippleInterestLocation) > FMath::Square(60.f));
				OtherMorphs.Add(OtherDragonfly);
			}
			const FVector BeforeSwoop = WatchableDragonfly->GetActorLocation();
			WatchableDragonfly->Tick(0.35f);
			TestTrue(TEXT("The dragonfly begins curving toward the ripple rather than staying on its patrol point"),
				FVector::DistSquared(WatchableDragonfly->GetActorLocation(), WatchableDragonfly->RippleInterestLocation) <
				FVector::DistSquared(BeforeSwoop, WatchableDragonfly->RippleInterestLocation));
			const FVector FirstRippleTarget = WatchableDragonfly->RippleInterestLocation;
			VisitorRipple->SetActorLocation(WatchableDragonfly->GetActorLocation() + FVector(40.f, 0.f, -160.f));
			WatchableDragonfly->RippleInterestRemaining = 0.f;
			WatchableDragonfly->CheckForNearbyNaturalSurfaceRipple();
			TestTrue(TEXT("A second nearby ripple cannot retrigger during cooldown"),
				FMath::IsNearlyZero(WatchableDragonfly->RippleInterestRemaining));
			TestTrue(TEXT("Cooldown preserves the first ephemeral focus point"),
				WatchableDragonfly->RippleInterestLocation.Equals(FirstRippleTarget));
			for (AIslandTideglassDragonfly* OtherDragonfly : OtherMorphs)
				if (OtherDragonfly) OtherDragonfly->Destroy();
		}

		const FVector DragonflyLocation = WatchableDragonfly->GetActorLocation();
		const FVector LowFlybyLocation = DragonflyLocation + FVector(-200.f, 0.f, 250.f);
		Controller->LocomotionState = ERavenLocomotionState::Flying;
		Observer->SetActorLocation(LowFlybyLocation);
		WatchableDragonfly->CheckForLowRavenFlyby();
		TestTrue(TEXT("A nearby low raven pass triggers a short dragonfly dart"),
			WatchableDragonfly->ScatterRemaining > 0.f && WatchableDragonfly->ScatterRemaining <= 1.6f);
		const FVector AwayFromRaven = (DragonflyLocation - LowFlybyLocation).GetSafeNormal2D();
		TestTrue(TEXT("The dragonfly darts away from the low wing shadow"),
			FVector::DotProduct(WatchableDragonfly->ScatterDirection, AwayFromRaven) > 0.95f);
		TestEqual(TEXT("A close pass starts an eight-second response cooldown"), WatchableDragonfly->RavenFlybyCooldownRemaining, 8.f);

		WatchableDragonfly->ScatterRemaining = 0.f;
		WatchableDragonfly->CheckForLowRavenFlyby();
		TestTrue(TEXT("The same hovering raven cannot retrigger the dart during cooldown"), FMath::IsNearlyZero(WatchableDragonfly->ScatterRemaining));
		WatchableDragonfly->RavenFlybyCooldownRemaining = 0.f;
		Observer->SetActorLocation(DragonflyLocation + FVector(-200.f, 0.f, 700.f));
		WatchableDragonfly->CheckForLowRavenFlyby();
		TestTrue(TEXT("High raven flight stays outside the dragonfly's disturbance band"), FMath::IsNearlyZero(WatchableDragonfly->ScatterRemaining));
		Observer->SetActorLocation(DragonflyLocation + FVector(-600.f, 0.f, 250.f));
		WatchableDragonfly->CheckForLowRavenFlyby();
		TestTrue(TEXT("A low but distant raven does not disturb the dragonfly"), FMath::IsNearlyZero(WatchableDragonfly->ScatterRemaining));
		Controller->LocomotionState = ERavenLocomotionState::Perched;
		Observer->SetActorLocation(LowFlybyLocation);
		WatchableDragonfly->CheckForLowRavenFlyby();
		TestTrue(TEXT("A perched raven does not disturb the airborne dragonfly"), FMath::IsNearlyZero(WatchableDragonfly->ScatterRemaining));
		Controller->LocomotionState = ERavenLocomotionState::Flying;
		WatchableDragonfly->ScatterRemaining = 0.f;
		WatchableDragonfly->RavenFlybyCooldownRemaining = 0.f;
		WatchableDragonfly->RavenCheckRemaining = 0.f;
		Observer->SetActorLocation(LowFlybyLocation);
		WatchableDragonfly->Tick(0.36f);
		TestTrue(TEXT("Periodic dragonfly sensing notices a low raven pass without a direct trigger"),
			WatchableDragonfly->ScatterRemaining > 0.f && WatchableDragonfly->RavenFlybyCooldownRemaining > 0.f);
		Controller->LocomotionState = ERavenLocomotionState::Grounded;
		WatchableDragonfly->Destroy();
	}
	ACharacter* Visitor = World->SpawnActor<ACharacter>(TestPoolLocation + FVector(80.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	WindTarget->SetActorLocation(TestPoolLocation + FVector(220.f, 0.f, 0.f));
	StonesTarget->SetActorLocation(TestPoolLocation + FVector(300.f, 0.f, 0.f));
	PoolTarget->SetActorHiddenInGame(false);
	WindTarget->SetActorHiddenInGame(false);
	StonesTarget->SetActorHiddenInGame(false);
	if (TestNotNull(TEXT("Visitor spawned for target-selection checks"), Visitor) && WatchableCrab && WatchableFirefly)
	{
		FString VisitorFact;
		TestTrue(TEXT("A non-agent visitor can clearly reach the Tideglass landmark"), IslandInteractionUtility::CanInteract(Visitor, PoolTarget));
		TestTrue(TEXT("A non-agent visitor can clearly reach a nearby wild crab"), IslandInteractionUtility::CanInteract(Visitor, WatchableCrab));
		// Quiet observation may leave the crab anywhere within its short scurry radius. Put the test
		// candidates at controlled distances so this checks selection rather than random creature drift.
		WatchableCrab->SetActorLocation(TestPoolLocation + FVector(60.f, 0.f, 0.f));
		WatchableFirefly->SetActorLocation(TestPoolLocation + FVector(900.f, 0.f, 0.f));
		AActor* FirstTarget = IslandInteractionUtility::FindNearestVisibleTarget(Visitor, World);
		TestTrue(*FString::Printf(TEXT("Visitor input selects the nearest wild crab; selected %s (%s)"),
			FirstTarget ? *FirstTarget->GetName() : TEXT("none"), FirstTarget ? *IslandInteractionUtility::GetTargetTag(FirstTarget).ToString() : TEXT("no tag")),
			FirstTarget == WatchableCrab);
		WatchableCrab->SetActorHiddenInGame(true);
		TestTrue(TEXT("Hiding the nearest creature exposes the nearest visible landmark"), IslandInteractionUtility::FindNearestVisibleTarget(Visitor, World) == PoolTarget);
		PoolTarget->SetActorHiddenInGame(true);
		for (TActorIterator<AIslandFirefly> It(World); It; ++It) It->SetActorHiddenInGame(true);
		AActor* NextVisible = IslandInteractionUtility::FindNearestVisibleTarget(Visitor, World);
		TestTrue(*FString::Printf(TEXT("Hiding the nearest landmark exposes the next visible choice (selected %s, tag %s)"),
			NextVisible ? *NextVisible->GetName() : TEXT("none"), NextVisible ? *IslandInteractionUtility::GetTargetTag(NextVisible).ToString() : TEXT("none")),
			NextVisible == WindTarget);
		PoolTarget->SetActorHiddenInGame(false);
		WatchableCrab->SetActorHiddenInGame(false);
		TestTrue(TEXT("The shared landmark response accepts a human visitor without agent components"), IslandInteractionUtility::Perform(Visitor, PoolTarget, VisitorFact));
		TestTrue(TEXT("The visitor receives an honest transient-effect description"), VisitorFact.Contains(TEXT("no permanent level state")));

		WatchableCrab->SetActorHiddenInGame(true);
		UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
		if (TestNotNull(TEXT("Caption widget created for bound visitor input"), Caption))
		{
			Caption->SetVisibility(ESlateVisibility::Collapsed);
			AIslandInteractionTestPlayerController* VisitorController = World->SpawnActor<AIslandInteractionTestPlayerController>(Spawn);
			if (TestNotNull(TEXT("Local visitor controller spawned without a game session"), VisitorController))
			{
				VisitorController->SetFixturePawn(Visitor);
				VisitorController->SetCaptionWidget(Caption);
				VisitorController->BindFixtureInput();
				VisitorController->RefreshInteractionHintForTest();
				TestTrue(TEXT("Fixture controller satisfies the production local-player guard"), VisitorController->IsLocalPlayerController());
				TestTrue(TEXT("Fixture controller has the visitor pawn"), VisitorController->GetPawn() == Visitor);
				TestEqual(TEXT("A nearby visible landmark shows a discoverable, non-triggering visitor hint"),
					Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: send a brief ripple across the pool")));
				TestTrue(TEXT("E is bound to the local visitor interaction handler"), VisitorController->PressBoundE());
				const FString BoundCaption = Caption->GetDisplayedCaption().ToString();
				TestEqual(TEXT("The visitor hint immediately reflects the target cooldown"),
					Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: let that response settle")));
				TestTrue(*FString::Printf(TEXT("Bound E displays the selected landmark's factual transient response; caption was: %s"), *BoundCaption),
					BoundCaption.Contains(TEXT("TideglassPool: Your interaction sent")) &&
					BoundCaption.Contains(TEXT("changes no permanent level state")));
				TestTrue(TEXT("Visitor interaction caption becomes visible"), Caption->GetVisibility() == ESlateVisibility::HitTestInvisible);
				TestTrue(TEXT("An interaction caption can appear while the in-range hint remains visible"),
					!Caption->GetDisplayedCaption().IsEmpty() && !Caption->GetDisplayedInteractionHint().IsEmpty());
				Caption->HideCaption();
				TestTrue(TEXT("Hiding a transient caption does not erase the still-relevant visitor hint"),
					Caption->GetDisplayedCaption().IsEmpty() && !Caption->GetDisplayedInteractionHint().IsEmpty() &&
					Caption->GetVisibility() == ESlateVisibility::HitTestInvisible);
				TestTrue(TEXT("A repeated E press produces a cooldown caption instead of another effect"), VisitorController->PressBoundE() &&
					Caption->GetDisplayedCaption().ToString().Contains(TEXT("has already answered your attention")));
				VisitorController->Destroy();
			}
		}
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
			const double SavedNextCloudDiscoveryTime = MapWeather->NextCloudDiscoveryTime;
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
			// Editor-world time can remain nearly static between automation reruns. The prior run
			// restores the cloud references, but cloud discovery's one-second throttle is runtime-only
			// state; clear it so this test can deterministically recreate the transient material.
			MapWeather->NextCloudDiscoveryTime = 0.0;
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
			MapWeather->NextCloudDiscoveryTime = SavedNextCloudDiscoveryTime;
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
