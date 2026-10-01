#include "IslandTideglassSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

const TCHAR* UIslandTideglassSubsystem::MaterialPath = TEXT("/Game/Materials/M_TideglassPool.M_TideglassPool");

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

bool UIslandTideglassSubsystem::ApplyPoolMaterial(UMaterialInterface* Material)
{
	if (!Material) return false;
	UStaticMeshComponent* PoolSurface = FindPoolSurface(GetWorld());
	if (!PoolSurface) return false;
	if (AppliedTo.Get() == PoolSurface)
	{
		PoolSurface->SetMaterial(0, Material);
		return true;
	}
	RestorePoolMaterial();
	OriginalMaterial = PoolSurface->GetMaterial(0);
	PoolSurface->SetMaterial(0, Material);
	AppliedTo = PoolSurface;
	return true;
}

void UIslandTideglassSubsystem::RestorePoolMaterial()
{
	if (UStaticMeshComponent* PoolSurface = AppliedTo.Get())
		PoolSurface->SetMaterial(0, OriginalMaterial.Get());
	AppliedTo.Reset();
	OriginalMaterial = nullptr;
}

void UIslandTideglassSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UMaterialInterface* Material = MaterialOverride
		? MaterialOverride.Get()
		: LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Material && ApplyPoolMaterial(Material))
		UE_LOG(LogTemp, Log, TEXT("IslandTideglass: runtime water material applied to the shallow pool"));
}

void UIslandTideglassSubsystem::Deinitialize()
{
	RestorePoolMaterial();
	Super::Deinitialize();
}
