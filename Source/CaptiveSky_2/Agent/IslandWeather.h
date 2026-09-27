#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "IslandWeather.generated.h"

class AIslandFirefly;
class AIslandPoolRippleEffect;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UVolumetricCloudComponent;

struct FIslandTransientGust
{
	FVector Center = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float PeakSpeed = 0.f;
	float Radius = 0.f;
	double StartedAt = 0.0;
	double ExpiresAt = 0.0;
};

/** Repeatable island-scale wind/cloud/rain signals with lightweight visual weather consumers. */
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Clouds")
	FName CloudCoverageParameter = TEXT("Cloud_GlobalCoverage");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Clouds")
	FName CloudDensityParameter = TEXT("Cloud_GlobalDensity");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Clouds")
	FName StormCloudsParameter = TEXT("StormClouds");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Rain", meta=(ClampMin="1", ClampMax="8"))
	float RainCycleMultiplier = 2.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Rain", meta=(ClampMin="16", ClampMax="192"))
	int32 RainStreakCount = 96;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Rain", meta=(ClampMin="1000", ClampMax="12000"))
	float RainVisualizationRadius = 2400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Island|Weather|Rain", meta=(ClampMin="1000", ClampMax="5000"))
	float RainVisualizationHeight = 2600.f;

	// Pure and repeatable for a given position/time; speeds are Unreal cm/s.
	FVector SampleWind(const FVector& Position, double Seconds) const;
	float SampleCloudCover(double Seconds) const;
	float SampleRainIntensity(double Seconds) const;
	FVector GetLocalWind(const FVector& Position, const AActor* Observer = nullptr) const;
	FString DescribeAt(const FVector& Position, const AActor* Observer = nullptr) const;
	/** Describe only current upwind line-of-sight shelter; this does not claim roof cover or perch support. */
	FString DescribeWindShelterAt(const FVector& Position, const AActor* Observer = nullptr) const;
	/** Add a bounded, temporary local wind response; it naturally fades in space and time. */
	void AddTransientGust(const FVector& Center, const FVector& Direction, float PeakSpeed, float Radius, float DurationSeconds);
	static FVector EvaluateTransientGust(const FIslandTransientGust& Gust, const FVector& Position, double CurrentTime);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class FIslandNightEcologyTest;
	TArray<FIslandTransientGust> TransientGusts;
	TArray<TWeakObjectPtr<AIslandFirefly>> NightFireflies;
	TWeakObjectPtr<AIslandPoolRippleEffect> RainPoolRipple;
	FTimerHandle EcologyTimerHandle;
	TWeakObjectPtr<UVolumetricCloudComponent> CloudComponent;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalCloudMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WeatherCloudMaterial;
	UPROPERTY(VisibleAnywhere, Transient, Category="Island|Weather|Rain")
	TObjectPtr<UInstancedStaticMeshComponent> RainStreaks;
	UPROPERTY(VisibleAnywhere, Transient, Category="Island|Weather|Rain")
	TObjectPtr<UInstancedStaticMeshComponent> RainGroundImpactStreaks;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RainStreakMaterial;
	float OriginalCloudCoverage = 0.f;
	float OriginalCloudDensity = 0.f;
	float OriginalStormClouds = 0.f;
	bool bHasCloudCoverageParameter = false;
	bool bHasCloudDensityParameter = false;
	bool bHasStormCloudsParameter = false;
	bool bCloudParameterWarningLogged = false;
	bool bRainPoolInitialized = false;
	bool bRainGroundImpactPoolInitialized = false;
	int32 ActiveRainStreakCount = 0;
	int32 ActiveRainGroundImpactCount = 0;
	float CurrentRainIntensity = 0.f;
	double NextCloudDiscoveryTime = 0.0;
	double NextRainPoolRippleTime = 0.0;
	double NextRainGroundImpactTime = 0.0;
	double RainGroundImpactStartedAt = 0.0;
	FVector LastRainGroundImpactLocation = FVector::ZeroVector;
	uint32 RainGroundImpactSequence = 0;
	void RefreshNightEcology();
	bool InitializeCloudRendering();
	void UpdateCloudRendering();
	bool InitializeRainRendering();
	void UpdateRainRendering();
	void UpdateRainPoolResponse();
	void UpdateRainGroundResponse(const FVector& Center, const AActor* Observer, double Now);
	void ClearRainGroundResponse();
	bool HasUpwindObstruction(const FVector& Position, const FVector& Wind, const AActor* Observer) const;
};
