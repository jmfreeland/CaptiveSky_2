#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandFirefly.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> GlowingBody;

	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UPointLightComponent> Glow;

	FVector HomeLocation = FVector::ZeroVector;
	float Phase = 0.f;
	float MotionRate = 1.f;
	float PulseRate = 1.f;
	void UpdateGlow(double IslandTimeSeconds);
};
