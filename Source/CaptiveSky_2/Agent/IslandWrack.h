#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandWrack.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/** What a storm left on the shore. */
enum class EIslandWrackKind : uint8
{
	Driftwood,
	Kelp,
	Shells,
	Float,
	Count
};

struct FIslandWrackItem
{
	int32 Id = 0;
	EIslandWrackKind Kind = EIslandWrackKind::Driftwood;
	FVector Position = FVector::ZeroVector;
	float Yaw = 0.f;
	int32 Seed = 0;
	/** Island day the storm left it. */
	int32 Day = 0;
	/** Someone has lifted it and looked underneath. */
	bool bTurned = false;
	FString Find;
	FString TurnedBy;
};

enum class EIslandWrackTurn : uint8
{
	Missing,
	Turned,
	AlreadyTurned
};

/** The shore's wrack: pure data, saved as JSON. The sea takes things back with time. */
struct CAPTIVESKY_2_API FIslandWrackLedger
{
	static constexpr int32 MaxItems = 24;

	TArray<FIslandWrackItem> Items;
	int32 NextId = 1;

	/** Island days an item lies on the shore before the tide takes it back. */
	static int32 LifespanDays(EIslandWrackKind Kind);
	/** Adds an item; a full shore loses its oldest first. Returns the new item. */
	const FIslandWrackItem& Add(EIslandWrackKind Kind, const FVector& Position, float Yaw, int32 Seed, int32 Day);
	/** Removes items the tide has reclaimed by Today. Returns true when any went. */
	bool Weather(int32 Today);
	const FIslandWrackItem* Find(int32 Id) const;
	/** Lift an item and look underneath. Reports whether this call turned it, it was already turned, or it is gone. */
	EIslandWrackTurn Turn(int32 Id, const FString& AgentId, FIslandWrackItem& OutItem);
	FString ToJson() const;
	bool FromJson(const FString& Json);
};

/** One loose piece of an item, in the item's local space. */
struct FIslandWrackPiece
{
	FTransform Transform;
};

/** One heap of wrack on the sand: logs, a tangle of kelp, a scatter of shells or a glass float. */
UCLASS()
class CAPTIVESKY_2_API AIslandWrack : public AActor
{
	GENERATED_BODY()
public:
	AIslandWrack();

	/** How far above the sand the actor origin floats, so inspection traces clear the ground. */
	static constexpr float OriginLift = 30.f;

	/** The loose pieces of a kind for a seed. Long things are cylinders laid flat; the rest are flattened spheres. Pure. */
	static TArray<FIslandWrackPiece> Layout(EIslandWrackKind Kind, int32 Seed);
	/** Colour of an item: fresh and dark with seawater, bleaching and drying with the days. */
	static FLinearColor ItemColor(EIslandWrackKind Kind, int32 AgeDays, bool bTurned);
	/** Whether this kind is made of long pieces (cylinders) rather than blobs (spheres). */
	static bool UsesLongPieces(EIslandWrackKind Kind);

	/** Build the pieces and tint them for the item's age. Safe to call again to retint. */
	void Show(int32 ItemId, EIslandWrackKind Kind, int32 Seed, int32 AgeDays, bool bTurned);
	int32 GetItemId() const { return ItemId; }
	int32 GetPieceCount() const;

private:
	UPROPERTY(VisibleAnywhere, Category="Island|Wrack")
	TObjectPtr<UInstancedStaticMeshComponent> Long;
	UPROPERTY(VisibleAnywhere, Category="Island|Wrack")
	TObjectPtr<UInstancedStaticMeshComponent> Blobs;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LongMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BlobMaterial;
	int32 ItemId = 0;
	int32 BuiltSeed = INDEX_NONE;
	EIslandWrackKind BuiltKind = EIslandWrackKind::Count;
};

/**
 * Storms leave things on the shore. After one passes, driftwood, kelp, shells and the odd glass float
 * lie along the tide line; residents can turn them over and find what was underneath, and the sea
 * gradually takes them back. The shore is saved beside the world state, so it persists across sessions.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandWrackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	/** Distance (cm) within which a resident notices wrack. */
	static constexpr float NoticeRadius = 2500.f;
	/** A flying Raven may learn about fresh shorefall from up to 600 m away. */
	static constexpr float RavenShoreAwarenessRadius = 60000.f;

	/** The kind a seed picks: mostly driftwood and kelp, fewer shells, rarely a float. Pure. */
	static EIslandWrackKind PickKind(int32 Seed);
	/** What turning an item over reveals. Deterministic in kind and seed. Pure. */
	static FString FindFor(EIslandWrackKind Kind, int32 Seed);
	/** How an item looks to a resident now. Pure. */
	static FString DescribeItem(EIslandWrackKind Kind, int32 AgeDays, bool bTurned);

	/** Leave fresh wrack along the shore after a storm. Returns how many items landed; zero where there is no shore. */
	int32 DepositAfterStorm(int32 Today, int32 Count = 0);
	/** Turn an item over. OutFact says what happened. Returns false when there is no such item. */
	bool Examine(int32 ItemId, int32 Today, const FString& AgentId, FString& OutFact);
	/** What a resident at Position notices of wrack near them, with move_to/interact targets. Empty when none. */
	FString DescribeNearby(const FVector& Position, int32 Today) const;
	/** Optional distant shore cue for the Raven only; grounded residents still receive strictly local observations. */
	static FString DescribeShoreForRaven(const TArray<FIslandWrackItem>& Items, const FVector& Position, int32 Today);
	/** The unique inspection tag an item's actor carries. */
	static FName TargetTagFor(int32 ItemId);
	/** The item id behind an inspection tag, or 0 when the tag is not a wrack tag. */
	static int32 ItemIdFromTag(const FName& Tag);

	const FIslandWrackLedger& GetLedger() const { return Ledger; }
	FIslandWrackLedger& GetLedgerMutable() { return Ledger; }
	/** Tests set these to redirect or suppress saving; empty means derive from the map. */
	FString StorageFileOverride;
	bool bAllowStorage = true;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TMap<int32, TWeakObjectPtr<AIslandWrack>> Actors;
	FIslandWrackLedger Ledger;
	bool bDirty = false;
	int32 SyncedDay = -1;
	float SinceCheck = 0.f;

	FString GetStorageFilePath() const;
	void Load();
	void Save();
	void SyncActors(int32 Today);
};
