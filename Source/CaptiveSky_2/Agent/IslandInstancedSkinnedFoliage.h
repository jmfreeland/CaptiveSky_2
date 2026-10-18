#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandInstancedSkinnedFoliage.generated.h"

class UInstancedSkinnedMeshComponent;
class USceneComponent;
class USkeletalMesh;
class UTransformProviderData;

/**
 * A collisionless, batched host for skeletal foliage meshes such as baked
 * Megaplant outputs. Enabled DynamicWind mesh profiles are auto-bound unless
 * the caller supplies an explicit transform provider.
 */
UCLASS(BlueprintType)
class CAPTIVESKY_2_API AIslandInstancedSkinnedFoliage : public AActor
{
	GENERATED_BODY()

public:
	AIslandInstancedSkinnedFoliage();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage")
	TObjectPtr<USkeletalMesh> SourceMesh;

	/** Optional UE 5.8 animation source shared by the batched skeletal instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage")
	TObjectPtr<UTransformProviderData> TransformProvider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage")
	TArray<FTransform> InstanceTransforms;

	/** Start fading instances at this camera distance; zero disables a per-instance fade start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage|Culling", meta=(ClampMin="0"))
	int32 InstanceStartCullDistance = 0;

	/** Fully cull instances at this distance; zero disables the far cutoff. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage|Culling", meta=(ClampMin="0"))
	int32 InstanceEndCullDistance = 0;

	/** Screen-size threshold for stopping per-instance animation; zero inherits the UE global threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Foliage|Culling")
	float AnimationMinScreenSize = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Foliage")
	TObjectPtr<UInstancedSkinnedMeshComponent> Instances;

	/** Rebuild from the editable transform list; safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, Category="Island|Foliage")
	void RebuildInstances();

private:
	/** Tracks the provider created from SourceMesh user data so it can be cleared on mesh changes. */
	UPROPERTY(Transient)
	TObjectPtr<UTransformProviderData> AutoTransformProvider;
};
