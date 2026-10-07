#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTideglassDragonfly.generated.h"

class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AIslandWeather;

/** A small, untargetable dragonfly that patrols above the Tideglass shore by day. */
UCLASS()
class CAPTIVESKY_2_API AIslandTideglassDragonfly : public AActor
{
	GENERATED_BODY()

public:
	AIslandTideglassDragonfly();

	/** A brief, reversible flight burst away from a quiet nearby observer. */
	void RespondToQuietObservation(const FVector& ObserverLocation);
	/** Assign one of three stable natural color morphs after spawning. */
	void SetColorVariant(int32 Variant);
	static FVector WindDisplacement(const FVector& LocalWind);
	static float RainMovementScale(float RainIntensity);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Thorax;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Abdomen;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UProceduralMeshComponent>> Wings;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> WingBaseMaterial;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WingMaterials;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
	TWeakObjectPtr<AIslandWeather> Weather;
	FVector HomeLocation = FVector::ZeroVector;
	FVector ScatterDirection = FVector::ZeroVector;
	float Phase = 0.f;
	float MotionRate = 1.f;
	float WingPhase = 0.f;
	float ScatterRemaining = 0.f;
	float RavenFlybyCooldownRemaining = 0.f;
	float RavenCheckRemaining = 0.f;
	float RippleInterestRemaining = 0.f;
	float RippleInterestCooldownRemaining = 0.f;
	float RippleCheckRemaining = 0.f;
	FVector RippleInterestLocation = FVector::ZeroVector;
	int32 ColorVariant = 0;
	void ConfigureAppearance();
	void CheckForLowRavenFlyby();
	void CheckForNearbyNaturalSurfaceRipple();
	bool RespondToSurfaceRipple(const FVector& RippleLocation);
	float GetRippleInterestAlpha() const;
	FVector ResolveFlightPath(const FVector& Start, const FVector& Desired) const;
};
