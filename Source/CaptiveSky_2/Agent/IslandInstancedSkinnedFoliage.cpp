#include "IslandInstancedSkinnedFoliage.h"

#include "Components/InstancedSkinnedMeshComponent.h"
#include "Components/SceneComponent.h"
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
	if (!SourceMesh)
	{
		return;
	}

	for (const FTransform& Transform : InstanceTransforms)
	{
		Instances->AddInstance(Transform, 0, false);
	}
}
