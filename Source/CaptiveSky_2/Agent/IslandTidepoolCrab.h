#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolCrab.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class AIslandWeather;
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
	/** Conceal at night without destroying this resident; restores its visible local routine at dawn. */
	void SetSheltered(bool bSheltered);
	bool IsSheltered() const { return bIsSheltered; }
	/** Strong showers gently reduce exposed roaming without stopping escape or changing habitat. */
	static float RainMovementScale(float RainIntensity);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
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
