#include "Misc/AutomationTest.h"
#include "Agent/IslandTideglassSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "NavigationSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCrossingSiteAuditTest, "CaptiveSky2.Agent.CrossingSiteAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandCrossingSiteAuditTest::RunTest(const FString& Parameters)
{
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Candidate = Context.World();
		if (Context.WorldType == EWorldType::Editor && Candidate && Candidate->GetMapName() == TEXT("Island"))
		{
			Island = Candidate;
			break;
		}
	}
	if (!Island)
	{
		AddInfo(TEXT("Open the saved Island in the editor to audit Tideglass as a possible crossing site."));
		return true;
	}

	UStaticMeshComponent* Pool = UIslandTideglassSubsystem::FindPoolSurface(Island);
	if (!TestNotNull(TEXT("The saved Island has its Tideglass blockout footprint"), Pool)) return false;

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Island);
	ANavigationData* NavData = Navigation ? Navigation->GetDefaultNavDataInstance() : nullptr;
	if (!Navigation || !NavData)
	{
		AddInfo(TEXT("The saved Island has no ready nav data; crossing-site suitability remains unknown."));
		return true;
	}
	const FBox NavBounds = NavData->GetBounds();
	AddInfo(FString::Printf(TEXT("Saved Island nav bounds: %s to %s (%.1f m x %.1f m)."),
		*NavBounds.Min.ToString(), *NavBounds.Max.ToString(), NavBounds.GetSize().X / 100.f, NavBounds.GetSize().Y / 100.f));

	const FVector Center = Pool->Bounds.Origin;
	const FVector Extent = Pool->Bounds.BoxExtent;
	const FVector Axes[] = { FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f) };
	const TCHAR* AxisNames[] = { TEXT("X"), TEXT("Y") };
	bool bAnyCompleteRoute = false;
	for (int32 AxisIndex = 0; AxisIndex < UE_ARRAY_COUNT(Axes); ++AxisIndex)
	{
		const FVector Axis = Axes[AxisIndex];
		const float HalfSpan = AxisIndex == 0 ? Extent.X : Extent.Y;
		const FVector DesiredA = Center - Axis * (HalfSpan + 250.f);
		const FVector DesiredB = Center + Axis * (HalfSpan + 250.f);
		FNavLocation BankA, BankB;
		const FVector ProjectionExtent(400.f, 400.f, 1000.f);
		if (!Navigation->ProjectPointToNavigation(DesiredA, BankA, ProjectionExtent) ||
			!Navigation->ProjectPointToNavigation(DesiredB, BankB, ProjectionExtent))
		{
			AddInfo(FString::Printf(TEXT("Tideglass %s-axis candidate: one or both banks do not project to navmesh."), AxisNames[AxisIndex]));
			continue;
		}

		const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(Island, BankA.Location, BankB.Location);
		const bool bComplete = Path && Path->IsValid() && !Path->IsPartial();
		const float ChordLength = FVector::Dist2D(BankA.Location, BankB.Location);
		const float PathLength = bComplete ? Path->GetPathLength() : 0.f;
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandCrossingSiteAudit), true);
		const FVector TraceA(BankA.Location.X, BankA.Location.Y, Center.Z);
		const FVector TraceB(BankB.Location.X, BankB.Location.Y, Center.Z);
		const bool bHit = Island->LineTraceSingleByChannel(Hit, TraceA, TraceB, ECC_Visibility, Query);
		const bool bHitPool = bHit && Hit.GetComponent() == Pool;
		const FString HitActorName = bHit && Hit.GetActor() ? Hit.GetActor()->GetName() : FString();
		bAnyCompleteRoute |= bComplete;

		AddInfo(FString::Printf(TEXT("Tideglass %s-axis banks %s / %s; baseline %s (%.1f m chord, %.1f m path, %.2fx); center-height visibility trace %s%s."),
			AxisNames[AxisIndex], *BankA.Location.ToString(), *BankB.Location.ToString(), bComplete ? TEXT("complete") : TEXT("incomplete"),
			ChordLength / 100.f, PathLength / 100.f, ChordLength > 1.f ? PathLength / ChordLength : 0.f,
			bHit ? TEXT("hit ") : TEXT("clear"), bHit ? *HitActorName : TEXT("")));
		AddInfo(FString::Printf(TEXT("Tideglass %s-axis pool obstruction: %s."), AxisNames[AxisIndex], bHitPool ? TEXT("yes") : TEXT("no")));
	}

	// Tideglass is not a route-shortening site, so screen a bounded, deterministic set of
	// opposite-bank corridors around the other authored landmarks. Four orientations catch
	// diagonal obstacles that an X/Y-only pass misses; short radii favor walkable crossings.
	// This only reports candidates; it does not create markers, alter navigation, or bless a construction site.
	struct FCrossingCandidate
	{
		float BypassDistance = 0.f;
		FString Description;
	};
	TArray<FCrossingCandidate> Candidates;
	constexpr float Diagonal = 0.70710678f;
	const FVector RadialAxes[] = {
		FVector(1.f, 0.f, 0.f), FVector(Diagonal, Diagonal, 0.f),
		FVector(0.f, 1.f, 0.f), FVector(-Diagonal, Diagonal, 0.f)
	};
	const TCHAR* RadialAxisNames[] = { TEXT("E-W"), TEXT("NE-SW"), TEXT("N-S"), TEXT("NW-SE") };
	const float ProbeRadii[] = { 750.f, 1000.f, 1500.f, 2250.f, 3000.f };
	TArray<FVector> LandmarkCenters;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("IslandLandmark")) || It->ActorHasTag(TEXT("IslandInn")))
			LandmarkCenters.Add(It->GetActorLocation());

	for (TActorIterator<AActor> It(Island); It; ++It)
	{
		AActor* Landmark = *It;
		if (!Landmark->ActorHasTag(TEXT("IslandLandmark")) || Landmark->ActorHasTag(TEXT("TideglassPool"))) continue;

		FString LandmarkId = Landmark->GetName();
		for (const FName Tag : Landmark->Tags)
		{
			if (Tag != FName(TEXT("IslandLandmark")) && Tag != FName(TEXT("IslandLife")))
			{
				LandmarkId = Tag.ToString();
				break;
			}
		}

		for (int32 AxisIndex = 0; AxisIndex < UE_ARRAY_COUNT(RadialAxes); ++AxisIndex)
		{
			for (const float Radius : ProbeRadii)
			{
				const FVector Axis = RadialAxes[AxisIndex];
				FNavLocation BankA, BankB;
				const FVector DesiredA = Landmark->GetActorLocation() - Axis * Radius;
				const FVector DesiredB = Landmark->GetActorLocation() + Axis * Radius;
				const FVector ProjectionExtent(450.f, 450.f, 1200.f);
				if (!Navigation->ProjectPointToNavigation(DesiredA, BankA, ProjectionExtent) ||
					!Navigation->ProjectPointToNavigation(DesiredB, BankB, ProjectionExtent)) continue;

				const float ChordLength = FVector::Dist2D(BankA.Location, BankB.Location);
				if (ChordLength < 1000.f) continue;
				const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(Island, BankA.Location, BankB.Location);
				if (!Path || !Path->IsValid() || Path->IsPartial()) continue;

				const float PathLength = Path->GetPathLength();
				const float BypassDistance = PathLength - ChordLength;
				if (BypassDistance < 500.f || PathLength / ChordLength < 1.2f) continue;

				FHitResult Hit;
				FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandCrossingCandidateAudit), true);
				Query.AddIgnoredActor(Landmark);
				const FVector TraceOffset(0.f, 0.f, 100.f);
				const bool bHit = Island->LineTraceSingleByChannel(Hit, BankA.Location + TraceOffset,
					BankB.Location + TraceOffset, ECC_Visibility, Query);
				if (!bHit) continue;

				const UStaticMeshComponent* ObstacleMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
				AActor* ObstacleActor = Hit.GetActor();
				if (!ObstacleMesh || !ObstacleMesh->CanEverAffectNavigation() ||
					ObstacleMesh->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block || !ObstacleActor ||
					ObstacleActor->ActorHasTag(TEXT("IslandLandmark")) || ObstacleActor->ActorHasTag(TEXT("IslandInn")) ||
					ObstacleActor->ActorHasTag(TEXT("IslandLife")) ||
					FVector::Dist2D(Hit.ImpactPoint, Landmark->GetActorLocation()) < 500.f) continue;

				const FString ObstacleName = ObstacleActor->GetName();
				const FString ObstacleAsset = ObstacleMesh && ObstacleMesh->GetStaticMesh()
					? ObstacleMesh->GetStaticMesh()->GetPathName() : TEXT("no static mesh");
				FString ObstacleLabel = ObstacleName;
				FString ObstacleTags;
				if (ObstacleActor)
				{
#if WITH_EDITOR
					ObstacleLabel = ObstacleActor->GetActorLabel();
#endif
					for (const FName Tag : ObstacleActor->Tags)
					{
						if (!ObstacleTags.IsEmpty()) ObstacleTags += TEXT(",");
						ObstacleTags += Tag.ToString();
					}
				}
				if (ObstacleTags.IsEmpty()) ObstacleTags = TEXT("none");
				const FString ObstacleDetails = FString::Printf(TEXT("%s [%s] (%s; nav-relevant %s; blocks pawn %s; tags %s)"),
					*ObstacleLabel, *ObstacleName, *ObstacleAsset,
					ObstacleMesh && ObstacleMesh->CanEverAffectNavigation() ? TEXT("yes") : TEXT("no"),
					ObstacleMesh && ObstacleMesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block ? TEXT("yes") : TEXT("no"),
					*ObstacleTags);
				Candidates.Add({ BypassDistance, FString::Printf(
					TEXT("%s %s bearing, radius %.0f cm: banks %s / %s; direct chord %.1f m, current nav route %.1f m (%.2fx), trace hits %s at %s."),
					*LandmarkId, RadialAxisNames[AxisIndex], Radius, *BankA.Location.ToString(), *BankB.Location.ToString(),
					ChordLength / 100.f, PathLength / 100.f, PathLength / ChordLength,
					*ObstacleDetails, *Hit.ImpactPoint.ToString()) });
			}
		}
	}

	// Landmark-centred probes alone can miss useful spaces between landmarks. Cover the
	// walkable extent with a 7.5 m grid, retaining the same bank length, route-detour,
	// and obstruction checks. This is a shortlist pass, not an exhaustive geometric proof.
	constexpr float GridMargin = 3000.f;
	constexpr float GridSpacing = 750.f;
	const float GridMinX = NavBounds.Min.X + GridMargin;
	const float GridMaxX = NavBounds.Max.X - GridMargin;
	const float GridMinY = NavBounds.Min.Y + GridMargin;
	const float GridMaxY = NavBounds.Max.Y - GridMargin;
	const float GridZ = NavBounds.GetCenter().Z;
	const FVector GridProjectionExtent(450.f, 450.f, 1500.f);
	for (float X = GridMinX; X <= GridMaxX; X += GridSpacing)
	{
		for (float Y = GridMinY; Y <= GridMaxY; Y += GridSpacing)
		{
			const FVector GridCenter(X, Y, GridZ);
			for (int32 AxisIndex = 0; AxisIndex < UE_ARRAY_COUNT(RadialAxes); ++AxisIndex)
			{
				const FVector Axis = RadialAxes[AxisIndex];
				for (const float Radius : ProbeRadii)
				{
					FNavLocation BankA, BankB;
					if (!Navigation->ProjectPointToNavigation(GridCenter - Axis * Radius, BankA, GridProjectionExtent) ||
						!Navigation->ProjectPointToNavigation(GridCenter + Axis * Radius, BankB, GridProjectionExtent)) continue;
					const float ChordLength = FVector::Dist2D(BankA.Location, BankB.Location);
					if (ChordLength < 1000.f) continue;
					const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(Island, BankA.Location, BankB.Location);
					if (!Path || !Path->IsValid() || Path->IsPartial()) continue;
					const float PathLength = Path->GetPathLength();
					const float BypassDistance = PathLength - ChordLength;
					if (BypassDistance < 500.f || PathLength / ChordLength < 1.2f) continue;

					FHitResult Hit;
					FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandCrossingGridAudit), true);
					const FVector TraceOffset(0.f, 0.f, 100.f);
					if (!Island->LineTraceSingleByChannel(Hit, BankA.Location + TraceOffset,
						BankB.Location + TraceOffset, ECC_Visibility, Query)) continue;
					const UStaticMeshComponent* ObstacleMesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
					AActor* ObstacleActor = Hit.GetActor();
					if (!ObstacleMesh || !ObstacleMesh->CanEverAffectNavigation() ||
						ObstacleMesh->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block || !ObstacleActor ||
						ObstacleActor->ActorHasTag(TEXT("IslandLandmark")) || ObstacleActor->ActorHasTag(TEXT("IslandInn")) ||
						ObstacleActor->ActorHasTag(TEXT("IslandLife"))) continue;
					bool bNearLandmark = false;
					for (const FVector& LandmarkCenter : LandmarkCenters)
						bNearLandmark |= FVector::Dist2D(Hit.ImpactPoint, LandmarkCenter) < 500.f;
					if (bNearLandmark) continue;

					const FString ObstacleName = ObstacleActor->GetName();
					const FString ObstacleAsset = ObstacleMesh->GetStaticMesh()
						? ObstacleMesh->GetStaticMesh()->GetPathName() : TEXT("no static mesh");
					FString ObstacleLabel = ObstacleName;
					FString ObstacleTags;
					for (const FName Tag : ObstacleActor->Tags)
					{
						if (!ObstacleTags.IsEmpty()) ObstacleTags += TEXT(",");
						ObstacleTags += Tag.ToString();
					}
					if (ObstacleTags.IsEmpty()) ObstacleTags = TEXT("none");
#if WITH_EDITOR
					ObstacleLabel = ObstacleActor->GetActorLabel();
#endif
					const FString ObstacleDetails = FString::Printf(TEXT("%s [%s] (%s; nav-relevant yes; blocks pawn yes; tags %s)"),
						*ObstacleLabel, *ObstacleName, *ObstacleAsset, *ObstacleTags);
					Candidates.Add({ BypassDistance, FString::Printf(
						TEXT("Nav grid center X=%.0f Y=%.0f, %s bearing radius %.0f cm: banks %s / %s; direct chord %.1f m, current nav route %.1f m (%.2fx), trace hits %s at %s."),
						X, Y, RadialAxisNames[AxisIndex], Radius, *BankA.Location.ToString(), *BankB.Location.ToString(),
						ChordLength / 100.f, PathLength / 100.f, PathLength / ChordLength,
						*ObstacleDetails, *Hit.ImpactPoint.ToString()) });
				}
			}
		}
	}

	Candidates.Sort([](const FCrossingCandidate& A, const FCrossingCandidate& B)
	{
		return A.BypassDistance > B.BypassDistance;
	});
	if (Candidates.IsEmpty())
	{
		AddInfo(TEXT("No sampled corridor passed the conservative screen (complete nav route, >1.2x detour, >5 m bypass, blocked direct visibility, and a pedestrian-blocking navigation-relevant static mesh outside landmark/building/wildlife space and at least 5 m from a landmark). This bounded screen does not prove there is no suitable crossing elsewhere."));
	}
	else
	{
		const int32 ReportedCount = FMath::Min(5, Candidates.Num());
		AddInfo(FString::Printf(TEXT("Top %d read-only crossing-site candidates (manual water/terrain/footprint review and a later traversal probe are still required):"), ReportedCount));
		for (int32 Index = 0; Index < ReportedCount; ++Index) AddInfo(Candidates[Index].Description);
	}

	AddInfo(bAnyCompleteRoute
		? TEXT("Audit only records the existing route and a straight trace. A crossing is viable only if a completed span later proves a shorter route and physical traversal.")
		: TEXT("No complete opposite-bank route was found; this pool is not yet a validated resident-project site."));
	return true;
}
