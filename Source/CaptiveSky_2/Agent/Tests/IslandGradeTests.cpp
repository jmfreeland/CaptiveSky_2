#include "Misc/AutomationTest.h"
#include "IslandGrade.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandGradeTest, "CaptiveSky2.Agent.IslandGrade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGradeTest::RunTest(const FString& Parameters)
{
	const FVector3f One(1.f, 1.f, 1.f);
	const FIslandGrade Noon = ComputeIslandGrade(FIslandGradeInputs(), 1.f);
	TestTrue(TEXT("A clear dry noon is neutral"), FMath::IsNearlyEqual(Noon.Saturation, 1.f) && Noon.Gain.Equals(One) && FMath::IsNearlyEqual(Noon.Contrast, 1.f));

	FIslandGradeInputs Golden;
	Golden.GoldenHour = 1.f;
	const FIslandGrade Warm = ComputeIslandGrade(Golden, 1.f);
	TestTrue(TEXT("Golden hour warms the light"), Warm.Gain.X > 1.f && Warm.Gain.Z < 1.f && Warm.Saturation > 1.f);

	FIslandGradeInputs Dark;
	Dark.Daylight = 0.f;
	const FIslandGrade Night = ComputeIslandGrade(Dark, 1.f);
	TestTrue(TEXT("Night leans blue"), Night.Gain.Z > 1.f && Night.Gain.X < 1.f && Night.Saturation < 1.f);

	FIslandGradeInputs Storm;
	Storm.Storm = 1.f;
	Storm.Rain = 1.f;
	Storm.CloudCover = 1.f;
	const FIslandGrade Gray = ComputeIslandGrade(Storm, 1.f);
	TestTrue(TEXT("A storm drains colour"), Gray.Saturation < 0.95f && Gray.Contrast < 1.f);

	FIslandGradeInputs Wet;
	Wet.Wetness = 1.f;
	TestTrue(TEXT("Wet ground after rain looks richer"), ComputeIslandGrade(Wet, 1.f).Saturation > 1.f);

	FIslandGradeInputs Stormy = Golden;
	Stormy.Storm = 1.f;
	TestTrue(TEXT("A storm mutes the golden hour"), ComputeIslandGrade(Stormy, 1.f).Gain.X < Warm.Gain.X);

	TestTrue(TEXT("Strength 0 is neutral"), FMath::IsNearlyEqual(ComputeIslandGrade(Storm, 0.f).Saturation, 1.f) && ComputeIslandGrade(Golden, 0.f).Gain.Equals(One));
	TestTrue(TEXT("Strength scales the effect"), ComputeIslandGrade(Golden, 2.f).Gain.X > Warm.Gain.X);

	FIslandGradeInputs Everything;
	Everything.GoldenHour = Everything.Storm = Everything.Rain = Everything.Wetness = Everything.Mist = Everything.CloudCover = 1.f;
	Everything.Daylight = 0.f;
	const FIslandGrade Extreme = ComputeIslandGrade(Everything, 1.f);
	TestTrue(TEXT("Even the extreme stays subtle"), FMath::Abs(Extreme.Saturation - 1.f) < 0.2f && FMath::Abs(Extreme.Contrast - 1.f) < 0.1f
		&& FMath::Abs(Extreme.Gain.X - 1.f) < 0.1f && FMath::Abs(Extreme.Gain.Y - 1.f) < 0.1f && FMath::Abs(Extreme.Gain.Z - 1.f) < 0.1f);
	FIslandGradeInputs Nonsense;
	Nonsense.Daylight = Nonsense.GoldenHour = Nonsense.Storm = Nonsense.Rain = Nonsense.Wetness = Nonsense.Mist = Nonsense.CloudCover = 5.f;
	TestTrue(TEXT("Nonsense inputs are clamped"), ComputeIslandGrade(Nonsense, 1.f).Saturation > 0.5f);

	const FIslandGrade Half = UIslandGradeSubsystem::Settle(FIslandGrade(), Warm, UIslandGradeSubsystem::SettleSeconds);
	TestTrue(TEXT("The grade moves toward the target, not all at once"), Half.Gain.X > 1.f && Half.Gain.X < Warm.Gain.X);
	TestTrue(TEXT("No time means no movement"), UIslandGradeSubsystem::Settle(FIslandGrade(), Warm, 0.f).Gain.Equals(One));
	TestTrue(TEXT("A long time arrives"), UIslandGradeSubsystem::Settle(FIslandGrade(), Warm, 600.f).Gain.Equals(Warm.Gain, 0.001f));
	return true;
}
