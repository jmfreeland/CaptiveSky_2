#include "IslandWildlifeHome.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	const TCHAR* RatHollowMeshPath = TEXT("/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01.SM_RatHollow_01");
	const TCHAR* HazelnutCacheMeshPath = TEXT("/Game/Megaplant_Library/Tree_Common_Hazel/Instances/Branch_Common_Hazel_Nut_01.Branch_Common_Hazel_Nut_01");
	constexpr int32 MaxVisibleCacheItems = 4;
}

AIslandWildlifeHome::AIslandWildlifeHome()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ShelterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelterMesh"));
	ShelterMesh->SetupAttachment(SceneRoot);
	ShelterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShelterMesh->SetGenerateOverlapEvents(false);
	ShelterMesh->SetCanEverAffectNavigation(false);
	ShelterMesh->SetRelativeLocation(FVector(RatHollowEntranceOffsetCm, 0.f, 0.f));

	CacheDisplayMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CacheDisplayMesh"));
	CacheDisplayMesh->SetupAttachment(SceneRoot);
	CacheDisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CacheDisplayMesh->SetGenerateOverlapEvents(false);
	CacheDisplayMesh->SetCanEverAffectNavigation(false);
	CacheDisplayMesh->SetCastShadow(false);

	Tags.AddUnique(TEXT("IslandWildlifeHome"));
	Tags.AddUnique(TEXT("VisualOnly"));
}

bool AIslandWildlifeHome::ApplyHomeRecord(const FIslandWildlifeHomeRecord& Record)
{
	if (Record.Id.IsNone() || Record.Species != FName(TEXT("Rat")) || !ShelterMesh || !CacheDisplayMesh) return false;

	UStaticMesh* HollowMesh = LoadObject<UStaticMesh>(nullptr, RatHollowMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!HollowMesh) return false;

	HomeId = Record.Id;
	Species = Record.Species;
	ShelterMesh->SetStaticMesh(HollowMesh);
	CacheDisplayMesh->ClearInstances();
	CacheDisplayMesh->SetStaticMesh(nullptr);
	const bool bCachesArePubliclyVisible = Record.ShelterState == EIslandWildlifeShelterState::Settled;
	CacheDisplayMesh->SetVisibility(bCachesArePubliclyVisible, true);
	// Canonical supplies remain in world state throughout every shelter state.
	// Only a settled, trusted home places its cache outside the hollow.
	if (bCachesArePubliclyVisible)
	{
		for (const FIslandWildlifeHomeCache& Cache : Record.CachedSupplies)
		{
			if (Cache.ItemId != FName(TEXT("Hazelnut")) || Cache.Quantity < 1) continue;

			UStaticMesh* HazelnutMesh = LoadObject<UStaticMesh>(nullptr, HazelnutCacheMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!HazelnutMesh) break;
			CacheDisplayMesh->SetStaticMesh(HazelnutMesh);

			const FBoxSphereBounds MeshBounds = HazelnutMesh->GetBounds();
			const float Extent = MeshBounds.BoxExtent.GetMax();
			if (Extent <= KINDA_SMALL_NUMBER) break;
			const float MeshScale = 7.f / Extent;
			const int32 VisibleCount = FMath::Clamp(Cache.Quantity, 0, MaxVisibleCacheItems);
			for (int32 Index = 0; Index < VisibleCount; ++Index)
			{
				const float Side = (Index % 2 == 0) ? -1.f : 1.f;
				const float Row = static_cast<float>(Index / 2);
				const float GroundedZ = 2.f + (MeshBounds.BoxExtent.Z - MeshBounds.Origin.Z) * MeshScale;
				const FVector LocalLocation(-18.f - Row * 7.f, Side * (18.f + Row * 5.f), GroundedZ + (Index % 3) * 0.6f);
				const FRotator LocalRotation(0.f, static_cast<float>((Index * 67) % 360), (Index % 2 == 0) ? -8.f : 8.f);
				CacheDisplayMesh->AddInstance(FTransform(LocalRotation, LocalLocation, FVector(MeshScale)));
			}
			break;
		}
	}
	SetActorLocationAndRotation(Record.EntranceLocation, Record.EntranceRotation, false, nullptr, ETeleportType::TeleportPhysics);
	Tags.AddUnique(FName(*(TEXT("WildlifeHome_") + Record.Id.ToString())));
	Tags.AddUnique(FName(*(TEXT("Species_") + Record.Species.ToString())));
	return true;
}
