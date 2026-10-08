#include "Misc/AutomationTest.h"
#include "IslandRainBasin.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandRainBasinTest, "CaptiveSky2.Agent.IslandRainBasin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandRainBasinTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Dry sky, dry night: nothing to gain or lose"), FIslandBasinState::Advance(0.f, 0.f, -0.5f, 600.f), 0.f);
	TestTrue(TEXT("Heavy rain fills it"), FIslandBasinState::Advance(0.f, 1.f, -0.5f, 120.f) > 0.4f);
	TestEqual(TEXT("Rain cannot overfill it"), FIslandBasinState::Advance(0.9f, 1.f, 0.f, 600.f), 1.f);
	TestTrue(TEXT("A sprinkle gathers nothing"), FIslandBasinState::Advance(0.f, 0.03f, 0.f, 600.f) <= 0.f);
	const float Noon = FIslandBasinState::Advance(0.5f, 0.f, 1.f, 300.f);
	const float Night = FIslandBasinState::Advance(0.5f, 0.f, -1.f, 300.f);
	TestTrue(TEXT("Sun dries it faster than night"), Noon < Night && Night < 0.5f);
	TestTrue(TEXT("It stays above zero while drying slowly"), FIslandBasinState::Advance(0.5f, 0.f, 1.f, 10.f) > 0.49f);
	TestEqual(TEXT("It never dries below zero"), FIslandBasinState::Advance(0.01f, 0.f, 1.f, 3600.f), 0.f);
	TestEqual(TEXT("No time passing changes nothing"), FIslandBasinState::Advance(0.4f, 1.f, 1.f, 0.f), 0.4f);
	const float Nonsense = FIslandBasinState::Advance(NAN, 5.f, -9.f, 50.f);
	TestTrue(TEXT("Non-finite water is treated as dry and stays in range"), Nonsense >= 0.f && Nonsense <= 1.f);

	FIslandBasinState State;
	State.bPlaced = true;
	State.Water = 0.1f;
	TestTrue(TEXT("Too little water to float a leaf"), State.FloatLeaf(TEXT("a"), 3, 11) == EIslandBasinFloat::TooDry);
	TestEqual(TEXT("Nothing was added when dry"), State.Leaves.Num(), 0);
	State.Water = 0.7f;
	TestTrue(TEXT("A leaf floats on a good pool"), State.FloatLeaf(TEXT("a"), 3, 11) == EIslandBasinFloat::Floated);
	TestTrue(TEXT("Same resident, same day: no second leaf"), State.FloatLeaf(TEXT("a"), 3, 12) == EIslandBasinFloat::AlreadyToday);
	TestTrue(TEXT("Same resident, next day: another leaf"), State.FloatLeaf(TEXT("a"), 4, 13) == EIslandBasinFloat::Floated);
	TestTrue(TEXT("Another resident the same day: a leaf"), State.FloatLeaf(TEXT("b"), 4, 14) == EIslandBasinFloat::Floated);
	TestEqual(TEXT("Leaves are counted per resident"), State.LeavesFrom(TEXT("a")), 2);
	for (int32 Day = 5; Day < 5 + (FIslandBasinState::MaxLeaves - 3); ++Day) State.FloatLeaf(TEXT("c"), Day, Day);
	TestEqual(TEXT("The basin holds a bounded number of leaves"), State.Leaves.Num(), FIslandBasinState::MaxLeaves);
	TestTrue(TEXT("A crowded basin drops its oldest leaf"), State.FloatLeaf(TEXT("d"), 99, 5) == EIslandBasinFloat::FloatedReplacingOldest);
	TestEqual(TEXT("The oldest leaf (resident a, day 3) is gone"), State.LeavesFrom(TEXT("a")), 1);

	FIslandBasinState Loaded;
	TestTrue(TEXT("Saved state loads back"), Loaded.FromJson(State.ToJson()));
	TestEqual(TEXT("Leaves survive a round trip"), Loaded.Leaves.Num(), State.Leaves.Num());
	TestEqual(TEXT("Water survives a round trip"), Loaded.Water, State.Water);
	TestTrue(TEXT("Place survives a round trip"), Loaded.bPlaced);
	if (Loaded.Leaves.Num() > 0) TestEqual(TEXT("A leaf keeps its author"), Loaded.Leaves.Last().AgentId, State.Leaves.Last().AgentId);
	TestFalse(TEXT("Garbage is rejected"), Loaded.FromJson(TEXT("not json")));
	TestFalse(TEXT("A future version is rejected"), Loaded.FromJson(TEXT("{\"version\":99}")));

	TestTrue(TEXT("Water is deeper when fuller"), AIslandRainBasin::WaterDepth(0.9f) > AIslandRainBasin::WaterDepth(0.2f));
	TestTrue(TEXT("Water stays below the rim"), AIslandRainBasin::WaterDepth(9.f) < AIslandRainBasin::RimHeight);
	TestTrue(TEXT("Leaves brown with age"), AIslandRainBasin::LeafColor(0).G > AIslandRainBasin::LeafColor(8).G);

	TestTrue(TEXT("A dry basin says so"), UIslandRainBasinSubsystem::DescribeWater(0.f, 0).Contains(TEXT("dry")));
	TestTrue(TEXT("A full basin says so"), UIslandRainBasinSubsystem::DescribeWater(1.f, 0).Contains(TEXT("full")));
	TestTrue(TEXT("One leaf is singular"), UIslandRainBasinSubsystem::DescribeWater(0.5f, 1).Contains(TEXT("1 leaf drifts")));
	TestTrue(TEXT("Several leaves are plural"), UIslandRainBasinSubsystem::DescribeWater(0.5f, 3).Contains(TEXT("3 leaves drift")));
	TestTrue(TEXT("Leaves on a dry basin lie on the stone"), UIslandRainBasinSubsystem::DescribeWater(0.f, 2).Contains(TEXT("dried leaves lie")));
	return true;
}
