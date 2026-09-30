#include "IslandOceanSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

const TCHAR* UIslandOceanSubsystem::MaterialPath = TEXT("/Game/Materials/M_IslandOcean.M_IslandOcean");
const FName UIslandOceanSubsystem::OceanActorName(TEXT("OceanPlane"));

bool UIslandOceanSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

AStaticMeshActor* UIslandOceanSubsystem::FindOceanPlane(UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		if (It->GetActorNameOrLabel() == OceanActorName.ToString() || It->GetFName() == OceanActorName) return *It;
	return nullptr;
}

bool UIslandOceanSubsystem::ApplyOceanMaterial(UMaterialInterface* Material)
{
	if (!Material) return false;
	AStaticMeshActor* Plane = FindOceanPlane(GetWorld());
	UStaticMeshComponent* Mesh = Plane ? Plane->GetStaticMeshComponent() : nullptr;
	if (!Mesh) return false;
	if (AppliedTo.Get() == Plane) { Mesh->SetMaterial(0, Material); return true; }
	RestoreOceanMaterial();
	OriginalMaterial = Mesh->GetMaterial(0);
	Mesh->SetMaterial(0, Material);
	AppliedTo = Plane;
	return true;
}

void UIslandOceanSubsystem::RestoreOceanMaterial()
{
	if (AStaticMeshActor* Plane = AppliedTo.Get())
		if (UStaticMeshComponent* Mesh = Plane->GetStaticMeshComponent())
			Mesh->SetMaterial(0, OriginalMaterial);
	AppliedTo.Reset();
	OriginalMaterial = nullptr;
}

void UIslandOceanSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UMaterialInterface* Material = MaterialOverride ? MaterialOverride.Get() : LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Material && ApplyOceanMaterial(Material))
		UE_LOG(LogTemp, Log, TEXT("IslandOcean: ocean plane now uses %s"), *Material->GetPathName());
}

void UIslandOceanSubsystem::Deinitialize()
{
	RestoreOceanMaterial();
	Super::Deinitialize();
}
