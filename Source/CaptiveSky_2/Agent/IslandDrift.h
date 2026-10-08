#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandDrift.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class AIslandWeather;

/** What a drifting mote is. Heavier things need more wind to lift. */
enum class EIslandDriftKind : uint8
{
	Seed,
	Petal,
	DryLeaf,
	GreenLeaf,
	Count
};

struct FIslandDriftMote
{
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector SpinAxis = FVector::UpVector;
	float GroundZ = 0.f;
	float Age = 0.f;
	float Life = 0.f;
	float Spin = 0.f;
	float SpinRate = 0.f;
	float Phase = 0.f;
	float Size = 1.f;
	bool bActive = false;
};

/**
 * Leaves, petals and seeds carried on the island wind around whoever is looking, so wind that was
 * only a number becomes something you can see: still air leaves a few seeds hanging, a breeze lifts
 * petals, a gale tears leaves loose. Purely presentational (no collision, no saved state) and capped.
 */
UCLASS()
class CAPTIVESKY_2_API AIslandDrift : public AActor
{
	GENERATED_BODY()
public:
	AIslandDrift();

	static constexpr int32 MaxMotes = 96;
	/** Motes live in a disc this wide (cm) around the viewer. */
	static constexpr float Radius = 1800.f;
	static constexpr float MinHeightAboveGround = 40.f;
	static constexpr float MaxHeightAboveGround = 420.f;

	/** How many motes the conditions call for. Wind in cm/s; rain and daylight 0..1. */
	static int32 DesiredCount(float WindSpeed, float Rain, float Daylight);
	/** The wind speed (cm/s) at which this kind starts to lift. */
	static float LiftWindSpeed(EIslandDriftKind Kind);
	/** Steady fall speed (cm/s) of this kind in still air. */
	static float SinkSpeed(EIslandDriftKind Kind);
	/** One step of a mote's velocity: it relaxes toward the wind while sinking. Pure. */
	static FVector StepVelocity(const FVector& Velocity, const FVector& Wind, EIslandDriftKind Kind, float Seconds);
	/** The kind that mote slot Index belongs to (slots are partitioned by kind). */
	static EIslandDriftKind KindForSlot(int32 Index);
	/** What a resident notices drifting past in this wind (cm/s); empty when nothing is lifting. */
	static FString DescribeDrift(float WindSpeed, float Rain, float Daylight);

	/**
	 * Advance every mote by Dt seconds around Viewer. WindAt gives the wind (cm/s) at a point; Rain and
	 * Daylight are 0..1. Fresh motes enter on the upwind edge once the first population is placed.
	 */
	void Advance(float Dt, const FVector& Viewer, const TFunctionRef<FVector(const FVector&)>& WindAt, float Rain, float Daylight);

	int32 GetActiveCount() const;
	const TArray<FIslandDriftMote>& GetMotes() const { return Motes; }

private:
	friend class FIslandDriftTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Weather")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> MoteLayers;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LayerMaterials;
	TArray<FIslandDriftMote> Motes;
	TArray<int32> FirstSlotOfKind;
	FRandomStream Random;
	float SpawnBudget = 0.f;
	bool bPopulated = false;

	float GroundHeightAt(const FVector& XY, float ReferenceZ) const;
	void Place(FIslandDriftMote& Mote, int32 Slot, const FVector& Viewer, const FVector& Wind, bool bAnywhere);
	void EnsureMaterials();
};

/** Spawns the drift actor around the local viewer and feeds it the island weather. */
UCLASS()
class CAPTIVESKY_2_API UIslandDriftSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TWeakObjectPtr<AIslandDrift> Drift;
	TWeakObjectPtr<AIslandWeather> Weather;
	double NextWeatherSearch = 0.0;
};
