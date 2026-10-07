#include "IslandWindArchPresentation.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IslandWeather.h"
#include "IslandWindMoteEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWindArchPresentation, Log, All);

namespace
{
	constexpr TCHAR WindArchRockMeshPath[] = TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock");
	constexpr TCHAR WindArchRockSurfacePath[] = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	constexpr TCHAR EngineCubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr float PillarStoneFootprintRatio = 0.86f;
	constexpr float MaximumPillarStoneScaleVariation = 0.98f;
	constexpr float MaximumPillarVerticalOverlap = 0.74f;
	constexpr int32 MaximumPillarLayerCount = 20;
}

AWindArchStonework::AWindArchStonework()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Stones = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stonework"));
	Stones->SetupAttachment(RootComponent);
	Stones->SetMobility(EComponentMobility::Movable);
	Stones->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stones->SetCanEverAffectNavigation(false);
	Stones->SetGenerateOverlapEvents(false);
	Stones->SetCastShadow(true);
}

bool AWindArchStonework::ShouldEmitNaturalWindMotes(float WindSpeed, float CooldownRemaining)
{
	return FMath::IsFinite(WindSpeed) && WindSpeed >= AmbientMoteWindThreshold && CooldownRemaining <= 0.f;
}

void AWindArchStonework::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AmbientMoteCooldownRemaining = FMath::Max(0.f, AmbientMoteCooldownRemaining - FMath::Max(0.f, DeltaSeconds));
	CheckForAmbientWind();
}

void AWindArchStonework::CheckForAmbientWind()
{
	UWorld* World = GetWorld();
	if (!World || AmbientMotes.IsValid()) return;
	if (!Weather.IsValid())
	{
		for (TActorIterator<AIslandWeather> It(World); It; ++It)
		{
			Weather = *It;
			break;
		}
	}
	if (!Weather.IsValid()) return;

	const FVector LocalWind = Weather->SampleWind(GetActorLocation(), World->GetTimeSeconds());
	if (!ShouldEmitNaturalWindMotes(LocalWind.Size2D(), AmbientMoteCooldownRemaining)) return;

	// An explicit user gust may already have created its own short-lived motes here.
	// Avoid doubling the light effect at the landmark.
	for (TActorIterator<AIslandWindMoteEffect> It(World); It; ++It)
	{
		if (FVector::DistSquared2D(It->GetActorLocation(), GetActorLocation()) <= FMath::Square(900.f))
		{
			AmbientMoteCooldownRemaining = 3.f;
			return;
		}
	}

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandWindMoteEffect* Motes = World->SpawnActor<AIslandWindMoteEffect>(
		GetActorLocation() + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, Spawn);
	if (!Motes) return;
	Motes->InitializeGust(LocalWind, 800.f, 8.f);
	AmbientMotes = Motes;
	AmbientMoteCooldownRemaining = AmbientMoteCooldownSeconds;
}

int32 AWindArchStonework::GetStoneCount() const
{
	return Stones ? Stones->GetInstanceCount() : 0;
}

float AWindArchStonework::GetPillarStoneScale(const FVector& LocalHalfExtent, const FVector& RockHalfExtent)
{
	if (LocalHalfExtent.X <= KINDA_SMALL_NUMBER || LocalHalfExtent.Y <= KINDA_SMALL_NUMBER ||
		RockHalfExtent.X <= KINDA_SMALL_NUMBER || RockHalfExtent.Y <= KINDA_SMALL_NUMBER)
		return 0.f;

	const float Scale = FMath::Min(LocalHalfExtent.X / RockHalfExtent.X, LocalHalfExtent.Y / RockHalfExtent.Y) *
		PillarStoneFootprintRatio;
	return FMath::IsFinite(Scale) && Scale > KINDA_SMALL_NUMBER ? Scale : 0.f;
}

int32 AWindArchStonework::GetPillarLayerCount(const FVector& LocalHalfExtent, const FVector& RockHalfExtent)
{
	const float Scale = GetPillarStoneScale(LocalHalfExtent, RockHalfExtent);
	const float MaximumSegmentHeight = RockHalfExtent.Z * Scale * MaximumPillarStoneScaleVariation * 2.f;
	const float TargetHeight = LocalHalfExtent.Z * 2.f;
	if (!FMath::IsFinite(MaximumSegmentHeight) || MaximumSegmentHeight <= KINDA_SMALL_NUMBER ||
		!FMath::IsFinite(TargetHeight) || TargetHeight <= KINDA_SMALL_NUMBER)
		return 0;

	const int32 LayerCount = FMath::Max(2, FMath::CeilToInt(TargetHeight / (MaximumSegmentHeight * 0.78f)));
	return LayerCount <= MaximumPillarLayerCount ? LayerCount : 0;
}

bool AWindArchStonework::BuildStonework(UStaticMesh* RockMesh, const FTransform& MarkerTransform,
	const TArray<AStaticMeshActor*>& Pillars, const AStaticMeshActor* Beam)
{
	if (!Stones || !RockMesh || Pillars.Num() != 2 || !Beam) return false;
	Stones->SetStaticMesh(RockMesh);
	UMaterialInterface* RockSurface = LoadObject<UMaterialInterface>(nullptr, WindArchRockSurfacePath,
		nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInstanceDynamic* StoneSurface = RockSurface ? UMaterialInstanceDynamic::Create(RockSurface, this) : nullptr;
	if (!StoneSurface)
	{
		UE_LOG(LogIslandWindArchPresentation, Warning, TEXT("Wind Arch rock presentation skipped: its subdued stone surface could not be created."));
		return false;
	}
	// Match the Island's Listening Stones: keep the irregular Starter Content rock silhouette,
	// but avoid M_Rock's stark black-white mottling that made the Arch dominate the approach.
	StoneSurface->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.28f, 0.26f, 0.22f));
	Stones->SetMaterial(0, StoneSurface);
	Stones->ClearInstances();

	// Preserve the original pillar widths and heights without stretching one rock into a
	// column. Small overlapping, aspect-preserving stones make the arch read as assembled work.
	const FVector RockExtent = RockMesh->GetBounds().BoxExtent;
	if (RockExtent.IsNearlyZero()) return false;
	int32 PillarLayerTotal = 0;
	for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
	{
		const UStaticMeshComponent* ProxyMesh = Pillars[SideIndex]->GetStaticMeshComponent();
		if (!ProxyMesh) return false;
		const FVector LocalCenter = MarkerTransform.InverseTransformPosition(ProxyMesh->Bounds.Origin);
		const FVector LocalExtent = MarkerTransform.InverseTransformVectorNoScale(ProxyMesh->Bounds.BoxExtent).GetAbs();
		const float StoneScale = GetPillarStoneScale(LocalExtent, RockExtent);
		const int32 LayerCount = GetPillarLayerCount(LocalExtent, RockExtent);
		if (StoneScale <= 0.f || LayerCount < 2) return false;

		const float SegmentHeight = 2.f * RockExtent.Z * StoneScale * MaximumPillarStoneScaleVariation;
		const float TargetHeight = 2.f * LocalExtent.Z;
		const float Spacing = FMath::Min(SegmentHeight * MaximumPillarVerticalOverlap,
			(TargetHeight - SegmentHeight) / (LayerCount - 1));
		if (!FMath::IsFinite(Spacing) || Spacing <= 0.f) return false;
		const float StackHeight = SegmentHeight + Spacing * (LayerCount - 1);
		const float FirstOffset = -0.5f * StackHeight + 0.5f * SegmentHeight;
		const float DepthOffsetRadius = LocalExtent.Y * 0.035f;
		const float ScaleVariations[3] = { 0.94f, 0.96f, MaximumPillarStoneScaleVariation };
		for (int32 Layer = 0; Layer < LayerCount; ++Layer)
		{
			const float DepthPhase = (Layer + SideIndex * 2) * 1.7f;
			const float DepthOffset = FMath::Sin(DepthPhase) * DepthOffsetRadius;
			const FVector Location(LocalCenter.X, LocalCenter.Y + DepthOffset,
				LocalCenter.Z + FirstOffset + Layer * Spacing);
			const float YawDegrees = (static_cast<float>((Layer * 7 + SideIndex * 3) % 5) - 2.f) * 3.f;
			const FQuat Rotation = MarkerTransform.InverseTransformRotation(Pillars[SideIndex]->GetActorQuat()) *
				FQuat(FVector::UpVector, FMath::DegreesToRadians(YawDegrees));
			const float Scale = StoneScale * ScaleVariations[Layer % UE_ARRAY_COUNT(ScaleVariations)];
			Stones->AddInstance(FTransform(Rotation, Location, FVector(Scale)));
		}
		PillarLayerTotal += LayerCount;
	}

	// Five rough voussoirs fit the saved lintel span and height, with only a low crown.
	const UStaticMeshComponent* BeamMesh = Beam->GetStaticMeshComponent();
	if (!BeamMesh) return false;
	const FVector BeamCenter = MarkerTransform.InverseTransformPosition(BeamMesh->Bounds.Origin);
	const float BeamOffsets[5] = { -285.f, -142.f, 0.f, 142.f, 285.f };
	const float BeamCrown[5] = { 0.f, 6.f, 10.f, 6.f, 0.f };
	const float BeamYaw[5] = { -2.f, 1.5f, 0.f, -1.5f, 2.f };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BeamOffsets); ++Index)
	{
		Stones->AddInstance(FTransform(FRotator(0.f, BeamYaw[Index], 0.f),
			FVector(BeamCenter.X + BeamOffsets[Index], BeamCenter.Y, BeamCenter.Z + BeamCrown[Index]),
			FVector(1.f, 0.35f, 0.22f)));
	}

	return Stones->GetInstanceCount() == PillarLayerTotal + UE_ARRAY_COUNT(BeamOffsets);
}

bool UIslandWindArchPresentationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandWindArchPresentationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ApplyPresentation(&InWorld);
}

void UIslandWindArchPresentationSubsystem::Deinitialize()
{
	RestorePresentation();
	Super::Deinitialize();
}

bool UIslandWindArchPresentationSubsystem::FindWindArchProxies(UWorld* World, AActor*& OutMarker,
	TArray<AStaticMeshActor*>& OutPillars, AStaticMeshActor*& OutBeam)
{
	OutMarker = nullptr;
	OutPillars.Reset();
	OutBeam = nullptr;
	if (!World) return false;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("WindArch")) && It->ActorHasTag(TEXT("IslandLandmark")))
		{
			OutMarker = *It;
			break;
		}
	}
	if (!OutMarker) return false;

	AStaticMeshActor* Pillars[2] = { nullptr, nullptr };
	const FTransform MarkerTransform = OutMarker->GetActorTransform();
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* MeshComponent = It->GetStaticMeshComponent();
		const UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		if (!Mesh || Mesh->GetPathName() != EngineCubeMeshPath) continue;

		const FVector Local = MarkerTransform.InverseTransformPosition(It->GetActorLocation());
		const FVector Extent = MeshComponent->Bounds.BoxExtent;
		if (FMath::Abs(FMath::Abs(Local.X) - 350.f) <= 20.f && FMath::Abs(Local.Y) <= 20.f &&
			FMath::Abs(Local.Z) <= 80.f && Extent.Z >= 250.f && Extent.X <= 80.f && Extent.Y <= 80.f)
		{
			const int32 PillarIndex = Local.X < 0.f ? 0 : 1;
			if (Pillars[PillarIndex]) return false;
			Pillars[PillarIndex] = *It;
			continue;
		}

		if (FMath::Abs(Local.X) <= 20.f && FMath::Abs(Local.Y) <= 20.f &&
			FMath::Abs(Local.Z - 380.f) <= 25.f && Extent.X >= 300.f && Extent.Z <= 80.f)
		{
			if (OutBeam) return false;
			OutBeam = *It;
		}
	}

	if (!Pillars[0] || !Pillars[1] || !OutBeam) return false;
	OutPillars.Add(Pillars[0]);
	OutPillars.Add(Pillars[1]);
	return true;
}

void UIslandWindArchPresentationSubsystem::ApplyPresentation(UWorld* World)
{
	if (!World || StoneworkActor.IsValid()) return;
	AActor* Marker = nullptr;
	TArray<AStaticMeshActor*> Pillars;
	AStaticMeshActor* Beam = nullptr;
	if (!FindWindArchProxies(World, Marker, Pillars, Beam)) return;

	UStaticMesh* RockMesh = LoadObject<UStaticMesh>(nullptr, WindArchRockMeshPath);
	if (!RockMesh)
	{
		UE_LOG(LogIslandWindArchPresentation, Warning, TEXT("Wind Arch rock presentation skipped: %s did not load."), WindArchRockMeshPath);
		return;
	}

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWindArchStonework* Presentation = World->SpawnActor<AWindArchStonework>(Marker->GetActorLocation(), Marker->GetActorRotation(), Spawn);
	if (!Presentation || !Presentation->BuildStonework(RockMesh, Marker->GetActorTransform(), Pillars, Beam))
	{
		if (Presentation) Presentation->Destroy();
		UE_LOG(LogIslandWindArchPresentation, Warning, TEXT("Wind Arch rock presentation could not build its transient stonework."));
		return;
	}

	HiddenProxies.Reset();
	PreviousProxyVisibility.Reset();
	for (AStaticMeshActor* Proxy : Pillars)
	{
		HiddenProxies.Add(Proxy);
		PreviousProxyVisibility.Add(Proxy->IsHidden());
	}
	HiddenProxies.Add(Beam);
	PreviousProxyVisibility.Add(Beam->IsHidden());
	for (AStaticMeshActor* Proxy : Pillars) Proxy->SetActorHiddenInGame(true);
	Beam->SetActorHiddenInGame(true);
	StoneworkActor = Presentation;
	UE_LOG(LogIslandWindArchPresentation, Log, TEXT("Wind Arch cube visuals replaced with %d transient rock forms; proxy collision and map data remain unchanged."),
		Presentation->GetStoneCount());
}

void UIslandWindArchPresentationSubsystem::RestorePresentation()
{
	for (int32 Index = 0; Index < HiddenProxies.Num(); ++Index)
	{
		if (AStaticMeshActor* Proxy = HiddenProxies[Index].Get())
			Proxy->SetActorHiddenInGame(PreviousProxyVisibility.IsValidIndex(Index) ? PreviousProxyVisibility[Index] : false);
	}
	HiddenProxies.Reset();
	PreviousProxyVisibility.Reset();
	if (AWindArchStonework* Presentation = StoneworkActor.Get()) Presentation->Destroy();
	StoneworkActor.Reset();
}
