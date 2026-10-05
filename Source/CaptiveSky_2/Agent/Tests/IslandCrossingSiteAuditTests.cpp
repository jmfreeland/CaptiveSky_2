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

	AddInfo(bAnyCompleteRoute
		? TEXT("Audit only records the existing route and a straight trace. A crossing is viable only if a completed span later proves a shorter route and physical traversal.")
		: TEXT("No complete opposite-bank route was found; this pool is not yet a validated resident-project site."));
	return true;
}
