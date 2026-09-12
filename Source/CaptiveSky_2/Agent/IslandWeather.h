#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandWeather.generated.h"

/** First-pass physical weather signal. Rendering/audio may consume the same sample later. */
UCLASS()
class CAPTIVESKY_2_API AIslandWeather : public AActor
{
	GENERATED_BODY()
public:
	AIslandWeather();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather", meta=(ClampMin="30"))
	float CycleSeconds = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather", meta=(ClampMin="0", ClampMax="300"))
	float MaximumWindSpeed = 180.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather")
	int32 WeatherSeed = 71;

	// Pure and repeatable for a given position/time; speeds are Unreal cm/s.
	FVector SampleWind(const FVector& Position, double Seconds) const;
	float SampleCloudCover(double Seconds) const;
	FVector GetLocalWind(const FVector& Position, const AActor* Observer = nullptr) const;
	FString DescribeAt(const FVector& Position, const AActor* Observer = nullptr) const;
};
