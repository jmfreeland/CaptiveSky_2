#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandRainBasin.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/** A leaf a resident set afloat. */
struct FIslandBasinLeaf
{
	FString AgentId;
	/** Island day it was set on the water. */
	int32 Day = 0;
	int32 Seed = 0;
};

enum class EIslandBasinFloat : uint8
{
	TooDry,
	AlreadyToday,
	Floated,
	FloatedReplacingOldest
};

/** The basin's lasting state: pure data, saved as JSON. */
struct CAPTIVESKY_2_API FIslandBasinState
{
	static constexpr int32 MaxLeaves = 8;
	/** Below this much water a leaf only lies on the stone. */
	static constexpr float FloatLevel = 0.2f;

	bool bPlaced = false;
	FVector Location = FVector::ZeroVector;
	/** 0 dry .. 1 full to the rim. */
	float Water = 0.f;
	TArray<FIslandBasinLeaf> Leaves;

	/** Water level after Seconds of Rain (0..1), sun (sine of elevation, -1..1), and wind speed (cm/s). Rain fills it; sun and wind dry it. Pure. */
	static float Advance(float Water, float Rain, float SunHeight, float Seconds, float WindSpeed = 0.f);
	/** Set a leaf afloat for an agent on Today. Reports what happened. */
	EIslandBasinFloat FloatLeaf(const FString& AgentId, int32 Today, int32 Seed);
	int32 LeavesFrom(const FString& AgentId) const;
	FString ToJson() const;
	bool FromJson(const FString& Json);
};

/** A shallow stone basin that holds rainwater, with the leaves residents have set afloat on it. */
UCLASS()
class CAPTIVESKY_2_API AIslandRainBasin : public AActor
{
	GENERATED_BODY()
public:
	AIslandRainBasin();

	/** The actor origin floats this far above the ground so inspection traces clear the terrain. */
	static constexpr float OriginLift = 30.f;
	static constexpr float FloorThickness = 6.f;
	static constexpr float RimRadius = 46.f;
	static constexpr float RimHeight = 16.f;

	/** Height (cm) of the water surface above the basin floor for a level. Pure. */
	static float WaterDepth(float Water);
	/** Colour of a leaf of this age: green when fresh, browning over a few days. Pure. */
	static FLinearColor LeafColor(int32 AgeDays);

	void Show(const FIslandBasinState& State, int32 Today);

private:
	UPROPERTY(VisibleAnywhere, Category="Island|RainBasin")
	TObjectPtr<UInstancedStaticMeshComponent> Rim;
	UPROPERTY(VisibleAnywhere, Category="Island|RainBasin")
	TObjectPtr<UStaticMeshComponent> Floor;
	UPROPERTY(VisibleAnywhere, Category="Island|RainBasin")
	TObjectPtr<UStaticMeshComponent> Surface;
	/** Fresh, aging and old leaves, each tinted for its age bucket. */
	UPROPERTY(VisibleAnywhere, Category="Island|RainBasin")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> LeafLayers;
	bool bBuilt = false;
};

/**
 * A stone basin near the ListeningStones that fills in rain and dries in sun. Residents may set a leaf
 * afloat on it once per Island day (it needs enough water); the leaves stay, browning, until newer ones
 * crowd them out. Saved beside the world state. Console: Island.BasinFill <0..1>, Island.BasinLeaf.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandRainBasinSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	/** How the basin looks to a resident. Pure. */
	static FString DescribeWater(float Water, int32 LeafCount);
	/** What a resident at Position notices, with the move_to/interact target; empty when out of range. */
	FString DescribeNearby(const FVector& Position) const;
	/** Try to set a leaf afloat; OutFact says what happened. Returns true when something lasting changed. */
	bool Examine(const FString& AgentId, int32 Today, FString& OutFact);

	static constexpr float NoticeRadius = 2500.f;

	const FIslandBasinState& GetState() const { return State; }
	FIslandBasinState& GetStateMutable() { return State; }
	void ForceWater(float Water);
	/** Tests set these to redirect or suppress saving. */
	FString StorageFileOverride;
	bool bAllowStorage = true;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	FIslandBasinState State;
	TWeakObjectPtr<AIslandRainBasin> Actor;
	bool bDirty = false;
	float SinceSave = 0.f;
	float SinceShow = 0.f;
	float ShownWater = -1.f;

	FString GetStorageFilePath() const;
	void Load();
	void Save();
	bool PlaceBasin();
	void Refresh();
};
