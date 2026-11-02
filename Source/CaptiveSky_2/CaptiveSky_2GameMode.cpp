// Copyright Epic Games, Inc. All Rights Reserved.

#include "CaptiveSky_2GameMode.h"
#include "Agent/AgentPlaySessionSubsystem.h"
#include "Agent/IslandDayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogHostedWorldPresence, Log, All);

ACaptiveSky_2GameMode::ACaptiveSky_2GameMode()
{
	// stub
}

bool ACaptiveSky_2GameMode::ShouldPauseHostedWorld(ENetMode NetMode, bool bExplicitlyEnabled, int32 RemainingPlayers)
{
	return NetMode == NM_DedicatedServer && bExplicitlyEnabled && RemainingPlayers <= 0;
}

bool ACaptiveSky_2GameMode::ShouldRejectHostedJoin(bool bHostedPolicyEnabled, bool bRecoveryRequired)
{
	return bHostedPolicyEnabled && bRecoveryRequired;
}

bool ACaptiveSky_2GameMode::IsHostedEmptyWorldPolicyEnabled() const
{
	const UWorld* World = GetWorld();
	return World && ShouldPauseHostedWorld(World->GetNetMode(),
		FParse::Param(FCommandLine::Get(), TEXT("CaptiveSkyPauseWhenEmpty")), 0);
}

void ACaptiveSky_2GameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (IsHostedEmptyWorldPolicyEnabled() && GetNumPlayers() <= 0)
	{
		// Close admission before resident actors can start their first thought. The actual pause and
		// durable clock flush wait until BeginPlay has initialized the saved clock actor.
		FenceResidentWorkForEmptyWorld();
	}
}

void ACaptiveSky_2GameMode::BeginPlay()
{
	Super::BeginPlay();
	if (IsHostedEmptyWorldPolicyEnabled() && GetNumPlayers() <= 0)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ACaptiveSky_2GameMode::PauseHostedWorldIfEmpty);
	}
}

void ACaptiveSky_2GameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty() || !ShouldRejectHostedJoin(IsHostedEmptyWorldPolicyEnabled(), bHostedWorldRecoveryRequired)) return;
	ErrorMessage = TEXT("The Island could not verify its last durable save and is waiting for operator recovery.");
	UE_LOG(LogHostedWorldPresence, Error, TEXT("Rejected visitor connection from %s because empty-world persistence needs recovery."), *Address);
}

void ACaptiveSky_2GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!bResidentWorkFencedForNoVisitors || !IsHostedEmptyWorldPolicyEnabled()) return;

	if (bHostedWorldPausedForNoVisitors && !UGameplayStatics::SetGamePaused(this, false))
	{
		UE_LOG(LogHostedWorldPresence, Error, TEXT("Could not resume the hosted Island after a visitor connected; resident work remains fenced."));
		return;
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAgentPlaySessionSubsystem* Session = GameInstance->GetSubsystem<UAgentPlaySessionSubsystem>())
		{
			Session->ResumeResidentWorkForVisitor();
		}
	}
	bResidentWorkFencedForNoVisitors = false;
	bHostedWorldPausedForNoVisitors = false;
	UE_LOG(LogHostedWorldPresence, Display, TEXT("Hosted Island resumed for %d connected visitor(s)."), GetNumPlayers());
}

void ACaptiveSky_2GameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	const UWorld* World = GetWorld();
	if (!World || !ShouldPauseHostedWorld(World->GetNetMode(),
		FParse::Param(FCommandLine::Get(), TEXT("CaptiveSkyPauseWhenEmpty")), GetNumPlayers()) ||
		bHostedWorldPausedForNoVisitors)
	{
		return;
	}
	PauseHostedWorldIfEmpty();
}

void ACaptiveSky_2GameMode::FenceResidentWorkForEmptyWorld()
{
	if (bResidentWorkFencedForNoVisitors) return;
	bResidentWorkFencedForNoVisitors = true;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAgentPlaySessionSubsystem* Session = GameInstance->GetSubsystem<UAgentPlaySessionSubsystem>())
		{
			Session->PauseResidentWorkForEmptyWorld();
		}
	}
}

void ACaptiveSky_2GameMode::PauseHostedWorldIfEmpty()
{
	UWorld* World = GetWorld();
	if (!World || !ShouldPauseHostedWorld(World->GetNetMode(),
		FParse::Param(FCommandLine::Get(), TEXT("CaptiveSkyPauseWhenEmpty")), GetNumPlayers())) return;
	if (bHostedWorldPausedForNoVisitors) return;

	// Fence model responses before the durable checkpoint. A late provider callback may complete,
	// but its generation will be stale and it cannot add memory or an action to the empty Island.
	FenceResidentWorkForEmptyWorld();

	int32 ClockCount = 0;
	bool bCheckpointSucceeded = true;
	for (TActorIterator<AIslandDayNight> It(World); It; ++It)
	{
		++ClockCount;
		bCheckpointSucceeded &= It->FlushDurableState();
	}
	if (ClockCount == 0 || !bCheckpointSucceeded)
	{
		bHostedWorldRecoveryRequired = true;
		UE_LOG(LogHostedWorldPresence, Error,
			TEXT("Hosted Island clock checkpoint was unavailable or failed (%d clock actors); pausing with the last durable state."), ClockCount);
	}

	if (!UGameplayStatics::SetGamePaused(this, true))
	{
		UE_LOG(LogHostedWorldPresence, Error,
			TEXT("Could not pause the hosted Island after its final visitor left; resident work remains fenced until a visitor reconnects."));
		return;
	}
	bHostedWorldPausedForNoVisitors = true;
	UE_LOG(LogHostedWorldPresence, Display,
		TEXT("Hosted Island paused after final visitor left; clock checkpoint %s."),
		ClockCount > 0 && bCheckpointSucceeded ? TEXT("succeeded") : TEXT("failed"));
}
