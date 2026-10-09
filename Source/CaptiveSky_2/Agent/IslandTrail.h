#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandTrail.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class APawn;

/** One 1.5 m patch of ground, how often feet have crossed it, and where the last foot fell. */
struct FIslandTrailCell
{
	int32 Steps = 0;
	FVector Position = FVector::ZeroVector;
	float NormalX = 0.f;
	float NormalY = 0.f;
};

/** The island's memory of where people walk. Pure data, saved as JSON. */
struct CAPTIVESKY_2_API FIslandTrailLedger
{
	static constexpr int32 MaxCells = 6000;
	static constexpr float CellSize = 150.f;

	TMap<FIntPoint, FIslandTrailCell> Cells;
	/** Island day the ledger was last weathered, or -1 before the first day is known. */
	int32 LastDecayDay = -1;

	/** Grass creeps back over unused ground: each cell loses a few percent (at least one step) per day. */
	void Decay(int32 Days);
	/** Weather the ledger up to Today. Returns true when anything changed. */
	bool WeatherTo(int32 Today);

	static FIntPoint CellFor(const FVector& Position);
	/** Count one footfall on the ground at Position. Returns false when the ledger is full and the cell is new. */
	bool AddStep(const FVector& Position, const FVector& GroundNormal);
	int32 StepsAt(const FVector& Position) const;
	FString ToJson() const;
	bool FromJson(const FString& Json);
};

/** The marks themselves: wet footprints that dry away, and bare worn earth along much-used routes. */
UCLASS()
class CAPTIVESKY_2_API AIslandTrailMarks : public AActor
{
	GENERATED_BODY()
public:
	AIslandTrailMarks();

	UPROPERTY(VisibleAnywhere, Category="Island|Trail")
	TObjectPtr<UInstancedStaticMeshComponent> Prints;
	UPROPERTY(VisibleAnywhere, Category="Island|Trail")
	TObjectPtr<UInstancedStaticMeshComponent> Wear;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PrintMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WearMaterial;
	void EnsureMaterials();
};

/**
 * Watches everyone who walks on the island's ground. After rain their feet leave dark prints that
 * shrink away as the ground dries; wherever many feet pass, the ground wears bare and the wear is
 * saved beside the world state, so paths outlast play sessions.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandTrailSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	static constexpr int32 MaxPrints = 220;
	static constexpr int32 MaxWearDiscs = 360;
	static constexpr float StrideLength = 75.f;
	static constexpr float MinWalkSpeed = 40.f;
	static constexpr float PrintWetness = 0.15f;
	static constexpr int32 WearStartSteps = 4;
	static constexpr int32 WearFullSteps = 40;
	static constexpr float WearDrawRadius = 5500.f;

	/** Seconds a print lasts on ground of this wetness; zero when too dry to leave one. */
	static float PrintLifetime(float Wetness);
	/** 0..1 size of a print: full until it begins to dry out, then shrinking away. Wetness is the ground now. */
	static float PrintScale(float Age, float Life, float Wetness);
	/** True when a print needs a transform write: visible prints animate, hidden slots stay untouched. */
	static bool ShouldRefreshPrintInstance(bool bActive, bool bRendered, float Scale);
	/** 0..1 how worn a cell is after this many footfalls. */
	static float WearAmount(int32 Steps);
	/** Dusty when dry, dark mud when the ground is wet. */
	static FLinearColor WearColor(float Wetness);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

	const FIslandTrailLedger& GetLedger() const { return Ledger; }
	FIslandTrailLedger& GetLedgerMutable() { return Ledger; }
	int32 GetActivePrintCount() const;
	/** What a resident notices of the ground itself: worn tracks and wet prints. Pure; counts come from the ledger. */
	static FString DescribeTrail(int32 Steps, int32 NearbyPrints, float Wetness);
	/** The same, for a point on the island. */
	FString DescribeUnderfoot(const FVector& Position, float Wetness) const;
	/** Tests set this to redirect or suppress saving; empty means derive from the map. */
	FString StorageFileOverride;
	bool bAllowStorage = true;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	friend class FIslandTrailTest;
	struct FWalker
	{
		FVector LastPrint = FVector::ZeroVector;
		bool bHasLast = false;
		bool bRight = false;
	};
	struct FPrint
	{
		FVector Position = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		double Born = 0.0;
		float Life = 0.f;
		bool bActive = false;
		bool bRendered = false;
	};

	TWeakObjectPtr<AIslandTrailMarks> Marks;
	TMap<TWeakObjectPtr<const APawn>, FWalker> Walkers;
	TArray<FPrint> PrintRing;
	int32 NextPrint = 0;
	FIslandTrailLedger Ledger;
	double NextRefresh = 0.0;
	double NextWearRefresh = 0.0;
	double NextSave = 0.0;
	double NextWeathering = 0.0;
	bool bLedgerDirty = false;

	FString GetStorageFilePath() const;
	void Load();
	void Save();
	void WeatherLedger();
	void Footfall(const APawn& Pawn, FWalker& Walker, double Now, float Wetness);
	void RefreshPrints(double Now, float Wetness);
	void RefreshWear(const FVector& Viewer, float Wetness);
};
