#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandWindMoteEffect.generated.h"

class UPointLightComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** Three short-lived light motes make a WindArch's simulated local gust legible. */
UCLASS()
class CAPTIVESKY_2_API AIslandWindMoteEffect : public AActor
{
	GENERATED_BODY()

public:
	AIslandWindMoteEffect();
	void InitializeGust(const FVector& Direction, float Radius, float DurationSeconds);
	/** Finds the nearest still-visible gust mote for a nearby listener. */
	bool FindNearestVisibleMote(const FVector& Origin, float MaxDistance, FVector& OutLocation) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Interaction")
	TArray<TObjectPtr<UStaticMeshComponent>> MoteMeshes;
	UPROPERTY(VisibleAnywhere, Category="Island|Interaction")
	TArray<TObjectPtr<UPointLightComponent>> MoteLights;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MoteMaterials;

	FVector FlowDirection = FVector::ForwardVector;
	float FlowRadius = 1400.f;
	float DurationSeconds = 18.f;
	float ElapsedSeconds = 0.f;
	void UpdateMotes(float Alpha);
};
