#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "IslandWeather.generated.h"

class AIslandFirefly;
class AIslandLightning;
class AIslandTidepoolCrab;
class AIslandTidepoolMinnows;
class AIslandPoolRippleEffect;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UAudioComponent;
class UVolumetricCloudComponent;
class USoundWaveProcedural;

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

	/**
	 * Weather already lived through in earlier sessions, in seconds. Every sample adds it to the session
	 * time it is given, so the weather carries on where it left off (saved in the world state).
	 */
	double WeatherTimeOffset = 0.0;
	/** Storms add up to this fraction on top of the usual wind. */
	static constexpr float StormWindBoost = 0.9f;

	// Pure and repeatable for a given position/time; speeds are Unreal cm/s.
	FVector SampleWind(const FVector& Position, double Seconds) const;
	/** Slow multi-day tendency: 0 is a settled dry spell, 1 an unsettled wet spell (a few Island days each). */
	float SampleSpell(double Seconds) const;
	/** 0 calm, up to 1 a full storm: only when a wet spell peaks and a heavy front arrives. */
	float SampleStormIntensity(double Seconds) const;
	/** Brightness of the current lightning flash, 0..1 (for materials and presentation). */
	float GetLightningFlash() const;
	/** Developer override (Island.Storm): a full storm until this session time. Not saved. */
	double ForcedStormUntil = -1.0;
	float SampleCloudCover(double Seconds) const;
	float SampleRainIntensity(double Seconds) const;
	FVector GetLocalWind(const FVector& Position, const AActor* Observer = nullptr) const;
	/** Sample local wind at a world-session time, including geometry shelter and active transient gusts. */
	FVector SampleLocalWind(const FVector& Position, double SessionSeconds, const AActor* Observer = nullptr) const;
	/** Bounded ambient wind/rain bed gains; an enclosed inn listener hears a softened version. */
	static FVector2D CalculateAmbienceGains(float HorizontalWindSpeed, float RainIntensity, bool bIndoors = false);
	FString DescribeAt(const FVector& Position, const AActor* Observer = nullptr) const;
	/** Describe only current upwind line-of-sight shelter; this does not claim roof cover or perch support. */
	FString DescribeWindShelterAt(const FVector& Position, const AActor* Observer = nullptr) const;
	/** Add a bounded, temporary local wind response; it naturally fades in space and time. */
	void AddTransientGust(const FVector& Center, const FVector& Direction, float PeakSpeed, float Radius, float DurationSeconds);
	/** Clear editor-only ground-cover instances after an offscreen visual preview. */
	void ClearGroundCoverPreview();
	static FVector EvaluateTransientGust(const FIslandTransientGust& Gust, const FVector& Position, double CurrentTime);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class FIslandWeatherTest;
	friend class FIslandNightEcologyTest;
	friend class FIslandGroundCoverTest;
	friend class FIslandMinnowTest;
	friend class FIslandViewpointCaptureTest;
	TArray<FIslandTransientGust> TransientGusts;
	TArray<TWeakObjectPtr<AIslandFirefly>> NightFireflies;
	TArray<TWeakObjectPtr<AIslandTidepoolCrab>> DayCrabs;
	TWeakObjectPtr<AIslandTidepoolMinnows> DayMinnowSchool;
	TArray<FTransform> ShoreGrassABaseTransforms;
	TArray<FTransform> ShoreGrassBBaseTransforms;
	TArray<FTransform> ShoreGroundPlantBaseTransforms;
	TWeakObjectPtr<AIslandPoolRippleEffect> RainPoolRipple;
	TWeakObjectPtr<AIslandPoolRippleEffect> WindPoolRipple;
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
	UPROPERTY(VisibleAnywhere, Transient, Category="Island|Ecology")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShoreGrassA;
	UPROPERTY(VisibleAnywhere, Transient, Category="Island|Ecology")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShoreGrassB;
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShoreGroundPlants;
	UPROPERTY(VisibleAnywhere, Transient, Category="Island|Weather|Audio")
	TObjectPtr<UAudioComponent> WindAmbienceAudio;
	UPROPERTY(VisibleAnywhere, Transient, Category="Island|Weather|Audio")
	TObjectPtr<UAudioComponent> RainAmbienceAudio;
	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> WindAmbienceWave;
	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> RainAmbienceWave;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RainStreakMaterial;
	FRandomStream WindNoiseStream;
	FRandomStream RainNoiseStream;
	float WindNoiseFilterLeft = 0.f;
	float WindNoiseFilterRight = 0.f;
	float RainNoiseFilterLeft = 0.f;
	float RainNoiseFilterRight = 0.f;
	float AmbienceUpdateAccumulator = 0.f;
	float OriginalCloudCoverage = 0.f;
	float OriginalCloudDensity = 0.f;
	float OriginalStormClouds = 0.f;
	bool bHasCloudCoverageParameter = false;
	bool bHasCloudDensityParameter = false;
	bool bHasStormCloudsParameter = false;
	bool bCloudParameterWarningLogged = false;
	bool bRainPoolInitialized = false;
	bool bRainGroundImpactPoolInitialized = false;
	bool bGroundCoverInitialized = false;
	int32 ActiveRainStreakCount = 0;
	int32 ActiveRainGroundImpactCount = 0;
	int32 GroundCoverInstanceCount = 0;
	float CurrentRainIntensity = 0.f;
	double NextCloudDiscoveryTime = 0.0;
	double NextRainPoolRippleTime = 0.0;
	double NextWindPoolRippleTime = 0.0;
	double NextRainGroundImpactTime = 0.0;
	double RainGroundImpactStartedAt = 0.0;
	FVector LastRainGroundImpactLocation = FVector::ZeroVector;
	uint32 RainGroundImpactSequence = 0;
	float SecondsSinceWeatherSave = 0.f;
	FRandomStream StrikeStream;
	double NextStrikeAt = 0.0;
	double LastStrikeTime = -1.0e9;
	FVector LastStrikeGround = FVector::ZeroVector;
	float LastThunderDelay = 0.f;
	int32 StrikeCount = 0;
	TWeakObjectPtr<AIslandLightning> LastStrike;
	/** Schedules and triggers lightning while a storm is overhead; Now is session time. */
	void UpdateStorm(double Now);
	void StrikeNear(const FVector& Listener, float Storm);
	void PersistWeatherTime();
	/** Deterministic local offsets, used by the runtime scatter and its editor fixture. */
	static void BuildGroundCoverOffsets(int32 Seed, TArray<FTransform>& OutTransforms);
	static void BuildGroundCoverOffsets(int32 Seed, int32 ClumpCount, float InnerRadius, float OuterRadius,
		TArray<FTransform>& OutTransforms);
	/** Calculate one bounded wind lean from an immutable base transform (never accumulates drift). */
	static FTransform CalculateGroundCoverSway(const FTransform& BaseTransform, const FVector& LocalWind,
		double TimeSeconds, int32 InstanceIndex, int32 Seed, float ReferenceWindSpeed);
	void InitializeGroundCover();
	void UpdateGroundCoverSway();
	void ClearGroundCover();
	/** 0..1 strength of the passing rain front, before clouds gate it. */
	float SampleFrontStrength(double Seconds) const;
	void RefreshNightEcology();
	bool InitializeCloudRendering();
	void UpdateCloudRendering();
	bool InitializeRainRendering();
	void UpdateRainRendering();
	void UpdateRainPoolResponse();
	void UpdateWindPoolResponse();
	void UpdateRainGroundResponse(const FVector& Center, const AActor* Observer, double Now);
	void ClearRainGroundResponse();
	void InitializeWeatherAmbience();
	void UpdateWeatherAmbience(float DeltaSeconds);
	void QueueAmbienceSamples(USoundWaveProcedural* Wave, FRandomStream& Random, float& FilterLeft, float& FilterRight, bool bHighPass);
	bool HasUpwindObstruction(const FVector& Position, const FVector& Wind, const AActor* Observer) const;
};
