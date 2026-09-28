#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

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
