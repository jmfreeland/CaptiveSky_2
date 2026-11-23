#include "IslandOceanSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterMeshComponent.h"
#include "WaterZoneActor.h"

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
	int32 WaterPawnBlockersChanged = 0;
	if (AStaticMeshActor* Plane = FindOceanPlane(&InWorld))
	{
		if (UStaticMeshComponent* Mesh = Plane->GetStaticMeshComponent())
		{
			// The legacy plane is hidden but may retain stale QueryAndPhysics state
			// in an already-open editor/map. Never let it become a walkable sea floor.
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UE_LOG(LogTemp, Log, TEXT("IslandOcean: disabled collision on legacy OceanPlane"));
		}
	}
	// The Water plugin can generate collision after the legacy plane is disabled.
	// Keep its underwater overlap/query behavior, but never let a generated water
	// hull or visible water mesh become a walkable floor for the player capsule.
	for (TActorIterator<AWaterBodyOcean> It(&InWorld); It; ++It)
	{
		UWaterBodyComponent* WaterBody = It->GetWaterBodyComponent();
		if (!WaterBody) continue;
		for (UPrimitiveComponent* Collision : WaterBody->GetCollisionComponents(true))
		{
			if (Collision && Collision->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
			{
				Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
				++WaterPawnBlockersChanged;
				UE_LOG(LogTemp, Warning, TEXT("IslandOcean: changed generated water collision %s from Pawn Block to Overlap"), *Collision->GetPathName());
			}
		}
	}
	for (TActorIterator<AWaterZone> It(&InWorld); It; ++It)
	{
		UWaterMeshComponent* WaterMesh = It->GetWaterMeshComponent();
		if (WaterMesh && WaterMesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
		{
			WaterMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			++WaterPawnBlockersChanged;
			UE_LOG(LogTemp, Warning, TEXT("IslandOcean: changed WaterMesh %s from Pawn Block to Overlap"), *WaterMesh->GetPathName());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("IslandOcean: checked native ocean surfaces; changed %d Pawn-blocking water components"), WaterPawnBlockersChanged);
	UMaterialInterface* Material = MaterialOverride ? MaterialOverride.Get() : LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Material && ApplyOceanMaterial(Material))
		UE_LOG(LogTemp, Log, TEXT("IslandOcean: ocean plane now uses %s"), *Material->GetPathName());
}

void UIslandOceanSubsystem::Deinitialize()
{
	RestoreOceanMaterial();
	Super::Deinitialize();
}
