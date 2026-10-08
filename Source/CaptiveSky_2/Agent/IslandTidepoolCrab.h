#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolCrab.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class AIslandWeather;
class AIslandDayNight;
class AIslandPoolRippleEffect;
class ARavenAgentAIController;

/** Small, untargetable shore life: independently scuttles near its Tideglass habitat. */
UCLASS()
class CAPTIVESKY_2_API AIslandTidepoolCrab : public AActor
{
	GENERATED_BODY()

public:
	AIslandTidepoolCrab();

	/** Briefly scuttle away from a quiet observer; never changes ownership or saved state. */
	void RespondToQuietObservation(const FVector& ObserverLocation);
	/** True only while the crab is actively moving away from a recent disturbance. */
	bool IsScurrying() const { return ScurryRemaining > 0.f && !bIsSheltered; }
	/** Conceal at night without destroying this resident; restores its visible local routine at dawn. */
	void SetSheltered(bool bSheltered);
	bool IsSheltered() const { return bIsSheltered; }
	/** Strong showers gently reduce exposed roaming without stopping escape or changing habitat. */
	static float RainMovementScale(float RainIntensity);
	/** Ebbing water slightly widens ordinary foraging drift; high water gently tucks it in. */
	static float TideMovementScale(float TideOffsetCm);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	friend class FRavenPerchTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Shell;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Claws;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Eyes;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseShapeMaterial;

	FVector HomeLocation = FVector::ZeroVector;
	FVector ScurryDirection = FVector::ZeroVector;
	TWeakObjectPtr<AIslandWeather> Weather;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	float Phase = 0.f;
	float ScurryRemaining = 0.f;
	float RavenFlybyCooldownRemaining = 0.f;
	float RavenCheckRemaining = 0.f;
	float RippleResponseCooldownRemaining = 0.f;
	float RippleCheckRemaining = 0.f;
	TWeakObjectPtr<ARavenAgentAIController> RavenPresenceLatch;
	bool bIsSheltered = false;
	void CheckForNearbyRavenDisturbance();
	void CheckForNearbyNaturalRipple();
	FVector ResolveGroundPath(const FVector& Start, const FVector& Desired) const;
};
