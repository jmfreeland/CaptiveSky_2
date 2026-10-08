#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandEnvironmentSubsystem.generated.h"

class UMaterialParameterCollection;
class AExponentialHeightFog;
class ULandscapeComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMaterialInstanceConstant;

USTRUCT()
struct FIslandLandscapeMaterialBackup
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<ULandscapeComponent> Component;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

/**
 * Environment-presentation layer: turns the simulation (IslandWeather, IslandDayNight) into a few
 * smoothly changing values that materials and VFX can read from one Material Parameter Collection,
 * /Game/Environment/MPC_IslandEnvironment. Without that asset, or without weather/clock actors,
 * it simply publishes nothing (or neutral clear-day values).
 *
 * Published parameters (all 0..1 unless noted):
 *   RainIntensity, Wetness (builds in rain, dries with sun and wind), CloudCover, WindSpeed (cm/s),
 *   WindDirection (vector: xyz unit direction, w = speed in cm/s), Daylight, SunHeight (-1..1),
 *   GoldenHour (peaks while the sun is low but up), IslandHour (0..24), Storm, LightningFlash, Mist,
 *   Indoors (1 only when an IslandInn roof is overhead and its tagged walls enclose the viewer).
 * Mist also thickens the level's exponential height fog (one is added for the session if the level has none).
 * CaptiveSky2.Tools.CreateEnvironmentCollection creates or updates the asset.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandEnvironmentSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* CollectionPath;
	static const TCHAR* FoliageWindCollectionPath;
	static const TArray<FName>& ScalarParameterNames();
	static const FName WindDirectionParameter;
	/** The Fab collection exposes WindDirection as a scalar; this publisher supplies radians from Atan2. */
	static const FName FoliageWindDirectionParameter;
	static const FName FoliageWindStrengthParameter;
	/** Weather-driven foliage wind is the normal runtime behavior; the opt-out restores Fab's fixed MPC defaults. */
	static bool ShouldPublishWeatherDrivenFoliageWind(bool bDisabledByCommandLine, bool bHasCollectionOverride)
	{
		return !bDisabledByCommandLine || bHasCollectionOverride;
	}
	static const FName LandscapeWetnessParameter;
	/** Wet-ground variant of the landscape graph, built by Scripts/Create-LandscapeWetMaterial.py; an optional local asset. */
	static const TCHAR* WetLandscapeMaterialPath;

	/** Tests supply a transient collection here; empty uses CollectionPath. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> CollectionOverride;
	/** Optional transient override for the Fab foliage wind collection, used by automation fixtures. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> FoliageWindCollectionOverride;

	float GetRainIntensity() const { return RainIntensity; }
	float GetWetness() const { return Wetness; }
	float GetGoldenHour() const { return GoldenHour; }
	float GetDaylight() const { return Daylight; }
	float GetIndoors() const { return Indoors; }
	float GetStorm() const { return Storm; }
	float GetCloudCover() const { return CloudCover; }

	/** Wetness after Seconds with the given rain and drying conditions. */
	static float StepWetness(float Wetness, float Rain, float Daylight, float WindSpeed, float Seconds);
	/** Preserves a material's authored dry baseline, rising smoothly to fully wet. */
	static float LandscapeWetnessValue(float AuthoredWetness, float EnvironmentWetness);
	/** Reuse an existing MID instead of creating an invalid MID-from-MID parent chain. */
	static UMaterialInstanceDynamic* GetOrCreateLandscapeWetnessInstance(UMaterialInterface* Original, UObject* Outer, bool& bOutReused);
	static float GoldenHourFor(float SunHeight);
	/** What a resident notices underfoot: lingering wetness after rain has stopped. Empty when dry or still raining. */
	static FString DescribeGround(float Wetness, float Rain);
	/**
	 * How misty the air is, 0..1: low fog on calm dawns after wet nights, a lighter haze in rain and storms,
	 * nothing in dry wind. Wind speed is cm/s; hour is Island time.
	 */
	static float MistFor(float Wetness, float Hour, float WindSpeed, float Rain, float Storm);
	/** What a resident notices about the air when mist is present. Empty when clear. */
	static FString DescribeAir(float Mist);
	/** True only when a point is horizontally enclosed by the inn and has its tagged roof overhead. */
	static bool IsInsideInnAt(UWorld* World, const FVector& Position, const AActor* Observer = nullptr);
	/** Factual, deliberately bounded report of geometric shelter at a position; empty outdoors. */
	static FString DescribeInnInteriorAt(UWorld* World, const FVector& Position, const AActor* Observer = nullptr);
	float GetMist() const { return Mist; }
	/** Developer override (Island.Mist): hold this mist amount until the given world time. Not saved. */
	float ForcedMist = -1.f;
	double ForcedMistUntil = -1.0;
	/** Developer override (Island.Wetness): hold ground wetness at this 0..1 amount; negative follows the weather. Not saved. */
	float ForcedWetness = -1.f;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	friend class FIslandEnvironmentTest;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> Collection;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> FoliageWindCollection;
	UPROPERTY(Transient)
	TArray<FIslandLandscapeMaterialBackup> LandscapeMaterialBackups;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> LandscapeOriginalMaterials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LandscapeMaterialInstances;
	UPROPERTY(Transient)
	TArray<float> LandscapeWetnessBaselines;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceConstant>> WetSwappedInstances;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> WetSwappedParents;
	TArray<uint8> LandscapeWetnessInstanceWasReused;

	float RainIntensity = 0.f;
	float Wetness = 0.f;
	float CloudCover = 0.f;
	float Daylight = 1.f;
	float SunHeight = 1.f;
	float GoldenHour = 0.f;
	float IslandHour = 12.f;
	float Storm = 0.f;
	float LightningFlash = 0.f;
	float Mist = 0.f;
	float Indoors = 0.f;
	TWeakObjectPtr<AExponentialHeightFog> Fog;
	float BaseFogDensity = 0.f;
	float BaseFogFalloff = 0.f;
	FVector Wind = FVector::ZeroVector;
	FVector FoliageWind = FVector::ZeroVector;
	bool bFoliageMaterialWindEnabled = false;
	bool bWetnessInitialized = false;
	bool bLandscapeMaterialsInitialized = false;
	float LastAppliedLandscapeWetness = -1.f;

	void InitializeLandscapeMaterials();
	void ApplyLandscapeWetness();
	/** Editor builds: points the map's landscape instance at the wet graph in memory (nothing is saved). The graph reads the collection's Wetness. */
	void UseWetLandscapeGraph();
	void RestoreLandscapeGraph();
};
