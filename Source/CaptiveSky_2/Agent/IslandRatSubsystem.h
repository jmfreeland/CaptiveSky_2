#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandRatSubsystem.generated.h"

class AAutonomousAgentCharacter;
struct FIslandWildlifeHomeRecord;

/** Spawns the rat resident near its registered wildlife home, falling back to the Inn until one exists. */
UCLASS()
class CAPTIVESKY_2_API UIslandRatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* GetBodyClassPath() { return TEXT("/Game/Agents/BP_Rat_Placeholder.BP_Rat_Placeholder_C"); }
	static const TCHAR* GetAgentId() { return TEXT("Agent_Rat_01"); }
	static bool FindRatHomeLocation(const TArray<FIslandWildlifeHomeRecord>& Homes, FVector& OutLocation,
		FRotator* OutEntranceRotation = nullptr);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	void SpawnRat(UWorld& World);
};
