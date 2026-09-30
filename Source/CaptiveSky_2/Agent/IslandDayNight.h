#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandDayNight.generated.h"

class ADirectionalLight;
class ASkyLight;
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
	/** Approximate synodic month used for the repeating Island moon-light cycle. */
	static constexpr double LunarCycleDays = 29.53059;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Lighting")
	TObjectPtr<UDirectionalLightComponent> Moon;

	static float WrapHour(double Hour);
	static float SunHeight(float Hour);
	static float LunarPhaseProgress(int32 IslandDay, float IslandHour);
	static float LunarIllumination(int32 IslandDay, float IslandHour);
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
	void PersistHour();
	friend class FIslandClockTest;
	friend class FIslandClockPersistenceTest;
	void UpdateLighting();
};
