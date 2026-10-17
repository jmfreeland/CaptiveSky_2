#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolPeriwinkle.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class AIslandDayNight;
class AIslandWeather;
class ARavenAgentAIController;

/** A tiny, slow-moving periwinkle that adds close-range life to Tideglass's rocky edge. */
UCLASS()
class CAPTIVESKY_2_API AIslandTidepoolPeriwinkle : public AActor
{
	GENERATED_BODY()

public:
	AIslandTidepoolPeriwinkle();
	void SetColorVariant(int32 Variant);
	/** A brief, reversible shell-tuck response to quiet observation; never moves or persists. */
	bool RespondToQuietObservation();
	/** Smoothly tucks an intertidal snail in during the most exposed part of the ebb. */
	static float ExposureRestAmount(float TideOffsetCm);
	/** Heavy rain causes a smaller, temporary tuck; dry weather leaves the snail unchanged. */
	static float RainRestAmount(float RainIntensity);
	/** Tide and rain keep the shell briefly darkened while its exposed surface is wet. */
	static float WetShellAmount(float TideOffsetCm, float RainIntensity);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Foot;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Shell;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Aperture;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> EyeStalks;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Eyes;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UProceduralMeshComponent> ShellSpiral;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseShapeMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FootMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ShellMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpiralMaterial;

	float ElapsedSeconds = 0.f;
	float Phase = 0.f;
	float AttentionYaw = 0.f;
	float ObservationRestRemaining = 0.f;
	float ObservationCooldownRemaining = 0.f;
	float RavenCheckRemaining = 0.f;
	float ShellWetness = 0.f;
	int32 ColorVariant = 0;
	FVector HomeLocation = FVector::ZeroVector;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	TWeakObjectPtr<AIslandWeather> IslandWeather;
	TWeakObjectPtr<ARavenAgentAIController> RavenPresenceLatch;
	float CalculateAttentionYaw(const FVector& TargetLocation) const;
	void UpdateAttentionYaw(float TargetYaw, float DeltaSeconds);
	void CheckForNearbyRavenDisturbance();
	void BuildShellSpiral();
	void ApplyPalette();
};
