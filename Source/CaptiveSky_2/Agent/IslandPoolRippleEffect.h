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
	/** Reduce the size and brightness for a subtle weather-driven water impact. */
	void ConfigureAsRainImpact();
	/** Size a gentle wind ripple from the measured local wind speed (cm/s). */
	void ConfigureAsWindImpact(float HorizontalWindSpeed);
	/** A brief, subtle surface break caused by a fish near the shallows. */
	void ConfigureAsMinnowImpact();
	/** A restrained surface cue when the school startles from nearby quiet attention. */
	void ConfigureAsMinnowStartleImpact();
	static float WindRippleActivity(float HorizontalWindSpeed);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandMinnowTest;
	friend class FIslandNightEcologyTest;
	friend class FIslandWeatherTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Interaction")
	TArray<TObjectPtr<UPointLightComponent>> RippleLights;

	float ElapsedSeconds = 0.f;
	float DurationSeconds = 1.6f;
	float SurfaceRadius = 150.f;
	float PeakLightIntensity = 55.f;
	void UpdateRipple(float Alpha);
};
