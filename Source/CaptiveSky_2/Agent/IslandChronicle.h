#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandChronicle.generated.h"

/**
 * A plain-text record of what happens on the Island, for whoever is watching: what residents chose to do
 * and say, what they made or changed, the weather turning, days passing. It is written as one JSON object
 * per line to WorldState/chronicle.jsonl and never calls a model. Scripts/Build-Chronicle.py turns it
 * into a readable account per Island day. Residents never read it; it is not memory.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandChronicleSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Storm strength at which the chronicle says a storm has arrived, and below which it says it has passed. */
	static constexpr float StormArrives = 0.6f;
	static constexpr float StormPasses = 0.3f;

	/** Adds one entry to the chronicle of Island worlds; does nothing in worlds without one (editor previews, test fixtures). */
	static void Record(const UWorld* World, const FString& Type, const FString& Agent, const FString& Text,
		const TMap<FString, FString>& Extra = TMap<FString, FString>());

	/** One JSON line (no trailing newline). Extra fields never replace the fixed ones. */
	static FString FormatEntry(const FDateTime& Utc, int32 Day, const FString& Clock, const FString& Type,
		const FString& Agent, const FString& Text, const TMap<FString, FString>& Extra);

	/** Appends one line to the chronicle file; returns false if it could not be written. */
	bool AppendLine(const FString& Line) const;

	void RecordEntry(const FString& Type, const FString& Agent, const FString& Text, const TMap<FString, FString>& Extra);

	/** Tests point this at a scratch file. */
	FString ChronicleFileOverride;
	FString GetChroniclePath() const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	friend class FIslandChronicleTest;
	float SinceCheck = 0.f;
	bool bStormSeen = false;
	bool bStormActive = false;
	bool bClosing = false;
	bool bSessionOpened = false;
	int32 LastDay = 0;
	FString LastClock;

	void RefreshClock();
	void WatchSky();
};
