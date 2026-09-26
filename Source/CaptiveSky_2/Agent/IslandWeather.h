#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "IslandWeather.generated.h"

class AIslandFirefly;

struct FIslandTransientGust
{
	FVector Center = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float PeakSpeed = 0.f;
	float Radius = 0.f;
	double StartedAt = 0.0;
	double ExpiresAt = 0.0;
};

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
	/** Add a bounded, temporary local wind response; it naturally fades in space and time. */
	void AddTransientGust(const FVector& Center, const FVector& Direction, float PeakSpeed, float Radius, float DurationSeconds);
	static FVector EvaluateTransientGust(const FIslandTransientGust& Gust, const FVector& Position, double CurrentTime);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TArray<FIslandTransientGust> TransientGusts;
	TArray<TWeakObjectPtr<AIslandFirefly>> NightFireflies;
	FTimerHandle EcologyTimerHandle;
	void RefreshNightEcology();
};
