#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandLightning.h"
#include "HAL/IConsoleManager.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "IslandListeningStonesChime.h"
#include "IslandTidepoolCrab.h"
#include "IslandTidepoolMinnows.h"
#include "IslandPoolRippleEffect.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "LandscapeProxy.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWeather, Log, All);

AIslandWeather::AIslandWeather()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RainStreaks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainStreaks"));
	RainStreaks->SetupAttachment(RootComponent);
	RainStreaks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainStreaks->SetCastShadow(false);
	RainStreaks->bReceivesDecals = false;
	RainStreaks->SetVisibility(false);
	RainGroundImpactStreaks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainGroundImpactStreaks"));
	RainGroundImpactStreaks->SetupAttachment(RootComponent);
	RainGroundImpactStreaks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainGroundImpactStreaks->SetCastShadow(false);
	RainGroundImpactStreaks->bReceivesDecals = false;
	RainGroundImpactStreaks->SetVisibility(false);
	ShoreGrassA = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ShoreGrassA"));
	ShoreGrassA->SetupAttachment(RootComponent);
	ShoreGrassA->SetMobility(EComponentMobility::Movable);
	ShoreGrassA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreGrassA->SetCanEverAffectNavigation(false);
	ShoreGrassA->SetCastShadow(false);
	ShoreGrassA->bReceivesDecals = false;
	ShoreGrassA->SetVisibility(false);
	ShoreGrassB = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ShoreGrassB"));
	ShoreGrassB->SetupAttachment(RootComponent);
	ShoreGrassB->SetMobility(EComponentMobility::Movable);
	ShoreGrassB->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreGrassB->SetCanEverAffectNavigation(false);
	ShoreGrassB->SetCastShadow(false);
	ShoreGrassB->bReceivesDecals = false;
	ShoreGrassB->SetVisibility(false);
	ShoreGroundPlants = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ShoreGroundPlants"));
	ShoreGroundPlants->SetupAttachment(RootComponent);
	ShoreGroundPlants->SetMobility(EComponentMobility::Movable);
	ShoreGroundPlants->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreGroundPlants->SetCanEverAffectNavigation(false);
	ShoreGroundPlants->SetCastShadow(false);
	ShoreGroundPlants->bReceivesDecals = false;
	ShoreGroundPlants->SetVisibility(false);
	ShoreGroundPlantLowA = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ShoreGroundPlantLowA"));
	ShoreGroundPlantLowA->SetupAttachment(RootComponent);
	ShoreGroundPlantLowA->SetMobility(EComponentMobility::Movable);
	ShoreGroundPlantLowA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreGroundPlantLowA->SetCanEverAffectNavigation(false);
	ShoreGroundPlantLowA->SetCastShadow(false);
	ShoreGroundPlantLowA->bReceivesDecals = false;
	ShoreGroundPlantLowA->SetVisibility(false);
	ShoreGroundPlantLowB = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ShoreGroundPlantLowB"));
	ShoreGroundPlantLowB->SetupAttachment(RootComponent);
	ShoreGroundPlantLowB->SetMobility(EComponentMobility::Movable);
	ShoreGroundPlantLowB->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreGroundPlantLowB->SetCanEverAffectNavigation(false);
	ShoreGroundPlantLowB->SetCastShadow(false);
	ShoreGroundPlantLowB->bReceivesDecals = false;
	ShoreGroundPlantLowB->SetVisibility(false);
	WindAmbienceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WindAmbience"));
	WindAmbienceAudio->SetupAttachment(RootComponent);
	WindAmbienceAudio->bAutoActivate = false;
	WindAmbienceAudio->bAllowSpatialization = false;
	RainAmbienceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RainAmbience"));
	RainAmbienceAudio->SetupAttachment(RootComponent);
	RainAmbienceAudio->bAutoActivate = false;
	RainAmbienceAudio->bAllowSpatialization = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RainMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (RainMesh.Succeeded())
	{
		RainStreaks->SetStaticMesh(RainMesh.Object);
		RainGroundImpactStreaks->SetStaticMesh(RainMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GrassMeshA(TEXT("/Game/PN_FoliageCollection/Meshes/grassMesh/grass_01_02_mesh.grass_01_02_mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GrassMeshB(TEXT("/Game/PN_FoliageCollection/Meshes/grassMesh/grass_01_03_mesh.grass_01_03_mesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundPlantMesh(TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_05_01.ground_05_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundPlantLowAMesh(TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_01_01.ground_01_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundPlantLowBMesh(TEXT("/Game/PN_FoliageCollection/Meshes/groundPlantMesh/ground_01_02.ground_01_02"));
	if (GrassMeshA.Succeeded()) ShoreGrassA->SetStaticMesh(GrassMeshA.Object);
	if (GrassMeshB.Succeeded()) ShoreGrassB->SetStaticMesh(GrassMeshB.Object);
	if (GroundPlantMesh.Succeeded()) ShoreGroundPlants->SetStaticMesh(GroundPlantMesh.Object);
	if (GroundPlantLowAMesh.Succeeded()) ShoreGroundPlantLowA->SetStaticMesh(GroundPlantLowAMesh.Object);
	if (GroundPlantLowBMesh.Succeeded()) ShoreGroundPlantLowB->SetStaticMesh(GroundPlantLowBMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RainMaterial(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (RainMaterial.Succeeded()) RainStreaks->SetMaterial(0, RainMaterial.Object);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

void AIslandWeather::BeginPlay()
{
	Super::BeginPlay();
	// Carry on from the weather the Island was having when it was last left.
	if (const UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>())
		WeatherTimeOffset = WorldState->GetSavedWeatherSeconds().Get(0.0);
	SecondsSinceWeatherSave = 0.f;
	InitializeGroundCover();
	InitializeWeatherAmbience();
	UpdateCloudRendering();
	UpdateRainRendering();
	RefreshNightEcology();
	GetWorldTimerManager().SetTimer(EcologyTimerHandle, this, &AIslandWeather::RefreshNightEcology, 30.f, true, 30.f);
}

void AIslandWeather::PersistWeatherTime()
{
	SecondsSinceWeatherSave = 0.f;
	if (UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr)
		WorldState->SaveWeatherSeconds(WeatherTimeOffset + GetWorld()->GetTimeSeconds());
}

void AIslandWeather::BuildGroundCoverOffsets(int32 Seed, TArray<FTransform>& OutTransforms)
{
	BuildGroundCoverOffsets(Seed, 96, 260.f, 720.f, OutTransforms);
}

void AIslandWeather::BuildGroundCoverOffsets(int32 Seed, int32 ClumpCount, float InnerRadius, float OuterRadius,
	TArray<FTransform>& OutTransforms)
{
	ClumpCount = FMath::Max(0, ClumpCount);
	InnerRadius = FMath::Max(0.f, InnerRadius);
	OuterRadius = FMath::Max(InnerRadius, OuterRadius);
	OutTransforms.Reset(ClumpCount);
	FRandomStream Random(Seed);
	for (int32 Index = 0; Index < ClumpCount; ++Index)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		// Uniform-in-area annulus leaves a little breathing room around each landmark.
		const float Radius = FMath::Sqrt(FMath::Lerp(FMath::Square(InnerRadius), FMath::Square(OuterRadius), Random.FRand()));
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const FQuat Rotation(FVector::UpVector, FMath::DegreesToRadians(Random.FRandRange(0.f, 360.f)));
		const float Scale = Random.FRandRange(0.8f, 1.2f);
		OutTransforms.Emplace(Rotation, Offset, FVector(Scale));
	}
}

FTransform AIslandWeather::CalculateGroundCoverSway(const FTransform& BaseTransform, const FVector& LocalWind,
	double TimeSeconds, int32 InstanceIndex, int32 Seed, float ReferenceWindSpeed)
{
	if (LocalWind.ContainsNaN() || !FMath::IsFinite(TimeSeconds)) return BaseTransform;
	const FVector HorizontalWind(LocalWind.X, LocalWind.Y, 0.f);
	const float Intensity = FMath::Clamp(HorizontalWind.Size() / FMath::Max(1.f, ReferenceWindSpeed), 0.f, 1.f);
	if (Intensity <= KINDA_SMALL_NUMBER || HorizontalWind.IsNearlyZero()) return BaseTransform;

	const FVector WindDirection = HorizontalWind.GetSafeNormal();
	const FVector LeanAxis = FVector::CrossProduct(FVector::UpVector, WindDirection).GetSafeNormal();
	const double Phase = TimeSeconds * 1.15 + InstanceIndex * 2.39996323 + Seed * 0.113;
	const float LeanDegrees = 7.f * Intensity;
	const float SwayDegrees = FMath::Sin(Phase) * 2.5f * Intensity;
	const FQuat Lean(LeanAxis, FMath::DegreesToRadians(LeanDegrees));
	const FQuat Flutter(FVector::ForwardVector, FMath::DegreesToRadians(SwayDegrees));
	FTransform Result = BaseTransform;
	Result.SetRotation((BaseTransform.GetRotation() * Lean * Flutter).GetNormalized());
	return Result;
}

void AIslandWeather::InitializeGroundCover()
{
	if (bGroundCoverInitialized || !GetWorld() || !ShoreGrassA || !ShoreGrassB || !ShoreGroundPlants || !ShoreGroundPlantLowA || !ShoreGroundPlantLowB ||
		!ShoreGrassA->GetStaticMesh() || !ShoreGrassB->GetStaticMesh() || !ShoreGroundPlants->GetStaticMesh() ||
		!ShoreGroundPlantLowA->GetStaticMesh() || !ShoreGroundPlantLowB->GetStaticMesh()) return;
	bGroundCoverInitialized = true;
	ShoreGrassA->ClearInstances();
	ShoreGrassB->ClearInstances();
	ShoreGroundPlants->ClearInstances();
	ShoreGroundPlantLowA->ClearInstances();
	ShoreGroundPlantLowB->ClearInstances();
	GroundCoverInstanceCount = 0;
	GroundCoverMeadowInstanceCount = 0;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandGroundCover), false, this);
	for (TActorIterator<AActor> IgnoreIt(GetWorld()); IgnoreIt; ++IgnoreIt)
	{
		bool bIgnoreActor = (*IgnoreIt)->ActorHasTag(TEXT("IslandLandmark")) || (*IgnoreIt)->GetName().Contains(TEXT("Ocean"));
		TArray<UStaticMeshComponent*> Components;
		(*IgnoreIt)->GetComponents<UStaticMeshComponent>(Components);
		for (const UStaticMeshComponent* Component : Components)
			if (Component && Component->GetStaticMesh() && Component->GetStaticMesh()->GetName().Contains(TEXT("WaterPlane"), ESearchCase::IgnoreCase))
				bIgnoreActor = true;
		if (bIgnoreActor) Query.AddIgnoredActor(*IgnoreIt);
	}

	auto PlaceFoliage = [this](const FHitResult& GroundHit, const FTransform& Offset, int32 Index)
	{
		const FQuat AlignToGround = FQuat::FindBetweenNormals(FVector::UpVector, GroundHit.ImpactNormal);
		const FQuat Rotation = AlignToGround * Offset.GetRotation();
		const float JitterScale = Offset.GetScale3D().X;
		FVector Scale = FVector(JitterScale * 1.7f);
		FVector Location = GroundHit.ImpactPoint + GroundHit.ImpactNormal * 1.2f;
		UHierarchicalInstancedStaticMeshComponent* Species = nullptr;
		const int32 PlantVariant = Index % 12;
		if (PlantVariant == 0 || PlantVariant == 4 || PlantVariant == 8)
		{
			Species = PlantVariant == 0 ? ShoreGroundPlants : PlantVariant == 4 ? ShoreGroundPlantLowA : ShoreGroundPlantLowB;
			const FBoxSphereBounds PlantBounds = Species->GetStaticMesh()->GetBounds();
			const float PlantHalfHeight = FMath::Max(1.f, PlantBounds.BoxExtent.Z);
			const float HeightScale = 70.f / (2.f * PlantHalfHeight);
			const float PlantFootprintDiameter = 2.f * FMath::Max(PlantBounds.BoxExtent.X, PlantBounds.BoxExtent.Y);
			const float WidthScale = 48.f / FMath::Max(1.f, PlantFootprintDiameter);
			const float PlantScale = (Species == ShoreGroundPlants ? HeightScale : FMath::Min(HeightScale, WidthScale)) * JitterScale;
			Scale = FVector(PlantScale);
			Location = GroundHit.ImpactPoint + GroundHit.ImpactNormal * (PlantHalfHeight * PlantScale + 1.2f) -
				Rotation.RotateVector(PlantBounds.Origin * PlantScale);
		}
		else
		{
			Species = (Index % 2 == 0) ? ShoreGrassA : ShoreGrassB;
		}
		Species->AddInstance(FTransform(Rotation, Location, Scale), true);
		++GroundCoverInstanceCount;
	};

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const AActor* Landmark = *It;
		const bool bTideglass = Landmark->ActorHasTag(TEXT("IslandLandmark")) && Landmark->ActorHasTag(TEXT("TideglassPool"));
		const bool bListeningStones = Landmark->ActorHasTag(TEXT("IslandLandmark")) && Landmark->ActorHasTag(TEXT("ListeningStones"));
		const bool bWindArch = Landmark->ActorHasTag(TEXT("IslandLandmark")) && Landmark->ActorHasTag(TEXT("WindArch"));
		const bool bInnEntrance = Landmark->ActorHasTag(TEXT("InnDoorLantern"));
		if (!bTideglass && !bListeningStones && !bWindArch && !bInnEntrance) continue;

		float TideglassClearRadius = 0.f;
		if (bTideglass)
		{
			for (TActorIterator<AActor> SurfaceIt(GetWorld()); SurfaceIt; ++SurfaceIt)
			{
				if (FVector::Dist(SurfaceIt->GetActorLocation(), Landmark->GetActorLocation()) > 25.f) continue;
				TArray<UStaticMeshComponent*> MeshComponents;
				SurfaceIt->GetComponents<UStaticMeshComponent>(MeshComponents);
				for (const UStaticMeshComponent* Mesh : MeshComponents)
				{
					if (!Mesh || !Mesh->GetStaticMesh() || Mesh->GetStaticMesh()->GetName() != TEXT("Sphere")) continue;
					const FVector Scale = Mesh->GetComponentScale();
					if (Scale.X <= 2.f || Scale.Y <= 2.f || Scale.Z >= 0.25f) continue;
					const FBoxSphereBounds LocalBounds = Mesh->GetStaticMesh()->GetBounds();
					const float SurfaceRadius = FMath::Max(LocalBounds.BoxExtent.X * Scale.X, LocalBounds.BoxExtent.Y * Scale.Y);
					// The generated water outline extends slightly beyond the source sphere bounds;
					// keep full-sized grass clumps outside it, plus the marker's allowed offset.
					TideglassClearRadius = FVector::Dist2D(Mesh->GetComponentLocation(), Landmark->GetActorLocation()) + SurfaceRadius * 1.13f + 125.f;
					break;
				}
				if (TideglassClearRadius > 0.f) break;
			}
			if (TideglassClearRadius > 0.f)
				UE_LOG(LogIslandWeather, Log, TEXT("Tideglass ground cover leaves a %.0f cm pool-edge clearance."), TideglassClearRadius);
		}

		TArray<FTransform> Offsets;
		const uint32 Seed = static_cast<uint32>(WeatherSeed) ^
			(bTideglass ? 0x2f6e2b1u : bListeningStones ? 0x6d2b79f5u : bWindArch ? 0x32b4d8e1u : 0x51a7e2d3u);
		constexpr int32 InnClumpCount = 108;
		constexpr float InnInnerRadius = 350.f;
		constexpr float InnOuterRadius = 1200.f;
		constexpr int32 LandmarkClumpCount = 144;
		constexpr int32 WindArchClumpCount = 288;
		if (bInnEntrance)
			BuildGroundCoverOffsets(static_cast<int32>(Seed), InnClumpCount, InnInnerRadius, InnOuterRadius, Offsets);
		else if (bWindArch)
			BuildGroundCoverOffsets(static_cast<int32>(Seed), WindArchClumpCount, InnInnerRadius, InnOuterRadius, Offsets);
		else
			BuildGroundCoverOffsets(static_cast<int32>(Seed), LandmarkClumpCount, 260.f, 720.f, Offsets);
		for (int32 Index = 0; Index < Offsets.Num(); ++Index)
		{
			const FTransform& Offset = Offsets[Index];
			const FVector Candidate = Landmark->GetActorLocation() + Offset.GetLocation();
			if (TideglassClearRadius > 0.f && Offset.GetLocation().Size2D() < TideglassClearRadius) continue;
			const FVector TraceStart = Candidate + FVector(0.f, 0.f, (bInnEntrance || bWindArch) ? 5000.f : 1400.f);
			const FVector TraceEnd = Candidate - FVector(0.f, 0.f, 5000.f);
			FHitResult GroundHit;
			if (!GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, Query) ||
				GroundHit.ImpactNormal.Z < 0.72f) continue;
			if (bInnEntrance)
			{
				// The lantern hangs at the entrance, but a scatter ring also reaches behind and beside
				// the building. Reject roofs and indoor floors, then require open sky above each clump.
				const float HeightFromMarker = GroundHit.ImpactPoint.Z - Landmark->GetActorLocation().Z;
				if (HeightFromMarker < -700.f || HeightFromMarker > 100.f) continue;
				FHitResult OverheadHit;
				const FVector OpenSkyStart = GroundHit.ImpactPoint + FVector(0.f, 0.f, 25.f);
				const FVector OpenSkyEnd = OpenSkyStart + FVector(0.f, 0.f, 8000.f);
				if (GetWorld()->LineTraceSingleByChannel(OverheadHit, OpenSkyStart, OpenSkyEnd, ECC_WorldStatic, Query)) continue;
			}

			PlaceFoliage(GroundHit, Offset, Index);
		}
	}

	// Place separated, deterministic broadleaf/grass patches across exposed Island landscape.
	// Close POI verges above remain denser, while the broad patch layer fills walkable hillside sightlines.
	ALandscapeProxy* IslandLandscape = nullptr;
	for (TActorIterator<ALandscapeProxy> LandscapeIt(GetWorld()); LandscapeIt; ++LandscapeIt)
	{
		IslandLandscape = *LandscapeIt;
		break;
	}
	float SeaLevel = TNumericLimits<float>::Lowest();
	for (TActorIterator<AActor> OceanIt(GetWorld()); OceanIt; ++OceanIt)
	{
		TArray<UStaticMeshComponent*> Components;
		OceanIt->GetComponents<UStaticMeshComponent>(Components);
		for (const UStaticMeshComponent* Component : Components)
			if (Component && Component->GetStaticMesh() && Component->GetStaticMesh()->GetName().Contains(TEXT("WaterPlane"), ESearchCase::IgnoreCase))
			{
				SeaLevel = OceanIt->GetActorLocation().Z;
				break;
			}
		if (SeaLevel > TNumericLimits<float>::Lowest()) break;
	}
	if (IslandLandscape && SeaLevel > TNumericLimits<float>::Lowest())
	{
		FVector BoundsOrigin, BoundsExtent;
		IslandLandscape->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		TArray<FVector> ExclusionLocations;
		for (TActorIterator<AActor> AnchorIt(GetWorld()); AnchorIt; ++AnchorIt)
			if (AnchorIt->ActorHasTag(TEXT("IslandLandmark")) || AnchorIt->ActorHasTag(TEXT("InnDoorLantern")))
				ExclusionLocations.Add(AnchorIt->GetActorLocation());
		TArray<FVector> MeadowCenters;
		const int32 GroundCoverBeforeMeadowPatches = GroundCoverInstanceCount;
		FRandomStream MeadowRandom(static_cast<int32>(static_cast<uint32>(WeatherSeed) ^ 0x7ac4e291u));
		constexpr int32 MeadowPatchCount = 160;
		constexpr int32 AnchorPatchesPerLandmark = 12;
		constexpr int32 MeadowClumpsPerPatch = 192;
		constexpr float MeadowPatchInnerRadius = 400.f;
		constexpr float MeadowPatchOuterRadius = 2800.f;
		constexpr float MeadowCenterExclusionRadius = 5500.f;
		constexpr float LandmarkPatchMinRadius = 1500.f;
		constexpr float LandmarkPatchMaxRadius = 4400.f;
		constexpr float LandmarkPatchMinSpacing = 1200.f;
		constexpr float MeadowCenterSpacing = 3200.f;
		int32 MeadowTraceCount = 0;
		const float TraceTop = BoundsOrigin.Z + BoundsExtent.Z + 2500.f;
		const float TraceBottom = BoundsOrigin.Z - BoundsExtent.Z - 2500.f;
		auto AddMeadowPatch = [this, &MeadowCenters, &MeadowRandom, &MeadowTraceCount, &TraceTop, &TraceBottom, &PlaceFoliage,
			&Query, MeadowClumpsPerPatch, MeadowPatchInnerRadius, MeadowPatchOuterRadius, SeaLevel](FVector Center)
		{
			MeadowCenters.Add(Center);
			TArray<FTransform> PatchOffsets;
			const int32 PatchSeed = static_cast<int32>(static_cast<uint32>(WeatherSeed) ^ (0x3e5a93b7u + MeadowCenters.Num() * 7919u));
			BuildGroundCoverOffsets(PatchSeed, MeadowClumpsPerPatch, MeadowPatchInnerRadius, MeadowPatchOuterRadius, PatchOffsets);
			for (int32 ClumpIndex = 0; ClumpIndex < PatchOffsets.Num(); ++ClumpIndex)
			{
				const FVector PatchCandidate = MeadowCenters.Last() + PatchOffsets[ClumpIndex].GetLocation();
				FHitResult PatchHit;
				++MeadowTraceCount;
				if (!GetWorld()->LineTraceSingleByChannel(PatchHit,
					FVector(PatchCandidate.X, PatchCandidate.Y, TraceTop), FVector(PatchCandidate.X, PatchCandidate.Y, TraceBottom), ECC_WorldStatic, Query) ||
					!Cast<ALandscapeProxy>(PatchHit.GetActor()) || PatchHit.ImpactNormal.Z < 0.72f || PatchHit.ImpactPoint.Z < SeaLevel + 100.f) continue;
				PlaceFoliage(PatchHit, PatchOffsets[ClumpIndex], ClumpIndex + MeadowCenters.Num());
			}
		};
		auto TryAddMeadowCenter = [this, &MeadowCenters, &MeadowTraceCount, &Query, &TraceTop, &TraceBottom, SeaLevel](const FVector& Candidate)
		{
			FHitResult CenterHit;
			++MeadowTraceCount;
			if (!GetWorld()->LineTraceSingleByChannel(CenterHit,
				FVector(Candidate.X, Candidate.Y, TraceTop), FVector(Candidate.X, Candidate.Y, TraceBottom), ECC_WorldStatic, Query) ||
				!Cast<ALandscapeProxy>(CenterHit.GetActor()) || CenterHit.ImpactNormal.Z < 0.78f || CenterHit.ImpactPoint.Z < SeaLevel + 100.f) return false;
			MeadowCenters.Add(CenterHit.ImpactPoint);
			return true;
		};

		// Keep part of the patch budget in the actual landmark sightlines, outside each close-up verge.
		for (int32 AnchorIndex = 0; AnchorIndex < ExclusionLocations.Num() && MeadowCenters.Num() < MeadowPatchCount; ++AnchorIndex)
		{
			int32 AddedForAnchor = 0;
			for (int32 Attempt = 0; Attempt < AnchorPatchesPerLandmark * 24 && AddedForAnchor < AnchorPatchesPerLandmark && MeadowCenters.Num() < MeadowPatchCount; ++Attempt)
			{
				const float Angle = MeadowRandom.FRandRange(0.f, 2.f * PI);
				const float Radius = MeadowRandom.FRandRange(LandmarkPatchMinRadius, LandmarkPatchMaxRadius);
				const FVector Candidate = ExclusionLocations[AnchorIndex] + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
				bool bTooCloseToOtherAnchor = false;
				for (int32 OtherAnchorIndex = 0; OtherAnchorIndex < ExclusionLocations.Num(); ++OtherAnchorIndex)
					if (OtherAnchorIndex != AnchorIndex && FVector::Dist2D(Candidate, ExclusionLocations[OtherAnchorIndex]) < 1600.f)
						{ bTooCloseToOtherAnchor = true; break; }
				if (bTooCloseToOtherAnchor) continue;
				bool bTooCloseToPatch = false;
				for (const FVector& ExistingCenter : MeadowCenters)
					if (FVector::Dist2D(Candidate, ExistingCenter) < LandmarkPatchMinSpacing) { bTooCloseToPatch = true; break; }
				if (bTooCloseToPatch || !TryAddMeadowCenter(Candidate)) continue;
				AddMeadowPatch(MeadowCenters.Last());
				++AddedForAnchor;
			}
		}
		// The remaining patches use bounded, seeded samples over the rest of the island footprint.
		for (int32 Probe = 0; Probe < 1536 && MeadowCenters.Num() < MeadowPatchCount; ++Probe)
		{
			const FVector Candidate(MeadowRandom.FRandRange(BoundsOrigin.X - BoundsExtent.X, BoundsOrigin.X + BoundsExtent.X),
				MeadowRandom.FRandRange(BoundsOrigin.Y - BoundsExtent.Y, BoundsOrigin.Y + BoundsExtent.Y), BoundsOrigin.Z);
			bool bNearAnchor = false;
			for (const FVector& Exclusion : ExclusionLocations)
				if (FVector::Dist2D(Candidate, Exclusion) < MeadowCenterExclusionRadius) { bNearAnchor = true; break; }
			if (bNearAnchor) continue;
			bool bNearOtherPatch = false;
			for (const FVector& ExistingCenter : MeadowCenters)
				if (FVector::Dist2D(Candidate, ExistingCenter) < MeadowCenterSpacing) { bNearOtherPatch = true; break; }
			if (bNearOtherPatch || !TryAddMeadowCenter(Candidate)) continue;
			AddMeadowPatch(MeadowCenters.Last());
		}
		GroundCoverMeadowInstanceCount = GroundCoverInstanceCount - GroundCoverBeforeMeadowPatches;
		// The patch population is recorded separately from the closer anchor rings for capture diagnostics.
		UE_LOG(LogIslandWeather, Log, TEXT("Landscape meadow scatter placed %d patches with %d ground-cover instances after %d bounded traces."),
			MeadowCenters.Num(), GroundCoverMeadowInstanceCount, MeadowTraceCount);
	}

	ShoreGrassABaseTransforms.Reset(ShoreGrassA->GetInstanceCount());
	ShoreGrassBBaseTransforms.Reset(ShoreGrassB->GetInstanceCount());
	ShoreGroundPlantBaseTransforms.Reset(ShoreGroundPlants->GetInstanceCount());
	ShoreGroundPlantLowABaseTransforms.Reset(ShoreGroundPlantLowA->GetInstanceCount());
	ShoreGroundPlantLowBBaseTransforms.Reset(ShoreGroundPlantLowB->GetInstanceCount());
	for (int32 Index = 0; Index < ShoreGrassA->GetInstanceCount(); ++Index)
	{
		FTransform Transform;
		if (ShoreGrassA->GetInstanceTransform(Index, Transform, false)) ShoreGrassABaseTransforms.Add(Transform);
	}
	for (int32 Index = 0; Index < ShoreGrassB->GetInstanceCount(); ++Index)
	{
		FTransform Transform;
		if (ShoreGrassB->GetInstanceTransform(Index, Transform, false)) ShoreGrassBBaseTransforms.Add(Transform);
	}
	for (int32 Index = 0; Index < ShoreGroundPlants->GetInstanceCount(); ++Index)
	{
		FTransform Transform;
		if (ShoreGroundPlants->GetInstanceTransform(Index, Transform, false)) ShoreGroundPlantBaseTransforms.Add(Transform);
	}
	for (int32 Index = 0; Index < ShoreGroundPlantLowA->GetInstanceCount(); ++Index)
	{
		FTransform Transform;
		if (ShoreGroundPlantLowA->GetInstanceTransform(Index, Transform, false)) ShoreGroundPlantLowABaseTransforms.Add(Transform);
	}
	for (int32 Index = 0; Index < ShoreGroundPlantLowB->GetInstanceCount(); ++Index)
	{
		FTransform Transform;
		if (ShoreGroundPlantLowB->GetInstanceTransform(Index, Transform, false)) ShoreGroundPlantLowBBaseTransforms.Add(Transform);
	}

	const bool bVisible = GroundCoverInstanceCount > 0;
	ShoreGrassA->SetVisibility(bVisible, true);
	ShoreGrassB->SetVisibility(bVisible, true);
	ShoreGroundPlants->SetVisibility(bVisible, true);
	ShoreGroundPlantLowA->SetVisibility(bVisible, true);
	ShoreGroundPlantLowB->SetVisibility(bVisible, true);
	if (GroundCoverInstanceCount == 0)
		UE_LOG(LogIslandWeather, Warning, TEXT("No Island ground-cover instances placed; check landmark tags and ground collision."));
}

void AIslandWeather::ClearGroundCover()
{
	if (ShoreGrassA) { ShoreGrassA->ClearInstances(); ShoreGrassA->SetVisibility(false, true); }
	if (ShoreGrassB) { ShoreGrassB->ClearInstances(); ShoreGrassB->SetVisibility(false, true); }
	if (ShoreGroundPlants) { ShoreGroundPlants->ClearInstances(); ShoreGroundPlants->SetVisibility(false, true); }
	if (ShoreGroundPlantLowA) { ShoreGroundPlantLowA->ClearInstances(); ShoreGroundPlantLowA->SetVisibility(false, true); }
	if (ShoreGroundPlantLowB) { ShoreGroundPlantLowB->ClearInstances(); ShoreGroundPlantLowB->SetVisibility(false, true); }
	ShoreGrassABaseTransforms.Reset();
	ShoreGrassBBaseTransforms.Reset();
	ShoreGroundPlantBaseTransforms.Reset();
	ShoreGroundPlantLowABaseTransforms.Reset();
	ShoreGroundPlantLowBBaseTransforms.Reset();
	GroundCoverInstanceCount = 0;
	GroundCoverMeadowInstanceCount = 0;
	bGroundCoverInitialized = false;
}

void AIslandWeather::UpdateGroundCoverSway()
{
	if (!GetWorld() || !bGroundCoverInitialized) return;
	const double Now = GetWorld()->GetTimeSeconds();
	auto UpdateSpecies = [this, Now](UHierarchicalInstancedStaticMeshComponent* Grass, const TArray<FTransform>& BaseTransforms)
	{
		if (!Grass || BaseTransforms.IsEmpty()) return;
		const FTransform ComponentTransform = Grass->GetComponentTransform();
		for (int32 Index = 0; Index < BaseTransforms.Num(); ++Index)
		{
			const FTransform& Base = BaseTransforms[Index];
			const FVector WorldLocation = ComponentTransform.TransformPosition(Base.GetLocation());
			const FVector WorldWind = GetLocalWind(WorldLocation, this);
			const FVector ComponentWind = ComponentTransform.InverseTransformVectorNoScale(WorldWind);
			const FVector LocalWind = Base.GetRotation().UnrotateVector(ComponentWind);
			const FTransform Swayed = CalculateGroundCoverSway(Base, LocalWind, Now, Index, WeatherSeed, MaximumWindSpeed);
			Grass->UpdateInstanceTransform(Index, Swayed, false, Index == BaseTransforms.Num() - 1, true);
		}
	};
	UpdateSpecies(ShoreGrassA, ShoreGrassABaseTransforms);
	UpdateSpecies(ShoreGrassB, ShoreGrassBBaseTransforms);
	UpdateSpecies(ShoreGroundPlants, ShoreGroundPlantBaseTransforms);
	UpdateSpecies(ShoreGroundPlantLowA, ShoreGroundPlantLowABaseTransforms);
	UpdateSpecies(ShoreGroundPlantLowB, ShoreGroundPlantLowBBaseTransforms);
}

void AIslandWeather::ClearGroundCoverPreview()
{
	ClearGroundCover();
}

void AIslandWeather::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PersistWeatherTime();
	GetWorldTimerManager().ClearTimer(EcologyTimerHandle);
	if (UVolumetricCloudComponent* Cloud = CloudComponent.Get())
		if (OriginalCloudMaterial) Cloud->SetMaterial(OriginalCloudMaterial);
	CloudComponent.Reset();
	WeatherCloudMaterial = nullptr;
	OriginalCloudMaterial = nullptr;
	if (RainStreaks) RainStreaks->SetVisibility(false, true);
	if (RainGroundImpactStreaks) RainGroundImpactStreaks->SetVisibility(false, true);
	if (WindAmbienceAudio) WindAmbienceAudio->Stop();
	if (RainAmbienceAudio) RainAmbienceAudio->Stop();
	WindAmbienceWave = nullptr;
	RainAmbienceWave = nullptr;
	RainStreakMaterial = nullptr;
	ClearGroundCover();
	ActiveRainStreakCount = 0;
	ActiveRainGroundImpactCount = 0;
	for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
		if (Firefly.IsValid()) Firefly->Destroy();
	NightFireflies.Reset();
	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabs)
		if (Crab.IsValid()) Crab->Destroy();
	DayCrabs.Reset();
	if (DayMinnowSchool.IsValid()) DayMinnowSchool->Destroy();
	DayMinnowSchool.Reset();
	Super::EndPlay(EndPlayReason);
}

void AIslandWeather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SecondsSinceWeatherSave += FMath::Max(0.f, DeltaSeconds);
	if (SecondsSinceWeatherSave >= 60.f) PersistWeatherTime();
	UpdateStorm(GetWorld()->GetTimeSeconds());
	UpdateCloudRendering();
	UpdateRainRendering();
	UpdateWindPoolResponse();
	UpdateWeatherAmbience(DeltaSeconds);
	UpdateGroundCoverSway();
}

bool AIslandWeather::InitializeRainRendering()
{
	if (bRainPoolInitialized) return true;
	if (!RainStreaks || !RainStreaks->GetStaticMesh()) return false;
	UMaterialInterface* BaseMaterial = RainStreaks->GetMaterial(0);
	if (!BaseMaterial || (BaseMaterial->GetBlendMode() != BLEND_Translucent && BaseMaterial->GetBlendMode() != BLEND_Additive))
	{
		UE_LOG(LogIslandWeather, Warning, TEXT("Rain streak rendering requires a translucent particle material; keeping the visual disabled."));
		return false;
	}
	RainStreakMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (!RainStreakMaterial) return false;
	RainStreaks->SetMaterial(0, RainStreakMaterial);
	const int32 PoolSize = FMath::Clamp(RainStreakCount, 16, 192);
	RainStreaks->ClearInstances();
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		RainStreaks->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), false);
	}
	bRainPoolInitialized = true;
	return true;
}

void AIslandWeather::UpdateRainRendering()
{
	if (!GetWorld() || !RainStreaks) return;
	CurrentRainIntensity = SampleRainIntensity(GetWorld()->GetTimeSeconds());
	if (CurrentRainIntensity <= 0.04f || !InitializeRainRendering())
	{
		ActiveRainStreakCount = 0;
		RainStreaks->SetVisibility(false, true);
		ClearRainGroundResponse();
		return;
	}

	const int32 PoolSize = FMath::Clamp(RainStreakCount, 16, 192);
	ActiveRainStreakCount = FMath::Clamp(FMath::RoundToInt(PoolSize * CurrentRainIntensity), 1, PoolSize);
	const double Now = GetWorld()->GetTimeSeconds();
	FVector VisualizationCenter = GetActorLocation();
	const AActor* WindObserver = this;
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		if (APawn* PlayerPawn = PlayerController->GetPawn())
		{
			VisualizationCenter = PlayerPawn->GetActorLocation();
			WindObserver = PlayerPawn;
		}
	// The Island-wide weather and Tideglass response continue, but local streaks and roof impacts
	// must not appear inside a verified room merely because their instances are camera-centred.
	if (UIslandEnvironmentSubsystem::IsInsideInnAt(GetWorld(), VisualizationCenter, WindObserver))
	{
		ActiveRainStreakCount = 0;
		RainStreaks->SetVisibility(false, true);
		ClearRainGroundResponse();
		if (CurrentRainIntensity >= 0.55f) UpdateRainPoolResponse();
		return;
	}
	RainStreaks->SetVisibility(true, true);
	RainStreaks->SetWorldLocation(VisualizationCenter);
	RainGroundImpactStreaks->SetWorldLocation(VisualizationCenter);
	if (CurrentRainIntensity >= 0.55f) UpdateRainPoolResponse();
	UpdateRainGroundResponse(VisualizationCenter, WindObserver, Now);
	const FVector Wind = GetLocalWind(VisualizationCenter, WindObserver);
	const FVector Flow = FVector(Wind.X, Wind.Y, -1800.f).GetSafeNormal();
	const FQuat StreakRotation = FQuat::FindBetweenNormals(FVector::UpVector, Flow);
	const float Radius = FMath::Clamp(RainVisualizationRadius, 1000.f, 12000.f);
	const float Height = FMath::Clamp(RainVisualizationHeight, 1000.f, 5000.f);
	const float FallSpeed = 1800.f;
	const float FallPeriod = Height / FallSpeed;

	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		FVector Position = FVector::ZeroVector;
		FVector Scale = FVector::ZeroVector;
		if (Index < ActiveRainStreakCount)
		{
			const double Seed = WeatherSeed * 0.071 + Index * 0.6180339887498949;
			const float X = static_cast<float>(FMath::Frac(Seed * 1.37) * 2.0 - 1.0);
			const float Y = static_cast<float>(FMath::Frac(Seed * 2.11) * 2.0 - 1.0);
			const float StartHeight = static_cast<float>(FMath::Frac(Seed * 3.17 + Now / FallPeriod) * Height);
			const float FallAge = (Height - StartHeight) / FallSpeed;
			const float OffsetX = static_cast<float>(FMath::Fmod(X * Radius + Wind.X * FallAge, Radius * 2.f));
			const float OffsetY = static_cast<float>(FMath::Fmod(Y * Radius + Wind.Y * FallAge, Radius * 2.f));
			Position = FVector(OffsetX, OffsetY, StartHeight - Height * 0.5f);
			Scale = FVector(0.008f, 0.008f, 0.45f);
		}
		const FTransform Transform(StreakRotation, Position, Scale);
		RainStreaks->UpdateInstanceTransform(Index, Transform, false, Index == PoolSize - 1, true);
	}
}

void AIslandWeather::UpdateRainGroundResponse(const FVector& Center, const AActor* Observer, double Now)
{
	if (!RainGroundImpactStreaks || !RainStreakMaterial || CurrentRainIntensity < 0.35f)
	{
		ClearRainGroundResponse();
		return;
	}

	if (!bRainGroundImpactPoolInitialized)
	{
		RainGroundImpactStreaks->SetMaterial(0, RainStreakMaterial);
		RainGroundImpactStreaks->ClearInstances();
		for (int32 Index = 0; Index < 3; ++Index)
			RainGroundImpactStreaks->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), false);
		bRainGroundImpactPoolInitialized = true;
	}

	if (Now >= NextRainGroundImpactTime)
	{
		const double Seed = WeatherSeed * 0.071 + (++RainGroundImpactSequence) * 0.6180339887498949;
		const double Angle = FMath::Frac(Seed * 1.37) * 2.0 * PI;
		const float Radius = FMath::Sqrt(static_cast<float>(FMath::Frac(Seed * 2.11))) * FMath::Min(900.f, FMath::Max(250.f, RainVisualizationRadius * 0.35f));
		const FVector Candidate = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const float TraceHeight = FMath::Clamp(RainVisualizationHeight, 1000.f, 5000.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandRainGroundImpact), false, this);
		if (Observer) Params.AddIgnoredActor(Observer);
		FHitResult Hit;
		const FVector TraceStart = Candidate + FVector(0.f, 0.f, TraceHeight * 0.5f);
		const FVector TraceEnd = Candidate - FVector(0.f, 0.f, TraceHeight * 1.5f);
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) &&
			Hit.GetActor() && !Hit.GetActor()->ActorHasTag(TEXT("TideglassPool")))
		{
			LastRainGroundImpactLocation = Hit.ImpactPoint;
			RainGroundImpactStartedAt = Now;
			ActiveRainGroundImpactCount = 3;
		}
		const float RainAlpha = FMath::Clamp((CurrentRainIntensity - 0.35f) / 0.65f, 0.f, 1.f);
		NextRainGroundImpactTime = Now + FMath::Lerp(3.0f, 0.75f, RainAlpha);
	}

	const float Age = static_cast<float>(Now - RainGroundImpactStartedAt);
	const float Alpha = FMath::Clamp(Age / 0.8f, 0.f, 1.f);
	const float Envelope = FMath::Sin(PI * Alpha);
	if (ActiveRainGroundImpactCount == 0 || Alpha >= 1.f)
	{
		ActiveRainGroundImpactCount = 0;
		RainGroundImpactStreaks->SetVisibility(false, true);
		return;
	}

	RainGroundImpactStreaks->SetVisibility(true, true);
	const FVector LocalImpact = RainGroundImpactStreaks->GetComponentTransform().InverseTransformPosition(LastRainGroundImpactLocation);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Angle = Index * (2.f * PI / 3.f) + RainGroundImpactSequence * 0.41f;
		const FVector Direction = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.85f).GetSafeNormal();
		const FVector Position = LocalImpact + FVector(Direction.X, Direction.Y, 0.f) * (4.f + 22.f * Alpha) + FVector(0.f, 0.f, 18.f * Alpha);
		const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);
		const FVector Scale(0.004f * Envelope, 0.004f * Envelope, 0.16f * Envelope);
		RainGroundImpactStreaks->UpdateInstanceTransform(Index, FTransform(Rotation, Position, Scale), false, Index == 2, true);
	}
}

void AIslandWeather::ClearRainGroundResponse()
{
	ActiveRainGroundImpactCount = 0;
	NextRainGroundImpactTime = 0.0;
	if (RainGroundImpactStreaks) RainGroundImpactStreaks->SetVisibility(false, true);
}

FVector2D AIslandWeather::CalculateAmbienceGains(float HorizontalWindSpeed, float RainIntensity, bool bIndoors)
{
	const float WindStrength = FMath::SmoothStep(18.f, 150.f, FMath::Clamp(HorizontalWindSpeed, 0.f, 300.f));
	const float RainStrength = FMath::SmoothStep(0.06f, 0.72f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	// Deliberately low ceilings: these are a quiet environmental bed, not foreground effects.
	const float ShelterScale = bIndoors ? 0.2f : 1.f;
	return FVector2D(0.055f * WindStrength * ShelterScale, 0.035f * RainStrength * ShelterScale);
}

void AIslandWeather::InitializeWeatherAmbience()
{
	if (!WindAmbienceAudio || !RainAmbienceAudio || (WindAmbienceWave && RainAmbienceWave)) return;
	constexpr int32 SampleRate = 24000;
	auto MakeWave = [this](const TCHAR* Name)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this, Name);
		if (!Wave) return static_cast<USoundWaveProcedural*>(nullptr);
		Wave->SetSampleRate(24000);
		Wave->NumChannels = 2;
		Wave->Duration = 10000.f;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Effects;
		return Wave;
	};
	WindNoiseStream.Initialize(0x49A31);
	RainNoiseStream.Initialize(0x67C21);
	WindAmbienceWave = MakeWave(TEXT("GeneratedWindAmbience"));
	RainAmbienceWave = MakeWave(TEXT("GeneratedRainAmbience"));
	WindAmbienceAudio->SetSound(WindAmbienceWave);
	RainAmbienceAudio->SetSound(RainAmbienceWave);
	WindAmbienceAudio->VolumeMultiplier = 0.f;
	RainAmbienceAudio->VolumeMultiplier = 0.f;
	QueueAmbienceSamples(WindAmbienceWave, WindNoiseStream, WindNoiseFilterLeft, WindNoiseFilterRight, false);
	QueueAmbienceSamples(RainAmbienceWave, RainNoiseStream, RainNoiseFilterLeft, RainNoiseFilterRight, true);
	QueueAmbienceSamples(WindAmbienceWave, WindNoiseStream, WindNoiseFilterLeft, WindNoiseFilterRight, false);
	QueueAmbienceSamples(RainAmbienceWave, RainNoiseStream, RainNoiseFilterLeft, RainNoiseFilterRight, true);
	static_assert(SampleRate == 24000, "Keep weather PCM and procedural-wave sample rates aligned.");
}

void AIslandWeather::QueueAmbienceSamples(USoundWaveProcedural* Wave, FRandomStream& Random,
	float& FilterLeft, float& FilterRight, bool bHighPass)
{
	if (!Wave) return;
	constexpr int32 SampleRate = 24000;
	constexpr int32 Frames = SampleRate / 2;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(Frames * 2);
	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		const float WhiteLeft = Random.FRandRange(-1.f, 1.f);
		const float WhiteRight = Random.FRandRange(-1.f, 1.f);
		// Low-pass noise gives wind a soft body; the complementary high-pass is a distant rain hiss.
		FilterLeft += (WhiteLeft - FilterLeft) * (bHighPass ? 0.10f : 0.018f);
		FilterRight += (WhiteRight - FilterRight) * (bHighPass ? 0.10f : 0.018f);
		const float SignalLeft = bHighPass ? WhiteLeft - FilterLeft : FilterLeft;
		const float SignalRight = bHighPass ? WhiteRight - FilterRight : FilterRight;
		const float Amplitude = bHighPass ? 0.10f : 0.20f;
		Samples[Frame * 2] = static_cast<int16>(FMath::Clamp(SignalLeft * Amplitude, -1.f, 1.f) * 32767.f);
		Samples[Frame * 2 + 1] = static_cast<int16>(FMath::Clamp(SignalRight * Amplitude, -1.f, 1.f) * 32767.f);
	}
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}

void AIslandWeather::UpdateWeatherAmbience(float DeltaSeconds)
{
	if (!GetWorld() || !WindAmbienceWave || !RainAmbienceWave || !WindAmbienceAudio || !RainAmbienceAudio) return;
	AmbienceUpdateAccumulator += FMath::Max(0.f, DeltaSeconds);
	if (AmbienceUpdateAccumulator < 0.25f) return;
	AmbienceUpdateAccumulator = 0.f;
	APawn* Listener = nullptr;
	if (APlayerController* Player = GetWorld()->GetFirstPlayerController()) Listener = Player->GetPawn();
	if (!Listener)
	{
		WindAmbienceAudio->SetVolumeMultiplier(0.f);
		RainAmbienceAudio->SetVolumeMultiplier(0.f);
		WindAmbienceAudio->Stop();
		RainAmbienceAudio->Stop();
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector ListenerLocation = Listener->GetActorLocation();
	const bool bIndoors = UIslandEnvironmentSubsystem::IsInsideInnAt(GetWorld(), ListenerLocation, Listener);
	const FVector2D Gains = CalculateAmbienceGains(GetLocalWind(ListenerLocation, Listener).Size(), SampleRainIntensity(Now), bIndoors);
	WindAmbienceAudio->SetVolumeMultiplier(Gains.X);
	RainAmbienceAudio->SetVolumeMultiplier(Gains.Y);
	if (Gains.X > 0.0001f)
	{
		if (!WindAmbienceAudio->IsPlaying()) WindAmbienceAudio->Play();
	}
	else WindAmbienceAudio->Stop();
	if (Gains.Y > 0.0001f)
	{
		if (!RainAmbienceAudio->IsPlaying()) RainAmbienceAudio->Play();
	}
	else RainAmbienceAudio->Stop();
	constexpr int32 BytesPerSecond = 24000 * 2 * sizeof(int16);
	if (WindAmbienceWave->GetAvailableAudioByteCount() < BytesPerSecond)
		QueueAmbienceSamples(WindAmbienceWave, WindNoiseStream, WindNoiseFilterLeft, WindNoiseFilterRight, false);
	if (RainAmbienceWave->GetAvailableAudioByteCount() < BytesPerSecond)
		QueueAmbienceSamples(RainAmbienceWave, RainNoiseStream, RainNoiseFilterLeft, RainNoiseFilterRight, true);
}

void AIslandWeather::UpdateRainPoolResponse()
{
	if (!GetWorld() || RainPoolRipple.IsValid() || GetWorld()->GetTimeSeconds() < NextRainPoolRippleTime) return;
	AActor* Pool = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("TideglassPool"))) continue;
		Pool = *It;
		break;
	}
	if (!Pool) return;
	const double Now = GetWorld()->GetTimeSeconds();
	const double Phase = Now * 1.7 + WeatherSeed * 0.37;
	const FVector Offset(FMath::Cos(Phase) * 38.f, FMath::Sin(Phase * 1.13) * 38.f, 0.f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(Pool->GetActorLocation() + Offset, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsRainImpact();
		RainPoolRipple = Ripple;
		NextRainPoolRippleTime = Now + 4.5;
	}
}

void AIslandWeather::UpdateWindPoolResponse()
{
	if (!GetWorld() || CurrentRainIntensity >= 0.55f || RainPoolRipple.IsValid() || WindPoolRipple.IsValid()) return;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextWindPoolRippleTime) return;

	AActor* Pool = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("TideglassPool"))) continue;
		Pool = *It;
		break;
	}
	if (!Pool) return;

	const FVector LocalWind = GetLocalWind(Pool->GetActorLocation(), Pool);
	const float Speed = LocalWind.Size2D();
	const float Activity = AIslandPoolRippleEffect::WindRippleActivity(Speed);
	if (Activity <= 0.f) return;

	const FVector Flow = LocalWind.GetSafeNormal2D();
	const FVector Side(-Flow.Y, Flow.X, 0.f);
	const double Phase = Now * 0.41 + WeatherSeed * 0.19;
	const FVector Offset = Flow * 28.f + Side * static_cast<float>(FMath::Sin(Phase) * 28.0);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(Pool->GetActorLocation() + Offset, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsWindImpact(Speed);
		WindPoolRipple = Ripple;
		NextWindPoolRippleTime = Now + FMath::Lerp(12.0, 5.0, static_cast<double>(Activity));
	}
}

bool AIslandWeather::InitializeCloudRendering()
{
	if (CloudComponent.IsValid() && WeatherCloudMaterial) return true;
	if (!GetWorld()) return false;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextCloudDiscoveryTime) return false;
	NextCloudDiscoveryTime = Now + 1.0;
	for (TActorIterator<AVolumetricCloud> It(GetWorld()); It; ++It)
	{
		UVolumetricCloudComponent* Cloud = It->FindComponentByClass<UVolumetricCloudComponent>();
		if (!Cloud) continue;
		UMaterialInterface* BaseMaterial = Cloud->GetMaterial();
		if (!BaseMaterial) continue;

		TArray<FMaterialParameterInfo> ScalarParameters;
		TArray<FGuid> ParameterIds;
		BaseMaterial->GetAllScalarParameterInfo(ScalarParameters, ParameterIds);
		bHasCloudCoverageParameter = ScalarParameters.ContainsByPredicate([this](const FMaterialParameterInfo& Parameter) { return Parameter.Name == CloudCoverageParameter; });
		bHasCloudDensityParameter = ScalarParameters.ContainsByPredicate([this](const FMaterialParameterInfo& Parameter) { return Parameter.Name == CloudDensityParameter; });
		bHasStormCloudsParameter = ScalarParameters.ContainsByPredicate([this](const FMaterialParameterInfo& Parameter) { return Parameter.Name == StormCloudsParameter; });
		if (!bHasCloudCoverageParameter && !bHasCloudDensityParameter && !bHasStormCloudsParameter)
		{
			if (!bCloudParameterWarningLogged)
			{
				UE_LOG(LogIslandWeather, Warning, TEXT("Cloud material %s exposes neither configured weather parameter; leaving it unchanged."), *BaseMaterial->GetName());
				bCloudParameterWarningLogged = true;
			}
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (!DynamicMaterial) return false;
		CloudComponent = Cloud;
		OriginalCloudMaterial = BaseMaterial;
		WeatherCloudMaterial = DynamicMaterial;
		if (bHasCloudCoverageParameter) OriginalCloudCoverage = DynamicMaterial->K2_GetScalarParameterValue(CloudCoverageParameter);
		if (bHasCloudDensityParameter) OriginalCloudDensity = DynamicMaterial->K2_GetScalarParameterValue(CloudDensityParameter);
		if (bHasStormCloudsParameter) OriginalStormClouds = DynamicMaterial->K2_GetScalarParameterValue(StormCloudsParameter);
		Cloud->SetMaterial(DynamicMaterial);
		UE_LOG(LogIslandWeather, Log, TEXT("Weather linked cloud material %s (coverage %s, density %s, storm %s)."), *BaseMaterial->GetName(), bHasCloudCoverageParameter ? TEXT("enabled") : TEXT("unavailable"), bHasCloudDensityParameter ? TEXT("enabled") : TEXT("unavailable"), bHasStormCloudsParameter ? TEXT("enabled") : TEXT("unavailable"));
		return true;
	}
	return false;
}

void AIslandWeather::UpdateCloudRendering()
{
	if (!GetWorld()) return;
	if (!WeatherCloudMaterial && !InitializeCloudRendering()) return;
	const float Cover = SampleCloudCover(GetWorld()->GetTimeSeconds());
	if (bHasCloudCoverageParameter)
		WeatherCloudMaterial->SetScalarParameterValue(CloudCoverageParameter, OriginalCloudCoverage + (Cover - 0.5f) * 0.08f);
	if (bHasCloudDensityParameter)
		WeatherCloudMaterial->SetScalarParameterValue(CloudDensityParameter, OriginalCloudDensity * FMath::Lerp(0.82f, 1.18f, Cover));
	if (bHasStormCloudsParameter)
		WeatherCloudMaterial->SetScalarParameterValue(StormCloudsParameter, OriginalStormClouds + 0.45f * SampleRainIntensity(GetWorld()->GetTimeSeconds()));
}

void AIslandWeather::RefreshNightEcology()
{
	NightFireflies.RemoveAll([](const TWeakObjectPtr<AIslandFirefly>& Firefly) { return !Firefly.IsValid(); });
	DayCrabs.RemoveAll([](const TWeakObjectPtr<AIslandTidepoolCrab>& Crab) { return !Crab.IsValid(); });
	if (!DayMinnowSchool.IsValid()) DayMinnowSchool.Reset();
	if (!GetWorld()) return;

	float CurrentHour = -1.f;
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		CurrentHour = It->CurrentHour;
		break;
	}
	const bool bNight = CurrentHour >= 19.f || (CurrentHour >= 0.f && CurrentHour < 5.f);
	const bool bDay = CurrentHour >= 6.f && CurrentHour < 19.f;

	AActor* Habitat = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TideglassPool")))
		{
			Habitat = *It;
			break;
		}
	}

	constexpr int32 NightPopulation = 3;
	if (CurrentHour < 0.f || !Habitat)
	{
		for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
			if (Firefly.IsValid()) Firefly->Destroy();
		NightFireflies.Reset();
		for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabs)
			if (Crab.IsValid()) Crab->Destroy();
		DayCrabs.Reset();
		if (DayMinnowSchool.IsValid()) DayMinnowSchool->Destroy();
		DayMinnowSchool.Reset();
		return;
	}

	if (!bNight)
	{
		for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
			if (Firefly.IsValid()) Firefly->Destroy();
		NightFireflies.Reset();
	}
	else
	{
		AActor* NearbyListeningStones = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TEXT("ListeningStones")) || !It->ActorHasTag(TEXT("IslandLandmark"))) continue;
			const float LandmarkSeparation = FVector::Dist2D(It->GetActorLocation(), Habitat->GetActorLocation());
			if (LandmarkSeparation > AIslandListeningStonesChime::AudibleRadius * 2.f && LandmarkSeparation <= 3500.f)
			{
				NearbyListeningStones = *It;
				break;
			}
		}
		while (NightFireflies.Num() < NightPopulation)
		{
			FVector SpawnLocation = Habitat->GetActorLocation();
			if (NightFireflies.IsEmpty() && NearbyListeningStones)
			{
				const FVector Route = (Habitat->GetActorLocation() - NearbyListeningStones->GetActorLocation()).GetSafeNormal2D();
				SpawnLocation = NearbyListeningStones->GetActorLocation() + Route * (AIslandListeningStonesChime::AudibleRadius * 0.65f);
				SpawnLocation.Z = FMath::Lerp(NearbyListeningStones->GetActorLocation().Z, Habitat->GetActorLocation().Z,
					AIslandListeningStonesChime::AudibleRadius * 0.65f / FVector::Dist2D(NearbyListeningStones->GetActorLocation(), Habitat->GetActorLocation()));
				SpawnLocation.Z += FMath::FRandRange(15.f, 35.f);
			}
			else
			{
				SpawnLocation += FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(15.f, 35.f));
			}
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AIslandFirefly* Firefly = GetWorld()->SpawnActor<AIslandFirefly>(SpawnLocation, FRotator::ZeroRotator, SpawnParameters))
				NightFireflies.Add(Firefly);
			else
				break;
		}
	}

	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabs)
		if (Crab.IsValid()) Crab->SetSheltered(!bDay);

	if (bDay)
	{
		if (!DayMinnowSchool.IsValid())
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			DayMinnowSchool = GetWorld()->SpawnActor<AIslandTidepoolMinnows>(Habitat->GetActorLocation() + FVector(0.f, 0.f, 24.f), FRotator::ZeroRotator, SpawnParameters);
		}
	}
	else if (DayMinnowSchool.IsValid())
	{
		DayMinnowSchool->Destroy();
		DayMinnowSchool.Reset();
	}

	if (!bDay)
		return;
	constexpr int32 DayPopulation = 2;
	while (DayCrabs.Num() < DayPopulation)
	{
		const int32 Index = DayCrabs.Num();
		const float Angle = Index * PI;
		const FVector ShoreOffset(FMath::Cos(Angle) * 720.f, FMath::Sin(Angle) * 720.f, 900.f);
		const FVector TraceStart = Habitat->GetActorLocation() + ShoreOffset;
		FHitResult GroundHit;
		FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(IslandTidepoolCrabShore), false, this);
		GroundParams.AddIgnoredActor(Habitat);
		if (!GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceStart - FVector(0.f, 0.f, 2200.f), ECC_WorldStatic, GroundParams)) break;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Location = GroundHit.Location + FVector(0.f, 0.f, 12.f);
		if (AIslandTidepoolCrab* Crab = GetWorld()->SpawnActor<AIslandTidepoolCrab>(Location, FRotator::ZeroRotator, SpawnParameters))
			DayCrabs.Add(Crab);
		else
			break;
	}
}

float AIslandWeather::SampleSpell(double Seconds) const
{
	// Two slow, incommensurate swells: spells last a few Island days and never quite repeat.
	const double Time = Seconds + WeatherTimeOffset;
	const double Period = FMath::Max(30.f, CycleSeconds) * 14.0;
	const double Spell = 0.5 + 0.32 * FMath::Sin(Time / Period * 2.0 * PI + WeatherSeed * 0.71)
		+ 0.18 * FMath::Sin(Time / (Period * 2.3) * 2.0 * PI + WeatherSeed * 1.9);
	return FMath::Clamp(static_cast<float>(Spell), 0.f, 1.f);
}

float AIslandWeather::SampleCloudCover(double Seconds) const
{
	const double Time = Seconds + WeatherTimeOffset;
	const double Phase = Time / FMath::Max(30.f, CycleSeconds) * 2.0 * PI + WeatherSeed * 0.37;
	// Wet spells hold the sky cloudier; dry spells clear it.
	return FMath::Clamp(static_cast<float>(0.5 + 0.5 * FMath::Sin(Phase)) + (SampleSpell(Seconds) - 0.5f) * 0.7f, 0.f, 1.f);
}

float AIslandWeather::SampleFrontStrength(double Seconds) const
{
	const double Time = Seconds + WeatherTimeOffset;
	const double RainPeriod = FMath::Max(30.f, CycleSeconds) * FMath::Clamp(RainCycleMultiplier, 1.f, 8.f);
	const double FrontPhase = Time / RainPeriod * 2.0 * PI + WeatherSeed * 0.13 + 2.1;
	return static_cast<float>(0.5 + 0.5 * FMath::Sin(FrontPhase));
}

float AIslandWeather::SampleStormIntensity(double Seconds) const
{
	// Rare by construction: the peak of a wet spell and the heart of a front at the same time.
	if (Seconds < ForcedStormUntil) return 1.f;
	const float Wet = FMath::SmoothStep(0.74f, 0.92f, SampleSpell(Seconds));
	const float Front = FMath::SmoothStep(0.80f, 0.97f, SampleFrontStrength(Seconds));
	return FMath::Clamp(Wet * Front, 0.f, 1.f);
}

float AIslandWeather::SampleRainIntensity(double Seconds) const
{
	// Wet spells let weaker fronts bring rain; dry spells need a strong one.
	const float Wetness = SampleSpell(Seconds) - 0.5f;
	const float RainFront = FMath::SmoothStep(0.62f - Wetness * 0.4f, 0.90f - Wetness * 0.2f, SampleFrontStrength(Seconds));
	const float CloudGate = FMath::SmoothStep(0.48f, 0.78f, SampleCloudCover(Seconds));
	return FMath::Clamp(FMath::Max(RainFront * CloudGate, SampleStormIntensity(Seconds)), 0.f, 1.f);
}

float AIslandWeather::GetLightningFlash() const
{
	return LastStrike.IsValid() ? LastStrike->GetFlash() : 0.f;
}

void AIslandWeather::UpdateStorm(double Now)
{
	const float Storm = SampleStormIntensity(Now);
	if (Storm < 0.3f) { NextStrikeAt = 0.0; return; }
	if (NextStrikeAt <= 0.0)
	{
		StrikeStream.Initialize(WeatherSeed * 7919 + FMath::FloorToInt(Now + WeatherTimeOffset));
		NextStrikeAt = Now + StrikeStream.FRandRange(2.f, 8.f);
		return;
	}
	if (Now < NextStrikeAt) return;
	FVector Listener = GetActorLocation();
	if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
		if (const APawn* Pawn = Player->GetPawn()) Listener = Pawn->GetActorLocation();
	StrikeNear(Listener, Storm);
	// Deeper storms strike more often.
	NextStrikeAt = Now + FMath::Lerp(24.f, 7.f, Storm) * StrikeStream.FRandRange(0.6f, 1.4f);
}

void AIslandWeather::StrikeNear(const FVector& Listener, float Storm)
{
	const float Angle = StrikeStream.FRandRange(0.f, 2.f * PI);
	const float Distance = StrikeStream.FRandRange(100000.f, 600000.f) * (1.2f - 0.5f * Storm);
	FVector Ground = Listener + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Ground + FVector(0.f, 0.f, 200000.f), Ground - FVector(0.f, 0.f, 200000.f), ECC_Visibility))
		Ground = Hit.ImpactPoint;
	else
		Ground.Z = Listener.Z;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandLightning* Lightning = GetWorld()->SpawnActor<AIslandLightning>(Ground, FRotator::ZeroRotator, Spawn))
	{
		Lightning->Strike(Ground, Listener, StrikeStream.RandHelper(INT32_MAX));
		LastStrike = Lightning;
		LastStrikeGround = Ground;
		LastThunderDelay = Lightning->GetThunderDelay();
		LastStrikeTime = GetWorld()->GetTimeSeconds();
		++StrikeCount;
	}
}

FVector AIslandWeather::SampleWind(const FVector& Position, double Seconds) const
{
	const double Time = Seconds + WeatherTimeOffset;
	const double Phase = Time / FMath::Max(30.f, CycleSeconds) * 2.0 * PI + WeatherSeed * 0.37;
	const double Heading = Phase * 0.3 + FMath::Sin(Time / 43.0) * 0.25;
	const float Storm = SampleStormIntensity(Seconds);
	// Storm gusts are quicker and harder than ordinary ones.
	const double Gust = 0.65 + 0.35 * FMath::Sin(Time / (7.0 - 3.5 * Storm) + Position.X / 1700.0 + Position.Y / 2300.0);
	const double Speed = FMath::Clamp(MaximumWindSpeed, 0.f, 300.f) * (0.25 + 0.75 * SampleCloudCover(Seconds)) * Gust * (1.0 + StormWindBoost * Storm);
	return FVector(FMath::Cos(Heading), FMath::Sin(Heading),
		0.18 * FMath::Sin(Position.X / 2100.0 + Time / 19.0)) .GetSafeNormal() * Speed;
}

FVector AIslandWeather::GetLocalWind(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return FVector::ZeroVector;
	return SampleLocalWind(Position, GetWorld()->GetTimeSeconds(), Observer);
}

FVector AIslandWeather::SampleLocalWind(const FVector& Position, double SessionSeconds, const AActor* Observer) const
{
	FVector Wind = SampleWind(Position, SessionSeconds);
	for (const FIslandTransientGust& Gust : TransientGusts)
	{
		Wind += EvaluateTransientGust(Gust, Position, SessionSeconds);
	}
	if (HasUpwindObstruction(Position, Wind, Observer)) Wind *= 0.15f;
	return Wind;
}

bool AIslandWeather::HasUpwindObstruction(const FVector& Position, const FVector& Wind, const AActor* Observer) const
{
	if (!GetWorld() || Wind.IsNearlyZero()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandWindShelter), false);
	if (Observer) Params.AddIgnoredActor(Observer);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Position, Position - Wind.GetSafeNormal() * 600.f, ECC_Visibility, Params);
}

FString AIslandWeather::DescribeWindShelterAt(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return TEXT("Wind shelter cannot be assessed because no world weather is active.");
	const FVector AmbientWind = SampleWind(Position, GetWorld()->GetTimeSeconds());
	if (AmbientWind.Size() < 35.f)
		return TEXT("The ambient wind is currently too light to judge this site's wind shelter. This check does not assess overhead rain cover or perch support.");
	if (HasUpwindObstruction(Position, AmbientWind, Observer))
		return TEXT("Solid geometry currently blocks the upwind visibility trace within six metres, so this point is sheltered from the present horizontal wind. This does not establish overhead rain cover or safe perch support.");
	return TEXT("No solid geometry blocks the current six-metre upwind trace, so this point is exposed to the present horizontal wind. This does not assess overhead rain cover or perch support.");
}

FVector AIslandWeather::EvaluateTransientGust(const FIslandTransientGust& Gust, const FVector& Position, double CurrentTime)
{
	const double Duration = Gust.ExpiresAt - Gust.StartedAt;
	if (Duration <= 0.0 || CurrentTime < Gust.StartedAt || CurrentTime >= Gust.ExpiresAt || Gust.Radius <= 0.f) return FVector::ZeroVector;
	const float SpatialWeight = 1.f - FMath::Clamp(FVector::Dist(Position, Gust.Center) / Gust.Radius, 0.f, 1.f);
	const float TemporalWeight = FMath::Clamp(static_cast<float>((Gust.ExpiresAt - CurrentTime) / Duration), 0.f, 1.f);
	return Gust.Direction * Gust.PeakSpeed * SpatialWeight * TemporalWeight;
}

void AIslandWeather::AddTransientGust(const FVector& Center, const FVector& Direction, float PeakSpeed, float Radius, float DurationSeconds)
{
	if (!GetWorld() || Direction.IsNearlyZero()) return;
	const double Now = GetWorld()->GetTimeSeconds();
	TransientGusts.RemoveAll([Now](const FIslandTransientGust& Gust) { return Gust.ExpiresAt <= Now; });
	// Keep the signal lightweight even if several residents act in quick succession.
	if (TransientGusts.Num() >= 16) TransientGusts.RemoveAt(0);

	FIslandTransientGust& Gust = TransientGusts.AddDefaulted_GetRef();
	Gust.Center = Center;
	Gust.Direction = Direction.GetSafeNormal();
	Gust.PeakSpeed = FMath::Clamp(PeakSpeed, 0.f, 300.f);
	Gust.Radius = FMath::Clamp(Radius, 100.f, 3000.f);
	Gust.StartedAt = Now;
	Gust.ExpiresAt = Now + FMath::Clamp(DurationSeconds, 1.f, 18.f);
}

FString AIslandWeather::DescribeAt(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return FString();
	const double Now = GetWorld()->GetTimeSeconds();
	const float Cloud = SampleCloudCover(Now);
	const float Rain = SampleRainIntensity(Now);
	const FVector Wind = GetLocalWind(Position, Observer);
	const bool bFeelingLocalGust = TransientGusts.ContainsByPredicate([&Position, Now](const FIslandTransientGust& Gust)
	{
		return !EvaluateTransientGust(Gust, Position, Now).IsNearlyZero(5.f);
	});
	const float Storm = SampleStormIntensity(Now);
	const float Spell = SampleSpell(Now);
	const TCHAR* Conditions = Storm > 0.35f ? TEXT("a storm: heavy rain driven by strong, gusting wind") : Rain > 0.55f ? TEXT("a passing rain shower") : Rain > 0.08f ? TEXT("light rain beginning or fading") : Cloud < 0.3f ? TEXT("mostly clear") : Cloud < 0.7f ? TEXT("cloud cover gathering or clearing") : TEXT("overcast, but currently dry");
	// Only what a resident would feel; how the weather is rendered is not part of their world.
	const float WindMetres = Wind.Size2D() / 100.f;
	const FString WindFelt = WindMetres < 0.5f ? FString(TEXT("The air is still."))
		: FString::Printf(TEXT("%s blows towards world XY (%.2f, %.2f)."),
			WindMetres < 3.f ? TEXT("A light breeze") : WindMetres < 7.f ? TEXT("A steady wind") : TEXT("A strong, gusting wind"),
			Wind.GetSafeNormal2D().X, Wind.GetSafeNormal2D().Y);
	return FString::Printf(TEXT(" Weather: %s. %s%s%s"),
		Conditions, *WindFelt,
		bFeelingLocalGust ? TEXT(" A fading local gust is still stirring the air nearby.") : TEXT(""),
		Rain > 0.55f ? TEXT(" Rain is falling hard enough to ripple the Tideglass.") : Rain > 0.08f ? TEXT(" A little rain is falling.") : TEXT(""))
		+ (Now - LastStrikeTime < 25.0
			? FString::Printf(TEXT(" Lightning flashed about %.1f kilometres away moments ago%s"), FVector::Dist2D(Position, LastStrikeGround) / 100000.f,
				Now - LastStrikeTime >= LastThunderDelay ? TEXT(", and its thunder rolled across the Island.") : TEXT("; its thunder has not reached here yet."))
			: FString())
		+ (Spell > 0.72f ? TEXT(" The weather has turned unsettled and wet over the last few days.") : Spell < 0.28f ? TEXT(" The weather has been settled and dry over the last few days.") : TEXT(""));
}

static FAutoConsoleCommandWithWorldAndArgs GIslandStormCommand(
	TEXT("Island.Storm"),
	TEXT("Developer override: a full storm with lightning for the given seconds of play (default 120). Not saved. Usage: Island.Storm [seconds]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const double Seconds = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 120.0;
		for (TActorIterator<AIslandWeather> It(World); It; ++It)
			It->ForcedStormUntil = World->GetTimeSeconds() + FMath::Clamp(Seconds, 0.0, 3600.0);
	}));
