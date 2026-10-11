#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandRatSubsystem.generated.h"

class AAutonomousAgentCharacter;

/** Spawns the rat resident near the Inn in the Island play world, until a real rat hollow anchor replaces this. */
UCLASS()
class CAPTIVESKY_2_API UIslandRatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* GetBodyClassPath() { return TEXT("/Game/Agents/BP_Rat_Placeholder.BP_Rat_Placeholder_C"); }
	static const TCHAR* GetAgentId() { return TEXT("Agent_Rat_01"); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	void SpawnRat(UWorld& World);
};
