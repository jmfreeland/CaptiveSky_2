#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandDayNight.generated.h"

class ADirectionalLight; class UMaterialInterface;
class ASkyLight; class UProceduralMeshComponent;
class UDirectionalLightComponent;

/** Shared Island clock; drives the existing atmosphere sun and a separate moon light. */
UCLASS()
class CAPTIVESKY_2_API AIslandDayNight : public AActor
{
	GENERATED_BODY()
public:
	AIslandDayNight();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Time", meta=(ClampMin="1"))
	float DayLengthMinutes = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Time", meta=(ClampMin="0", ClampMax="24"))
	float StartHour = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Time")
	bool bAdvanceTime = true;
	/** Resume play at the hour the previous session ended. Start Hour then applies only when no time has been saved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Time")
	bool bResumeSavedTime = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Time")
	float CurrentHour = 9.f;
	/** Counts Island days from 1, advancing at midnight; persisted with the hour. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Time")
	int32 DayNumber = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting")
	TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting")
	TObjectPtr<ASkyLight> Sky;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting", meta=(ClampMin="0"))
	float DaySunIntensity = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting", meta=(ClampMin="0"))
	float MoonIntensity = 1.5f;
	/** Fixed-exposure night lighting needs a stronger sky fill for the Island terrain to remain readable. */
	static constexpr float NightSkylightFloor = 2.5f;
	/** Share of full-moon light the moon keeps at new moon (starlit night) so the Island never goes pitch black. */
	static constexpr float NewMoonLightFloor = 0.45f;
	/** Approximate synodic month used for the repeating Island moon-light cycle. */
	static constexpr double LunarCycleDays = 29.53059;
	/** Deterministic depth-tested star points, grouped to emerge through twilight. */
	static constexpr int32 NightStarCount = 1200;
	static constexpr int32 NightStarSectionCount = 6;
	/** The sky-shell radius and card half-size keep stars above a pixel at gameplay scale. */
	static constexpr float NightStarShellRadius = 2000000.f;
	static constexpr float NightStarHalfSizeMin = 1200.f;
	static constexpr float NightStarHalfSizeMax = 2400.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Lighting")
	TObjectPtr<UDirectionalLightComponent> Moon;
	/** Dim, shadowless sky fill that stays above the horizon all night. The true moon sits below it for much of the cycle (a new moon never rises at night), so on its own it left the Island black. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Lighting")
	TObjectPtr<UDirectionalLightComponent> Starlight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting", meta=(ClampMin="0"))
	float StarlightIntensity = 1.2f;
	UProceduralMeshComponent* Starfield = nullptr;

	static float WrapHour(double Hour);
	static float SunHeight(float Hour);
	/** 0 by day, ramping to 1 once the sun is well below the horizon; the same ramp that fades the moon and starlight in. */
	static float NightAmount(float Hour);
	/** Gentle shadowless cool fill during low-angle sunrise/sunset, fading to zero under a high sun. */
	static constexpr float TwilightFillStrength = 0.6f;
	static float TwilightFillAmount(float Hour);
	static float LunarPhaseProgress(int32 IslandDay, float IslandHour);
	static float LunarIllumination(int32 IslandDay, float IslandHour);
	/** Moon light scale for a lunar illumination: NewMoonLightFloor at new moon, 1 at full. */
	static float MoonlightScale(float LunarIlluminationAmount);
	static float CloudSunlightTransmission(float CloudCover);
	static float CloudSkylightTransmission(float CloudCover);
	FString DescribeTime() const;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	float SecondsSinceSave = 0.f;
	UMaterialInterface* StarMaterial = nullptr;
	void BuildStarfield();
	void UpdateStarfieldVisibility(float SolarElevation);
	void PersistHour();
	friend class FIslandClockTest;
	friend class FIslandClockPersistenceTest;
	void UpdateLighting();
};
