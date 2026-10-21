#include "Misc/AutomationTest.h"
#include "IslandLightning.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandLightningTextTest, "CaptiveSky2.Agent.IslandLightningText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIslandLightningTextTest::RunTest(const FString& Parameters)
{
	const float Near = 20000.f;                       // 0.58 s of sound travel
	const float Mid = AIslandLightning::SoundSpeed * 3.f;
	const float Far = AIslandLightning::SoundSpeed * 9.f;
	TestTrue(TEXT("The flash is seen at once at any distance"),
		AIslandLightning::DescribeStrike(0.1f, Far).Contains(TEXT("flares")));
	TestTrue(TEXT("Far listeners wait for the thunder"),
		AIslandLightning::DescribeStrike(2.f, Far).Contains(TEXT("not yet reached")));
	TestTrue(TEXT("Overhead strike cracks at once"),
		AIslandLightning::DescribeStrike(0.9f, Near).Contains(TEXT("directly overhead")));
	TestTrue(TEXT("Mid-range thunder rolls close by"),
		AIslandLightning::DescribeStrike(3.5f, Mid).Contains(TEXT("close by")));
	TestTrue(TEXT("Far thunder rumbles"),
		AIslandLightning::DescribeStrike(9.5f, Far).Contains(TEXT("far off")));
	TestTrue(TEXT("The moment passes"), AIslandLightning::DescribeStrike(20.f, Near).IsEmpty());
	TestTrue(TEXT("Bad input is harmless"),
		AIslandLightning::DescribeStrike(NAN, 100.f).IsEmpty() && AIslandLightning::DescribeStrike(-1.f, 100.f).IsEmpty());
	return true;
}
