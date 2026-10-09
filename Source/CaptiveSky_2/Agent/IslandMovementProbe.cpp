#include "AutonomousAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "RavenAgentAIController.h"
#include "IslandWeather.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/TargetPoint.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandMovementProbe, Log, All);

class FIslandMovementProbeCommand
{
public:
	static void RunRavenShelterAudit(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Island.RavenShelterAudit requires a running game world."));
			return;
		}
		const bool bRuntimeReady = Args.ContainsByPredicate([](const FString& Arg)
			{ return Arg.Equals(TEXT("RuntimeReady"), ESearchCase::IgnoreCase); });
		if (!bRuntimeReady)
		{
			TWeakObjectPtr<UWorld> WeakWorld(World);
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(RetryTimer, FTimerDelegate::CreateLambda([WeakWorld, Args]()
			{
				TArray<FString> RetryArgs = Args;
				RetryArgs.Add(TEXT("RuntimeReady"));
				if (UWorld* RetryWorld = WeakWorld.Get())
					FIslandMovementProbeCommand::RunRavenShelterAudit(RetryArgs, RetryWorld);
			}), 5.f, false);
			UE_LOG(LogIslandMovementProbe, Log, TEXT("Shelter audit queued for five Game-world seconds so residents and IslandWeather can finish startup."));
			return;
		}

		ARavenAgentAIController* RavenController = nullptr;
		AIslandWeather* Weather = nullptr;
		AActor* EastRoost = nullptr;
		for (TActorIterator<AIslandWeather> It(World); It; ++It) { Weather = *It; break; }
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!EastRoost && It->ActorHasTag(TEXT("Roost_East")) && It->ActorHasTag(TEXT("RavenPerch"))) EastRoost = *It;
			if (!RavenController)
				if (APawn* Pawn = Cast<APawn>(*It)) RavenController = Cast<ARavenAgentAIController>(Pawn->GetController());
		}
		if (!Weather || !EastRoost || !RavenController || !RavenController->GetPawn())
		{
			UE_LOG(LogIslandMovementProbe, Error,
				TEXT("Shelter audit needs IslandWeather, the tagged Roost_East marker, and a possessed live Raven."));
			return;
		}

		TArray<UHierarchicalInstancedStaticMeshComponent*> GroundCoverComponents;
		Weather->GetComponents<UHierarchicalInstancedStaticMeshComponent>(GroundCoverComponents);
		int64 GroundCoverInstances = 0;
		int32 SpruceInstances = 0;
		for (const UHierarchicalInstancedStaticMeshComponent* Component : GroundCoverComponents)
		{
			if (!Component) continue;
			GroundCoverInstances += Component->GetInstanceCount();
			if (Component->GetName() == TEXT("IslandSpruce")) SpruceInstances = Component->GetInstanceCount();
		}
		if (SpruceInstances == 0)
		{
			UE_LOG(LogIslandMovementProbe, Error,
				TEXT("Shelter audit stopped: runtime IslandWeather has no initialized IslandSpruce HISM instances; canopy evidence would be incomplete (total other HISM instances=%lld)."),
				GroundCoverInstances);
			return;
		}

		struct FCandidate
		{
			FVector Location = FVector::ZeroVector;
			float Distance = TNumericLimits<float>::Max();
			int32 CoverCount = -1;
			bool bWindSheltered = false;
			FString SupportLabel;
			FString WindReport;
		};
		FCandidate Best;
		int32 SupportedCount = 0;
		const FVector Center = EastRoost->GetActorLocation();
		float SearchRadiusCm = 3000.f;
		float GridSpacingCm = 300.f;
		for (const FString& Arg : Args)
		{
			float OptionValue = 0.f;
			if (Arg.StartsWith(TEXT("RadiusCm="), ESearchCase::IgnoreCase))
			{
				if (!LexTryParseString(OptionValue, *Arg.RightChop(9)) || OptionValue < 1000.f || OptionValue > 30000.f)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("RadiusCm must be a number from 1000 to 30000."));
					return;
				}
				SearchRadiusCm = OptionValue;
			}
			else if (Arg.StartsWith(TEXT("GridSpacingCm="), ESearchCase::IgnoreCase))
			{
				if (!LexTryParseString(OptionValue, *Arg.RightChop(14)) || OptionValue < 200.f || OptionValue > 1500.f)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("GridSpacingCm must be a number from 200 to 1500."));
					return;
				}
				GridSpacingCm = OptionValue;
			}
		}
		const ACharacter* Raven = Cast<ACharacter>(RavenController->GetPawn());
		const float HalfHeight = Raven ? Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATargetPoint* Site = World->SpawnActor<ATargetPoint>(Center, FRotator::ZeroRotator, Spawn);
		if (!Site)
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Shelter audit could not create its transient candidate marker."));
			return;
		}
		Site->Tags.Add(TEXT("RavenPerch"));
		Site->Tags.Add(TEXT("RavenNestSite"));
		FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(RavenShelterCandidateGround), false, Raven);
		for (TActorIterator<APawn> It(World); It; ++It) GroundQuery.AddIgnoredActor(*It);

		for (float OffsetX = -SearchRadiusCm; OffsetX <= SearchRadiusCm; OffsetX += GridSpacingCm)
		{
			for (float OffsetY = -SearchRadiusCm; OffsetY <= SearchRadiusCm; OffsetY += GridSpacingCm)
			{
				if (FVector2D(OffsetX, OffsetY).SizeSquared() > FMath::Square(SearchRadiusCm)) continue;
				const FVector TraceTop = Center + FVector(OffsetX, OffsetY, 2000.f);
				const FVector TraceBottom = Center + FVector(OffsetX, OffsetY, -4000.f);
				FHitResult GroundHit;
				if (!World->LineTraceSingleByChannel(GroundHit, TraceTop, TraceBottom, ECC_Visibility, GroundQuery) ||
					GroundHit.ImpactNormal.Z < 0.5f) continue;

				const FVector CandidateLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 5.f);
				Site->SetActorLocation(CandidateLocation, false, nullptr, ETeleportType::TeleportPhysics);
				FHitResult SupportHit;
				if (!RavenController->HasSuitablePerchSupport(Site, &SupportHit)) continue;
				++SupportedCount;
				const int32 CoverCount = RavenController->CountOverheadCoverProbes(Site);
				const FString WindReport = Weather->DescribeWindShelterAt(CandidateLocation, Raven);
				const bool bWindSheltered = WindReport.Contains(TEXT("Solid geometry currently blocks"));
				const float Distance = FVector::Distance(Center, CandidateLocation);
				const bool bBetter = Best.CoverCount < 0 || CoverCount > Best.CoverCount ||
					(CoverCount == Best.CoverCount && bWindSheltered && !Best.bWindSheltered) ||
					(CoverCount == Best.CoverCount && bWindSheltered == Best.bWindSheltered && Distance < Best.Distance);
				if (!bBetter) continue;
				Best.Location = CandidateLocation;
				Best.Distance = Distance;
				Best.CoverCount = CoverCount;
				Best.bWindSheltered = bWindSheltered;
				Best.SupportLabel = SupportHit.GetActor() ? SupportHit.GetActor()->GetActorNameOrLabel() : TEXT("unknown support actor");
				Best.WindReport = WindReport;
			}
		}

		UE_LOG(LogIslandMovementProbe, Log,
			TEXT("Runtime East-roost shelter scan: verified %lld HISM foliage instances across %d components, including %d IslandSpruce instances; %d supported points in %.0f m at %.0f cm spacing."),
			GroundCoverInstances, GroundCoverComponents.Num(), SpruceInstances, SupportedCount, SearchRadiusCm / 100.f, GridSpacingCm);
		if (Best.CoverCount < 0)
		{
			UE_LOG(LogIslandMovementProbe, Warning, TEXT("No upward-supported candidate was found; no Raven action or map change occurred."));
			Site->Destroy();
			return;
		}
		UE_LOG(LogIslandMovementProbe, Log,
			TEXT("Best runtime candidate at %s, %.0f cm from East marker; overhead clue %d/5; support=%s; wind=%s"),
			*Best.Location.ToCompactString(), Best.Distance, Best.CoverCount, *Best.SupportLabel, *Best.WindReport);

		const bool bRequestLanding = Args.ContainsByPredicate([](const FString& Arg)
			{ return Arg.Equals(TEXT("Land"), ESearchCase::IgnoreCase); });
		if (!bRequestLanding || Best.CoverCount < 3)
		{
			if (bRequestLanding && Best.CoverCount < 3)
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Landing not attempted: candidate did not meet the conservative 3/5 overhead-cover screen."));
			Site->Destroy();
			return;
		}
		if (RavenController->LocomotionState != ERavenLocomotionState::Perched || RavenController->IsActionInProgress())
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Landing not attempted: live Raven must already be settled on a perch with no action in progress."));
			Site->Destroy();
			return;
		}

		Site->SetActorLocation(Best.Location, false, nullptr, ETeleportType::TeleportPhysics);
		const FName CandidateTag(TEXT("Codex_RavenShelterCandidate"));
		Site->Tags.Add(CandidateTag);
		if (!RavenController->RequestPerch(CandidateTag))
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Raven rejected the transient candidate perch request."));
			Site->Destroy();
			return;
		}

		TWeakObjectPtr<UWorld> WeakWorld(World);
		TWeakObjectPtr<ARavenAgentAIController> WeakController(RavenController);
		TWeakObjectPtr<ACharacter> WeakRaven(const_cast<ACharacter*>(Raven));
		TWeakObjectPtr<ATargetPoint> WeakSite(Site);
		const double StartedAt = World->GetTimeSeconds();
		TSharedRef<FTimerHandle> PollTimer = MakeShared<FTimerHandle>();
		World->GetTimerManager().SetTimer(*PollTimer, FTimerDelegate::CreateLambda(
			[WeakWorld, WeakController, WeakRaven, WeakSite, StartedAt, PollTimer]()
			{
				UWorld* AuditWorld = WeakWorld.Get();
				ARavenAgentAIController* Controller = WeakController.Get();
				ACharacter* Bird = WeakRaven.Get();
				ATargetPoint* Candidate = WeakSite.Get();
				if (!AuditWorld || !Controller || !Bird || !Candidate) return;
				const bool bLanded = Controller->LocomotionState == ERavenLocomotionState::Perched &&
					FVector::DistSquared(Bird->GetActorLocation(), Candidate->GetActorLocation()) <= FMath::Square(20.f);
				if (!bLanded && AuditWorld->GetTimeSeconds() - StartedAt < 30.0) return;
				AuditWorld->GetTimerManager().ClearTimer(*PollTimer);
				UE_LOG(LogIslandMovementProbe, Log,
					TEXT("Transient Raven candidate landing %s after %.1f simulated seconds; final=%s; site=%s"),
					bLanded ? TEXT("succeeded") : TEXT("did not complete within 30 simulated seconds"),
					AuditWorld->GetTimeSeconds() - StartedAt, *Bird->GetActorLocation().ToCompactString(),
					*Controller->AssessRoostSite(Candidate));
				Candidate->Destroy();
			} ), 0.25f, true);
	}

	static void RunRavenBranchAudit(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Island.RavenBranchAudit requires a running game world."));
			return;
		}
		const bool bRuntimeReady = Args.ContainsByPredicate([](const FString& Arg)
			{ return Arg.Equals(TEXT("RuntimeReady"), ESearchCase::IgnoreCase); });
		if (!bRuntimeReady)
		{
			TWeakObjectPtr<UWorld> WeakWorld(World);
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(RetryTimer, FTimerDelegate::CreateLambda([WeakWorld, Args]()
			{
				TArray<FString> RetryArgs = Args;
				RetryArgs.Add(TEXT("RuntimeReady"));
				if (UWorld* RetryWorld = WeakWorld.Get())
					FIslandMovementProbeCommand::RunRavenBranchAudit(RetryArgs, RetryWorld);
			}), 5.f, false);
			UE_LOG(LogIslandMovementProbe, Log, TEXT("Branch audit queued for five Game-world seconds so the Raven and foliage can finish startup."));
			return;
		}

		ARavenAgentAIController* RavenController = nullptr;
		AActor* EastRoost = nullptr;
		AActor* TreeActor = nullptr;
		UStaticMeshComponent* TreeComponent = nullptr;
		float BestTreeDistance = TNumericLimits<float>::Max();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!EastRoost && It->ActorHasTag(TEXT("Roost_East")) && It->ActorHasTag(TEXT("RavenPerch"))) EastRoost = *It;
			if (!RavenController)
				if (APawn* Pawn = Cast<APawn>(*It)) RavenController = Cast<ARavenAgentAIController>(Pawn->GetController());
		}
		if (!EastRoost || !RavenController || !RavenController->GetPawn())
		{
			UE_LOG(LogIslandMovementProbe, Error,
				TEXT("Branch audit needs the East roost marker and a possessed live Raven."));
			return;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<UStaticMeshComponent*> Meshes;
			It->GetComponents<UStaticMeshComponent>(Meshes);
			for (UStaticMeshComponent* Mesh : Meshes)
			{
				if (!Mesh || !Mesh->GetStaticMesh() || Mesh->GetStaticMesh()->GetName() != TEXT("spruce_half_01")) continue;
				const float Distance = FVector::DistSquared2D(Mesh->GetComponentLocation(), EastRoost->GetActorLocation());
				if (Distance < BestTreeDistance)
				{
					BestTreeDistance = Distance;
					TreeActor = *It;
					TreeComponent = Mesh;
				}
			}
		}
		if (!TreeActor || !TreeComponent)
		{
			UE_LOG(LogIslandMovementProbe, Error,
				TEXT("Branch audit needs the East roost, a possessed live Raven, and the nearest authored spruce_half_01 actor."));
			return;
		}
		if (BestTreeDistance > FMath::Square(1200.f))
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Nearest authored spruce is %.0f cm from Roost_East, beyond the 12 m branch-audit limit."), FMath::Sqrt(BestTreeDistance));
			return;
		}

		const FBox TreeBounds = TreeComponent->Bounds.GetBox();
		if (!TreeBounds.IsValid)
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Nearest authored spruce has invalid world bounds."));
			return;
		}
		const bool bVisibilityBlocks = TreeComponent->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block;
		const bool bQueryCollision = TreeComponent->IsQueryCollisionEnabled();
		const ACharacter* Raven = Cast<ACharacter>(RavenController->GetPawn());
		if (!Raven)
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Branch audit's live Raven pawn is not an ACharacter."));
			return;
		}
		const float HalfHeight = Raven ? Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATargetPoint* Site = World->SpawnActor<ATargetPoint>(TreeBounds.GetCenter(), FRotator::ZeroRotator, Spawn);
		if (!Site)
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Branch audit could not create its transient candidate marker."));
			return;
		}
		Site->Tags.Add(TEXT("RavenPerch"));
		Site->Tags.Add(TEXT("RavenNestSite"));
		const AActor* IgnoredRaven = Raven;
		FCollisionQueryParams TreeQuery(SCENE_QUERY_STAT(RavenSpruceBranchSurface), true, IgnoredRaven);
		TreeQuery.AddIgnoredActor(Site);

		const float ScanStepCm = 75.f;
		const FVector Extent = TreeBounds.GetExtent();
		const FVector TreeCenter = TreeComponent->GetComponentLocation();
		int32 SurfaceTraceCount = 0;
		int32 TreeSurfaceHitCount = 0;
		int32 SupportedBranchCount = 0;
		struct FBranchCandidate
		{
			FVector Location = FVector::ZeroVector;
			float RavenDistance = TNumericLimits<float>::Max();
			int32 CoverCount = INDEX_NONE;
			FVector SurfaceNormal = FVector::UpVector;
		};
		FBranchCandidate Best;
		for (float OffsetX = -Extent.X; OffsetX <= Extent.X; OffsetX += ScanStepCm)
		{
			for (float OffsetY = -Extent.Y; OffsetY <= Extent.Y; OffsetY += ScanStepCm)
			{
				const FVector2D Offset(OffsetX, OffsetY);
				if (Offset.SizeSquared() > FMath::Square(FMath::Max(Extent.X, Extent.Y))) continue;
				const FVector TraceTop(TreeCenter.X + OffsetX, TreeCenter.Y + OffsetY, TreeBounds.Max.Z + 100.f);
				const FVector TraceBottom(TreeCenter.X + OffsetX, TreeCenter.Y + OffsetY, TreeBounds.Min.Z - 100.f);
				FHitResult SurfaceHit;
				++SurfaceTraceCount;
				if (!World->LineTraceSingleByChannel(SurfaceHit, TraceTop, TraceBottom, ECC_Visibility, TreeQuery) ||
					SurfaceHit.GetActor() != TreeActor || SurfaceHit.GetComponent() != TreeComponent) continue;
				++TreeSurfaceHitCount;
				if (SurfaceHit.ImpactNormal.Z < 0.65f || FVector::Dist2D(SurfaceHit.ImpactPoint, TreeCenter) < 80.f) continue;
				const FVector CandidateLocation = SurfaceHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
				Site->SetActorLocation(CandidateLocation, false, nullptr, ETeleportType::TeleportPhysics);
				FHitResult SupportHit;
				if (!RavenController->HasSuitablePerchSupport(Site, &SupportHit) || SupportHit.GetActor() != TreeActor) continue;
				++SupportedBranchCount;
				const int32 CoverCount = RavenController->CountOverheadCoverProbes(Site);
				const float RavenDistance = FVector::DistSquared(Raven->GetActorLocation(), CandidateLocation);
				if (Best.CoverCount > CoverCount ||
					(Best.CoverCount == CoverCount && Best.RavenDistance <= RavenDistance)) continue;
				Best.Location = CandidateLocation;
				Best.RavenDistance = RavenDistance;
				Best.CoverCount = CoverCount;
				Best.SurfaceNormal = SurfaceHit.ImpactNormal;
			}
		}

		Site->SetActorLocation(Best.Location, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogIslandMovementProbe, Log,
			TEXT("Transient East-spruce branch scan: actor=%s mesh=%s, distance=%.0f cm, bounds=%s, queryCollision=%d blocksVisibility=%d, %d complex visibility samples, %d hit the spruce, %d upward supported branch points."),
			*TreeActor->GetActorNameOrLabel(), *TreeComponent->GetStaticMesh()->GetPathName(), FMath::Sqrt(BestTreeDistance),
			*TreeBounds.GetExtent().ToCompactString(), bQueryCollision, bVisibilityBlocks,
			SurfaceTraceCount, TreeSurfaceHitCount, SupportedBranchCount);
		if (SupportedBranchCount == 0)
		{
			UE_LOG(LogIslandMovementProbe, Warning,
				TEXT("No queryable upward-facing spruce branch surface passed the Raven support test; no map or Raven state changed."));
			Site->Destroy();
			return;
		}
		UE_LOG(LogIslandMovementProbe, Log,
			TEXT("Best supported branch point=%s normal=%s cover=%d/5, Raven distance=%.0f cm; assessment=%s"),
			*Best.Location.ToCompactString(), *Best.SurfaceNormal.ToCompactString(), Best.CoverCount,
			FMath::Sqrt(Best.RavenDistance), *RavenController->AssessRoostSite(Site));

		const bool bRequestLanding = Args.ContainsByPredicate([](const FString& Arg)
			{ return Arg.Equals(TEXT("Land"), ESearchCase::IgnoreCase); });
		if (!bRequestLanding || Best.CoverCount < 3)
		{
			if (bRequestLanding && Best.CoverCount < 3)
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Branch landing not attempted: the best real mesh surface did not meet the 3/5 overhead-cover screen."));
			Site->Destroy();
			return;
		}
		if (RavenController->LocomotionState != ERavenLocomotionState::Perched || RavenController->IsActionInProgress())
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Branch landing not attempted: live Raven must already be settled with no action in progress."));
			Site->Destroy();
			return;
		}
		Site->SetActorLocation(Best.Location, false, nullptr, ETeleportType::TeleportPhysics);
		const FName CandidateTag(TEXT("Codex_RavenBranchCandidate"));
		Site->Tags.Add(CandidateTag);
		if (!RavenController->RequestPerch(CandidateTag))
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Raven rejected the transient supported branch request."));
			Site->Destroy();
			return;
		}
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Raven accepted the transient branch perch; the normal controller tick is handling landing."));
	}

	static void Run(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogIslandMovementProbe, Error, TEXT("Island.MoveProbe requires a running game world."));
			return;
		}

		const TSharedRef<FProbeState> State = MakeShared<FProbeState>();
		State->World = World;
		State->QueuedAt = World->GetTimeSeconds();
		TArray<FString> PositionalArgs;
		for (const FString& Arg : Args)
		{
			double OptionValue = 0.0;
			if (Arg.StartsWith(TEXT("DelaySeconds="), ESearchCase::IgnoreCase))
			{
				if (!LexTryParseString(OptionValue, *Arg.RightChop(13)) || OptionValue < 0.0 || OptionValue > 30.0)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("DelaySeconds must be a number from 0 to 30."));
					return;
				}
				State->DispatchDelaySeconds = OptionValue;
			}
			else if (Arg.StartsWith(TEXT("HoldSeconds="), ESearchCase::IgnoreCase))
			{
				if (!LexTryParseString(OptionValue, *Arg.RightChop(12)) || OptionValue < 0.0 || OptionValue > 20.0)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("HoldSeconds must be a number from 0 to 20."));
					return;
				}
				State->PostCompletionHoldSeconds = OptionValue;
			}
			else
			{
				PositionalArgs.Add(Arg);
			}
		}
		State->MoverTag = FName(PositionalArgs.IsEmpty() ? TEXT("IslandInnkeeper") : *PositionalArgs[0]);
		State->bApproachProbe = PositionalArgs.Num() > 1 && PositionalArgs[1].Equals(TEXT("Approach"), ESearchCase::IgnoreCase);
		State->bWanderProbe = PositionalArgs.Num() > 1 && PositionalArgs[1].Equals(TEXT("Wander"), ESearchCase::IgnoreCase);
		State->bForceCuriosityProbe = State->bWanderProbe && PositionalArgs.Num() > 2 && PositionalArgs[2].Equals(TEXT("Curious"), ESearchCase::IgnoreCase);
		State->bInteractAfterMove = !State->bWanderProbe && !State->bApproachProbe && PositionalArgs.Num() > 2 && PositionalArgs[2].Equals(TEXT("Interact"), ESearchCase::IgnoreCase);
		State->TargetTag = State->bApproachProbe
			? FName(PositionalArgs.Num() > 2 ? *PositionalArgs[2] : TEXT("ApproachAgent_Raven_01"))
			: FName(PositionalArgs.Num() > 1 ? *PositionalArgs[1] : TEXT("InnDoorLantern"));
		State->PerchTag = State->bApproachProbe ? FName(PositionalArgs.Num() > 3 ? *PositionalArgs[3] : TEXT("Roost_East")) : NAME_None;
		const int32 StartOverrideIndex = State->bApproachProbe ? 4 : (State->bWanderProbe ? (State->bForceCuriosityProbe ? 3 : 2) : (State->bInteractAfterMove ? 3 : INDEX_NONE));
		if (StartOverrideIndex != INDEX_NONE && PositionalArgs.Num() > StartOverrideIndex)
		{
			if (PositionalArgs.Num() <= StartOverrideIndex + 2)
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Start override needs all three numeric world coordinates."));
				return;
			}
			double StartX = 0.0;
			double StartY = 0.0;
			double StartZ = 0.0;
			if (!LexTryParseString(StartX, *PositionalArgs[StartOverrideIndex]) ||
				!LexTryParseString(StartY, *PositionalArgs[StartOverrideIndex + 1]) ||
				!LexTryParseString(StartZ, *PositionalArgs[StartOverrideIndex + 2]))
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Start override needs three numeric world coordinates."));
				return;
			}
			State->bHasMoverStartOverride = true;
			State->MoverStartOverride = FVector(StartX, StartY, StartZ);
		}
		World->GetTimerManager().SetTimer(State->PollTimer,
			FTimerDelegate::CreateLambda([State]() { Poll(State); }), State->bForceCuriosityProbe ? 0.1f : 0.5f, true);
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Queued isolated %s%s probe for %s; camera warm-up %.1f s, post-probe hold %.1f s; waiting for runtime actors."),
			State->bForceCuriosityProbe ? TEXT("forced-curiosity ") : (State->bWanderProbe ? TEXT("wander ") : TEXT("")),
			State->bWanderProbe ? TEXT("flight-wander") : TEXT("movement"), *State->MoverTag.ToString(),
			State->DispatchDelaySeconds, State->PostCompletionHoldSeconds);
	}

private:
	struct FProbeState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AAutonomousAgentAIController> Controller;
		TWeakObjectPtr<APawn> Pawn;
		FName MoverTag;
		FName TargetTag;
		FName PerchTag;
		FTimerHandle PollTimer;
		FTimerHandle ExitTimer;
		FVector StartLocation = FVector::ZeroVector;
		double StartedAt = 0.0;
		double QueuedAt = 0.0;
		double DispatchDelaySeconds = 0.0;
		double PostCompletionHoldSeconds = 0.0;
		double PerchRequestedAt = 0.0;
		double GroundingStartedAt = 0.0;
		int32 StartupPolls = 0;
		bool bMoveStarted = false;
		bool bInteractAfterMove = false;
		bool bWaitingForGround = false;
		bool bWanderProbe = false;
		bool bForceCuriosityProbe = false;
		bool bApproachProbe = false;
		bool bPerchRequested = false;
		bool bHasMoverStartOverride = false;
		FVector MoverStartOverride = FVector::ZeroVector;
		TWeakObjectPtr<AActor> TargetActor;
		bool bHasMinnowBaseline = false;
		bool bLoggedMinnowBandEntry = false;
		FVector MinnowBaselineCentroid = FVector::ZeroVector;
		int32 MinnowBodyCount = 0;
		int32 MinnowLowFlybySampleCount = 0;
		float MaxMinnowCentroidShiftInBand = 0.f;
		bool bLoggedMinnowStartleCue = false;
		double MinnowStartleObservedAt = 0.0;
		FVector MinnowStartleBaselineCentroid = FVector::ZeroVector;
		int32 MinnowPostStartleSampleCount = 0;
		float MaxMinnowCentroidShiftAfterStartle = 0.f;
		bool bRequestedMinnowStartleScreenshot = false;
		FTimerHandle MinnowScreenshotTimer;
		FString MinnowStartleScreenshotPath;
	};

	static bool SampleMinnowSchool(UWorld* World, FVector& OutSchoolLocation, FVector& OutBodyCentroid, int32& OutBodyCount)
	{
		if (!World) return false;
		AActor* School = nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("MinnowSchool")))
			{
				School = *It;
				break;
			}
		}
		if (!School) return false;

		TArray<UActorComponent*> Components;
		School->GetComponents(Components);
		FVector BodyLocationSum = FVector::ZeroVector;
		OutBodyCount = 0;
		for (UActorComponent* Component : Components)
		{
			const USceneComponent* Scene = Cast<USceneComponent>(Component);
			if (!Scene || !Scene->GetName().StartsWith(TEXT("Minnow_"), ESearchCase::CaseSensitive)) continue;
			BodyLocationSum += Scene->GetRelativeLocation();
			++OutBodyCount;
		}
		if (OutBodyCount == 0) return false;

		OutSchoolLocation = School->GetActorLocation();
		OutBodyCentroid = BodyLocationSum / static_cast<float>(OutBodyCount);
		return true;
	}

	static void SampleRavenMinnowProximity(const TSharedRef<FProbeState>& State, UWorld* World,
		const ARavenAgentAIController* RavenController, const APawn* Raven)
	{
		if (!State->bForceCuriosityProbe || !RavenController || !Raven || !State->bHasMinnowBaseline) return;

		FVector SchoolLocation = FVector::ZeroVector;
		FVector BodyCentroid = FVector::ZeroVector;
		int32 BodyCount = 0;
		if (!SampleMinnowSchool(World, SchoolLocation, BodyCentroid, BodyCount)) return;

		const FVector Offset = Raven->GetActorLocation() - SchoolLocation;
		const float HorizontalDistance = Offset.Size2D();
		const bool bLowFlybyEnvelope = RavenController->LocomotionState == ERavenLocomotionState::Flying &&
			Offset.Z >= 150.f && Offset.Z <= 700.f && HorizontalDistance <= 550.f;
		if (bLowFlybyEnvelope)
		{
			++State->MinnowLowFlybySampleCount;
			const float CentroidShift = FVector::Dist(BodyCentroid, State->MinnowBaselineCentroid);
			State->MaxMinnowCentroidShiftInBand = FMath::Max(State->MaxMinnowCentroidShiftInBand, CentroidShift);
			if (!State->bLoggedMinnowBandEntry)
			{
				State->bLoggedMinnowBandEntry = true;
				UE_LOG(LogIslandMovementProbe, Log,
					TEXT("Raven entered the minnow low-flyby envelope: horizontal %.0f cm, vertical %.0f cm, school bodies %d, centroid shift %.0f cm."),
					HorizontalDistance, Offset.Z, BodyCount, CentroidShift);
			}
		}

		if (!State->bLoggedMinnowStartleCue)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!It->ActorHasTag(TEXT("MinnowStartleImpact"))) continue;
				State->bLoggedMinnowStartleCue = true;
				State->MinnowStartleObservedAt = World->GetTimeSeconds();
				State->MinnowStartleBaselineCentroid = BodyCentroid;
				UE_LOG(LogIslandMovementProbe, Log,
					TEXT("Captured post-startle minnow baseline: %d fish bodies at relative centroid %s; tracking their full 2.4-second response."),
					BodyCount, *BodyCentroid.ToCompactString());
				UE_LOG(LogIslandMovementProbe, Log,
					TEXT("Observed the live minnow startle cue at %s during Raven low-flyby sample %d."),
					*It->GetActorLocation().ToCompactString(), State->MinnowLowFlybySampleCount);
				if (!State->bRequestedMinnowStartleScreenshot && FParse::Param(FCommandLine::Get(), TEXT("SpectatorShots")))
				{
					FString ScreenshotDirectory;
					if (FParse::Value(FCommandLine::Get(), TEXT("SpectatorScreenshotDir="), ScreenshotDirectory) && !ScreenshotDirectory.IsEmpty())
					{
						IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
						State->MinnowStartleScreenshotPath = FPaths::Combine(ScreenshotDirectory, TEXT("000_MinnowStartleMidPulse.png"));
						// The ring deliberately grows from a quiet first frame. Capture near
						// its mid-pulse so the probe judges the cue at its intended visibility.
						World->GetTimerManager().SetTimer(State->MinnowScreenshotTimer,
							FTimerDelegate::CreateLambda([State]()
							{
								FScreenshotRequest::RequestScreenshot(State->MinnowStartleScreenshotPath, true, false);
								UE_LOG(LogIslandMovementProbe, Log, TEXT("Requested mid-pulse startle screenshot at %s."), *State->MinnowStartleScreenshotPath);
							}), 0.45f, false);
						State->bRequestedMinnowStartleScreenshot = true;
						UE_LOG(LogIslandMovementProbe, Log, TEXT("Scheduled mid-pulse startle screenshot at %s."), *State->MinnowStartleScreenshotPath);
					}
				}
				break;
			}
		}

		if (State->bLoggedMinnowStartleCue && World->GetTimeSeconds() - State->MinnowStartleObservedAt <= 2.5)
		{
			++State->MinnowPostStartleSampleCount;
			const float PostStartleShift = FVector::Dist(BodyCentroid, State->MinnowStartleBaselineCentroid);
			State->MaxMinnowCentroidShiftAfterStartle = FMath::Max(State->MaxMinnowCentroidShiftAfterStartle, PostStartleShift);
		}
	}

	static void LogMinnowProbeSummary(const TSharedRef<FProbeState>& State)
	{
		if (!State->bHasMinnowBaseline) return;
		UE_LOG(LogIslandMovementProbe, Log,
			TEXT("Raven/minnow probe summary: %d low-flyby samples; maximum fish-body centroid shift inside the envelope %.0f cm from the pre-action baseline; %d post-startle samples and %.0f cm maximum shift from the cue-time baseline."),
			State->MinnowLowFlybySampleCount, State->MaxMinnowCentroidShiftInBand,
			State->MinnowPostStartleSampleCount, State->MaxMinnowCentroidShiftAfterStartle);
	}

	static void Poll(const TSharedRef<FProbeState>& State)
	{
		UWorld* World = State->World.Get();
		if (!World) return;

		if (!State->bMoveStarted)
		{
			if (World->GetTimeSeconds() - State->QueuedAt < State->DispatchDelaySeconds) return;

			AActor* Mover = nullptr;
			AActor* Target = nullptr;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (!Mover && It->ActorHasTag(State->MoverTag)) Mover = *It;
				if (!Mover)
				{
					const AAutonomousAgentCharacter* Resident = Cast<AAutonomousAgentCharacter>(*It);
					if (Resident && Resident->Memory &&
						Resident->Memory->GetResolvedAgentId().Equals(State->MoverTag.ToString(), ESearchCase::IgnoreCase))
					{
						Mover = *It;
					}
				}
				if (!State->bWanderProbe && !Target && It->ActorHasTag(State->TargetTag)) Target = *It;
			}
			APawn* Pawn = Cast<APawn>(Mover);
			AAutonomousAgentAIController* Controller = Pawn ? Cast<AAutonomousAgentAIController>(Pawn->GetController()) : nullptr;
			if (!Pawn || !Controller || (!State->bWanderProbe && !Target))
			{
				if (++State->StartupPolls <= 40) return;
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Timed out finding mover/controller/target (mover=%s controller=%s target=%s)."),
					Pawn ? TEXT("yes") : TEXT("no"), Controller ? TEXT("yes") : TEXT("no"),
					(State->bWanderProbe || Target) ? TEXT("yes") : TEXT("no"));
				Finish(State, false);
				return;
			}
			if (State->bWanderProbe && !Cast<ARavenAgentAIController>(Controller))
			{
				const ACharacter* Character = Cast<ACharacter>(Pawn);
				const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
				if (Movement && !Movement->IsMovingOnGround())
				{
					if (!State->bWaitingForGround)
					{
						State->bWaitingForGround = true;
						State->GroundingStartedAt = World->GetTimeSeconds();
						UE_LOG(LogIslandMovementProbe, Log,
							TEXT("Wander probe is waiting for the grounded resident to land (location=%s mode=%d)."),
							*Pawn->GetActorLocation().ToCompactString(), static_cast<int32>(Movement->MovementMode));
					}
					if (World->GetTimeSeconds() - State->GroundingStartedAt < 12.0) return;
					UE_LOG(LogIslandMovementProbe, Warning,
						TEXT("Wander probe timed out before its grounded resident landed (location=%s mode=%d)."),
						*Pawn->GetActorLocation().ToCompactString(), static_cast<int32>(Movement->MovementMode));
					Finish(State, false);
					return;
				}
				if (State->bWaitingForGround)
				{
					UE_LOG(LogIslandMovementProbe, Log, TEXT("Grounded resident landed at %s; issuing wander now."),
						*Pawn->GetActorLocation().ToCompactString());
				}
			}
			if (State->bApproachProbe)
			{
				const AAutonomousAgentCharacter* TargetResident = Cast<AAutonomousAgentCharacter>(Target);
				ARavenAgentAIController* RavenController = TargetResident
					? Cast<ARavenAgentAIController>(TargetResident->GetController()) : nullptr;
				if (!RavenController)
				{
					UE_LOG(LogIslandMovementProbe, Error,
						TEXT("Approach probe target %s is not an embodied raven with a raven controller."), *State->TargetTag.ToString());
					Finish(State, false);
					return;
				}
				if (RavenController->LocomotionState != ERavenLocomotionState::Perched)
				{
					if (!State->bPerchRequested)
					{
						State->PerchRequestedAt = World->GetTimeSeconds();
						State->bPerchRequested = RavenController->RequestPerch(State->PerchTag);
						if (!State->bPerchRequested)
						{
							UE_LOG(LogIslandMovementProbe, Error,
								TEXT("Approach probe could not start the raven's perch request for %s."), *State->PerchTag.ToString());
							Finish(State, false);
							return;
						}
						UE_LOG(LogIslandMovementProbe, Log,
							TEXT("Approach probe is preparing the raven at %s before issuing resident movement."), *State->PerchTag.ToString());
					}
					else if (World->GetTimeSeconds() - State->PerchRequestedAt >= 45.0)
					{
						UE_LOG(LogIslandMovementProbe, Warning,
							TEXT("Approach probe timed out waiting for the raven to perch at %s."), *State->PerchTag.ToString());
						Finish(State, false);
					}
					return;
				}
			}
			if (State->bHasMoverStartOverride &&
				!Pawn->SetActorLocation(State->MoverStartOverride, false, nullptr, ETeleportType::TeleportPhysics))
			{
				UE_LOG(LogIslandMovementProbe, Error, TEXT("Could not place the mover at the requested diagnostic start %s."),
					*State->MoverStartOverride.ToCompactString());
				Finish(State, false);
				return;
			}

			State->Pawn = Pawn;
			State->Controller = Controller;
			State->TargetActor = Target;
			State->StartLocation = Pawn->GetActorLocation();
			State->StartedAt = World->GetTimeSeconds();
			if (State->bForceCuriosityProbe)
			{
				ARavenAgentAIController* RavenController = Cast<ARavenAgentAIController>(Controller);
				if (!RavenController)
				{
					UE_LOG(LogIslandMovementProbe, Error, TEXT("Curiosity flight probe requires an embodied raven controller."));
					Finish(State, false);
					return;
				}
				FVector SchoolLocation = FVector::ZeroVector;
				if (SampleMinnowSchool(World, SchoolLocation, State->MinnowBaselineCentroid, State->MinnowBodyCount))
				{
					State->bHasMinnowBaseline = true;
					UE_LOG(LogIslandMovementProbe, Log,
						TEXT("Captured pre-flight minnow baseline: %d fish bodies at school %s (relative centroid %s)."),
						State->MinnowBodyCount, *SchoolLocation.ToCompactString(), *State->MinnowBaselineCentroid.ToCompactString());
				}
				RavenController->BeginTakeoff(RavenController->MakeCruiseTarget(true));
			}
			else
			{
				FAgentDecision Decision;
				Decision.bValid = true;
				Decision.ActionType = State->bWanderProbe ? EAgentActionType::Wander : EAgentActionType::MoveTo;
				if (!State->bWanderProbe) Decision.ActionTarget = State->TargetTag.ToString();
				Controller->ActOnDecision(Decision);
			}
			State->bMoveStarted = true;
			if (Target)
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Move issued from %s toward %s (%s)."),
					*State->StartLocation.ToCompactString(), *Target->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			else
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("%s issued from %s (%s)."),
					State->bForceCuriosityProbe ? TEXT("Forced-curiosity flight-wander") : TEXT("Wander"),
					*State->StartLocation.ToCompactString(), *Controller->DescribeActionState());
			}
			return;
		}

		AAutonomousAgentAIController* Controller = State->Controller.Get();
		APawn* Pawn = State->Pawn.Get();
		if (!Controller || !Pawn)
		{
			Finish(State, false);
			return;
		}
		const ARavenAgentAIController* RavenController = Cast<ARavenAgentAIController>(Controller);
		SampleRavenMinnowProximity(State, World, RavenController, Pawn);
		const FString ActionState = Controller->DescribeActionState();
		const bool bComplete = RavenController
			? !Controller->IsActionInProgress() &&
				(ActionState.Contains(TEXT("Reached the flight destination")) ||
				 ActionState.Contains(TEXT("short ground hop")) ||
				 ActionState.Contains(TEXT("Landed and perched on solid support")) ||
				 ActionState.Contains(TEXT("Already perched at this site")))
			: Controller->GetMoveStatus() != EPathFollowingStatus::Moving &&
				ActionState.Contains(TEXT("Reached the requested destination"));
		// The Raven may need to cross a large portion of the Island to reach a
		// shore marker. Keep the probe bounded, but allow a real long-distance
		// flight to complete before reporting it as stuck.
		constexpr double MovementTimeoutSeconds = 210.0;
		const bool bTimedOut = World->GetTimeSeconds() - State->StartedAt >= MovementTimeoutSeconds;
		const bool bMovementStopped = RavenController ? !Controller->IsActionInProgress()
			: Controller->GetMoveStatus() != EPathFollowingStatus::Moving;
		if (bComplete || bTimedOut || bMovementStopped)
		{
			if (bComplete && State->bLoggedMinnowStartleCue &&
				World->GetTimeSeconds() - State->MinnowStartleObservedAt < 2.5)
			{
				return;
			}
			const float Distance = FVector::Dist2D(State->StartLocation, Pawn->GetActorLocation());
			if (bComplete)
			{
				UE_LOG(LogIslandMovementProbe, Log, TEXT("Probe completed: %.0f cm in %.1f simulated seconds; final location %s; %s"),
					Distance, World->GetTimeSeconds() - State->StartedAt, *Pawn->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
				if (State->bInteractAfterMove)
				{
					if (!State->TargetActor.IsValid())
					{
						UE_LOG(LogIslandMovementProbe, Error, TEXT("Movement arrived, but the interaction target no longer exists."));
						Finish(State, false);
						return;
					}
					FAgentDecision Decision;
					Decision.bValid = true;
					Decision.ActionType = EAgentActionType::Interact;
					Decision.ActionTarget = State->TargetTag.ToString();
					Controller->ActOnDecision(Decision);
					const FString InteractionResult = Controller->DescribeActionState();
					UE_LOG(LogIslandMovementProbe, Log, TEXT("Issued the resident's normal interact action for %s after arrival: %s"),
						*State->TargetTag.ToString(), *InteractionResult);
					const FString ExpectedInteractionPrefix = State->TargetTag.ToString() + TEXT(":");
					if (!InteractionResult.Contains(ExpectedInteractionPrefix, ESearchCase::IgnoreCase))
					{
						UE_LOG(LogIslandMovementProbe, Error, TEXT("Movement arrived, but the normal interaction did not turn over %s."),
							*State->TargetTag.ToString());
						Finish(State, false);
						return;
					}
				}
			}
			else
			{
				UE_LOG(LogIslandMovementProbe, Warning, TEXT("Probe did not complete within 210 simulated seconds: %.0f cm in %.1f simulated seconds; final location %s; %s"),
					Distance, World->GetTimeSeconds() - State->StartedAt, *Pawn->GetActorLocation().ToCompactString(), *Controller->DescribeActionState());
			}
			Finish(State, bComplete);
		}
	}

	static void Finish(const TSharedRef<FProbeState>& State, bool bSuccess)
	{
		UWorld* World = State->World.Get();
		if (World) World->GetTimerManager().ClearTimer(State->PollTimer);
		LogMinnowProbeSummary(State);
		UE_LOG(LogIslandMovementProbe, Log, TEXT("Isolated movement probe finished: %s."), bSuccess ? TEXT("success") : TEXT("failure"));
		if (bSuccess && State->PostCompletionHoldSeconds > 0.0 && World)
		{
			UE_LOG(LogIslandMovementProbe, Log, TEXT("Holding the runtime view for %.1f seconds before the bounded diagnostic exit."), State->PostCompletionHoldSeconds);
			World->GetTimerManager().SetTimer(State->ExitTimer,
				FTimerDelegate::CreateLambda([]() { FPlatformMisc::RequestExit(false); }),
				static_cast<float>(State->PostCompletionHoldSeconds), false);
			return;
		}
		FPlatformMisc::RequestExit(false);
	}
};

static FAutoConsoleCommandWithWorldAndArgs GIslandMovementProbeCommand(
	TEXT("Island.MoveProbe"),
	TEXT("Safely probes a runtime resident move, wander, or approach with agent thinking disabled. Add DelaySeconds=0..30 for camera warm-up and HoldSeconds=0..20 for post-success observation. Usage: Island.MoveProbe [mover-tag] [target-tag [Interact] [optional-start-x start-y start-z]|Wander [Curious] [optional-start-x start-y start-z]]; for a perched-raven approach: Island.MoveProbe [mover-tag] Approach [raven-approach-tag] [roost-tag] [optional-start-x start-y start-z]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FIslandMovementProbeCommand::Run));

static FAutoConsoleCommandWithWorldAndArgs GRavenShelterAuditCommand(
	TEXT("Island.RavenShelterAudit"),
	TEXT("Read-only scan of runtime supported Raven sites near Roost_East; optional RadiusCm=1000..30000 and GridSpacingCm=200..1500; append Land to attempt a transient, bounded perch landing only when overhead cover is at least 3/5."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FIslandMovementProbeCommand::RunRavenShelterAudit));

static FAutoConsoleCommandWithWorldAndArgs GRavenBranchAuditCommand(
	TEXT("Island.RavenBranchAudit"),
	TEXT("Read-only scan of the authored spruce_half_01 mesh nearest Roost_East for real upward-facing perch surfaces; append Land to request the best transient site only if it also reaches 3/5 overhead-cover clues."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FIslandMovementProbeCommand::RunRavenBranchAudit));
