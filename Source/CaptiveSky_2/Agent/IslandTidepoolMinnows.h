#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolMinnows.generated.h"

class AIslandWeather;
class AIslandDayNight;
class AIslandCommonFrog;
class ARavenAgentAIController;
class ARatAgentAIController;
class APawn;
class UMaterialInterface;
class UProceduralMeshComponent;

/** A small Tideglass school that settles into a slower, tighter orbit at night. */
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
	/** True only during the brief visible fish hop that produces a surface break. */
	bool IsMakingSurfaceBreak() const;
	/** World position of the currently breaking fish, or the school origin if none is active. */
	FVector GetSurfaceBreakWorldLocation() const;
	/** An awake rat close to the shore startles the school within bounded horizontal/vertical limits. */
	static bool ShouldScatterForRat(bool bRatAwake, float DistanceCm, float VerticalOffsetCm);
	/** A nearby frog hop briefly scatters the school; a settled frog is ignored. */
	static bool ShouldScatterForFrog(bool bFrogHopping, float DistanceCm, float VerticalOffsetCm);
	static float RainMovementScale(float RainIntensity);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandMinnowTest;
	friend class FIslandMinnowFrogCueTest;
	friend class FIslandMinnowSwimmingVisitorTest;
	friend class FIslandNightEcologyTest;
	friend class FIslandCommonFrogMinnowTests;
	static constexpr int32 FishCount = 5;
	static constexpr float PoolSwimmingFootprintFraction = 0.85f;
	static constexpr float StartleRippleEdgeClearanceCm = 120.f;
	static constexpr float SurfaceBreakHopDurationSeconds = 0.54f;
	static constexpr float SurfaceBreakHopHeightCm = 14.f;
	static constexpr float SurfaceBreakContextLifetime = 305.f;
	static constexpr float ScatterSurfaceCueCooldownSeconds = 5.f;
	static constexpr float RavenCheckIntervalSeconds = 0.06f;
	static constexpr float RatCheckIntervalSeconds = 0.4f;
	static constexpr float RatPresenceRadiusCm = 550.f;
	static constexpr float RatPresenceRearmRadiusCm = 850.f;
	static constexpr float RatPresenceHeightToleranceCm = 350.f;
	static constexpr float PlayerWaterCheckIntervalSeconds = 0.2f;
	static constexpr float PlayerWaterStartleRadiusCm = 500.f;
	static constexpr float PlayerWaterRearmRadiusCm = 850.f;
	static constexpr float PlayerWaterHeightToleranceCm = 250.f;
	static constexpr float PlayerWaterStartleCooldownSeconds = 8.f;
	static constexpr float RatPresenceCooldownSeconds = 8.f;
	static constexpr float FrogPresenceRadiusCm = 700.f;
	static constexpr float FrogPresenceRearmRadiusCm = 900.f;
	static constexpr float FrogPresenceHeightToleranceCm = 350.f;
	static constexpr float FrogPresenceCooldownSeconds = 8.f;
	static float NightRestAmount(float CurrentHour);
	static float NightActivityScale(float RestAmount);
	static float SwimRateForRest(float RestAmount);
	static float SurfaceBreakInterval(float RestAmount);
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
	float SurfaceBreakHopRemaining = 0.f;
	float ElapsedSeconds = 0.f;
	float SwimPhaseRadians = 0.f;
	int32 SurfaceBreakFishIndex = 0;
	int32 SurfaceBreakHopFishIndex = INDEX_NONE;
	TWeakObjectPtr<ARavenAgentAIController> RavenPresenceLatch;
	TWeakObjectPtr<ARatAgentAIController> RatPresenceLatch;
	TWeakObjectPtr<AIslandCommonFrog> FrogPresenceLatch;
	TWeakObjectPtr<APawn> PlayerWaterPresenceLatch;
	float RavenCheckRemaining = 0.f;
	float RavenFlybyCooldownRemaining = 0.f;
	float RatCheckRemaining = 0.f;
	float RatPresenceCooldownRemaining = 0.f;
	float FrogPresenceCooldownRemaining = 0.f;
	float PlayerWaterCheckRemaining = 0.f;
	float PlayerWaterStartleCooldownRemaining = 0.f;
	float Phase = 0.73f;
	bool bHasPoolSwimmingBounds = false;

	float GetScatterAlpha() const;
	float GetSurfacePulseAlpha() const;
	float GetTideOffsetCm() const;
	float GetNightRestAmount() const;
	void CachePoolSwimmingBounds();
	FVector ClampToPoolSwimmingBounds(const FVector& DesiredRelativeLocation, float EdgeClearanceCm = 0.f) const;
	void ConfigureAppearance();
	void CheckForNearbyRavenDisturbance();
	void CheckForNearbyRatDisturbance();
	void CheckForNearbyFrogDisturbance();
	void CheckForPlayerWaterDisturbance();
	void UpdatePlayerWaterPresence(APawn* PlayerPawn, bool bIsSurfaceSwimming);
	void CheckForNearbySurfaceCue();
	void TryCreateSurfaceBreak(float RainIntensity);
	void CreateScatterSurfaceCue(const FVector& ObserverLocation);
	void UpdateSchool(float RainIntensity);
};
