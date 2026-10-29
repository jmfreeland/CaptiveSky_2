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
class AIslandTidepoolCrab;
class AIslandCommonFrog;
class AIslandTideglassDragonfly;
class AIslandTidepoolMinnows;
class ARatAgentAIController;
class ARavenAgentAIController;
class APawn;

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
	/** Converts the Island tide into the waterline offset experienced at a shore elevation. */
	static float LocalWaterlineOffsetCm(float TideOffsetCm, float GroundZCm, float SeaLevelZCm);
	/** Heavy rain causes a smaller, temporary tuck; dry weather leaves the snail unchanged. */
	static float RainRestAmount(float RainIntensity);
	/** Stronger ambient winds cause a gentle temporary tuck, without a geometry trace. */
	static float WindRestAmount(float HorizontalWindSpeed);
	/** A nearby crab startles the snail only while actively scurrying, not while calmly foraging. */
	static bool ShouldTuckForCrab(bool bCrabScurrying, float Distance, float VerticalOffset);
	/** A close rat approach can briefly startle the snail, but a lingering rat cannot repeat it. */
	static bool ShouldTuckForRat(float Distance, float VerticalOffset);
	/** Tide and rain keep the shell briefly darkened while its exposed surface is wet. */
	static float WetShellAmount(float TideOffsetCm, float RainIntensity);
	/** A small closed graze path, in cm from home, over one calm two-minute cycle. */
	static FVector2D GrazingOffset(float ActiveSeconds, float InitialPhaseRadians);
	/** Active grazing time advances with openness and stops completely while fully tucked. */
	static float AdvanceGrazingTime(float ActiveSeconds, float DeltaSeconds, float ExposureRest);
	/** Daytime grazing remains possible, with a gentle increase after dusk. */
	static float GrazingActivityForHour(float CurrentHour);
	static constexpr float GrazingMaxExcursionCm = 16.f;
	static constexpr float MinnowAttentionHoldSeconds = 0.9f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	friend class FIslandPeriwinkleWildlifeAttentionTest;
	friend class FIslandPeriwinkleMinnowAttentionTest;
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
	float GrazingActiveSeconds = 0.f;
	float MinnowAttentionRemaining = 0.f;
	FVector MinnowAttentionLocation = FVector::ZeroVector;
	TWeakObjectPtr<APawn> PlayerPresenceLatch;
	TWeakObjectPtr<AIslandTidepoolCrab> CrabPresenceLatch;
	float ShellWetness = 0.f;
	float SeaLevelZ = 0.f;
	bool bHasSeaLevel = false;
	int32 ColorVariant = 0;
	FVector HomeLocation = FVector::ZeroVector;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	TWeakObjectPtr<AIslandWeather> IslandWeather;
	TWeakObjectPtr<ARavenAgentAIController> RavenPresenceLatch;
	TWeakObjectPtr<ARatAgentAIController> RatPresenceLatch;
	TWeakObjectPtr<AActor> WildlifeAttentionTarget;
	FVector WildlifeAttentionLocation = FVector::ZeroVector;
	float CalculateAttentionYaw(const FVector& TargetLocation) const;
	AActor* FindNearbyWildlifeAttentionTarget(FVector& OutTargetLocation) const;
	static bool ShouldAttendWildlife(float Distance, float VerticalOffset);
	void UpdateAttentionYaw(float TargetYaw, float DeltaSeconds);
	void CheckForNearbyCrabDisturbance();
	void CheckForNearbyRavenDisturbance();
	void CheckForNearbyRatDisturbance();
	void CheckForNearbyPlayerDisturbance(APawn* PlayerPawn);
	void BuildShellSpiral();
	void ApplyPalette();
};
