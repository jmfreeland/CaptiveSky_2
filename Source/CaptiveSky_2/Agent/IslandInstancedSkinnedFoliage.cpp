#include "IslandInstancedSkinnedFoliage.h"

#include "Components/InstancedSkinnedMeshComponent.h"
#include "Components/SceneComponent.h"
#include "DynamicWindData.h"
#include "DynamicWindSkeletalData.h"
#include "Engine/SkeletalMesh.h"

AIslandInstancedSkinnedFoliage::AIslandInstancedSkinnedFoliage()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Instances = CreateDefaultSubobject<UInstancedSkinnedMeshComponent>(TEXT("SkinnedInstances"));
	Instances->SetupAttachment(SceneRoot);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetGenerateOverlapEvents(false);
	Instances->SetCanEverAffectNavigation(false);
}

void AIslandInstancedSkinnedFoliage::RebuildInstances()
{
	if (!Instances)
	{
		return;
	}

	Instances->ClearInstances();
	Instances->SetSkinnedAssetAndUpdate(SourceMesh);
	if (!TransformProvider && SourceMesh)
	{
		const UDynamicWindSkeletalData* WindData = Cast<UDynamicWindSkeletalData>(
			SourceMesh->GetAssetUserDataOfClass(UDynamicWindSkeletalData::StaticClass()));
		if (WindData && WindData->bIsEnabled)
		{
			// PVE-exported Megaplants carry this skeletal wind profile as mesh user data.
			// UE's DynamicWind transform provider consumes that profile on the render thread.
			TransformProvider = NewObject<UDynamicWindData>(this, NAME_None, RF_Transient);
		}
	}
	Instances->SetTransformProvider(TransformProvider);
	if (!SourceMesh)
	{
		return;
	}

	for (const FTransform& Transform : InstanceTransforms)
	{
		Instances->AddInstance(Transform, 0, false);
	}
}
