#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandCurio.h"
#include "IslandArrangement.h"
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
 * Nests, curios (hidden stones, the seed pod, the cairn), resident stone arrangements and the Island clock are stored today; the file keeps a top-level version so other
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
	/** Island day number (starting at 1) saved alongside the hour. */
	TOptional<int32> GetSavedDay() const { return SavedDay; }

	/** Records the current Island hour and day so the next session can resume from them. */
	bool SaveClock(float Hour, int32 Day);

	/** The running clock's day number, or 1 when the level has no clock. */
	static int32 CurrentIslandDay(const UWorld* World);

	const TArray<FIslandCurioRecord>& GetCurios() const { return Curios; }
	const FIslandCurioRecord* FindCurio(FName Id) const;

	/**
	 * A resident examines a curio on Island day Today. Applies any lasting change (pod opening,
	 * cairn stone) and returns a factual description of what happened.
	 */
	FString ExamineCurio(FName Id, int32 Today, const FString& ContributorAgentId = FString());

	/**
	 * Computes where curios would go in World (near its ListeningStones landmark) without saving or
	 * spawning anything. All-or-nothing: false leaves OutLayout empty.
	 */
	static bool BuildCurioLayout(UWorld* World, TArray<FIslandCurioRecord>& OutLayout);

	/** Developer reset: forgets every curio so fresh ones are placed next time play begins. */
	bool ForgetCurios();

	const TArray<FIslandArrangementSite>& GetArrangementSites() const { return ArrangementSites; }
	const FIslandArrangementSite* FindArrangementSite(FName Id) const;
	static bool ParseArrangementForm(const FString& Text, EIslandArrangementForm& OutForm);
	static FString FormName(EIslandArrangementForm Form);

	/**
	 * A resident arranges stones at an arranging site on Island day Today: a new work on empty
	 * ground, or a small response beside someone else's work. Bounded to one arrangement per
	 * resident per Island day. Returns a factual outcome; bOutChanged is true only when the
	 * lasting change was saved.
	 */
	FString ArrangeStones(FName SiteId, const FString& Form, const FString& Title, const FString& Intent,
		const FString& AgentId, int32 Today, bool& bOutChanged);

	/** Where arranging sites would go around the ListeningStones, clear of Avoid points. No saving or spawning. */
	static bool BuildArrangementSiteLayout(UWorld* World, const TArray<FVector>& Avoid, TArray<FIslandArrangementSite>& OutSites);

	/** Developer reset: forgets every arranging site and work; fresh empty sites are placed next play. */
	bool ForgetArrangements();

	/**
	 * Parses a world-state file into this object's records without spawning, saving, or placing
	 * anything; usable on a bare instance (e.g. to preview lasting changes in the editor world).
	 * Returns false if the file is missing or unreadable.
	 */
	bool ReadStateFile(const FString& Path);

	/** Re-reads the storage file and respawns visible nests. Called automatically when play begins. */
	void LoadAndSpawn();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	TArray<FIslandNestRecord> Nests;
	TOptional<float> SavedHour;
	TOptional<int32> SavedDay;
	TArray<FIslandCurioRecord> Curios;
	TMap<FName, TWeakObjectPtr<AIslandCurio>> CurioActors;
	TArray<FIslandArrangementSite> ArrangementSites;
	TMap<FName, TWeakObjectPtr<AIslandArrangement>> ArrangementActors;
	int32 ShownArrangementDay = 0;
	TMap<FName, TWeakObjectPtr<AIslandNest>> NestActors;
	// Set when an existing file cannot be parsed, so a save never overwrites what it may still hold.
	bool bStorageUnreadable = false;

	bool Save() const;
	void RefreshNestActor(const FIslandNestRecord& Record);
	void DestroyNestActors();
	/** First-time placement near the ListeningStones; all-or-nothing, then saved so it never moves. */
	bool PlaceCurios();
	void RefreshCurioActor(const FIslandCurioRecord& Record);
	bool PlaceArrangementSites();
	void RefreshArrangementActor(const FIslandArrangementSite& Site);
	/** Island day used for weathering: the running clock once it has begun, else the saved day. */
	int32 DisplayDay() const;
};
