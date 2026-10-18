#include "Agent/IslandCommonFrog.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCommonFrogTests, "CaptiveSky2.Agent.CommonFrogEcology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandCommonFrogTests::RunTest(const FString& Parameters)
{
	using Frog = AIslandCommonFrog;

	TestTrue(TEXT("A nearby low observer can startle the frog"), Frog::ShouldHopFromObserver(180.f, 35.f));
	TestTrue(TEXT("The horizontal startle limit is inclusive"), Frog::ShouldHopFromObserver(240.f, 0.f));
	TestFalse(TEXT("Distant observers do not startle it"), Frog::ShouldHopFromObserver(241.f, 0.f));
	TestFalse(TEXT("An observer above the local height band is ignored"), Frog::ShouldHopFromObserver(100.f, 181.f));
	TestFalse(TEXT("Invalid distances are ignored"), Frog::ShouldHopFromObserver(std::numeric_limits<float>::infinity(), 0.f));
	TestTrue(TEXT("A nearby actively scurrying crab can startle the frog"), Frog::ShouldHopFromScurryingCrab(true, 180.f, 35.f));
	TestFalse(TEXT("A settled crab is not a startle cue"), Frog::ShouldHopFromScurryingCrab(false, 180.f, 35.f));
	TestFalse(TEXT("A scurrying crab outside the local band is ignored"), Frog::ShouldHopFromScurryingCrab(true, 241.f, 0.f));
	TestFalse(TEXT("A lingering visitor does not rearm at the boundary"), Frog::ShouldRearmFromObserver(520.f, 520.f));
	TestTrue(TEXT("A visitor must retreat beyond the wider ring to rearm"), Frog::ShouldRearmFromObserver(521.f, 520.f));
	TestFalse(TEXT("Invalid rearm distances are ignored"), Frog::ShouldRearmFromObserver(521.f,
		std::numeric_limits<float>::quiet_NaN()));

	const float DryDay = Frog::AmbientActivityScale(0.f, 0.f);
	const float WetDay = Frog::AmbientActivityScale(1.f, 0.f);
	const float DryNight = Frog::AmbientActivityScale(0.f, 1.f);
	TestTrue(TEXT("Dry daylight is quieter than wet weather"), DryDay < WetDay);
	TestTrue(TEXT("Dry daylight is quieter than night activity"), DryDay < DryNight);
	TestTrue(TEXT("Activity scale remains bounded"), WetDay <= 1.f && DryNight <= 1.f);
	TestEqual(TEXT("Invalid weather yields a safe resting activity"),
		Frog::AmbientActivityScale(std::numeric_limits<float>::quiet_NaN(), 1.f), 0.f);

	TestTrue(TEXT("Hop starts at ground height"), FMath::IsNearlyZero(Frog::HopArcHeight(0.f, 13.f)));
	TestTrue(TEXT("Hop ends back at ground height"), FMath::IsNearlyZero(Frog::HopArcHeight(1.f, 13.f)));
	TestTrue(TEXT("Hop rises at its midpoint"), FMath::IsNearlyEqual(Frog::HopArcHeight(0.5f, 13.f), 13.f));
	TestEqual(TEXT("Invalid hop progress resolves safely"),
		Frog::HopArcHeight(std::numeric_limits<float>::quiet_NaN(), 13.f), 0.f);
	return true;
}
