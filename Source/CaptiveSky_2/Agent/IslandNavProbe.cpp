#include "CoreMinimal.h"
#include "AgentMemoryComponent.h"
#include "AutonomousAgentAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "IslandInteractionUtility.h"
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

// Non-interactive audit of the same grounded approach criteria used by Aster. A complete
// navmesh route alone is not enough if the pawn capsule clips blocking collision.
static bool ProjectGroundedAuditTarget(UNavigationSystemV1* Navigation, const FVector& Target,
	const FNavAgentProperties& AgentProperties, FNavLocation& OutLocation)
{
	if (!Navigation) return false;
	if (Navigation->ProjectPointToNavigation(Target, OutLocation, FVector(250.f, 250.f, 120.f), &AgentProperties)) return true;

	for (const float Radius : { 100.f, 200.f, 300.f, 400.f })
	{
		bool bFoundAtRadius = false;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		FNavLocation BestLocation;
		for (int32 AngleDegrees = 0; AngleDegrees < 360; AngleDegrees += 45)
		{
			const float Angle = FMath::DegreesToRadians(static_cast<float>(AngleDegrees));
			const FVector Candidate = Target + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
			FNavLocation Projected;
			if (!Navigation->ProjectPointToNavigation(Candidate, Projected, FVector(75.f, 75.f, 120.f), &AgentProperties)) continue;
			const float DistanceSquared = FVector::DistSquared2D(Target, Projected.Location);
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestLocation = Projected;
				bFoundAtRadius = true;
			}
		}
		if (bFoundAtRadius)
		{
			OutLocation = BestLocation;
			return true;
		}
	}

	return Navigation->ProjectPointToNavigation(Target, OutLocation, FVector(250.f, 250.f, 1000.f), &AgentProperties);
}

static void AuditMovementLandmarks(UWorld* World)
{
	UNavigationSystemV1* Navigation = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!Navigation || !World || !World->IsGameWorld())
	{
		UE_LOG(LogIslandNavProbe, Warning, TEXT("LandmarkNavAudit needs a running game world with navigation."));
		return;
	}

	APawn* Aster = nullptr;
	for (TActorIterator<APawn> PawnIt(World); PawnIt; ++PawnIt)
	{
		const UAgentMemoryComponent* Memory = PawnIt->FindComponentByClass<UAgentMemoryComponent>();
		if (Memory && Memory->GetResolvedAgentId().Equals(TEXT("Agent_Aster_01"), ESearchCase::IgnoreCase))
		{
			Aster = *PawnIt;
			break;
		}
	}
	const UCapsuleComponent* Capsule = Aster ? Aster->FindComponentByClass<UCapsuleComponent>() : nullptr;
	if (!Aster || !Capsule)
	{
		UE_LOG(LogIslandNavProbe, Warning, TEXT("LandmarkNavAudit could not find Agent_Aster_01 and its capsule in this world."));
		return;
	}

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FNavAgentProperties& AgentProperties = Aster->GetNavAgentPropertiesRef();
	FNavLocation Start;
	const FVector StartOnFloor = Aster->GetActorLocation() - FVector(0.f, 0.f, HalfHeight + 2.f);
	if (!ProjectGroundedAuditTarget(Navigation, StartOnFloor, AgentProperties, Start))
	{
		UE_LOG(LogIslandNavProbe, Warning, TEXT("LandmarkNavAudit could not project Aster's grounded start %s."), *StartOnFloor.ToCompactString());
		return;
	}

	int32 LandmarkCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Landmark = *It;
		const FName TargetTag = IslandInteractionUtility::GetTargetTag(Landmark);
		if (!Landmark->ActorHasTag(TEXT("IslandLandmark")) || TargetTag.IsNone() ||
			!IslandInteractionUtility::IsMovementTargetAllowed(Landmark)) continue;
		++LandmarkCount;

		int32 Projected = 0;
		int32 InRange = 0;
		int32 Complete = 0;
		int32 Blocked = 0;
		int32 Clear = 0;
		FVector BestGoal = FVector::ZeroVector;
		float BestLength = TNumericLimits<float>::Max();
		FString FirstBlockerSummary;
		TArray<FVector> TestedGoals;
		FVector PreferredDirection = Start.Location - Landmark->GetActorLocation();
		PreferredDirection.Z = 0.f;
		if (!PreferredDirection.Normalize()) PreferredDirection = FVector::ForwardVector;
		const float PreferredAngle = FMath::Atan2(PreferredDirection.Y, PreferredDirection.X);

		for (const float Radius : { 150.f, 200.f, 250.f, 325.f, 375.f })
		{
			for (int32 Side = 0; Side < 8; ++Side)
			{
				const float Angle = PreferredAngle + Side * (PI / 4.f);
				const FVector Desired = Landmark->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
				FNavLocation Goal;
				if (!ProjectGroundedAuditTarget(Navigation, Desired, AgentProperties, Goal)) continue;
				++Projected;
				if (TestedGoals.ContainsByPredicate([&Goal](const FVector& Existing)
					{ return FVector::DistSquared2D(Existing, Goal.Location) < FMath::Square(50.f); })) continue;
				TestedGoals.Add(Goal.Location);
				const FVector BodyCenter = Goal.Location + FVector(0.f, 0.f, HalfHeight + 2.f);
				if (FVector::DistSquared(BodyCenter, Landmark->GetActorLocation()) >
					FMath::Square(IslandInteractionUtility::DefaultInteractionRange - IslandInteractionUtility::GroundedApproachRangeMargin)) continue;
				++InRange;

				const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(World, Start.Location, Goal.Location, Aster);
				if (!Path || !Path->IsValid() || Path->IsPartial()) continue;
				++Complete;
				if (Path->PathPoints.Num() < 2) continue;

				int32 BlockedSegment = INDEX_NONE;
				FHitResult Blocker;
				const bool bPhysicallyClear = AAutonomousAgentAIController::IsCapsulePathPhysicallyClear(
					World, Path, Capsule, Aster, &BlockedSegment, &Blocker);
				if (!bPhysicallyClear)
				{
					++Blocked;
					if (FirstBlockerSummary.IsEmpty())
					{
						const AActor* BlockingActor = Blocker.GetActor();
						const UPrimitiveComponent* BlockingComponent = Blocker.GetComponent();
						const UStaticMeshComponent* BlockingMesh = Cast<UStaticMeshComponent>(BlockingComponent);
						const UStaticMesh* StaticMesh = BlockingMesh ? BlockingMesh->GetStaticMesh() : nullptr;
						FirstBlockerSummary = FString::Printf(TEXT("first blocked at segment %d by %s / %s, mesh %s at %s"),
							BlockedSegment,
							BlockingActor ? *BlockingActor->GetName() : TEXT("<no actor>"),
							BlockingComponent ? *BlockingComponent->GetName() : TEXT("<no component>"),
							StaticMesh ? *StaticMesh->GetPathName() : TEXT("<no static mesh>"),
							*Blocker.ImpactPoint.ToCompactString());
					}
					continue;
				}
				++Clear;
				if (Path->GetPathLength() < BestLength)
				{
					BestLength = Path->GetPathLength();
					BestGoal = Goal.Location;
				}
			}
		}

		UE_LOG(LogIslandNavProbe, Display,
			TEXT("Landmark direct-approach audit %s [%s] at %s: projected=%d in-range=%d complete=%d capsule-blocked=%d capsule-clear=%d; %s%s."),
			*Landmark->GetName(), *TargetTag.ToString(), *Landmark->GetActorLocation().ToCompactString(),
			Projected, InRange, Complete, Blocked, Clear,
			Clear > 0 ? *FString::Printf(TEXT("best goal %s, %.0f cm"), *BestGoal.ToCompactString(), BestLength)
				: TEXT("no capsule-clear complete direct approach"),
			FirstBlockerSummary.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("; %s"), *FirstBlockerSummary));
	}
	UE_LOG(LogIslandNavProbe, Display, TEXT("LandmarkNavAudit complete: %d movement-eligible tagged landmarks; no actors were moved or changed."), LandmarkCount);
}

static FAutoConsoleCommandWithWorldAndArgs GIslandLandmarkNavAuditCommand(
	TEXT("Island.LandmarkNavAudit"),
	TEXT("Non-interactive audit of Aster's capsule-clear grounded routes to tagged movement landmarks. Usage: Island.LandmarkNavAudit [DelaySeconds] (5..90 also releases the async nav lock and builds navigation for this session)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogIslandNavProbe, Warning, TEXT("LandmarkNavAudit needs a running game world."));
			return;
		}
		if (Args.IsEmpty())
		{
			AuditMovementLandmarks(World);
			return;
		}

		double DelaySeconds = 0.0;
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!Navigation || Args.Num() != 1 || !LexTryParseString(DelaySeconds, *Args[0]) || DelaySeconds < 5.0 || DelaySeconds > 90.0)
		{
			UE_LOG(LogIslandNavProbe, Warning, TEXT("Usage: Island.LandmarkNavAudit [DelaySeconds] (5..90 when supplied)."));
			return;
		}

		const uint8 AsyncLoadLock = ENavigationBuildLock::AsyncLoadLock;
		if (Navigation->IsNavigationBuildingLocked(AsyncLoadLock))
		{
			Navigation->RemoveNavigationBuildLock(AsyncLoadLock, UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
			UE_LOG(LogIslandNavProbe, Display, TEXT("LandmarkNavAudit released AsyncLoadLock."));
		}
		Navigation->Build();
		UE_LOG(LogIslandNavProbe, Display, TEXT("LandmarkNavAudit scheduled after %.0f s; build in progress=%d, remaining tasks=%d."),
			DelaySeconds, Navigation->IsNavigationBuildInProgress(), Navigation->GetNumRemainingBuildTasks());

		const TWeakObjectPtr<UWorld> WeakWorld(World);
		FTimerHandle ProbeTimer;
		World->GetTimerManager().SetTimer(ProbeTimer, FTimerDelegate::CreateLambda([WeakWorld]()
		{
			if (UWorld* ProbeWorld = WeakWorld.Get()) AuditMovementLandmarks(ProbeWorld);
		}), static_cast<float>(DelaySeconds), false);
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
