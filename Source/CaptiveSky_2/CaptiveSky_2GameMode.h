// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CaptiveSky_2GameMode.generated.h"

class AController;
class APlayerController;
struct FUniqueNetIdRepl;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ACaptiveSky_2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ACaptiveSky_2GameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	static bool ShouldPauseHostedWorld(ENetMode NetMode, bool bExplicitlyEnabled, int32 RemainingPlayers);
	static bool ShouldRejectHostedJoin(bool bHostedPolicyEnabled, bool bRecoveryRequired);

private:
	bool bResidentWorkFencedForNoVisitors = false;
	bool bHostedWorldPausedForNoVisitors = false;
	bool bHostedWorldRecoveryRequired = false;
	bool IsHostedEmptyWorldPolicyEnabled() const;
	void FenceResidentWorkForEmptyWorld();
	void PauseHostedWorldIfEmpty();
};



