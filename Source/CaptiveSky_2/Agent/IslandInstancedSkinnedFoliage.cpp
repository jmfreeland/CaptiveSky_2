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
	if (TransformProvider != AutoTransformProvider)
	{
		// A caller replaced or cleared the generated value; do not overwrite an explicit choice.
		AutoTransformProvider = nullptr;
	}

	const UDynamicWindSkeletalData* WindData = SourceMesh
		? Cast<UDynamicWindSkeletalData>(SourceMesh->GetAssetUserDataOfClass(UDynamicWindSkeletalData::StaticClass()))
		: nullptr;
	const bool bHasEnabledWindData = WindData && WindData->bIsEnabled;
	if (AutoTransformProvider && !bHasEnabledWindData)
	{
		// A previously auto-bound tree may have been replaced with a static/ref-pose mesh.
		TransformProvider = nullptr;
		AutoTransformProvider = nullptr;
	}
	if (!TransformProvider && bHasEnabledWindData)
	{
		// PVE-exported Megaplants carry this skeletal wind profile as mesh user data.
		// UE's DynamicWind transform provider consumes that profile on the render thread.
		AutoTransformProvider = NewObject<UDynamicWindData>(this, NAME_None, RF_Transient);
		TransformProvider = AutoTransformProvider;
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
