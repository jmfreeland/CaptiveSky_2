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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Time")
	float CurrentHour = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting")
	TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting")
	TObjectPtr<ASkyLight> Sky;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting", meta=(ClampMin="0"))
	float DaySunIntensity = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Lighting", meta=(ClampMin="0"))
	float MoonIntensity = 0.08f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Island|Lighting")
	TObjectPtr<UDirectionalLightComponent> Moon;

	static float WrapHour(double Hour);
	static float SunHeight(float Hour);
	FString DescribeTime() const;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
protected:
	virtual void BeginPlay() override;
private:
	void UpdateLighting();
};
