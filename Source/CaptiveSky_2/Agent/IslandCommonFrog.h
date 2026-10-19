#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandCommonFrog.generated.h"

class AIslandDayNight;
class AIslandTidepoolCrab;
class AIslandTideglassDragonfly;
class AIslandWeather;
class APawn;
class ARatAgentAIController;
class ARavenAgentAIController;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** A small transient Common Frog at the damp Tideglass margin. */
UCLASS()
class CAPTIVESKY_2_API AIslandCommonFrog : public AActor
{
	GENERATED_BODY()

public:
	AIslandCommonFrog();

	/** A close, low approach can start one short hop away; distant/high observers are ignored. */
	bool RespondToQuietObservation(const FVector& ObserverLocation);
	static bool ShouldHopFromObserver(float DistanceCm, float VerticalOffsetCm);
	static bool ShouldHopFromScurryingCrab(bool bCrabScurrying, float DistanceCm, float VerticalOffsetCm);
	static bool ShouldRearmFromObserver(float DistanceCm, float RearmRadiusCm);
	/** Rain and darkness encourage brief ambient movement; dry daylight remains restful. */
	static float AmbientActivityScale(float RainIntensity, float NightAmount);
	/** A frog may notice only a low, nearby dragonfly within a small visual range. */
	static bool ShouldWatchLowDragonfly(float DistanceCm, float VerticalOffsetCm);
	/** Briefly turn toward a nearby low dragonfly without moving or calling an LLM. */
	bool RespondToNearbyDragonfly(const FVector& DragonflyLocation);
	static float HopArcHeight(float Alpha, float MaximumHeightCm);
	FVector GetHabitatHome() const { return HomeLocation; }
	bool IsHopping() const { return HopRemaining > 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandCommonFrogTests;
	friend class FIslandCommonFrogDragonflyTests;
	void FindEnvironment();
	void CheckForPlayerDisturbance();
	void CheckForNearbyWildlife();
	void CheckForNearbyDragonfly();
	bool BeginHop(const FVector& Direction, float PreferredDistanceCm);
	FVector ResolveGroundLocation(const FVector& Desired) const;
	void SetColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> DetailedBody;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> GreenParts;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> MaskParts;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> EyeParts;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> PupilParts;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseShapeMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GreenMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaskMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EyeMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PupilMaterial;

	TWeakObjectPtr<AIslandWeather> Weather;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	TWeakObjectPtr<APawn> PlayerPresenceLatch;
	TWeakObjectPtr<ARavenAgentAIController> RavenPresenceLatch;
	TWeakObjectPtr<ARatAgentAIController> RatPresenceLatch;
	TWeakObjectPtr<AIslandTidepoolCrab> CrabPresenceLatch;
	TWeakObjectPtr<AIslandTideglassDragonfly> DragonflyPresenceLatch;
	FVector HomeLocation = FVector::ZeroVector;
	FVector DragonflyLookLocation = FVector::ZeroVector;
	FVector HopStartLocation = FVector::ZeroVector;
	FVector HopTargetLocation = FVector::ZeroVector;
	FRotator RestingRotation = FRotator::ZeroRotator;
	FRandomStream Random;
	float HopElapsed = 0.f;
	float HopDuration = 0.45f;
	float HopRemaining = 0.f;
	float AmbientHopRemaining = 25.f;
	float ObserverCheckRemaining = 0.f;
	float DragonflyCheckRemaining = 0.f;
	float DragonflyAttentionRemaining = 0.f;
	float DragonflyAttentionCooldownRemaining = 0.f;
	float StartleCooldownRemaining = 0.f;
	float MaximumHomeRadiusCm = 420.f;
};
