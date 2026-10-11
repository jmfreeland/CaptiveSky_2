#include "IslandWildlifeHome.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	const TCHAR* RatHollowMeshPath = TEXT("/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01.SM_RatHollow_01");
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

	Tags.AddUnique(TEXT("IslandWildlifeHome"));
	Tags.AddUnique(TEXT("VisualOnly"));
}

bool AIslandWildlifeHome::ApplyHomeRecord(const FIslandWildlifeHomeRecord& Record)
{
	if (Record.Id.IsNone() || Record.Species != FName(TEXT("Rat")) || !ShelterMesh) return false;

	UStaticMesh* HollowMesh = LoadObject<UStaticMesh>(nullptr, RatHollowMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!HollowMesh) return false;

	HomeId = Record.Id;
	Species = Record.Species;
	ShelterMesh->SetStaticMesh(HollowMesh);
	SetActorLocationAndRotation(Record.EntranceLocation, Record.EntranceRotation, false, nullptr, ETeleportType::TeleportPhysics);
	Tags.AddUnique(FName(*(TEXT("WildlifeHome_") + Record.Id.ToString())));
	Tags.AddUnique(FName(*(TEXT("Species_") + Record.Species.ToString())));
	return true;
}
