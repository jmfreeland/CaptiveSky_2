#include "IslandListeningStonePresentation.h"

#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogListeningStonePresentation, Log, All);

namespace
{
	constexpr TCHAR ListeningStoneEngineCubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr TCHAR ListeningStoneRockSurfacePath[] = TEXT("/Game/Materials/M_StandingStoneRockSurface.M_StandingStoneRockSurface");
	constexpr float ChimeDurationSeconds = 2.8f;
	constexpr int32 StoneRingCount = 6;
	constexpr int32 StoneSideCount = 9;
	constexpr int32 StoneRingVertexCount = StoneSideCount + 1;
}

AListeningStonePresentation::AListeningStonePresentation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UProceduralMeshComponent* Stone = CreateDefaultSubobject<UProceduralMeshComponent>(
			*FString::Printf(TEXT("ListeningStone_%d"), Index));
		Stone->SetupAttachment(RootComponent);
		Stone->SetMobility(EComponentMobility::Movable);
		Stone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Stone->SetCanEverAffectNavigation(false);
		Stone->SetGenerateOverlapEvents(false);
		Stone->SetCastShadow(true);
		Stones.Add(Stone);

		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(
			*FString::Printf(TEXT("StoneResonanceLight_%d"), Index));
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensity(0.f);
		Light->SetAttenuationRadius(520.f);
		Light->SetSourceRadius(28.f);
		Light->SetLightColor(FLinearColor(0.48f, 0.76f, 1.f));
		Light->SetCastShadows(false);
		Light->SetVisibility(false);
		ResonanceLights.Add(Light);
	}
}

int32 AListeningStonePresentation::GetStoneCount() const
{
	return Stones.Num();
}

bool AListeningStonePresentation::ShouldResonateForWind(float PreviousSpeed, float CurrentSpeed)
{
	return FMath::IsFinite(PreviousSpeed) && FMath::IsFinite(CurrentSpeed) &&
		CurrentSpeed >= 90.f && CurrentSpeed - PreviousSpeed >= 45.f;
}

bool AListeningStonePresentation::BuildStoneForms(const FTransform& MarkerTransform,
	const TArray<AStaticMeshActor*>& Proxies)
{
	if (Stones.Num() != 3 || Proxies.Num() != 3 || ResonanceLights.Num() != 3) return false;
	// A single rounded mesh repeated in courses reads as a cartoon cairn at the scale of these
	// tall proxies. Build one tapered, irregular standing stone per proxy instead; the result
	// is transient, while the saved cubes continue to own collision and navigation.
	UMaterialInterface* BaseSurface = LoadObject<UMaterialInterface>(nullptr,
		ListeningStoneRockSurfacePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!BaseSurface)
	{
		UE_LOG(LogListeningStonePresentation, Warning, TEXT("Listening Stones could not load the local rock surface %s."), ListeningStoneRockSurfacePath);
		return false;
	}
	const float RingZ[StoneRingCount] = { 0.f, 0.10f, 0.31f, 0.58f, 0.82f, 0.95f };
	const float RingRadius[StoneRingCount] = { 0.56f, 0.88f, 0.83f, 0.72f, 0.53f, 0.31f };
	// The textured PBR surface needs a substantially lighter tint than the old flat blockout material.
	const FLinearColor StoneColors[3] = {
		FLinearColor(0.12f, 0.135f, 0.15f),
		FLinearColor(0.14f, 0.135f, 0.12f),
		FLinearColor(0.125f, 0.14f, 0.155f)
	};
	for (int32 Index = 0; Index < Proxies.Num(); ++Index)
	{
		const AStaticMeshActor* Proxy = Proxies[Index];
		const UStaticMeshComponent* ProxyMesh = Proxy ? Proxy->GetStaticMeshComponent() : nullptr;
		UProceduralMeshComponent* Stone = Stones[Index];
		if (!ProxyMesh || !ProxyMesh->GetStaticMesh() || !Stone) return false;

		const FBoxSphereBounds& Bounds = ProxyMesh->Bounds;
		const FVector Center = MarkerTransform.InverseTransformPosition(Bounds.Origin);
		const FVector WorldExtent = Bounds.BoxExtent;
		const FVector LocalExtent = MarkerTransform.InverseTransformVectorNoScale(WorldExtent).GetAbs();
		const FQuat LocalRotation = MarkerTransform.InverseTransformRotation(Proxy->GetActorQuat());
		const float TargetHeight = LocalExtent.Z * 2.f * StoneHeightRatio;
		const float RadiusX = LocalExtent.X * 0.48f;
		const float RadiusY = LocalExtent.Y * 0.48f;
		if (!FMath::IsFinite(TargetHeight) || TargetHeight <= KINDA_SMALL_NUMBER ||
			RadiusX <= KINDA_SMALL_NUMBER || RadiusY <= KINDA_SMALL_NUMBER) return false;

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector2D> UVs;
		Vertices.Reserve(StoneRingCount * StoneRingVertexCount + 2);
		UVs.Reserve(StoneRingCount * StoneRingVertexCount + 2);
		Triangles.Reserve((StoneRingCount - 1) * StoneSideCount * 6 + StoneSideCount * 6);
		const float BottomZ = Center.Z - LocalExtent.Z;
		FRandomStream ShapeRandom(0x531A + Index * 7919);
		const float RotationOffset = ShapeRandom.FRandRange(-0.28f, 0.28f);
		for (int32 Ring = 0; Ring < StoneRingCount; ++Ring)
		{
			const float Alpha = RingZ[Ring];
			const float DriftX = FMath::Sin(Alpha * PI * 1.6f + Index) * RadiusX * 0.10f;
			const float DriftY = FMath::Cos(Alpha * PI * 1.35f + Index * 0.7f) * RadiusY * 0.09f;
			for (int32 Side = 0; Side < StoneSideCount; ++Side)
			{
				const float Angle = RotationOffset + 2.f * PI * Side / StoneSideCount;
				const float AngularVariation = ShapeRandom.FRandRange(0.88f, 1.12f);
				const float Radius = RingRadius[Ring] * AngularVariation;
				const float CapVariation = Ring == StoneRingCount - 1 ? ShapeRandom.FRandRange(-0.025f, 0.015f) : 0.f;
				const FVector LocalPoint(
					Center.X + DriftX + FMath::Cos(Angle) * RadiusX * Radius,
					Center.Y + DriftY + FMath::Sin(Angle) * RadiusY * Radius,
					BottomZ + (Alpha + CapVariation) * TargetHeight);
				Vertices.Add(LocalRotation.RotateVector(LocalPoint - Center) + Center);
				UVs.Add(FVector2D(static_cast<float>(Side) / StoneSideCount, Alpha));
			}
			// Duplicate the seam vertex so the closing face interpolates over the small U step
			// between adjacent samples instead of smearing almost the entire texture.
			const FVector SeamVertex = Vertices[Ring * StoneRingVertexCount];
			Vertices.Add(SeamVertex);
			UVs.Add(FVector2D(1.f, Alpha));
		}
		const int32 BottomCenterIndex = Vertices.Add(LocalRotation.RotateVector(FVector(Center.X, Center.Y, BottomZ) - Center) + Center);
		UVs.Add(FVector2D(0.5f, 0.f));
		// Chipped, slightly uneven cap reads as broken stone rather than a perfect spire.
		const int32 CapCenterIndex = Vertices.Add(LocalRotation.RotateVector(
			FVector(Center.X, Center.Y, BottomZ + RingZ[StoneRingCount - 1] * TargetHeight) - Center) + Center);
		UVs.Add(FVector2D(0.5f, 1.f));
		for (int32 Ring = 0; Ring < StoneRingCount - 1; ++Ring)
		{
			for (int32 Side = 0; Side < StoneSideCount; ++Side)
			{
				const int32 A = Ring * StoneRingVertexCount + Side;
				const int32 B = A + 1;
				const int32 C = (Ring + 1) * StoneRingVertexCount + Side + 1;
				const int32 D = (Ring + 1) * StoneRingVertexCount + Side;
				Triangles.Add(A);
				Triangles.Add(B);
				Triangles.Add(C);
				Triangles.Add(A);
				Triangles.Add(C);
				Triangles.Add(D);
			}
		}
		const int32 TopRingStart = (StoneRingCount - 1) * StoneRingVertexCount;
		const int32 BottomRingStart = 0;
		for (int32 Side = 0; Side < StoneSideCount; ++Side)
		{
			const int32 NextSide = Side + 1;
			Triangles.Add(BottomCenterIndex);
			Triangles.Add(BottomRingStart + NextSide);
			Triangles.Add(BottomRingStart + Side);
			Triangles.Add(TopRingStart + Side);
			Triangles.Add(TopRingStart + NextSide);
			Triangles.Add(CapCenterIndex);
		}

		Stone->ClearAllMeshSections();
		const TArray<FVector> Normals;
		const TArray<FLinearColor> VertexColors;
		const TArray<FProcMeshTangent> Tangents;
		Stone->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
		UMaterialInstanceDynamic* StoneSurface = UMaterialInstanceDynamic::Create(BaseSurface, Stone);
		if (!StoneSurface) return false;
		StoneSurface->SetVectorParameterValue(TEXT("Color"), StoneColors[Index]);
		StoneSurface->SetVectorParameterValue(TEXT("BaseColor"), StoneColors[Index]);
		Stone->SetMaterial(0, StoneSurface);
		ResonanceLights[Index]->SetRelativeLocation(Center);
	}
	return GetStoneCount() == 3;
}

void AListeningStonePresentation::BeginResonance(float WindSpeed)
{
	SampledWindSpeed = FMath::IsFinite(WindSpeed) ? FMath::Clamp(WindSpeed, 0.f, 300.f) : 0.f;
	ResonanceElapsed = 0.f;
	bIsResonating = true;
	LastAmbientChimeAt = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAmbientChimeAt;
	SetActorTickEnabled(true);
	for (UPointLightComponent* Light : ResonanceLights)
	{
		if (Light) Light->SetVisibility(true);
	}
}

void AListeningStonePresentation::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
}

void AListeningStonePresentation::CheckForNaturalGust(float DeltaSeconds)
{
	AmbientWindCheckAccumulator += FMath::Max(0.f, DeltaSeconds);
	if (AmbientWindCheckAccumulator < 1.f) return;
	AmbientWindCheckAccumulator = FMath::Fmod(AmbientWindCheckAccumulator, 1.f);
	if (!Weather.IsValid() || !GetWorld()) return;

	ObserveAmbientWind(Weather->GetLocalWind(GetActorLocation(), this).Size2D());
}

void AListeningStonePresentation::ObserveAmbientWind(float CurrentSpeed)
{
	CurrentSpeed = FMath::IsFinite(CurrentSpeed) ? FMath::Clamp(CurrentSpeed, 0.f, 300.f) : 0.f;
	const bool bGustOnset = bHasAmbientWindSample && ShouldResonateForWind(LastAmbientWindSpeed, CurrentSpeed);
	LastAmbientWindSpeed = CurrentSpeed;
	bHasAmbientWindSample = true;
	if (!bGustOnset || GetResonanceRemaining() > 0.f || GetWorld()->GetTimeSeconds() - LastAmbientChimeAt < 45.0)
		return;

	BeginNaturalResonance(CurrentSpeed);
}

void AListeningStonePresentation::BeginNaturalResonance(float WindSpeed)
{
	BeginResonance(WindSpeed);
	UWorld* World = GetWorld();
	if (!World) return;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(GetActorLocation(), GetActorRotation(), Spawn))
		Chime->BeginChime(WindSpeed);
}

void AListeningStonePresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckForNaturalGust(DeltaSeconds);
	if (!bIsResonating) return;
	ResonanceElapsed += FMath::Max(0.f, DeltaSeconds);
	if (ResonanceElapsed >= ResonanceDuration)
	{
		for (UPointLightComponent* Light : ResonanceLights)
		{
			if (Light)
			{
				Light->SetIntensity(0.f);
				Light->SetVisibility(false);
			}
		}
		ResonanceElapsed = 0.f;
		bIsResonating = false;
		return;
	}

	const float Envelope = FMath::Exp(-0.82f * ResonanceElapsed) * FMath::Clamp(1.f - ResonanceElapsed / ResonanceDuration, 0.f, 1.f);
	const float WindLift = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 300.f), FVector2D(0.88f, 1.12f), SampledWindSpeed);
	for (int32 Index = 0; Index < ResonanceLights.Num(); ++Index)
	{
		UPointLightComponent* Light = ResonanceLights[Index];
		if (!Light) continue;
		const float Pulse = 0.68f + 0.32f * FMath::Sin((ResonanceElapsed * 2.8f + Index * 0.31f) * 2.f * PI);
		Light->SetIntensity(480.f * WindLift * Envelope * Pulse);
	}
}

bool UIslandListeningStonePresentationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandListeningStonePresentationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ApplyPresentation(&InWorld);
}

void UIslandListeningStonePresentationSubsystem::Deinitialize()
{
	RestorePresentation();
	Super::Deinitialize();
}

bool UIslandListeningStonePresentationSubsystem::FindStoneProxies(UWorld* World, AActor*& OutMarker,
	TArray<AStaticMeshActor*>& OutProxies)
{
	OutMarker = nullptr;
	OutProxies.Reset();
	if (!World) return false;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark")))
		{
			if (OutMarker) return false;
			OutMarker = *It;
		}
	}
	if (!OutMarker) return false;

	const FTransform MarkerTransform = OutMarker->GetActorTransform();
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* MeshComponent = It->GetStaticMeshComponent();
		const UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		if (!Mesh || Mesh->GetPathName() != ListeningStoneEngineCubeMeshPath) continue;

		const FVector Local = MarkerTransform.InverseTransformPosition(It->GetActorLocation());
		const FVector Extent = MeshComponent->Bounds.BoxExtent;
		if (FMath::Abs(Local.X) > 500.f || FMath::Abs(Local.Y) > 500.f || FMath::Abs(Local.Z) > 350.f ||
			Extent.Z < 90.f || Extent.Z > 180.f || Extent.X > 100.f || Extent.Y > 100.f) continue;
		OutProxies.Add(*It);
	}

	if (OutProxies.Num() != 3)
	{
		UE_LOG(LogListeningStonePresentation, Warning, TEXT("Listening Stones presentation skipped: expected 3 nearby cube proxies, found %d."), OutProxies.Num());
		OutProxies.Reset();
		return false;
	}
	OutProxies.Sort([&MarkerTransform](const AStaticMeshActor& A, const AStaticMeshActor& B)
	{
		const FVector LocalA = MarkerTransform.InverseTransformPosition(A.GetActorLocation());
		const FVector LocalB = MarkerTransform.InverseTransformPosition(B.GetActorLocation());
		return LocalA.X == LocalB.X ? LocalA.Y < LocalB.Y : LocalA.X < LocalB.X;
	});
	return true;
}

void UIslandListeningStonePresentationSubsystem::NotifyChime(float WindSpeed)
{
	if (AListeningStonePresentation* Presentation = PresentationActor.Get()) Presentation->BeginResonance(WindSpeed);
}

void UIslandListeningStonePresentationSubsystem::ApplyPresentation(UWorld* World)
{
	if (!World || PresentationActor.IsValid()) return;
	AActor* Marker = nullptr;
	TArray<AStaticMeshActor*> Proxies;
	if (!FindStoneProxies(World, Marker, Proxies)) return;

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AListeningStonePresentation* Presentation = World->SpawnActor<AListeningStonePresentation>(Marker->GetActorLocation(), Marker->GetActorRotation(), Spawn);
	if (!Presentation || !Presentation->BuildStoneForms(Marker->GetActorTransform(), Proxies))
	{
		if (Presentation) Presentation->Destroy();
		UE_LOG(LogListeningStonePresentation, Warning, TEXT("Listening Stones could not build three fitted rock forms."));
		return;
	}

	HiddenProxies.Reset();
	PreviousProxyVisibility.Reset();
	for (AStaticMeshActor* Proxy : Proxies)
	{
		HiddenProxies.Add(Proxy);
		PreviousProxyVisibility.Add(Proxy->IsHidden());
		Proxy->SetActorHiddenInGame(true);
	}
	PresentationActor = Presentation;
	UE_LOG(LogListeningStonePresentation, Log, TEXT("Three cube visuals now present as transient collisionless irregular standing stones; map proxies remain unchanged."));
}

void UIslandListeningStonePresentationSubsystem::RestorePresentation()
{
	for (int32 Index = 0; Index < HiddenProxies.Num(); ++Index)
	{
		if (AStaticMeshActor* Proxy = HiddenProxies[Index].Get())
			Proxy->SetActorHiddenInGame(PreviousProxyVisibility.IsValidIndex(Index) ? PreviousProxyVisibility[Index] : false);
	}
	HiddenProxies.Reset();
	PreviousProxyVisibility.Reset();
	if (AListeningStonePresentation* Presentation = PresentationActor.Get()) Presentation->Destroy();
	PresentationActor.Reset();
}
