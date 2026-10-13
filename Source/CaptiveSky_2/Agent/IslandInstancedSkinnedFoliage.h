#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandInstancedSkinnedFoliage.generated.h"

class UInstancedSkinnedMeshComponent;
class USceneComponent;
class USkeletalMesh;

/**
 * A collisionless, batched host for skeletal foliage meshes such as baked
 * Megaplant outputs. Instances use transform-provider animation index zero;
 * callers must supply a compatible provider before relying on animation.
 */
UCLASS(BlueprintType)
class CAPTIVESKY_2_API AIslandInstancedSkinnedFoliage : public AActor
{
	GENERATED_BODY()

public:
	AIslandInstancedSkinnedFoliage();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage")
	TObjectPtr<USkeletalMesh> SourceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage")
	TArray<FTransform> InstanceTransforms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Foliage")
	TObjectPtr<UInstancedSkinnedMeshComponent> Instances;

	/** Rebuild from the editable transform list; safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, Category="Island|Foliage")
	void RebuildInstances();
};
