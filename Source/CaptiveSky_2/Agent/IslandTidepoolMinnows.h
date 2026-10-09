#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolMinnows.generated.h"

class AIslandWeather;
class AIslandDayNight;
class ARavenAgentAIController;
class UMaterialInterface;
class UProceduralMeshComponent;

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
	/** Describes a recent surface break to a nearby observer for the next already-scheduled decision. */
	FString DescribeRecentSurfaceBreak(const FVector& ObserverLocation) const;
	static float RainMovementScale(float RainIntensity);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandMinnowTest;
	friend class FIslandNightEcologyTest;
	static constexpr int32 FishCount = 5;
	static constexpr float PoolSwimmingFootprintFraction = 0.85f;
	static constexpr float StartleRippleEdgeClearanceCm = 120.f;
	static constexpr float SurfaceBreakContextLifetime = 305.f;
	static constexpr float ScatterSurfaceCueCooldownSeconds = 5.f;
	static constexpr float RavenCheckIntervalSeconds = 0.06f;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UProceduralMeshComponent>> Fish;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UProceduralMeshComponent>> Tails;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UProceduralMeshComponent>> BodyFins;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UProceduralMeshComponent>> DorsalMarks;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseShapeMaterial;
	TWeakObjectPtr<AIslandWeather> Weather;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	FVector ScatterDirection = FVector::ZeroVector;
	FVector PoolSurfaceBoundsOrigin = FVector::ZeroVector;
	FVector2D PoolSwimmingRadii = FVector2D::ZeroVector;
	float ScatterRemaining = 0.f;
	float SurfacePulseRemaining = 0.f;
	float SurfacePulseCooldownRemaining = 0.f;
	float ScatterSurfaceCueCooldownRemaining = 0.f;
	float RippleCheckRemaining = 0.f;
	float SurfaceBreakRemaining = 13.f;
	float SurfaceBreakContextRemaining = 0.f;
	float ElapsedSeconds = 0.f;
	int32 SurfaceBreakFishIndex = 0;
	TWeakObjectPtr<ARavenAgentAIController> RavenPresenceLatch;
	float RavenCheckRemaining = 0.f;
	float RavenFlybyCooldownRemaining = 0.f;
	float Phase = 0.73f;
	bool bHasPoolSwimmingBounds = false;

	float GetScatterAlpha() const;
	float GetSurfacePulseAlpha() const;
	float GetTideOffsetCm() const;
	void CachePoolSwimmingBounds();
	FVector ClampToPoolSwimmingBounds(const FVector& DesiredRelativeLocation, float EdgeClearanceCm = 0.f) const;
	void ConfigureAppearance();
	void CheckForNearbyRavenDisturbance();
	void CheckForNaturalSurfaceRipple();
	void TryCreateSurfaceBreak(float RainIntensity);
	void CreateScatterSurfaceCue(const FVector& ObserverLocation);
	void UpdateSchool(float RainIntensity);
};
