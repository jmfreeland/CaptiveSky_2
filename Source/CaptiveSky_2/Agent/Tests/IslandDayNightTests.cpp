#include "Misc/AutomationTest.h"
#include "IslandDayNight.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandClockTest, "CaptiveSky2.Agent.DayNight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIslandClockTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Midnight wraps"), AIslandDayNight::WrapHour(24), 0.f);
	TestEqual(TEXT("Negative preview wraps"), AIslandDayNight::WrapHour(-1), 23.f);
	TestTrue(TEXT("Sun rises at six"), FMath::IsNearlyZero(AIslandDayNight::SunHeight(6)));
	TestTrue(TEXT("Sun overhead at noon"), FMath::IsNearlyEqual(AIslandDayNight::SunHeight(12), 1.f));
	TestTrue(TEXT("Sun below horizon at midnight"), FMath::IsNearlyEqual(AIslandDayNight::SunHeight(0), -1.f));
	return true;
}
