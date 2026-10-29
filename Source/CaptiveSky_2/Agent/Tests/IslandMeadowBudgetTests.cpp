#include "Misc/AutomationTest.h"
#include "IslandWeather.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandMeadowBudgetTest,
	"CaptiveSky2.Agent.MeadowBudgetSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandMeadowBudgetTest::RunTest(const FString& Parameters)
{
	constexpr int32 TotalCandidatesAtFivePercent = 96000;
	constexpr int32 CandidatesPerLandmarkPatch = 1875;
	const int32 LandmarkPatchLimit = AIslandWeather::CalculateLandmarkMeadowPatchLimit(
		TotalCandidatesAtFivePercent, CandidatesPerLandmarkPatch, 0.4f);
	TestEqual(TEXT("The 5% meadow density reserves no more than 40% of its total cap for landmark patches"),
		LandmarkPatchLimit, 20);
	const int32 ReservedLandmarkCandidates = LandmarkPatchLimit * CandidatesPerLandmarkPatch;
	const int32 BroadLandscapeCandidates = AIslandWeather::CalculateBroadLandscapeCandidateBudget(
		TotalCandidatesAtFivePercent, ReservedLandmarkCandidates);
	TestEqual(TEXT("Unused capacity after the landmark allocation remains available to broad terrain"),
		BroadLandscapeCandidates, 58500);
	TestTrue(TEXT("Landmark and broad candidate caps never exceed the original total"),
		ReservedLandmarkCandidates + BroadLandscapeCandidates <= TotalCandidatesAtFivePercent);

	TestEqual(TEXT("A full-density pass still respects the same total candidate cap"),
		AIslandWeather::CalculateLandmarkMeadowPatchLimit(1920000, CandidatesPerLandmarkPatch, 0.4f), 409);
	TestEqual(TEXT("Zero density schedules no landmark patches"),
		AIslandWeather::CalculateLandmarkMeadowPatchLimit(0, CandidatesPerLandmarkPatch, 0.4f), 0);
	TestEqual(TEXT("A share above one is clamped to the full budget"),
		AIslandWeather::CalculateLandmarkMeadowPatchLimit(1000, 100, 2.f), 10);
	TestEqual(TEXT("A negative share schedules no landmark patches"),
		AIslandWeather::CalculateLandmarkMeadowPatchLimit(1000, 100, -0.2f), 0);
	TestEqual(TEXT("Invalid shares fail closed"),
		AIslandWeather::CalculateLandmarkMeadowPatchLimit(1000, 100,
			std::numeric_limits<float>::quiet_NaN()), 0);
	TestEqual(TEXT("Broad landscape never receives a negative candidate budget"),
		AIslandWeather::CalculateBroadLandscapeCandidateBudget(1000, 1200), 0);
	TestEqual(TEXT("Negative local use cannot erase broad landscape capacity"),
		AIslandWeather::CalculateBroadLandscapeCandidateBudget(1000, -50), 1000);
	return true;
}
