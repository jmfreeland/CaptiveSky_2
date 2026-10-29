#include "Misc/AutomationTest.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "IslandTrail.h"
#include "IslandInteractionTestPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTrailTest, "CaptiveSky2.Agent.IslandTrail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTrailTest::RunTest(const FString& Parameters)
{
	using Trail = UIslandTrailSubsystem;
	TestEqual(TEXT("Dry ground leaves no print"), Trail::PrintLifetime(0.f), 0.f);
	TestEqual(TEXT("Barely damp ground leaves none"), Trail::PrintLifetime(Trail::PrintWetness - 0.01f), 0.f);
	TestTrue(TEXT("Wetter ground holds prints longer"), Trail::PrintLifetime(0.3f) < Trail::PrintLifetime(0.9f));
	TestTrue(TEXT("A print lasts at least some seconds"), Trail::PrintLifetime(Trail::PrintWetness) >= 20.f);

	const float Life = 60.f;
	TestEqual(TEXT("A fresh print is full size"), Trail::PrintScale(0.f, Life, 0.8f), 1.f);
	TestEqual(TEXT("It holds until it starts to dry"), Trail::PrintScale(Life * 0.5f, Life, 0.8f), 1.f);
	TestTrue(TEXT("Then it shrinks"), Trail::PrintScale(Life * 0.85f, Life, 0.8f) < 1.f);
	TestEqual(TEXT("And is gone at the end"), Trail::PrintScale(Life, Life, 0.8f), 0.f);
	TestEqual(TEXT("A print dries away with the ground"), Trail::PrintScale(1.f, Life, 0.f), 0.f);
	TestTrue(TEXT("Half-dried ground shrinks it"), Trail::PrintScale(1.f, Life, 0.06f) < 1.f);
	TestFalse(TEXT("An unused hidden print slot needs no transform update"), Trail::ShouldRefreshPrintInstance(false, false, 0.f));
	TestTrue(TEXT("A visible active print refreshes as it shrinks"), Trail::ShouldRefreshPrintInstance(true, true, 0.5f));
	TestTrue(TEXT("A newly visible print receives its first transform"), Trail::ShouldRefreshPrintInstance(true, false, 0.5f));
	TestTrue(TEXT("A print that just dried receives one hide transform"), Trail::ShouldRefreshPrintInstance(true, true, 0.f));
	TestFalse(TEXT("A hidden dried print is not rewritten every tick"), Trail::ShouldRefreshPrintInstance(true, false, 0.f));
	TestTrue(TEXT("An expired visible print receives one hide transform"), Trail::ShouldRefreshPrintInstance(false, true, 0.f));

	TestEqual(TEXT("A few footfalls wear nothing"), Trail::WearAmount(Trail::WearStartSteps), 0.f);
	const int32 FirstMidpoint = Trail::WearStartSteps + (Trail::WearFullSteps - Trail::WearStartSteps) / 3;
	const int32 SecondMidpoint = Trail::WearStartSteps + (Trail::WearFullSteps - Trail::WearStartSteps) * 2 / 3;
	TestTrue(TEXT("Repeated passes wear the same ground progressively"), Trail::WearAmount(FirstMidpoint) < Trail::WearAmount(SecondMidpoint));
	TestEqual(TEXT("A well-used path is fully worn"), Trail::WearAmount(Trail::WearFullSteps), 1.f);
	TestEqual(TEXT("Wear is capped"), Trail::WearAmount(100000), 1.f);
	const FLinearColor Dry = Trail::WearColor(0.f);
	const FLinearColor Wet = Trail::WearColor(1.f);
	TestTrue(TEXT("Wet worn earth is darker"), Wet.R < Dry.R && Wet.G < Dry.G);

	FIslandTrailRegionSummary PathBefore;
	PathBefore.State = EIslandTrailRegionState::Active;
	PathBefore.WornCellCount = Trail::TrailChronicleCellThreshold - 1;
	PathBefore.CurrentWearSteps = Trail::TrailChronicleUseThreshold - 4;
	FIslandTrailRegionSummary PathAfter;
	PathAfter.State = EIslandTrailRegionState::Active;
	PathAfter.WornCellCount = Trail::TrailChronicleCellThreshold;
	PathAfter.CurrentWearSteps = Trail::TrailChronicleUseThreshold;
	TestTrue(TEXT("An active path is chronicled only when repeated wear establishes it"),
		Trail::ShouldChronicleTrailEmergence(PathBefore, PathAfter, 0, 2));
	TestFalse(TEXT("An already active established path is not narrated on every later step"),
		Trail::ShouldChronicleTrailEmergence(PathAfter, PathAfter, 0, 2));
	TestFalse(TEXT("An undated path cannot be chronicled"),
		Trail::ShouldChronicleTrailEmergence(PathBefore, PathAfter, 0, 0));
	TestFalse(TEXT("Daily chronicle path notes remain capped"),
		Trail::ShouldChronicleTrailEmergence(PathBefore, PathAfter, Trail::MaxTrailChronicleEntriesPerDay, 2));
	PathAfter.CurrentWearSteps = Trail::TrailChronicleUseThreshold - 1;
	TestFalse(TEXT("A region without enough accumulated use is not chronicled"),
		Trail::ShouldChronicleTrailEmergence(PathBefore, PathAfter, 0, 2));

	const FIntPoint WearCenter(4, -2);
	auto AddWearCell = [](TMap<FIntPoint, FIslandTrailCell>& Cells, const FIntPoint& Key)
	{
		FIslandTrailCell& Cell = Cells.Add(Key);
		Cell.Steps = Trail::WearFullSteps;
	};
	TMap<FIntPoint, FIslandTrailCell> StraightWear;
	AddWearCell(StraightWear, WearCenter + FIntPoint(-1, 0));
	AddWearCell(StraightWear, WearCenter + FIntPoint(1, 0));
	float StraightAspect = 0.f;
	const FQuat StraightRotation = Trail::CalculateWearRotation(WearCenter, StraightWear, FVector::UpVector, StraightAspect);
	TestTrue(TEXT("Linked worn cells align their long axis with the route"),
		FMath::Abs(FVector::DotProduct(StraightRotation.RotateVector(FVector::ForwardVector), FVector::ForwardVector)) > 0.99f);
	TestTrue(TEXT("A straight path becomes gently elongated"), StraightAspect > 1.3f && StraightAspect <= 1.45f);

	TMap<FIntPoint, FIslandTrailCell> DiagonalWear;
	AddWearCell(DiagonalWear, WearCenter + FIntPoint(-1, -1));
	AddWearCell(DiagonalWear, WearCenter + FIntPoint(1, 1));
	float DiagonalAspect = 0.f;
	const FQuat DiagonalRotation = Trail::CalculateWearRotation(WearCenter, DiagonalWear, FVector::UpVector, DiagonalAspect);
	const FVector DiagonalAxis = FVector(1.f, 1.f, 0.f).GetSafeNormal();
	TestTrue(TEXT("Diagonal worn cells follow their diagonal route"),
		FMath::Abs(FVector::DotProduct(DiagonalRotation.RotateVector(FVector::ForwardVector), DiagonalAxis)) > 0.99f);

	TMap<FIntPoint, FIslandTrailCell> CrossWear;
	AddWearCell(CrossWear, WearCenter + FIntPoint(-1, 0));
	AddWearCell(CrossWear, WearCenter + FIntPoint(1, 0));
	AddWearCell(CrossWear, WearCenter + FIntPoint(0, -1));
	AddWearCell(CrossWear, WearCenter + FIntPoint(0, 1));
	float CrossAspect = 0.f;
	const FQuat CrossRotation = Trail::CalculateWearRotation(WearCenter, CrossWear, FVector::UpVector, CrossAspect);
	float RepeatedCrossAspect = 0.f;
	const FQuat RepeatedCrossRotation = Trail::CalculateWearRotation(WearCenter, CrossWear, FVector::UpVector, RepeatedCrossAspect);
	TestEqual(TEXT("A trail junction remains round instead of claiming one direction"), CrossAspect, 1.f);
	TestTrue(TEXT("Junction fallback rotation stays deterministic"), CrossRotation.Equals(RepeatedCrossRotation));

	FIslandTrailLedger Ledger;
	const FVector Here(1234.f, -560.f, 90.f);
	TestEqual(TEXT("Unwalked ground has no steps"), Ledger.StepsAt(Here), 0);
	for (int32 I = 0; I < 5; ++I) Ledger.AddStep(Here + FVector(I * 4.f, 0.f, 0.f), FVector(0.1f, 0.f, 0.995f));
	TestEqual(TEXT("Footfalls in one cell add up"), Ledger.StepsAt(Here), 5);
	TestEqual(TEXT("A neighbouring cell is separate"), Ledger.StepsAt(Here + FVector(FIslandTrailLedger::CellSize * 2.f, 0.f, 0.f)), 0);
	TestEqual(TEXT("Negative coordinates have their own cells"), FIslandTrailLedger::CellFor(FVector(-1.f, -1.f, 0.f)), FIntPoint(-1, -1));
	TestEqual(TEXT("Negative cells use floor division for stable region assignment"), FIslandTrailLedger::RegionForCell(FIntPoint(-1, -1)), FIntPoint(-1, -1));
	TestEqual(TEXT("The negative region boundary is inclusive"), FIslandTrailLedger::RegionForCell(FIntPoint(-8, -8)), FIntPoint(-1, -1));
	TestEqual(TEXT("Cells beyond that boundary enter the next region"), FIslandTrailLedger::RegionForCell(FIntPoint(-9, -9)), FIntPoint(-2, -2));
	TestEqual(TEXT("Positive region boundary follows the fixed cell span"), FIslandTrailLedger::RegionForCell(FIntPoint(8, 8)), FIntPoint(1, 1));

	FIslandTrailLedger RegionLedger;
	auto AddRegionCell = [](FIslandTrailLedger& TargetLedger, const FIntPoint& Key, int32 Steps, int32 Day)
	{
		FIslandTrailCell& Cell = TargetLedger.Cells.Add(Key);
		Cell.Steps = Steps;
		Cell.LastUseDay = Day;
	};
	AddRegionCell(RegionLedger, FIntPoint(0, 0), 4, 10);
	AddRegionCell(RegionLedger, FIntPoint(1, 0), 8, 19);
	AddRegionCell(RegionLedger, FIntPoint(7, 7), 12, -1);
	AddRegionCell(RegionLedger, FIntPoint(8, 0), 100, 20);
	const FIslandTrailRegionSummary ActiveRegion = RegionLedger.SummarizeRegionForCell(FIntPoint(3, 4), 20);
	TestEqual(TEXT("A region summary has a coordinate-derived stable ID"), ActiveRegion.StableId, FString(TEXT("trail-region:0:0")));
	TestEqual(TEXT("A region counts only worn cells inside its 12 m tile"), ActiveRegion.WornCellCount, 3);
	TestEqual(TEXT("Region use sums the current weather-decayed step weights"), ActiveRegion.CurrentWearSteps, static_cast<int64>(24));
	TestEqual(TEXT("Region summary carries the most recent known use"), ActiveRegion.LastUseDay, 19);
	TestTrue(TEXT("Recent dated use proves an active region despite older unknown cells"), ActiveRegion.State == EIslandTrailRegionState::Active);
	TestEqual(TEXT("Small regions return every contributing cell as a sample"), ActiveRegion.SampleCells.Num(), 3);
	RegionLedger.Cells.FindChecked(FIntPoint(7, 7)).LastUseDay = 14;
	const FIslandTrailRegionSummary RecoveringRegion = RegionLedger.SummarizeRegionForCell(FIntPoint(3, 4), 20);
	TestTrue(TEXT("Fully dated quiet regions are marked recovering"), RecoveringRegion.State == EIslandTrailRegionState::Recovering);
	RegionLedger.Cells.FindChecked(FIntPoint(7, 7)).LastUseDay = -1;
	const FIslandTrailRegionSummary UnknownRegion = RegionLedger.SummarizeRegionForCell(FIntPoint(3, 4), 20);
	TestTrue(TEXT("Undated legacy wear stays explicitly unknown"), UnknownRegion.State == EIslandTrailRegionState::Unknown);

	FIslandTrailLedger DenseRegionLedger;
	for (int32 X = 0; X < FIslandTrailLedger::RegionCellSpan; ++X)
		for (int32 Y = 0; Y < FIslandTrailLedger::RegionCellSpan; ++Y)
			AddRegionCell(DenseRegionLedger, FIntPoint(X, Y), 2, 1);
	const FIslandTrailRegionSummary DenseRegion = DenseRegionLedger.SummarizeRegionForCell(FIntPoint(0, 0), 10);
	const FIslandTrailRegionSummary RepeatedDenseRegion = DenseRegionLedger.SummarizeRegionForCell(FIntPoint(7, 7), 10);
	TestEqual(TEXT("Dense region summaries remain bounded to the tile's 64 cells"), DenseRegion.WornCellCount, 64);
	TestEqual(TEXT("Dense region step weights sum without overflow"), DenseRegion.CurrentWearSteps, static_cast<int64>(128));
	TestEqual(TEXT("A dense summary caps its representative cell keys"), DenseRegion.SampleCells.Num(), FIslandTrailLedger::MaxRegionSamples);
	TestEqual(TEXT("Repeated queries return the same first sample"), DenseRegion.SampleCells[0], RepeatedDenseRegion.SampleCells[0]);
	TestEqual(TEXT("Repeated queries return the same last sample"), DenseRegion.SampleCells.Last(), RepeatedDenseRegion.SampleCells.Last());

	FIslandTrailLedger HorizontalLedger;
	FIslandTrailLedger VerticalLedger;
	FIslandTrailLedger DiagonalLedger;
	FIslandTrailLedger BranchLedger;
	FIslandTrailLedger SparseLedger;
	for (int32 I = 0; I < 4; ++I)
	{
		AddRegionCell(HorizontalLedger, FIntPoint(I, 2), 5 + I, 20);
		AddRegionCell(VerticalLedger, FIntPoint(2, I), 5 + I, 20);
		AddRegionCell(DiagonalLedger, FIntPoint(I, I), 5 + I, 20);
		if (I < 3) AddRegionCell(SparseLedger, FIntPoint(I, 2), 5 + I, 20);
	}
	AddRegionCell(BranchLedger, FIntPoint(3, 3), 10, 20);
	AddRegionCell(BranchLedger, FIntPoint(2, 3), 10, 20);
	AddRegionCell(BranchLedger, FIntPoint(4, 3), 10, 20);
	AddRegionCell(BranchLedger, FIntPoint(3, 2), 10, 20);
	AddRegionCell(BranchLedger, FIntPoint(3, 4), 10, 20);

	const FIslandTrailRegionSummary Horizontal = HorizontalLedger.SummarizeRegionForCell(FIntPoint(0, 0), 20);
	TestTrue(TEXT("Four clear horizontal worn cells reveal a high-confidence axis"), Horizontal.WornAxisLinearity >= 0.99f);
	TestTrue(TEXT("The derived east-west axis has a stable positive sign"), Horizontal.WornAxis.X > 0.99f && FMath::Abs(Horizontal.WornAxis.Y) < 0.01f);
	TestTrue(TEXT("Visitors can read an east-west alignment without claiming a route"),
		UIslandTrailSubsystem::DescribeWornAlignment(Horizontal).Contains(TEXT("east-west line")));

	const FIslandTrailRegionSummary Vertical = VerticalLedger.SummarizeRegionForCell(FIntPoint(0, 0), 20);
	TestTrue(TEXT("A north-south cluster is described with the appropriate compass axis"),
		UIslandTrailSubsystem::DescribeWornAlignment(Vertical).Contains(TEXT("north-south line")));
	const FIslandTrailRegionSummary Diagonal = DiagonalLedger.SummarizeRegionForCell(FIntPoint(0, 0), 20);
	TestTrue(TEXT("A diagonal cluster uses a stable undirected compass label"),
		UIslandTrailSubsystem::DescribeWornAlignment(Diagonal).Contains(TEXT("northeast-southwest line")));
	const FIslandTrailRegionSummary Branch = BranchLedger.SummarizeRegionForCell(FIntPoint(0, 0), 20);
	TestTrue(TEXT("A cross-shaped cluster does not invent one dominant direction"),
		UIslandTrailSubsystem::DescribeWornAlignment(Branch).IsEmpty());
	const FIslandTrailRegionSummary Sparse = SparseLedger.SummarizeRegionForCell(FIntPoint(0, 0), 20);
	TestTrue(TEXT("Three cells are too sparse for an alignment claim"),
		UIslandTrailSubsystem::DescribeWornAlignment(Sparse).IsEmpty());

	FIslandTrailLedger NearbyLedger;
	for (int32 I = 1; I <= 3; ++I) AddRegionCell(NearbyLedger, FIntPoint(I, 1), 12, 20); // Current region: underfoot handles it.
	for (int32 I = 8; I <= 11; ++I) AddRegionCell(NearbyLedger, FIntPoint(I, 7), 12, 20); // Nearby east-west region.
	for (int32 I = 8; I <= 11; ++I) AddRegionCell(NearbyLedger, FIntPoint(I, 15), 12, 20); // Nearby second region.
	for (int32 I = 8; I <= 11; ++I) AddRegionCell(NearbyLedger, FIntPoint(I, 23), 12, 20); // Third region is beyond the two-region cue cap.
	for (int32 I = 40; I <= 43; ++I) AddRegionCell(NearbyLedger, FIntPoint(I, 7), 12, 20); // Farther than the perception radius.
	const int32 NearbyLedgerCellCount = NearbyLedger.Cells.Num();
	const FString NearbyTrailDescription = UIslandTrailSubsystem::DescribeNearbyWornRegions(NearbyLedger, FVector::ZeroVector, 20);
	TestTrue(TEXT("Residents can notice a nearby worn stretch by compass direction"),
		NearbyTrailDescription.Contains(TEXT("to the northeast")));
	TestTrue(TEXT("A nearby path clue carries its weighted shape rather than inventing an endpoint"),
		NearbyTrailDescription.Contains(TEXT("east-west line")));
	TestTrue(TEXT("Trail perception explicitly avoids claiming a route or destination"),
		NearbyTrailDescription.Contains(TEXT("not a verified route or destination")));
	TestFalse(TEXT("The underfoot region is not redundantly repeated as a nearby patch"),
		NearbyTrailDescription.Contains(TEXT("3 patches")));
	int32 NearbyMentionCount = 0;
	int32 SearchFrom = 0;
	while ((SearchFrom = NearbyTrailDescription.Find(TEXT("Nearby,"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom)) != INDEX_NONE)
	{
		++NearbyMentionCount;
		SearchFrom += UE_ARRAY_COUNT(TEXT("Nearby,")) - 1;
	}
	TestEqual(TEXT("Local trail perception returns no more than two additional regions"),
		NearbyMentionCount, UIslandTrailSubsystem::MaxNearbyWornRegions);
	TestEqual(TEXT("Read-only path awareness leaves the saved trail ledger unchanged"), NearbyLedger.Cells.Num(), NearbyLedgerCellCount);
	FIslandTrailLedger FarTrailLedger;
	for (int32 I = 40; I <= 43; ++I) AddRegionCell(FarTrailLedger, FIntPoint(I, 7), 12, 20);
	TestTrue(TEXT("Worn regions beyond the bounded perception radius stay unseen"),
		UIslandTrailSubsystem::DescribeNearbyWornRegions(FarTrailLedger, FVector::ZeroVector, 20).IsEmpty());
	TestTrue(TEXT("No nearby-worn summary is returned for an untouched ledger"),
		UIslandTrailSubsystem::DescribeNearbyWornRegions(FIslandTrailLedger(), FVector::ZeroVector, 20).IsEmpty());
	TestTrue(TEXT("Non-finite observer coordinates do not enter the spatial query"),
		UIslandTrailSubsystem::DescribeNearbyWornRegions(NearbyLedger,
			FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0), 20).IsEmpty());
	TestTrue(TEXT("Out-of-range observer coordinates do not overflow the region search"),
		UIslandTrailSubsystem::DescribeNearbyWornRegions(NearbyLedger,
			FVector(static_cast<double>(TNumericLimits<int32>::Max()) * FIslandTrailLedger::CellSize * 2.0, 0.0, 0.0), 20).IsEmpty());

	AddRegionCell(RegionLedger, FIntPoint(16, 0), 1, 20);
	const TArray<FIslandTrailRegionSummary> WornRegions = RegionLedger.SummarizeWornRegions(20);
	TestEqual(TEXT("A world-level snapshot includes each worn 12 m region once"), WornRegions.Num(), 2);
	if (WornRegions.Num() == 2)
	{
		TestEqual(TEXT("World-level path records use stable coordinate order"), WornRegions[0].RegionKey, FIntPoint(0, 0));
		TestEqual(TEXT("World-level path records retain stable place IDs"), WornRegions[0].StableId, FString(TEXT("trail-region:0:0")));
		TestEqual(TEXT("A region record aggregates worn cells into accumulated use"), WornRegions[0].CurrentWearSteps, static_cast<int64>(24));
		TestEqual(TEXT("A region record preserves its most recent use day"), WornRegions[0].LastUseDay, 19);
		TestEqual(TEXT("The next region follows in coordinate order"), WornRegions[1].RegionKey, FIntPoint(1, 0));
		TestEqual(TEXT("A second region keeps its own stable ID"), WornRegions[1].StableId, FString(TEXT("trail-region:1:0")));
		TestTrue(TEXT("A world-level record carries the derived active/recovering state"),
			WornRegions[0].State == EIslandTrailRegionState::Active && WornRegions[1].State == EIslandTrailRegionState::Active);
		TestEqual(TEXT("A faint single-step patch does not create an extra place record"), WornRegions.Num(), 2);
		TestEqual(TEXT("The second place record keeps its accumulated use distinct"), WornRegions[1].CurrentWearSteps, static_cast<int64>(100));
	}
	const TArray<FIslandTrailRegionSummary> RepeatedWornRegions = RegionLedger.SummarizeWornRegions(20);
	TestEqual(TEXT("Repeated world snapshots remain deterministic"), RepeatedWornRegions.Num(), WornRegions.Num());
	if (WornRegions.Num() == RepeatedWornRegions.Num() && !WornRegions.IsEmpty())
	{
		TestEqual(TEXT("The first path ID remains stable across snapshots"), RepeatedWornRegions[0].StableId, WornRegions[0].StableId);
		TestEqual(TEXT("Representative-cell sample counts remain stable"), RepeatedWornRegions[0].SampleCells.Num(), WornRegions[0].SampleCells.Num());
		for (int32 I = 0; I < WornRegions[0].SampleCells.Num(); ++I)
		{
			TestEqual(TEXT("Representative cells remain stable across snapshots"), RepeatedWornRegions[0].SampleCells[I], WornRegions[0].SampleCells[I]);
		}
	}

	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const float Infinity = std::numeric_limits<float>::infinity();
	TestFalse(TEXT("A non-finite position cannot enter persistent path memory"), Ledger.AddStep(FVector(NaN, 0.f, 0.f), FVector::UpVector));
	TestFalse(TEXT("A non-finite ground normal cannot enter persistent path memory"), Ledger.AddStep(Here, FVector(0.f, Infinity, 0.f)));
	TestFalse(TEXT("An invalid negative island day cannot enter persistent path memory"),
		Ledger.AddStep(Here, FVector::UpVector, -2));
	TestFalse(TEXT("A finite position outside the ledger's cell-key range is rejected"),
		Ledger.AddStep(FVector((static_cast<double>(TNumericLimits<int32>::Max()) + 1.0) * FIslandTrailLedger::CellSize, 0.f, 0.f), FVector::UpVector));
	TestEqual(TEXT("Rejected footfalls leave the existing trail unchanged"), Ledger.StepsAt(Here), 5);

	const FString Json = Ledger.ToJson();
	FIslandTrailLedger Loaded;
	TestTrue(TEXT("The ledger reloads"), Loaded.FromJson(Json));
	TestEqual(TEXT("Steps survive a save"), Loaded.StepsAt(Here), 5);
	const FIslandTrailCell* Cell = Loaded.Cells.Find(FIslandTrailLedger::CellFor(Here));
	if (TestNotNull(TEXT("Cell present"), Cell))
	{
		TestTrue(TEXT("Position survives"), Cell->Position.Equals(Here + FVector(16.f, 0.f, 0.f), 0.01f));
		TestTrue(TEXT("Slope survives"), FMath::IsNearlyEqual(Cell->NormalX, 0.1f, 0.001f));
		TestEqual(TEXT("Undated footfalls remain explicitly unknown"), Cell->LastUseDay, -1);
	}
	FIslandTrailLedger Dated;
	Dated.AddStep(Here, FVector::UpVector, 10);
	Dated.AddStep(Here + FVector(12.f, 0.f, 0.f), FVector::UpVector, 12);
	const FIslandTrailCell* DatedCell = Dated.Cells.Find(FIslandTrailLedger::CellFor(Here));
	if (TestNotNull(TEXT("Dated trail cell present"), DatedCell))
		TestEqual(TEXT("The last-use day advances with each valid footfall"), DatedCell->LastUseDay, 12);
	FIslandTrailLedger DatedReloaded;
	TestTrue(TEXT("A dated trail ledger reloads"), DatedReloaded.FromJson(Dated.ToJson()));
	const FIslandTrailCell* ReloadedDatedCell = DatedReloaded.Cells.Find(FIslandTrailLedger::CellFor(Here));
	if (TestNotNull(TEXT("Dated trail cell survives reload"), ReloadedDatedCell))
		TestEqual(TEXT("Last-use day survives save and load"), ReloadedDatedCell->LastUseDay, 12);
	FIslandTrailLedger Legacy;
	TestTrue(TEXT("Version-one trail ledgers remain readable"), Legacy.FromJson(
		TEXT("{\"version\":1,\"decay_day\":10,\"cells\":[[1,2,5,225,300,0,0,0]]}")));
	const FIslandTrailCell* LegacyCell = Legacy.Cells.Find(FIntPoint(1, 2));
	if (TestNotNull(TEXT("Legacy trail cell loads"), LegacyCell))
		TestEqual(TEXT("A legacy trail's unknown last-use day is preserved as unknown"), LegacyCell->LastUseDay, -1);
	FIslandTrailLedger Garbage;
	Garbage.AddStep(Here, FVector::UpVector);
	TestFalse(TEXT("Unreadable text is refused"), Garbage.FromJson(TEXT("not json")));
	TestFalse(TEXT("A future version is refused"), Garbage.FromJson(TEXT("{\"version\":99,\"cells\":[]}")));
	TestFalse(TEXT("Fractional schema versions are refused"), Garbage.FromJson(TEXT("{\"version\":1.5,\"cells\":[]}")));
	TestFalse(TEXT("A malformed row is refused"), Garbage.FromJson(TEXT("{\"version\":1,\"cells\":[[1,2,3]]}")));
	TestFalse(TEXT("Fractional cell coordinates are refused"), Garbage.FromJson(
		TEXT("{\"version\":1,\"cells\":[[1.5,2,3,225,300,0,0,0]]}")));
	TestFalse(TEXT("Out-of-range world coordinates are refused before cell conversion"), Garbage.FromJson(
		TEXT("{\"version\":1,\"cells\":[[1,2,3,1e100,300,0,0,0]]}")));
	TestFalse(TEXT("Impossible ground normals are refused"), Garbage.FromJson(
		TEXT("{\"version\":1,\"cells\":[[1,2,3,225,300,0,0.8,0.8]]}")));
	TestFalse(TEXT("Fractional last-use days are refused"), Garbage.FromJson(
		TEXT("{\"version\":2,\"cells\":[[1,2,3,4.5,225,300,0,0,0]]}")));
	TestFalse(TEXT("Out-of-range decay days are refused"), Garbage.FromJson(
		TEXT("{\"version\":1,\"decay_day\":2147483648,\"cells\":[]}")));
	TestEqual(TEXT("A refused load leaves the ledger alone"), Garbage.StepsAt(Here), 1);

	FIslandTrailLedger Weathered;
	for (int32 I = 0; I < 200; ++I) Weathered.AddStep(Here, FVector::UpVector);
	FIslandTrailLedger Faint;
	Faint.AddStep(Here + FVector(900.f, 0.f, 0.f), FVector::UpVector);
	TestTrue(TEXT("The first day only sets the clock"), Weathered.WeatherTo(10));
	TestEqual(TEXT("It changes no steps"), Weathered.StepsAt(Here), 200);
	TestFalse(TEXT("The same day changes nothing"), Weathered.WeatherTo(10));
	TestFalse(TEXT("An earlier day changes nothing"), Weathered.WeatherTo(4));
	Weathered.WeatherTo(11);
	TestTrue(TEXT("A day of disuse costs a path some steps"), Weathered.StepsAt(Here) < 200 && Weathered.StepsAt(Here) > 180);
	Weathered.WeatherTo(40);
	TestTrue(TEXT("A month of disuse mostly undoes it"), Weathered.StepsAt(Here) < 120);
	Faint.Decay(1);
	TestEqual(TEXT("A single footfall is gone in a day"), Faint.Cells.Num(), 0);
	FIslandTrailLedger Reloaded;
	TestTrue(TEXT("The weathering day is saved"), Reloaded.FromJson(Weathered.ToJson()));
	TestEqual(TEXT("And reloaded"), Reloaded.LastDecayDay, 40);

	TestTrue(TEXT("Virgin ground says nothing"), Trail::DescribeTrail(0, 0, 0.f).IsEmpty());
	TestTrue(TEXT("A faint track is noticed"), Trail::DescribeTrail(Trail::WearStartSteps / 2, 0, 0.f).Contains(TEXT("faintly")));
	TestTrue(TEXT("A worn track is noticed"), Trail::DescribeTrail(Trail::WearStartSteps, 0, 0.f).Contains(TEXT("worn thin")));
	TestTrue(TEXT("A bare track is noticed"), Trail::DescribeTrail(Trail::WearFullSteps, 0, 0.f).Contains(TEXT("trodden bare")));
	TestFalse(TEXT("A path used within three days is not described as recovering"),
		Trail::DescribeTrail(Trail::WearStartSteps, 0, 0.f, 10, 12).Contains(TEXT("softening")));
	TestTrue(TEXT("A quiet, older path is described as naturally recovering"),
		Trail::DescribeTrail(Trail::WearStartSteps, 0, 0.f, 10, 13).Contains(TEXT("softening")));
	TestFalse(TEXT("Legacy paths without use dates are not assigned invented recovery history"),
		Trail::DescribeTrail(Trail::WearStartSteps, 0, 0.f, -1, 13).Contains(TEXT("softening")));
	TestTrue(TEXT("Unused ground without a visible track stays undescribed"),
		Trail::DescribeTrail(0, 0, 0.f, 1, 30).IsEmpty());
	TestTrue(TEXT("Wet prints are noticed"), Trail::DescribeTrail(0, 5, 0.9f).Contains(TEXT("Fresh footprints")));
	TestTrue(TEXT("Drying prints are noticed"), Trail::DescribeTrail(0, 5, 0.2f).Contains(TEXT("shrinking")));
	TestTrue(TEXT("One print is not a trail"), Trail::DescribeTrail(0, 1, 0.9f).IsEmpty());

	FIslandTrailLedger Full;
	for (int32 I = 0; I < FIslandTrailLedger::MaxCells; ++I) Full.AddStep(FVector(I * FIslandTrailLedger::CellSize, 0.f, 0.f), FVector::UpVector);
	TestFalse(TEXT("A full ledger refuses new ground"), Full.AddStep(FVector(0.f, 99999.f, 0.f), FVector::UpVector));
	TestTrue(TEXT("But still counts old paths"), Full.AddStep(FVector(0.f, 0.f, 0.f), FVector::UpVector));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTrailVisitorTest, "CaptiveSky2.Agent.IslandTrailVisitor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTrailVisitorTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic visitor trail world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandTrailSubsystem* Trail = World->GetSubsystem<UIslandTrailSubsystem>();
	TestNotNull(TEXT("Synthetic world owns the persistent trail subsystem"), Trail);
	if (!Trail)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Trail->bAllowStorage = false;
	World->BeginPlay();

	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 90.f), FRotator::ZeroRotator);
	UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
	AIslandInteractionTestPlayerController* Controller = World->SpawnActor<AIslandInteractionTestPlayerController>();
	TestNotNull(TEXT("Visitor pawn created over the trail"), Visitor);
	TestNotNull(TEXT("Caption widget created for trail reading"), Caption);
	TestNotNull(TEXT("Local visitor controller created for trail reading"), Controller);
	if (Visitor && Caption && Controller)
	{
		FIslandTrailCell& Cell = Trail->GetLedgerMutable().Cells.Add(FIslandTrailLedger::CellFor(Visitor->GetActorLocation()));
		Cell.Steps = UIslandTrailSubsystem::WearFullSteps;
		Cell.Position = Visitor->GetActorLocation();
		Cell.NormalX = 0.f;
		Cell.NormalY = 0.f;
		for (int32 X = 1; X <= 2; ++X)
		{
			FIslandTrailCell& NearbyCell = Trail->GetLedgerMutable().Cells.Add(FIntPoint(X, 0));
			NearbyCell.Steps = UIslandTrailSubsystem::WearStartSteps;
		}
		Controller->SetFixturePawn(Visitor);
		Controller->SetCaptionWidget(Caption);
		Controller->BindFixtureInput();
		Controller->RefreshInteractionHintForTest();
		TestEqual(TEXT("A visitor over persistent worn tracks gets a read-only prompt when no landmark is nearer"),
			Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: read the nearby tracks")));
		TestTrue(TEXT("E is bound to the visitor's interaction handler"), Controller->PressBoundE());
		TestTrue(TEXT("The visitor learns that repeated feet wore this track"),
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("trodden bare along a track")));
		TestTrue(TEXT("The visitor can place nearby wear in a broader island stretch"),
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("Across this stretch, worn ground touches 3 patches")));
		TestTrue(TEXT("The visitor is told when the age of neighboring marks is unknown"),
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("age of some worn patches is no longer known")));
		TestEqual(TEXT("Reading the trail does not add footfalls"), Trail->GetLedger().StepsAt(Visitor->GetActorLocation()), UIslandTrailSubsystem::WearFullSteps);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
