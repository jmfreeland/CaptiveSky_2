#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandSeaStateSubsystem.generated.h"

class UGerstnerWaterWaveGeneratorSimple;
class UGerstnerWaterWaves;
class AIslandDayNight;
class AWaterBodyOcean;
class UWaterBodyOceanComponent;

/**
 * Lets the Water-plugin ocean (WaterBodyOcean) follow the Island tide and weather. The ocean height follows the
 * shared semidiurnal spring/neap curve; weather scales the Gerstner amplitude and steepness (never wavelength,
 * so wave phase does not jump) toward a smoothed sea state. Authored height and wave values are restored at end
 * play because the ocean and wave asset are shared. Authored wave values remain the full-storm ceiling.
 * Tides work without a Gerstner generator; the weather-driven swell requires one. Console: Island.Sea [0..1|-1].
 */
UCLASS()
class CAPTIVESKY_2_API UIslandSeaStateSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr float UpdateIntervalSeconds = 0.2f;
	static constexpr float RiseSeconds = 40.f;
	static constexpr float FallSeconds = 90.f;
	static constexpr float CalmAmplitudeScale = 0.4f;
	static constexpr float CalmSteepnessScale = 0.7f;
	static constexpr float StormSteepnessScale = 1.4f;
	static constexpr float OceanTideUpdateIntervalSeconds = 2.f;
	static constexpr float OceanTideUpdateThresholdCm = 0.2f;

	/** 0 glassy to 1 full storm from the weather's storm intensity and ambient wind (0..1 of its maximum). */
	static float TargetSea(float Storm, float Wind01);
	/** Seas build faster than they settle. */
	static float StepSea(float Current, float Target, float DeltaTime);
	static float AmplitudeScale(float Sea);
	static float SteepnessScale(float Sea);
	/** What a resident on the shore sees of the sea at this level (0..1); empty when it is unremarkable. Pure. */
	static FString DescribeSea(float Sea);

	/** Remembers the authored generator values; false unless the waves use a simple Gerstner generator. */
	bool BindWaves(UGerstnerWaterWaves* Waves);
	/** Writes the generator for this sea level and recomputes the waves. */
	void ApplySea(float Sea);
	void RestoreWaves();
	bool IsBound() const { return Waves.IsValid(); }
	bool IsOceanBound() const { return OceanComponent.IsValid(); }
	float GetSea() const { return Sea; }
	/** Developer override; negative follows the weather. */
	void SetForcedSea(float InForcedSea) { ForcedSea = InForcedSea; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	friend class FIslandSeaStateTest;

	TWeakObjectPtr<UWaterBodyOceanComponent> OceanComponent;
	TWeakObjectPtr<AWaterBodyOcean> OceanActor;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	float AuthoredOceanHeightOffsetCm = 0.f;
	FTransform AuthoredOceanTransform = FTransform::Identity;
	float OceanTideRebaseCm = 0.f;
	float AppliedOceanTideOffsetCm = 0.f;
	bool bHasAppliedOceanTideOffset = false;
	float SinceOceanTideUpdate = 0.f;
	TWeakObjectPtr<UGerstnerWaterWaves> Waves;
	TWeakObjectPtr<UGerstnerWaterWaveGeneratorSimple> Generator;
	float AuthoredMinAmplitude = 0.f;
	float AuthoredMaxAmplitude = 0.f;
	float AuthoredSmallSteepness = 0.f;
	float AuthoredLargeSteepness = 0.f;
	float Sea = 0.f;
	float AppliedSea = -1.f;
	float SinceUpdate = 0.f;
	float ForcedSea = -1.f;
	bool bSeaInitialized = false;

	float SampleWeatherSea() const;
	bool BindOcean(AWaterBodyOcean* InOceanActor);
	void ApplyOceanTideOffset();
	void RestoreOceanTideOffset();
};
