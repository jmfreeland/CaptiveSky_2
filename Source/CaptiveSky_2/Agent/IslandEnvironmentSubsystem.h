#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandEnvironmentSubsystem.generated.h"

class UMaterialParameterCollection;

/**
 * Environment-presentation layer: turns the simulation (IslandWeather, IslandDayNight) into a few
 * smoothly changing values that materials and VFX can read from one Material Parameter Collection,
 * /Game/Environment/MPC_IslandEnvironment. Without that asset, or without weather/clock actors,
 * it simply publishes nothing (or neutral clear-day values).
 *
 * Published parameters (all 0..1 unless noted):
 *   RainIntensity, Wetness (builds in rain, dries with sun and wind), CloudCover, WindSpeed (cm/s),
 *   WindDirection (vector: xyz unit direction, w = speed in cm/s), Daylight, SunHeight (-1..1),
 *   GoldenHour (peaks while the sun is low but up), IslandHour (0..24).
 * CaptiveSky2.Tools.CreateEnvironmentCollection creates or updates the asset.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandEnvironmentSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* CollectionPath;
	static const TArray<FName>& ScalarParameterNames();
	static const FName WindDirectionParameter;

	/** Tests supply a transient collection here; empty uses CollectionPath. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> CollectionOverride;

	float GetRainIntensity() const { return RainIntensity; }
	float GetWetness() const { return Wetness; }
	float GetGoldenHour() const { return GoldenHour; }
	float GetDaylight() const { return Daylight; }

	/** Wetness after Seconds with the given rain and drying conditions. */
	static float StepWetness(float Wetness, float Rain, float Daylight, float WindSpeed, float Seconds);
	static float GoldenHourFor(float SunHeight);
	/** What a resident notices underfoot: lingering wetness after rain has stopped. Empty when dry or still raining. */
	static FString DescribeGround(float Wetness, float Rain);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> Collection;

	float RainIntensity = 0.f;
	float Wetness = 0.f;
	float CloudCover = 0.f;
	float Daylight = 1.f;
	float SunHeight = 1.f;
	float GoldenHour = 0.f;
	float IslandHour = 12.f;
	FVector Wind = FVector::ZeroVector;
};
