#include "Misc/AutomationTest.h"
#include "IslandTidepoolPeriwinkle.h"
#include "IslandTideglassSubsystem.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleRatCueTest, "CaptiveSky2.Agent.TidepoolPeriwinkleRatCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleRatCueTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	TestTrue(TEXT("A close rat approach briefly startles the snail"),
		Periwinkle::ShouldTuckForRat(130.f, 15.f));
	TestTrue(TEXT("The rat cue radius edge is inclusive"),
		Periwinkle::ShouldTuckForRat(190.f, 0.f));
	TestFalse(TEXT("A rat still on the broader shore does not startle the snail"),
		Periwinkle::ShouldTuckForRat(191.f, 0.f));
	TestFalse(TEXT("A rat far above or below the rock does not trigger the cue"),
		Periwinkle::ShouldTuckForRat(120.f, 121.f));
	TestFalse(TEXT("Negative distance is rejected"),
		Periwinkle::ShouldTuckForRat(-1.f, 0.f));
	TestFalse(TEXT("Non-finite distance is rejected"),
		Periwinkle::ShouldTuckForRat(std::numeric_limits<float>::infinity(), 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleGrazingPathTest, "CaptiveSky2.Agent.TidepoolPeriwinkleGrazingPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleGrazingPathTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	const FVector2D AtStart = Periwinkle::GrazingOffset(0.f, 0.f);
	TestTrue(TEXT("The grazing path starts at its home point"), AtStart.IsNearlyZero());

	const FVector2D MidCycle = Periwinkle::GrazingOffset(30.f, 0.f);
	TestTrue(TEXT("The snail progresses across its patch while active"), MidCycle.Size() > 5.f);
	TestTrue(TEXT("Mid-cycle movement remains local"), MidCycle.Size() <= Periwinkle::GrazingMaxExcursionCm);

	const FVector2D FullCycle = Periwinkle::GrazingOffset(120.f, 0.f);
	TestTrue(TEXT("The closed path returns to its starting point"), FullCycle.Size() < 0.01f);

	bool bAllSamplesBounded = true;
	for (int32 Sample = 0; Sample <= 240; ++Sample)
	{
		const FVector2D Offset = Periwinkle::GrazingOffset(Sample * 0.5f, 1.37f);
		bAllSamplesBounded &= Offset.Size() <= Periwinkle::GrazingMaxExcursionCm + 0.001f;
	}
	TestTrue(TEXT("Every sampled step stays inside the bounded home patch"), bAllSamplesBounded);

	const FVector2D Invalid = Periwinkle::GrazingOffset(std::numeric_limits<float>::quiet_NaN(), 0.f);
	TestTrue(TEXT("Non-finite time cannot move the snail"), Invalid.IsNearlyZero());
	TestEqual(TEXT("Active grazing advances at full openness"), Periwinkle::AdvanceGrazingTime(4.f, 2.f, 0.f), 6.f);
	TestEqual(TEXT("A fully tucked snail pauses its graze path"), Periwinkle::AdvanceGrazingTime(4.f, 2.f, 1.f), 4.f);
	TestEqual(TEXT("Partial shelter proportionally slows the path"), Periwinkle::AdvanceGrazingTime(4.f, 2.f, 0.5f), 5.f);
	TestEqual(TEXT("Negative frame time cannot reverse the path"), Periwinkle::AdvanceGrazingTime(4.f, -2.f, 0.f), 4.f);
	TestEqual(TEXT("Invalid rest state resets safely"),
		Periwinkle::AdvanceGrazingTime(4.f, 2.f, std::numeric_limits<float>::quiet_NaN()), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleIntertidalZonationTest,
	"CaptiveSky2.Agent.TidepoolPeriwinkleIntertidalZonation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleIntertidalZonationTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	constexpr float SeaLevel = 1000.f;
	const float Ebb = -UIslandTideglassSubsystem::MaximumTideOffsetCm;
	const float SameLevel = Periwinkle::LocalWaterlineOffsetCm(Ebb, SeaLevel, SeaLevel);
	const float HighRock = Periwinkle::LocalWaterlineOffsetCm(Ebb, SeaLevel + 20.f, SeaLevel);
	const float LowRock = Periwinkle::LocalWaterlineOffsetCm(Ebb, SeaLevel - 20.f, SeaLevel);
	TestEqual(TEXT("A snail at mean water level keeps the existing tide response"), SameLevel, Ebb);
	TestTrue(TEXT("A higher rock experiences a more exposed local ebb"), HighRock < SameLevel);
	TestTrue(TEXT("A lower rock remains wetter through the same ebb"), LowRock > SameLevel);
	TestTrue(TEXT("The higher snail tucks more than the lower snail"),
		Periwinkle::ExposureRestAmount(HighRock) > Periwinkle::ExposureRestAmount(LowRock));
	TestTrue(TEXT("The lower snail keeps a wetter shell than the higher snail"),
		Periwinkle::WetShellAmount(LowRock, 0.f) > Periwinkle::WetShellAmount(HighRock, 0.f));
	TestEqual(TEXT("Missing shore height falls back to the global tide offset"),
		Periwinkle::LocalWaterlineOffsetCm(Ebb, std::numeric_limits<float>::quiet_NaN(), SeaLevel), Ebb);
	TestEqual(TEXT("A non-finite tide offset resolves safely"),
		Periwinkle::LocalWaterlineOffsetCm(std::numeric_limits<float>::infinity(), SeaLevel, SeaLevel), 0.f);
	TestTrue(TEXT("Extreme finite heights remain bounded to a finite response"),
		FMath::IsFinite(Periwinkle::LocalWaterlineOffsetCm(Ebb, -std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max())));
	return true;
}
