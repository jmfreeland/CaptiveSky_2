#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandFirefly.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;
class AIslandWeather;
class AIslandListeningStonesChime;

/** Lightweight ambient-life prototype: an independently wandering point of firefly light. */
UCLASS()
class CAPTIVESKY_2_API AIslandFirefly : public AActor
{
	GENERATED_BODY()

public:
	AIslandFirefly();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Ecology", meta=(ClampMin="50", ClampMax="600"))
	float WanderRadius = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Ecology", meta=(ClampMin="20", ClampMax="220"))
	float HoverHeight = 95.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Ecology", meta=(ClampMin="0", ClampMax="100"))
	float GlowIntensity = 18.f;

	/** Bounded displacement response; the base wander remains independent of wind. */
	static FVector WindDisplacement(const FVector& LocalWind);
	/** Smooth response to strong rain without removing or owning the wild firefly. */
	static float RainActivity(float RainIntensity);
	static float RainMovementScale(float RainIntensity);
	static float RainGlowScale(float RainIntensity);
	static float RainWingBeatScale(float RainIntensity);

	/** A nearby, non-contact observation briefly changes the natural glow pulse. */
	void RespondToQuietObservation();
	/** A nearby ListeningStones tone briefly lifts the natural glow pulse. */
	void RespondToSoftChime();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> GlowingBody;

	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> LeftWing;

	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> RightWing;

	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UPointLightComponent> Glow;

	FVector HomeLocation = FVector::ZeroVector;
	float Phase = 0.f;
	float MotionRate = 1.f;
	float PulseRate = 1.f;
	float WingBeatPhase = 0.f;
	float ObservationPulseRemaining = 0.f;
	float ChimeCheckRemaining = 0.f;
	float ChimeResponseRemaining = 0.f;
	TWeakObjectPtr<AIslandWeather> Weather;
	TArray<TWeakObjectPtr<AIslandListeningStonesChime>> RespondedChimes;
	/** Swept flight target with a single tangent-slide attempt; fireflies never teleport through WorldStatic geometry. */
	FVector ResolveFlightPath(const FVector& Start, const FVector& Desired) const;
	void CheckForNearbyStoneChime();
	void UpdateGlow(double IslandTimeSeconds, float RainIntensity);
	void UpdateWings(double IslandTimeSeconds, float RainIntensity);
};
