#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandNavProbe, Log, All);

// Developer check of the navigation a running game actually uses (the map's saved navmesh, not an editor rebuild):
// is there walkable ground at X Y Z, and a complete route from there to the ListeningStones and the inn counter?
static FAutoConsoleCommandWithWorldAndArgs GIslandNavProbeCommand(
	TEXT("Island.NavProbe"),
	TEXT("Developer check: logs whether X Y Z is on the running game's navmesh and whether complete routes lead from it to the ListeningStones and the inn counter. Usage: Island.NavProbe X Y Z"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UNavigationSystemV1* Navigation = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		if (!Navigation || Args.Num() < 3) { UE_LOG(LogIslandNavProbe, Warning, TEXT("Usage: Island.NavProbe X Y Z (needs a world with navigation).")); return; }
		const FVector At(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
		FNavLocation Start;
		if (!Navigation->ProjectPointToNavigation(At, Start, FVector(40, 40, 150)))
		{
			UE_LOG(LogIslandNavProbe, Log, TEXT("NavProbe %s: no navigation within 40 cm sideways / 150 cm vertically."), *At.ToString());
			return;
		}
		UE_LOG(LogIslandNavProbe, Log, TEXT("NavProbe %s: on navigation at %s."), *At.ToString(), *Start.Location.ToString());
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const bool bStones = It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"));
			const bool bCounter = It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnCounter"));
			if (!bStones && !bCounter) continue;
			FNavLocation Goal;
			if (!Navigation->ProjectPointToNavigation(It->GetActorLocation(), Goal, FVector(250, 250, 1000)))
			{
				UE_LOG(LogIslandNavProbe, Log, TEXT("  %s: no navigation near it."), bStones ? TEXT("ListeningStones") : TEXT("InnCounter"));
				continue;
			}
			const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(World, Start.Location, Goal.Location);
			UE_LOG(LogIslandNavProbe, Log, TEXT("  %s: goal %s; route %s%s"), bStones ? TEXT("ListeningStones") : TEXT("InnCounter"), *Goal.Location.ToString(),
				!Path || !Path->IsValid() ? TEXT("missing") : Path->IsPartial() ? TEXT("partial") : TEXT("complete"),
				Path && Path->IsValid() ? *FString::Printf(TEXT(", %.0f m"), Path->GetPathLength() / 100.f) : TEXT(""));
		}
	}));

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorldAndArgs GIslandNavBuildProbeCommand(
	TEXT("Island.NavBuildProbe"),
	TEXT("Development-only delayed nav build check. Releases the async-load nav lock, builds runtime navigation, and optionally moves Aster after verifying a complete route. Usage: Island.NavBuildProbe X Y Z DelaySeconds [MoveAsterToStones] (DelaySeconds 5..90)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UNavigationSystemV1* Navigation = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		double DelaySeconds = 0.0;
		if (!Navigation || !World->IsGameWorld() || Args.Num() < 4
			|| !LexTryParseString(DelaySeconds, *Args[3]) || DelaySeconds < 5.0 || DelaySeconds > 90.0)
		{
			UE_LOG(LogIslandNavProbe, Warning, TEXT("Usage: Island.NavBuildProbe X Y Z DelaySeconds [MoveAsterToStones] (running game world, delay 5..90 s)."));
			return;
		}

		const FVector At(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
		const bool bMoveAsterToStones = Args.Num() > 4 && Args[4].Equals(TEXT("MoveAsterToStones"), ESearchCase::IgnoreCase);
		const uint8 AsyncLoadLock = ENavigationBuildLock::AsyncLoadLock;
		if (Navigation->IsNavigationBuildingLocked(AsyncLoadLock))
		{
			Navigation->RemoveNavigationBuildLock(AsyncLoadLock, UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
			UE_LOG(LogIslandNavProbe, Display, TEXT("NavBuildProbe released AsyncLoadLock."));
		}

		Navigation->Build();
		UE_LOG(LogIslandNavProbe, Display, TEXT("NavBuildProbe scheduled after %.0f s; build in progress=%d, remaining tasks=%d."),
			DelaySeconds, Navigation->IsNavigationBuildInProgress(), Navigation->GetNumRemainingBuildTasks());

		const TWeakObjectPtr<UWorld> WeakWorld(World);
		FTimerHandle ProbeTimer;
		World->GetTimerManager().SetTimer(ProbeTimer, FTimerDelegate::CreateLambda([WeakWorld, At, bMoveAsterToStones]()
		{
			UWorld* ProbeWorld = WeakWorld.Get();
			UNavigationSystemV1* ProbeNavigation = ProbeWorld
				? FNavigationSystem::GetCurrent<UNavigationSystemV1>(ProbeWorld)
				: nullptr;
			if (!ProbeNavigation) return;

			UE_LOG(LogIslandNavProbe, Display, TEXT("NavBuildProbe check: build in progress=%d, remaining tasks=%d."),
				ProbeNavigation->IsNavigationBuildInProgress(), ProbeNavigation->GetNumRemainingBuildTasks());
			FNavLocation Start;
			if (!ProbeNavigation->ProjectPointToNavigation(At, Start, FVector(40, 40, 150)))
			{
				UE_LOG(LogIslandNavProbe, Warning, TEXT("NavBuildProbe found no navigation near %s."), *At.ToString());
				return;
			}

			for (TActorIterator<AActor> It(ProbeWorld); It; ++It)
			{
				if (!It->ActorHasTag(TEXT("ListeningStones")) || !It->ActorHasTag(TEXT("IslandLandmark"))) continue;
				FNavLocation Goal;
				if (!ProbeNavigation->ProjectPointToNavigation(It->GetActorLocation(), Goal, FVector(250, 250, 1000)))
				{
					UE_LOG(LogIslandNavProbe, Warning, TEXT("NavBuildProbe found no nav goal near ListeningStones."));
					return;
				}

				const UNavigationPath* Path = ProbeNavigation->FindPathToLocationSynchronously(ProbeWorld, Start.Location, Goal.Location);
				const bool bHasCompleteRoute = Path && Path->IsValid() && !Path->IsPartial();
				UE_LOG(LogIslandNavProbe, Display, TEXT("NavBuildProbe route to ListeningStones: %s%s"),
					!Path || !Path->IsValid() ? TEXT("missing") : Path->IsPartial() ? TEXT("partial") : TEXT("complete"),
					Path && Path->IsValid() ? *FString::Printf(TEXT(", %.0f m"), Path->GetPathLength() / 100.f) : TEXT(""));
				if (!bMoveAsterToStones || !bHasCompleteRoute) return;

				IConsoleObject* MoveCommandObject = IConsoleManager::Get().FindConsoleObject(TEXT("Island.MoveProbe"), false);
				IConsoleCommand* MoveCommand = MoveCommandObject ? MoveCommandObject->AsCommand() : nullptr;
				if (!MoveCommand)
				{
					UE_LOG(LogIslandNavProbe, Error, TEXT("NavBuildProbe could not find the Island.MoveProbe console command."));
					return;
				}

				const TArray<FString> MoveArgs = { TEXT("Agent_Aster_01"), TEXT("ListeningStones"), TEXT("Interact") };
				UE_LOG(LogIslandNavProbe, Display, TEXT("NavBuildProbe dispatched Aster's ListeningStones move: %s."),
					MoveCommand->Execute(MoveArgs, ProbeWorld, *GLog) ? TEXT("yes") : TEXT("no"));
				return;
			}

			UE_LOG(LogIslandNavProbe, Warning, TEXT("NavBuildProbe could not find the ListeningStones landmark."));
		}), static_cast<float>(DelaySeconds), false);
	}));
#endif
