#include "Misc/AutomationTest.h"
#include "IslandTidepoolPeriwinkle.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleCrabCueTest, "CaptiveSky2.Agent.TidepoolPeriwinkleCrabCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleCrabCueTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A nearby scurrying crab briefly startles the periwinkle"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 120.f, 20.f));
	TestTrue(TEXT("The edge of the threat radius remains inclusive"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 175.f, 0.f));
	TestFalse(TEXT("A calm crab does not repeatedly disturb its neighbor"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(false, 60.f, 0.f));
	TestFalse(TEXT("A crab outside close range does not trigger a tuck"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 176.f, 0.f));
	TestFalse(TEXT("A crab far above or below the shoreline does not trigger a tuck"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 100.f, 101.f));
	TestFalse(TEXT("Non-finite crab distance is ignored"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, std::numeric_limits<float>::infinity(), 0.f));
	TestFalse(TEXT("Negative crab distance is ignored"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, -1.f, 0.f));
	return true;
}
