#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolMinnows.generated.h"

class AIslandWeather;
class UMaterialInterface;
class UStaticMeshComponent;

/** A small daytime school that stays in the Tideglass shallows and briefly scatters from quiet attention. */
UCLASS()
class CAPTIVESKY_2_API AIslandTidepoolMinnows : public AActor
{
	GENERATED_BODY()

public:
	AIslandTidepoolMinnows();

	/** Fan away from an observer, then return to the ordinary local school path. */
	void RespondToQuietObservation(const FVector& ObserverLocation);
	/** Briefly widen the school's circling path in response to a visible, nearby surface ripple. */
	bool RespondToSurfaceRipple();
	static float RainMovementScale(float RainIntensity);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandMinnowTest;
	friend class FIslandNightEcologyTest;
	static constexpr int32 FishCount = 5;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Fish;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseShapeMaterial;
	TWeakObjectPtr<AIslandWeather> Weather;
	FVector ScatterDirection = FVector::ZeroVector;
	float ScatterRemaining = 0.f;
	float SurfacePulseRemaining = 0.f;
	float SurfacePulseCooldownRemaining = 0.f;
	float ElapsedSeconds = 0.f;
	float RavenCheckRemaining = 0.f;
	float RavenFlybyCooldownRemaining = 0.f;
	float Phase = 0.73f;

	float GetScatterAlpha() const;
	float GetSurfacePulseAlpha() const;
	void CheckForLowRavenFlyby();
	void UpdateSchool(float RainIntensity);
};
