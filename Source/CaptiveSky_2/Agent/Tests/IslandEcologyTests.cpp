#include "Misc/AutomationTest.h"
#include "CaptiveSky_2PlayerController.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "IslandInteractionTestPlayerController.h"
#include "IslandDayNight.h"
#include "IslandInteractionUtility.h"
#include "IslandFirefly.h"
#include "IslandTidepoolCrab.h"
#include "IslandPoolRippleEffect.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "IslandWindMoteEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
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
	const FVector SpeciesPatchProbe(-100500.f, 100500.f, 0.f);
	TestEqual(TEXT("Ground-cover species selection repeats for the same seed and world position"),
		AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe, 71), AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe, 71));
	TestEqual(TEXT("Nearby plants within one botanical patch share a species"),
		AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe, 71),
		AIslandWeather::SelectGroundCoverVariant(SpeciesPatchProbe + FVector(200.f, -300.f, 0.f), 71));
	int32 SpeciesCounts[9] = {};
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
	TestTrue(TEXT("Spatial patches use all six existing species with a reasonably balanced distribution"),
		MinimumSpeciesCells >= 2 && MaximumSpeciesCells <= 14);
	TestTrue(TEXT("Pool clearance, the inn roof filter, and hillside patches preserve varied cover within the 17,068-instance budget"),
		Weather->GroundCoverInstanceCount > 96 && Weather->GroundCoverInstanceCount <= 17068);
	const int32 FixturePlantCount = Weather->ShoreGroundPlants->GetInstanceCount() + Weather->ShoreGroundPlantLowA->GetInstanceCount() +
		Weather->ShoreGroundPlantLowB->GetInstanceCount();
	TestTrue(TEXT("Broadleaf ground plants make up roughly one-third of the fixed-budget vegetation mix"),
		Weather->GroundCoverInstanceCount > 0 && FixturePlantCount * 100 >= Weather->GroundCoverInstanceCount * 25 &&
		FixturePlantCount * 100 <= Weather->GroundCoverInstanceCount * 42);
	TestEqual(TEXT("The fixture has no landscape or sea plane, so no global meadow patches are generated"), Weather->GroundCoverMeadowInstanceCount, 0);
	for (UHierarchicalInstancedStaticMeshComponent* LowPlant : {Weather->ShoreGroundPlantLowA, Weather->ShoreGroundPlantLowB})
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
	AddInfo(FString::Printf(TEXT("Pool-edge fixture expects %.1f cm clearance; weather placed %d instances (%d grass A + %d grass B + %d grass C + %d broadleaf + %d low plant A + %d low plant B)."),
		PoolClearanceRadius, Weather->GroundCoverInstanceCount, Weather->ShoreGrassA->GetInstanceCount(), Weather->ShoreGrassB->GetInstanceCount(), GrassC->GetInstanceCount(),
		Weather->ShoreGroundPlants->GetInstanceCount(), Weather->ShoreGroundPlantLowA->GetInstanceCount(), Weather->ShoreGroundPlantLowB->GetInstanceCount()));
	int32 GrassInsidePoolClearance = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Grass : {Weather->ShoreGrassA.Get(), Weather->ShoreGrassB.Get(), GrassC, Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(), Weather->ShoreGroundPlantLowB.Get()})
	{
		for (int32 Index = 0; Index < Grass->GetInstanceCount(); ++Index)
		{
			FTransform Transform;
			if (Grass->GetInstanceTransform(Index, Transform, true) && FVector::Dist2D(Transform.GetLocation(), Tideglass->GetActorLocation()) < PoolClearanceRadius)
				++GrassInsidePoolClearance;
		}
	}
	TestEqual(TEXT("No ground-cover instance intrudes into the Tideglass water footprint or edge margin"), GrassInsidePoolClearance, 0);
	int32 InnApproachGrass = 0;
	int32 InnGrassUnderRoof = 0;
	for (UHierarchicalInstancedStaticMeshComponent* Grass : {Weather->ShoreGrassA.Get(), Weather->ShoreGrassB.Get(), GrassC, Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(), Weather->ShoreGroundPlantLowB.Get()})
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
	for (UHierarchicalInstancedStaticMeshComponent* Grass : {Weather->ShoreGrassA.Get(), Weather->ShoreGrassB.Get(), GrassC, Weather->ShoreGroundPlants.Get(), Weather->ShoreGroundPlantLowA.Get(), Weather->ShoreGroundPlantLowB.Get()})
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
	TestEqual(TEXT("Each wind-driven grass and ground-plant species retains an immutable baseline for every instance"),
		Weather->ShoreGrassABaseTransforms.Num() + Weather->ShoreGrassBBaseTransforms.Num() + Weather->ShoreGroundPlantBaseTransforms.Num() +
		Weather->ShoreGroundPlantLowABaseTransforms.Num() + Weather->ShoreGroundPlantLowBBaseTransforms.Num(), Weather->GroundCoverInstanceCount);
	TestEqual(TEXT("HISM populations match the reported transient ground-cover population"),
		Weather->ShoreGrassA->GetInstanceCount() + Weather->ShoreGrassB->GetInstanceCount() + GrassC->GetInstanceCount() + Weather->ShoreGroundPlants->GetInstanceCount() +
		Weather->ShoreGroundPlantLowA->GetInstanceCount() + Weather->ShoreGroundPlantLowB->GetInstanceCount(), Weather->GroundCoverInstanceCount);
	TestTrue(TEXT("Ground cover stays nonblocking and off navigation"),
		Weather->ShoreGrassA->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGrassB->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		GrassC->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlants->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlantLowA->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		Weather->ShoreGroundPlantLowB->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
		!Weather->ShoreGrassA->CanEverAffectNavigation() && !Weather->ShoreGrassB->CanEverAffectNavigation() && !GrassC->CanEverAffectNavigation() && !Weather->ShoreGroundPlants->CanEverAffectNavigation() &&
		!Weather->ShoreGroundPlantLowA->CanEverAffectNavigation() && !Weather->ShoreGroundPlantLowB->CanEverAffectNavigation());
	TestTrue(TEXT("All grass and ground-plant variants are visible around the landmarks and inn approach"),
		Weather->ShoreGrassA->IsVisible() && Weather->ShoreGrassB->IsVisible() && GrassC->IsVisible() && Weather->ShoreGroundPlants->IsVisible() &&
		Weather->ShoreGrassA->GetInstanceCount() > 0 && Weather->ShoreGrassB->GetInstanceCount() > 0 && GrassC->GetInstanceCount() > 0 &&
		Weather->ShoreGroundPlantLowA->IsVisible() && Weather->ShoreGroundPlantLowB->IsVisible() &&
		Weather->ShoreGroundPlants->GetInstanceCount() > 0 && Weather->ShoreGroundPlantLowA->GetInstanceCount() > 0 &&
		Weather->ShoreGroundPlantLowB->GetInstanceCount() > 0);
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
		FTransform OverheadPose;
		Weather->ShoreGrassA->GetInstanceTransform(0, OverheadPose, false);
		TestTrue(TEXT("A resident passing overhead does not bend ground cover"),
			OverheadPose.GetRotation().Equals(UnoccupiedWindPose.GetRotation(), 0.001f));
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
	TArray<FTransform> FirstSwayShrubs;
	CaptureTransforms(Weather->ShoreGrassA, FirstSwayA);
	CaptureTransforms(Weather->ShoreGrassB, FirstSwayB);
	CaptureTransforms(GrassC, FirstSwayC);
	CaptureTransforms(Weather->ShoreGroundPlants, FirstSwayPlants);
	CaptureTransforms(Weather->ShoreGroundPlantLowA, FirstSwayPlantsLowA);
	CaptureTransforms(Weather->ShoreGroundPlantLowB, FirstSwayPlantsLowB);
	CaptureTransforms(Weather->IslandShrubs, FirstSwayShrubs);
	Weather->UpdateGroundCoverSway();
	TArray<FTransform> SecondSwayA;
	TArray<FTransform> SecondSwayB;
	TArray<FTransform> SecondSwayC;
	TArray<FTransform> SecondSwayPlants;
	TArray<FTransform> SecondSwayPlantsLowA;
	TArray<FTransform> SecondSwayPlantsLowB;
	TArray<FTransform> SecondSwayShrubs;
	CaptureTransforms(Weather->ShoreGrassA, SecondSwayA);
	CaptureTransforms(Weather->ShoreGrassB, SecondSwayB);
	CaptureTransforms(GrassC, SecondSwayC);
	CaptureTransforms(Weather->ShoreGroundPlants, SecondSwayPlants);
	CaptureTransforms(Weather->ShoreGroundPlantLowA, SecondSwayPlantsLowA);
	CaptureTransforms(Weather->ShoreGroundPlantLowB, SecondSwayPlantsLowB);
	CaptureTransforms(Weather->IslandShrubs, SecondSwayShrubs);
	bool bRepeatedSwayIsStable = FirstSwayA.Num() == SecondSwayA.Num() && FirstSwayB.Num() == SecondSwayB.Num() && FirstSwayC.Num() == SecondSwayC.Num() &&
		FirstSwayPlants.Num() == SecondSwayPlants.Num() && FirstSwayPlantsLowA.Num() == SecondSwayPlantsLowA.Num() &&
		FirstSwayPlantsLowB.Num() == SecondSwayPlantsLowB.Num() && FirstSwayShrubs.Num() == SecondSwayShrubs.Num();
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
		LogFirstMismatch(TEXT("understory shrub"), FirstSwayShrubs, SecondSwayShrubs);
	}
	TestTrue(TEXT("Repeating a weather update at the same time does not accumulate transform drift"), bRepeatedSwayIsStable);
	const int32 GroundCoverCountAfterFirstInitialization = Weather->GroundCoverInstanceCount;
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
		Weather->ShoreGroundPlantLowABaseTransforms.Num() + Weather->ShoreGroundPlantLowBBaseTransforms.Num() + Weather->IslandShrubBaseTransforms.Num(), 0);
	TestTrue(TEXT("Transient cleanup releases nearby-sway indices and spatial cells"),
		Weather->SwayedShoreGrassAIndices.IsEmpty() && Weather->SwayedShoreGrassBIndices.IsEmpty() && Weather->SwayedShoreGrassCIndices.IsEmpty() &&
		Weather->SwayedGroundPlantIndices.IsEmpty() && Weather->SwayedGroundPlantLowAIndices.IsEmpty() && Weather->SwayedGroundPlantLowBIndices.IsEmpty() && Weather->SwayedShrubIndices.IsEmpty() &&
		Weather->ShoreGrassACells.IsEmpty() && Weather->ShoreGrassBCells.IsEmpty() && Weather->ShoreGrassCCells.IsEmpty() &&
		Weather->GroundPlantCells.IsEmpty() && Weather->GroundPlantLowACells.IsEmpty() && Weather->GroundPlantLowBCells.IsEmpty() && Weather->ShrubCells.IsEmpty());
	TestTrue(TEXT("Cleared ground cover is hidden"), !Weather->ShoreGrassA->IsVisible() && !Weather->ShoreGrassB->IsVisible() && !GrassC->IsVisible() && !Weather->ShoreGroundPlants->IsVisible() &&
		!Weather->ShoreGroundPlantLowA->IsVisible() && !Weather->ShoreGroundPlantLowB->IsVisible() && !Weather->IslandSpruce->IsVisible() && !Weather->IslandShrubs->IsVisible());
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
	TestTrue(TEXT("Local wind gently nudges the firefly drift"), AIslandFirefly::WindDisplacement(FVector(100.f, 0.f, 0.f)).Equals(FVector(12.f, 0.f, 0.f)));
	TestTrue(TEXT("Strong gust displacement stays bounded"), AIslandFirefly::WindDisplacement(FVector(1000.f, 0.f, 0.f)).Equals(FVector(30.f, 0.f, 0.f)));
	TestTrue(TEXT("Still air adds no wind displacement"), AIslandFirefly::WindDisplacement(FVector::ZeroVector).IsNearlyZero());
	TestTrue(TEXT("Dry conditions leave crab roaming unchanged"), FMath::IsNearlyEqual(AIslandTidepoolCrab::RainMovementScale(0.f), 1.f));
	TestTrue(TEXT("Heavy rain reduces but does not stop crab roaming"), AIslandTidepoolCrab::RainMovementScale(1.f) > 0.f && AIslandTidepoolCrab::RainMovementScale(1.f) < 1.f);
	TestTrue(TEXT("Crab rain response changes smoothly and monotonically"), AIslandTidepoolCrab::RainMovementScale(0.75f) < AIslandTidepoolCrab::RainMovementScale(0.45f));
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
		TestTrue(TEXT("Rain water response is subtler than a deliberate pool interaction"), RainRipple->PeakLightIntensity < 55.f && RainRipple->SurfaceRadius < 150.f && RainRipple->DurationSeconds < 1.6f);
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
		++CrabPopulation;
		DayCrabResidents.Add(*It);
		TestTrue(TEXT("Tidepool crab advertises as untargeted ambient life"), It->ActorHasTag(TEXT("IslandLife")) && It->ActorHasTag(TEXT("TidepoolCrab")) && !It->ActorHasTag(TEXT("IslandLandmark")));
		TestTrue(TEXT("Tidepool crab remains at the shoreline of its habitat"), FVector::Dist2D(It->GetActorLocation(), Habitat->GetActorLocation()) < 1000.f);
		TestEqual(TEXT("Tidepool crab has six visible leg placeholders"), It->Legs.Num(), 6);
		TestEqual(TEXT("Tidepool crab has two claws and two eye stalks"), It->Claws.Num(), 2);
		TestEqual(TEXT("Tidepool crab geometry cannot block the world"), It->Shell->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}
	TestEqual(TEXT("A small bounded crab population is active by day"), CrabPopulation, 2);
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
	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabResidents)
		TestTrue(TEXT("Each original crab actor survives and resumes in place"), Crab.IsValid() && !Crab->IsSheltered());

	Clock->CurrentHour = 5.5f;
	Weather->RefreshNightEcology();
	Population = 0;
	for (TActorIterator<AIslandFirefly> It(World); It; ++It) ++Population;
	TestEqual(TEXT("Fireflies leave during the twilight transition"), Population, 0);
	for (TActorIterator<AIslandTidepoolCrab> It(World); It; ++It)
		TestTrue(TEXT("Crabs remain concealed before their 06:00 emergence"), It->IsSheltered());
	Clock->CurrentHour = 6.f;
	Weather->RefreshNightEcology();
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
		TestTrue(TEXT("Generated sound actor cleans itself up after playback"), Chime->IsActorBeingDestroyed());
		if (ChimeListener)
		{
			ChimeListener->CheckForNearbyStoneChime();
			TestTrue(TEXT("Destroyed sound actors are pruned from the firefly's temporary response history"),
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
	TestEqual(TEXT("The controller's repeat cooldown still prevents another chime"), ChimeCountAfterRepeatedControllerAction, 1);
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
		WatchableCrab->CheckForLowRavenFlyby();
		TestTrue(TEXT("A nearby low raven pass briefly startles the shore crab"),
			WatchableCrab->ScurryRemaining > 0.f && WatchableCrab->ScurryRemaining <= 2.4f);
		const FVector AwayFromRaven = (CrabStart - LowPassLocation).GetSafeNormal2D();
		TestTrue(TEXT("The shore crab scurries away from the passing raven"),
			FVector::DotProduct(WatchableCrab->ScurryDirection, AwayFromRaven) > 0.95f);
		TestEqual(TEXT("One low pass starts a short response cooldown"), WatchableCrab->RavenFlybyCooldownRemaining, 8.f);

		WatchableCrab->ScurryRemaining = 0.f;
		WatchableCrab->CheckForLowRavenFlyby();
		TestTrue(TEXT("A hovering raven cannot retrigger the response during cooldown"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		WatchableCrab->RavenFlybyCooldownRemaining = 0.f;
		Observer->SetActorLocation(CrabStart + FVector(0.f, 0.f, 900.f));
		WatchableCrab->CheckForLowRavenFlyby();
		TestTrue(TEXT("A high raven flight does not startle a shore crab"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		Observer->SetActorLocation(CrabStart + FVector(800.f, 0.f, 260.f));
		WatchableCrab->CheckForLowRavenFlyby();
		TestTrue(TEXT("A low but distant flight does not startle a shore crab"),
			FMath::IsNearlyZero(WatchableCrab->ScurryRemaining));
		Controller->LocomotionState = ERavenLocomotionState::Grounded;
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
