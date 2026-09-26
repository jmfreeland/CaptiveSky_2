#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandPoolRippleEffect.generated.h"

class UPointLightComponent;

/** Short-lived ring of moving highlights across the TideglassPool prototype surface. */
UCLASS()
class CAPTIVESKY_2_API AIslandPoolRippleEffect : public AActor
{
	GENERATED_BODY()

public:
	AIslandPoolRippleEffect();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Interaction")
	TArray<TObjectPtr<UPointLightComponent>> RippleLights;

	float ElapsedSeconds = 0.f;
	float DurationSeconds = 1.6f;
	float SurfaceRadius = 150.f;
	void UpdateRipple(float Alpha);
};
