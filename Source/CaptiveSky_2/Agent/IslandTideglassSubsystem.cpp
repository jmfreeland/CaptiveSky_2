#include "IslandTideglassSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandTideglass, Log, All);

const TCHAR* UIslandTideglassSubsystem::MaterialPath = TEXT("/Game/Materials/M_TideglassPool_Lively.M_TideglassPool_Lively");

bool UIslandTideglassSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UStaticMeshComponent* UIslandTideglassSubsystem::FindPoolSurface(UWorld* World)
{
	if (!World) return nullptr;
	AActor* PoolMarker = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TideglassPool")))
		{
			PoolMarker = *It;
			break;
		}
	}
	if (!PoolMarker) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (FVector::Dist(It->GetActorLocation(), PoolMarker->GetActorLocation()) > 25.f) continue;
		TArray<UStaticMeshComponent*> MeshComponents;
		It->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (UStaticMeshComponent* Mesh : MeshComponents)
		{
			if (!Mesh || !Mesh->GetStaticMesh() || Mesh->GetStaticMesh()->GetName() != TEXT("Sphere")) continue;
			const FVector Scale = Mesh->GetComponentScale();
			if (Scale.X > 2.f && Scale.Y > 2.f && Scale.Z < 0.25f) return Mesh;
		}
	}
	return nullptr;
}

UProceduralMeshComponent* UIslandTideglassSubsystem::CreatePoolSurfaceMesh(UStaticMeshComponent* BlockoutSurface)
{
	if (!IsValid(BlockoutSurface) || !BlockoutSurface->GetStaticMesh() || !BlockoutSurface->GetOwner()) return nullptr;
	UWorld* World = BlockoutSurface->GetWorld();
	if (!World) return nullptr;
	const FBoxSphereBounds Bounds = BlockoutSurface->GetStaticMesh()->GetBounds();
	const float RadiusX = Bounds.BoxExtent.X;
	const float RadiusY = Bounds.BoxExtent.Y;
	if (RadiusX <= KINDA_SMALL_NUMBER || RadiusY <= KINDA_SMALL_NUMBER) return nullptr;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* WaterActor = World->SpawnActor<AActor>(AActor::StaticClass(), BlockoutSurface->GetComponentTransform(), SpawnParameters);
	if (!WaterActor) return nullptr;
	UProceduralMeshComponent* WaterSurface = NewObject<UProceduralMeshComponent>(WaterActor, NAME_None, RF_Transient);
	if (!WaterSurface)
	{
		WaterActor->Destroy();
		return nullptr;
	}
	WaterActor->SetRootComponent(WaterSurface);
	WaterActor->AddInstanceComponent(WaterSurface);
	WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurface->SetCastShadow(false);
	WaterSurface->RegisterComponent();
	WaterActor->SetActorTransform(BlockoutSurface->GetComponentTransform());

	constexpr int32 SegmentCount = 64;
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32> Triangles;
	Vertices.Reserve(1 + 2 * SegmentCount);
	Normals.Reserve(1 + 2 * SegmentCount);
	UVs.Reserve(1 + 2 * SegmentCount);
	Colors.Reserve(1 + 2 * SegmentCount);
	Tangents.Reserve(1 + 2 * SegmentCount);
	Triangles.Reserve(9 * SegmentCount);

	const float WaterHeight = Bounds.BoxExtent.Z;
	const float InnerHeight = WaterHeight - 12.f;
	const float EdgeHeight = WaterHeight - 22.f;
	auto AddVertex = [&](float X, float Y, float Z)
	{
		Vertices.Emplace(X, Y, Z);
		Normals.Emplace(FVector::UpVector);
		UVs.Emplace(0.5f + X / (2.f * RadiusX), 0.5f + Y / (2.f * RadiusY));
		Colors.Emplace(FLinearColor::White);
		Tangents.Emplace(FVector(1.f, 0.f, 0.f), false);
	};

	AddVertex(0.f, 0.f, WaterHeight);
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			const float Angle = 2.f * PI * static_cast<float>(Segment) / SegmentCount;
			const float Shape = 1.f + 0.060f * FMath::Sin(3.f * Angle + 0.45f)
				+ 0.038f * FMath::Sin(5.f * Angle - 1.1f) + 0.024f * FMath::Cos(7.f * Angle + 0.7f);
			const float RadialFraction = Ring == 0 ? 0.62f : Shape;
			const float Z = Ring == 0 ? InnerHeight : EdgeHeight;
			AddVertex(FMath::Cos(Angle) * RadiusX * RadialFraction,
				FMath::Sin(Angle) * RadiusY * RadialFraction, Z);
		}
	}
	for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
	{
		const int32 Next = (Segment + 1) % SegmentCount;
		const int32 Inner = 1 + Segment;
		const int32 InnerNext = 1 + Next;
		const int32 Outer = 1 + SegmentCount + Segment;
		const int32 OuterNext = 1 + SegmentCount + Next;
		Triangles.Add(0);
		Triangles.Add(InnerNext);
		Triangles.Add(Inner);
		Triangles.Add(Inner);
		Triangles.Add(OuterNext);
		Triangles.Add(Outer);
		Triangles.Add(Inner);
		Triangles.Add(InnerNext);
		Triangles.Add(OuterNext);
	}
	WaterSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
	return WaterSurface;
}

bool UIslandTideglassSubsystem::ApplyPoolMaterial(UMaterialInterface* Material)
{
	if (!Material) return false;
	UStaticMeshComponent* PoolSurface = FindPoolSurface(GetWorld());
	if (!PoolSurface) return false;
	if (AppliedTo.IsValid() && AppliedTo.Get() == RuntimeSurface.Get())
	{
		RuntimeSurface->SetMaterial(0, Material);
		return true;
	}
	RestorePoolMaterial();
	RuntimeSurface = CreatePoolSurfaceMesh(PoolSurface);
	if (!RuntimeSurface) return false;
	BlockoutSurface = PoolSurface;
	bBlockoutWasVisible = PoolSurface->IsVisible();
	bBlockoutWasHiddenInGame = PoolSurface->bHiddenInGame;
	PoolSurface->SetVisibility(false);
	PoolSurface->SetHiddenInGame(true);
	RuntimeSurface->SetMaterial(0, Material);
	AppliedTo = RuntimeSurface;
	return true;
}

void UIslandTideglassSubsystem::RestorePoolMaterial()
{
	if (BlockoutSurface)
	{
		BlockoutSurface->SetVisibility(bBlockoutWasVisible);
		BlockoutSurface->SetHiddenInGame(bBlockoutWasHiddenInGame);
	}
	if (RuntimeSurface)
	{
		if (AActor* Owner = RuntimeSurface->GetOwner()) Owner->Destroy();
	}
	RuntimeSurface = nullptr;
	BlockoutSurface = nullptr;
	AppliedTo.Reset();
}

void UIslandTideglassSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bool bHasTideglassMarker = false;
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TideglassPool")))
		{
			bHasTideglassMarker = true;
			break;
		}
	}
	if (!bHasTideglassMarker) return;

	UMaterialInterface* Material = MaterialOverride
		? MaterialOverride.Get()
		: LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Material)
	{
		UE_LOG(LogIslandTideglass, Warning, TEXT("Tideglass runtime water is unavailable: could not load %s; the white blockout surface remains visible."), MaterialPath);
		return;
	}
	if (!ApplyPoolMaterial(Material))
	{
		UE_LOG(LogIslandTideglass, Warning, TEXT("Tideglass runtime water could not find a flattened sphere beside the TideglassPool marker."));
		return;
	}
	UE_LOG(LogIslandTideglass, Log, TEXT("Tideglass runtime water material applied to the shallow pool."));
}

void UIslandTideglassSubsystem::Deinitialize()
{
	RestorePoolMaterial();
	Super::Deinitialize();
}
