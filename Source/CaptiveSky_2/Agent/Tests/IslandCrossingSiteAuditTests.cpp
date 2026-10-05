#include "Misc/AutomationTest.h"
#include "Agent/IslandTideglassSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
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
	if (!Navigation || !Navigation->GetDefaultNavDataInstance())
	{
		AddInfo(TEXT("The saved Island has no ready nav data; crossing-site suitability remains unknown."));
		return true;
	}

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

	// Tideglass is not a route-shortening site, so screen a small, deterministic set of
	// opposite-bank corridors around the other authored landmarks. This only reports
	// candidates; it does not create markers, alter navigation, or bless a construction site.
	struct FCrossingCandidate
	{
		float BypassDistance = 0.f;
		FString Description;
	};
	TArray<FCrossingCandidate> Candidates;
	const FVector RadialAxes[] = { FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f) };
	const TCHAR* RadialAxisNames[] = { TEXT("X"), TEXT("Y") };
	const float ProbeRadii[] = { 1500.f, 3000.f, 4500.f };
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

				const FString Obstacle = Hit.GetActor() ? Hit.GetActor()->GetName() : TEXT("unknown component");
				Candidates.Add({ BypassDistance, FString::Printf(
					TEXT("%s %s-axis radius %.0f cm: banks %s / %s; direct chord %.1f m, current nav route %.1f m (%.2fx), trace hits %s."),
					*LandmarkId, RadialAxisNames[AxisIndex], Radius, *BankA.Location.ToString(), *BankB.Location.ToString(),
					ChordLength / 100.f, PathLength / 100.f, PathLength / ChordLength, *Obstacle) });
			}
		}
	}

	Candidates.Sort([](const FCrossingCandidate& A, const FCrossingCandidate& B)
	{
		return A.BypassDistance > B.BypassDistance;
	});
	if (Candidates.IsEmpty())
	{
		AddInfo(TEXT("No landmark-centred corridor passed the conservative screen (complete nav route, >1.2x detour, >5 m bypass, blocked direct visibility). This bounded screen does not prove there is no suitable crossing elsewhere."));
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
