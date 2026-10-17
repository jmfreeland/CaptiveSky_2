#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandCurio.h"
#include "IslandArrangement.h"
#include "IslandWorldStateSubsystem.generated.h"

class AIslandNest;
class AIslandWildlifeHome;
class AIslandGuestBook;

UENUM(BlueprintType)
enum class EIslandWildlifeShelterState : uint8
{
	Exposed,
	Temporary,
	Settled,
	Disturbed
};

/** A small, bounded cache entry stored as part of a wildlife home's canonical record. */
USTRUCT(BlueprintType)
struct FIslandWildlifeHomeCache
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	int32 LastChangedDay = 0;
};

/** Renderer-independent facts about a settled or provisional wild-animal home. */
USTRUCT(BlueprintType)
struct FIslandWildlifeHomeRecord
{
	GENERATED_BODY()

	/** Stable place identity; not an Unreal actor name. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FName Species;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FString OccupantAgentId;

	/** A canonical approach point and facing direction for the shelter entrance. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FVector EntranceLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FRotator EntranceRotation = FRotator::ZeroRotator;

	/** Bounded factual shelter score, 0..5; presentation may derive from this later. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	int32 ShelterQuality = 0;

	/** Number of distinct Island days on which the occupant began resting here. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	int32 RestVisitDays = 0;

	/** Last Island day on which a rest visit began; zero means never. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	int32 LastRestStartIslandDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	EIslandWildlifeShelterState ShelterState = EIslandWildlifeShelterState::Temporary;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	TArray<FIslandWildlifeHomeCache> CachedSupplies;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FDateTime LastUsedUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Wildlife Home")
	FDateTime LastDisturbedUtc;
};

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

	/** Island day a storm last tore its outer layer loose, or -1. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|World State")
	int32 StormDamagedDay = -1;
};

/** One short, signed line left in the inn guest book. */
USTRUCT(BlueprintType)
struct FIslandGuestBookEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Guest Book")
	FString AgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Guest Book")
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Guest Book")
	FString Line;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Guest Book")
	FDateTime CreatedUtc;
};

UENUM(BlueprintType)
enum class EIslandCraftOutcome : uint8
{
	Unrated,
	Promising,
	Successful,
	NeedsRevision
};

/** A normalized, bounded property of one crafted attempt (for example hydration or crust). */
USTRUCT(BlueprintType)
struct FIslandCraftParameter
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Value = 0.f;
};

/** A later resident's explicitly attributed acquisition of a craft tradition. */
USTRUCT(BlueprintType)
struct FIslandCraftLesson
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FString TeacherAgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FString LearnerAgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	int32 Day = 1;
};

/** One resident's attempt to practice a known tradition. */
USTRUCT(BlueprintType)
struct FIslandCraftAttemptRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FString PractitionerAgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	TArray<FIslandCraftParameter> Parameters;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	EIslandCraftOutcome Outcome = EIslandCraftOutcome::Unrated;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FDateTime CreatedUtc;
};

/** Persistent, renderer-independent recipe lineage and its bounded practice history. */
USTRUCT(BlueprintType)
struct FIslandCraftTraditionRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FString OriginAgentId;

	/** A concise account of the first source (memory, observation, or discovery). */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FString SourceDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	FDateTime CreatedUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	TArray<FIslandCraftLesson> Lessons;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Craft Tradition")
	TArray<FIslandCraftAttemptRecord> Attempts;
};

/**
 * Persistent, per-level record of lasting changes residents have made to the world.
 *
 * Storage: <ProjectDir>/WorldState/<MapName>.json, rewritten atomically after each change.
 * Like agent memories, this lives outside Saved/ and is backed up externally rather than
 * tracked in git. Every change is bounded (fixed layer cap per site) and reversible by a
 * developer through Island.RemoveNest or by editing/deleting the file with play stopped.
 *
 * Nests, wildlife-home facts, curios (hidden stones, the seed pod, the cairn), resident stone arrangements, inn guest-book entries, craft traditions, and the Island clock are stored today; the file keeps a top-level version so other
 * kinds of lasting change can be added beside them later.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandWorldStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxNestLayers = 5;
	static constexpr int32 MaxGuestBookEntries = 24;
	static constexpr int32 MaxGuestBookLineLength = 180;
	static constexpr int32 MaxArrangementObservers = 32;
	static constexpr int32 MaxArrangementLessons = 32;
	static constexpr int32 MaxWildlifeHomes = 32;
	static constexpr int32 MaxWildlifeHomeCacheSlots = 8;
	static constexpr int32 MaxWildlifeHomeCacheQuantity = 20;
	static constexpr int32 MaxWildlifeHomeRestVisitDays = 10000;
	static constexpr int32 MaxCraftTraditions = 16;
	static constexpr int32 MaxCraftIdLength = 48;
	static constexpr int32 MaxCraftLessons = 32;
	static constexpr int32 MaxCraftAttemptsPerTradition = 24;
	static constexpr int32 MaxCraftParameters = 4;
	static constexpr int32 MaxCraftNameLength = 80;
	static constexpr int32 MaxCraftSourceLength = 160;

	/** Tests point this at a scratch file before the world begins play. Empty = the per-map project file. */
	FString StorageFileOverride;

	/** Empty for worlds without a saved map (code-created fixtures), which never persist anything. */
	FString GetStorageFilePath() const;

	const FIslandNestRecord* FindNest(FName SiteTag) const;
	const TArray<FIslandNestRecord>& GetNests() const { return Nests; }
	const TArray<FIslandWildlifeHomeRecord>& GetWildlifeHomes() const { return WildlifeHomes; }
	const FIslandWildlifeHomeRecord* FindWildlifeHome(FName Id) const;
	/** Stores validated canonical facts and refreshes a visual-only materializer for supported species; it does not validate or authorize the chosen site. */
	bool StoreWildlifeHome(const FIslandWildlifeHomeRecord& Record);
	/** Records a physically present rest start once per Island day; three distinct days settle a non-disturbed home. */
	bool RecordWildlifeHomeRestStart(FName Id, int32 IslandDay);
	/** Developer reversal for a home record. */
	bool RemoveWildlifeHome(FName Id);
	AIslandWildlifeHome* FindWildlifeHomeActor(FName Id) const;

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
	/** Environmental wetness (0..1) at the end of the previous session, if one was recorded. */
	TOptional<float> GetSavedWetness() const { return SavedWetness; }

	/**
	 * A full storm's marks, applied at most once per storm (storms closer than an hour of weather time
	 * count as one): the cairn loses its top stone (never below three) and each nest with more than one
	 * layer loses its outer layer. Saved; returns true when this storm had not yet left its marks.
	 */
	bool ApplyStormMarks(double WeatherSeconds, int32 Today, TArray<FString>& OutMarks);

	/** Total weather time lived so far (seconds), so the next session's weather carries on. */
	TOptional<double> GetSavedWeatherSeconds() const { return SavedWeatherSeconds; }
	bool SaveWeatherSeconds(double Seconds);

	/** Records the current Island hour and day so the next session can resume from them. */
	bool SaveClock(float Hour, int32 Day);
	/** Records lingering ground wetness independently of whether the Island clock resumes. */
	bool SaveWetness(float Wetness);

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
	/** Persists one visible twig bundle gathered from an empty site; a site renews once a later Island day begins. */
	bool GatherArrangementTwigs(FName SiteId, const FString& AgentId, int32 Today);
	/** Records a resident's first unobstructed sighting of public work; title and intent remain private. */
	bool RecordArrangementObservation(FName SiteId, const FString& AgentId);
	/** Records a bounded lesson passed by a resident who knows the work's visible form; does not mark it seen. */
	bool RecordArrangementTeaching(FName SiteId, const FString& TeacherAgentId, const FString& LearnerAgentId, int32 Today);
	/** True only if the resident made, saw, or was taught about this work. */
	bool HasArrangementKnowledge(FName SiteId, const FString& AgentId) const;
	static bool ParseArrangementForm(const FString& Text, EIslandArrangementForm& OutForm);
	static FString FormName(EIslandArrangementForm Form);

	/**
	 * A resident arranges stones at an arranging site on Island day Today: a new work on empty
	 * ground, or a small response beside someone else's work. Bounded to one arrangement per
	 * resident per Island day. Returns a factual outcome; bOutChanged is true only when the
	 * lasting change was saved.
	 */
	FString ArrangeStones(FName SiteId, const FString& Form, const FString& Title, const FString& Intent,
		const FString& AgentId, int32 Today, bool& bOutChanged, const FString& InfluenceSiteId = FString());
	/** Whether this identity has already made a work or response on this Island day. */
	bool HasArrangedStonesToday(const FString& AgentId, int32 Today) const;

	/** Where arranging sites would go around the ListeningStones, clear of Avoid points. No saving or spawning. */
	static bool BuildArrangementSiteLayout(UWorld* World, const TArray<FVector>& Avoid, TArray<FIslandArrangementSite>& OutSites);

	/** Developer reset: forgets every arranging site and work; fresh empty sites are placed next play. */
	bool ForgetArrangements();

	const TArray<FIslandGuestBookEntry>& GetGuestBookEntries() const { return GuestBookEntries; }
	/** Adds one cleaned line per resident per Island day. The oldest line rolls off at the fixed cap. */
	FString WriteGuestBook(const FString& AgentId, const FString& Line, int32 Today, bool& bOutChanged);
	/** Writes for the single local visitor identity using the active Island day and shared daily limit. */
	FString WriteVisitorGuestBook(const FString& Line, bool& bOutChanged);
	/** Whether this identity has already written today; used to present honest UI before accepting text. */
	bool HasGuestBookEntryToday(const FString& AgentId) const;
	/** Developer reversal for all guest-book lines. */
	bool ForgetGuestBook();

	const TArray<FIslandCraftTraditionRecord>& GetCraftTraditions() const { return CraftTraditions; }
	const FIslandCraftTraditionRecord* FindCraftTradition(FName Id) const;
	/** Knowledge is acquired only by originating a tradition or through its explicit lesson lineage. */
	bool HasCraftKnowledge(FName Id, const FString& AgentId) const;
	/** Records a first practice or a known resident's bounded daily attempt; never creates a prop. */
	bool RecordCraftAttempt(FName Id, const FString& DisplayName, const FString& PractitionerAgentId, int32 Day,
		const FString& SourceDescription, const TArray<FIslandCraftParameter>& Parameters, EIslandCraftOutcome Outcome);
	/** Adds one attributed lesson only when the teacher knows the tradition and the learner does not. */
	bool RecordCraftTeaching(FName Id, const FString& TeacherAgentId, const FString& LearnerAgentId, int32 Day);

	/**
	 * Parses a world-state file into this object's records without spawning, saving, or placing
	 * anything; usable on a bare instance (e.g. to preview lasting changes in the editor world).
	 * Returns false if the file is missing or unreadable.
	 */
	bool ReadStateFile(const FString& Path);

	/** Re-reads storage and respawns transient nest/home/curio/arrangement presentations. Called automatically when play begins. */
	void LoadAndSpawn();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	friend class FIslandNestTest;
	TArray<FIslandNestRecord> Nests;
	TArray<FIslandWildlifeHomeRecord> WildlifeHomes;
	TOptional<float> SavedHour;
	TOptional<int32> SavedDay;
	TOptional<double> SavedWeatherSeconds;
	double LastStormMarkSeconds = -1.0;
	TOptional<float> SavedWetness;
	TArray<FIslandCurioRecord> Curios;
	TMap<FName, TWeakObjectPtr<AIslandCurio>> CurioActors;
	TArray<FIslandArrangementSite> ArrangementSites;
	TArray<FIslandGuestBookEntry> GuestBookEntries;
	TArray<FIslandCraftTraditionRecord> CraftTraditions;
	TMap<FName, TWeakObjectPtr<AIslandArrangement>> ArrangementActors;
	TWeakObjectPtr<AIslandGuestBook> GuestBookActor;
	int32 ShownArrangementDay = 0;
	TMap<FName, TWeakObjectPtr<AIslandNest>> NestActors;
	TMap<FName, TWeakObjectPtr<AIslandWildlifeHome>> WildlifeHomeActors;
	// Set when an existing file cannot be parsed, so a save never overwrites what it may still hold.
	bool bStorageUnreadable = false;

	bool Save() const;
	void RefreshNestActor(const FIslandNestRecord& Record);
	void DestroyNestActors();
	void RefreshWildlifeHomeActor(const FIslandWildlifeHomeRecord& Record);
	void DestroyWildlifeHomeActors();
	/** First-time placement near the ListeningStones; all-or-nothing, then saved so it never moves. */
	bool PlaceCurios();
	void RefreshCurioActor(const FIslandCurioRecord& Record);
	bool PlaceArrangementSites();
	void RefreshArrangementActor(const FIslandArrangementSite& Site);
	void RefreshGuestBookActor();
	/** Island day used for weathering: the running clock once it has begun, else the saved day. */
	int32 DisplayDay() const;
};
