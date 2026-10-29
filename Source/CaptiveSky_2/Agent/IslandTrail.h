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
	/** Island day this ground was last crossed, or -1 when loaded from a legacy/unknown record. */
	int32 LastUseDay = -1;
	FVector Position = FVector::ZeroVector;
	float NormalX = 0.f;
	float NormalY = 0.f;
};

enum class EIslandTrailRegionState : uint8
{
	Unknown,
	Active,
	Recovering
};

/** A bounded, derived summary of one stable 12 m trail region. Never persisted separately from its cells. */
struct FIslandTrailRegionSummary
{
	FIntPoint RegionKey = FIntPoint::ZeroValue;
	FString StableId;
	int32 WornCellCount = 0;
	/** Sum of current per-cell step weights, which weathering can reduce over time. */
	int64 CurrentWearSteps = 0;
	int32 LastUseDay = -1;
	EIslandTrailRegionState State = EIslandTrailRegionState::Unknown;
	/** Undirected principal axis of worn cells, in world XY; zero when the cluster is too sparse or isotropic. */
	FVector2D WornAxis = FVector2D::ZeroVector;
	/** Weighted covariance anisotropy in [0,1]; higher values indicate a clearer line. */
	float WornAxisLinearity = 0.f;
	/** Step-weighted centre of the worn cells, in world XY; derived from cell coordinates. */
	FVector2D WornCenter = FVector2D::ZeroVector;
	/** Deterministic, bounded examples of the cell keys that contribute to the summary. */
	TArray<FIntPoint> SampleCells;
};

/** The island's memory of where people walk. Pure data, saved as JSON. */
struct CAPTIVESKY_2_API FIslandTrailLedger
{
	static constexpr int32 MaxCells = 6000;
	static constexpr float CellSize = 150.f;
	static constexpr int32 RegionCellSpan = 8;
	static constexpr int32 MaxRegionSamples = 8;
	static constexpr int32 MinRegionCellSteps = 2;
	static constexpr int32 RegionRecoveryDays = 3;

	TMap<FIntPoint, FIslandTrailCell> Cells;
	/** Island day the ledger was last weathered, or -1 before the first day is known. */
	int32 LastDecayDay = -1;

	/** Grass creeps back over unused ground: each cell loses a few percent (at least one step) per day. */
	void Decay(int32 Days);
	/** Weather the ledger up to Today. Returns true when anything changed. */
	bool WeatherTo(int32 Today);

	static FIntPoint CellFor(const FVector& Position);
	/** Stable tile identity; negative cell coordinates use mathematical floor division. */
	static FIntPoint RegionForCell(const FIntPoint& CellKey);
	static FString RegionId(const FIntPoint& RegionKey);
	/** Derive a bounded summary by inspecting only the cells inside this 12 m region. */
	FIslandTrailRegionSummary SummarizeRegionForCell(const FIntPoint& CellKey, int32 Today) const;
	/** Derive every worn 12 m region in stable coordinate order. At most MaxCells records can be returned. */
	TArray<FIslandTrailRegionSummary> SummarizeWornRegions(int32 Today) const;
	/** Count one footfall on the ground at Position. Returns false when the ledger is full and the cell is new. */
	bool AddStep(const FVector& Position, const FVector& GroundNormal, int32 IslandDay = -1);
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
	static constexpr int32 RecoveryNoticeAfterDays = FIslandTrailLedger::RegionRecoveryDays;
	static constexpr float WearDrawRadius = 5500.f;
	static constexpr int32 TrailChronicleCellThreshold = 3;
	static constexpr int64 TrailChronicleUseThreshold = 12;
	static constexpr int32 MaxTrailChronicleEntriesPerDay = 4;
	static constexpr float NearbyWornRegionRadius = 4500.f;
	static constexpr int32 MaxNearbyWornRegions = 2;

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
	/** A bounded chronicle note is due only when a region newly becomes an active, repeatedly worn path. */
	static bool ShouldChronicleTrailEmergence(const FIslandTrailRegionSummary& Before,
		const FIslandTrailRegionSummary& After, int32 EntriesToday, int32 IslandDay);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

	const FIslandTrailLedger& GetLedger() const { return Ledger; }
	FIslandTrailLedger& GetLedgerMutable() { return Ledger; }
	int32 GetActivePrintCount() const;
	/** What a resident notices of the ground itself: worn tracks and wet prints. Pure; counts come from the ledger. */
	static FString DescribeTrail(int32 Steps, int32 NearbyPrints, float Wetness, int32 LastUseDay = -1, int32 Today = -1);
	/** Describe only high-confidence, undirected alignment; never claim a continuous route from sparse cells. */
	static FString DescribeWornAlignment(const FIslandTrailRegionSummary& Region);
	/** Read nearby worn regions as bounded local clues, never as routes or destinations. */
	static FString DescribeNearbyWornRegions(const FIslandTrailLedger& TrailLedger, const FVector& Position, int32 Today);
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
	int32 TrailChronicleDay = -1;
	int32 TrailChronicleEntriesToday = 0;
	double NextRefresh = 0.0;
	double NextWearRefresh = 0.0;
	double NextSave = 0.0;
	double NextWeathering = 0.0;
	bool bLedgerDirty = false;

	FString GetStorageFilePath() const;
	void Load();
	void Save();
	void WeatherLedger();
	void Footfall(const APawn& Pawn, FWalker& Walker, double Now, float Wetness, int32 IslandDay);
	void RefreshPrints(double Now, float Wetness);
	void RefreshWear(const FVector& Viewer, float Wetness);
	/** Align adjacent wear patches to repeated travel while keeping junctions nearly round. */
	static FQuat CalculateWearRotation(const FIntPoint& Key, const TMap<FIntPoint, FIslandTrailCell>& Cells,
		const FVector& Normal, float& OutAspectRatio);
};
