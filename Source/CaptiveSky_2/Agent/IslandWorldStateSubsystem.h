#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandWorldStateSubsystem.generated.h"

class AIslandNest;

/** One nest woven by residents at a tagged roost site. Stored outside Saved/ so it outlives play sessions. */
USTRUCT(BlueprintType)
struct FIslandNestRecord
{
	GENERATED_BODY()

	/** Unique movement tag of the roost marker (its first tag), e.g. Roost_West. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	FName SiteTag;

	/** Where the woven material rests: the support surface beneath the marker. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	FVector Location = FVector::ZeroVector;

	/** Number of woven layers, 1..MaxNestLayers. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	int32 Layers = 0;

	/** Stable AgentIds of every resident who has added a layer, in order of first contribution. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	TArray<FString> Builders;

	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	FDateTime CreatedUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	FDateTime UpdatedUtc;
};

/**
 * Persistent, per-level record of lasting changes residents have made to the world.
 *
 * Storage: <ProjectDir>/WorldState/<MapName>.json, rewritten atomically after each change.
 * Like agent memories, this lives outside Saved/ and is backed up externally rather than
 * tracked in git. Every change is bounded (fixed layer cap per site) and reversible by a
 * developer through Island.RemoveNest or by editing/deleting the file with play stopped.
 *
 * Nests and the Island clock are stored today; the file keeps a top-level version so other
 * kinds of lasting change can be added beside them later.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandWorldStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxNestLayers = 5;

	/** Tests point this at a scratch file before the world begins play. Empty = the per-map project file. */
	FString StorageFileOverride;

	/** Empty for worlds without a saved map (code-created fixtures), which never persist anything. */
	FString GetStorageFilePath() const;

	const FIslandNestRecord* FindNest(FName SiteTag) const;
	const TArray<FIslandNestRecord>& GetNests() const { return Nests; }

	/**
	 * Weaves one more layer into the nest at SiteTag (creating it at SupportLocation if absent),
	 * persists the change, and refreshes the visible nest. Returns the new layer count, or 0 if
	 * the nest is already complete or the change could not be saved.
	 */
	int32 AddNestLayer(FName SiteTag, const FVector& SupportLocation, const FString& BuilderAgentId);

	/** Developer reversal: removes the record and its visible nest. Returns false if none existed. */
	bool RemoveNest(FName SiteTag);

	/** Island hour at the end of the previous session, if one was recorded. */
	TOptional<float> GetSavedHour() const { return SavedHour; }

	/** Records the current Island hour so the next session can resume from it. */
	bool SaveHour(float Hour);

	/** Re-reads the storage file and respawns visible nests. Called automatically when play begins. */
	void LoadAndSpawn();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	TArray<FIslandNestRecord> Nests;
	TOptional<float> SavedHour;
	TMap<FName, TWeakObjectPtr<AIslandNest>> NestActors;
	// Set when an existing file cannot be parsed, so a save never overwrites what it may still hold.
	bool bStorageUnreadable = false;

	bool Save() const;
	void RefreshNestActor(const FIslandNestRecord& Record);
	void DestroyNestActors();
};
