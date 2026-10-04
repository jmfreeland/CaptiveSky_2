#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandSeaStateSubsystem.generated.h"

class UGerstnerWaterWaveGeneratorSimple;
class UGerstnerWaterWaves;

/**
 * Lets the Water-plugin ocean (WaterBodyOcean) follow the island weather: a calm day lies low and gentle, wind
 * and storms raise the swell and sharpen the crests. It scales the ocean's Gerstner generator amplitude and
 * steepness (never wavelength, so wave phase does not jump) toward a smoothed "sea" level, and puts the authored
 * values back at end play because the wave asset is shared. Authored values are the full-storm ceiling, so the
 * water body's cached maximum wave height is never exceeded.
 * Does nothing without a WaterBodyOcean using a simple Gerstner generator. Console: Island.Sea [0..1|-1].
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

	/** 0 glassy to 1 full storm from the weather's storm intensity and ambient wind (0..1 of its maximum). */
	static float TargetSea(float Storm, float Wind01);
	/** Seas build faster than they settle. */
	static float StepSea(float Current, float Target, float DeltaTime);
	static float AmplitudeScale(float Sea);
	static float SteepnessScale(float Sea);

	/** Remembers the authored generator values; false unless the waves use a simple Gerstner generator. */
	bool BindWaves(UGerstnerWaterWaves* Waves);
	/** Writes the generator for this sea level and recomputes the waves. */
	void ApplySea(float Sea);
	void RestoreWaves();
	bool IsBound() const { return Waves.IsValid(); }
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
};
