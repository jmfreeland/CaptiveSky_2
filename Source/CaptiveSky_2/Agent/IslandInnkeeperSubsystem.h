#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandInnkeeperSubsystem.generated.h"

class AAutonomousAgentCharacter;
struct FNavLocation;

/** Spawns the optional inn resident only in the Island play world. */
UCLASS(Config=Game, DefaultConfig)
class CAPTIVESKY_2_API UIslandInnkeeperSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, Category="Island|Residents")
	TSoftClassPtr<AAutonomousAgentCharacter> BodyClass;

	UPROPERTY(Config, EditDefaultsOnly, Category="Island|Residents")
	FString AgentId = TEXT("Agent_Innkeeper_01");

	/** Accept the saved Island map name and PIE's UEDPIE_<n>_Island form only. */
	static bool IsIslandMapName(const FString& MapName);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	friend class FIslandInnkeeperSpawnTest;
	static void InitializeInnkeeper(AAutonomousAgentCharacter* Agent, const FString& InAgentId);
	static FVector GetSpawnCandidate(const AActor* InnMarker, const AActor* HearthMarker);
	static TArray<FVector> GetSpawnCandidates(const AActor* InnMarker, const AActor* HearthMarker);
	static bool FindSpawnLocation(UWorld& World, const TArray<FVector>& Candidates, const FVector& DoorCandidate,
		float CapsuleRadius, float CapsuleHalfHeight, const AActor* InnMarker, const AActor* HearthMarker,
		FNavLocation& OutLocation, int32& OutCandidateIndex, int32& OutExtraClearanceCm);
	void SpawnInnkeeper(UWorld& World);
};
