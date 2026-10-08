#include "Misc/AutomationTest.h"
#include "IslandRainbow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandRainbowTest, "CaptiveSky2.Agent.IslandRainbow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandRainbowTest::RunTest(const FString& Parameters)
{
	FIslandRainbowInputs Clearing;
	Clearing.SunHeight = 0.35f;
	Clearing.Wetness = 0.9f;
	Clearing.Rain = 0.f;
	Clearing.CloudCover = 0.3f;
	Clearing.Storm = 0.f;
	TestTrue(TEXT("Sun out over wet ground after a shower makes a bow"), ComputeRainbowStrength(Clearing) > 0.9f);

	FIslandRainbowInputs Dry = Clearing;
	Dry.Wetness = 0.f;
	TestEqual(TEXT("No bow over dry ground"), ComputeRainbowStrength(Dry), 0.f);

	FIslandRainbowInputs Noon = Clearing;
	Noon.SunHeight = 1.f;
	TestEqual(TEXT("No bow with the sun high"), ComputeRainbowStrength(Noon), 0.f);

	FIslandRainbowInputs Night = Clearing;
	Night.SunHeight = -0.4f;
	TestEqual(TEXT("No bow at night"), ComputeRainbowStrength(Night), 0.f);

	FIslandRainbowInputs Pouring = Clearing;
	Pouring.Rain = 1.f;
	TestEqual(TEXT("No bow in heavy rain"), ComputeRainbowStrength(Pouring), 0.f);

	FIslandRainbowInputs Grey = Clearing;
	Grey.CloudCover = 1.f;
	TestEqual(TEXT("No bow under solid cloud"), ComputeRainbowStrength(Grey), 0.f);

	FIslandRainbowInputs Stormy = Clearing;
	Stormy.Storm = 1.f;
	TestEqual(TEXT("No bow in a storm"), ComputeRainbowStrength(Stormy), 0.f);

	FIslandRainbowInputs Low = Clearing;
	Low.SunHeight = 0.09f;
	const float LowStrength = ComputeRainbowStrength(Low);
	TestTrue(TEXT("A bow fades in as the sun gets just above the horizon"), LowStrength > 0.f && LowStrength < ComputeRainbowStrength(Clearing));

	FIslandRainbowInputs Nonsense;
	Nonsense.SunHeight = 0.3f;
	Nonsense.Wetness = 9.f;
	Nonsense.Rain = -4.f;
	Nonsense.CloudCover = -2.f;
	Nonsense.Storm = -1.f;
	const float Clamped = ComputeRainbowStrength(Nonsense);
	TestTrue(TEXT("Out-of-range inputs stay within 0..1"), Clamped >= 0.f && Clamped <= 1.f);

	// The morning sun sits at yaw 215, so the anti-solar point is at yaw 35, below the horizon.
	const FVector Morning = UIslandRainbowSubsystem::AntiSolarAxis(8.f);
	TestTrue(TEXT("Morning anti-solar point is below the horizon"), Morning.Z < 0.f);
	TestTrue(TEXT("Axis is a unit vector"), FMath::IsNearlyEqual(Morning.Size(), 1.f, 0.001f));
	const FVector Evening = UIslandRainbowSubsystem::AntiSolarAxis(16.f);
	TestTrue(TEXT("Morning and evening bows stand on opposite sides"), FVector::DotProduct(Morning.GetSafeNormal2D(), Evening.GetSafeNormal2D()) < -0.9f);

	TestTrue(TEXT("Nothing is said without a bow"), UIslandRainbowSubsystem::DescribeRainbow(0.1f).IsEmpty());
	TestTrue(TEXT("A faint bow is mentioned"), UIslandRainbowSubsystem::DescribeRainbow(0.5f).Contains(TEXT("faint rainbow")));
	TestTrue(TEXT("A bright bow is mentioned"), UIslandRainbowSubsystem::DescribeRainbow(0.9f).Contains(TEXT("bright")));
	return true;
}
