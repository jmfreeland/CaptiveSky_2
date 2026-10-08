#include "Misc/AutomationTest.h"
#include "IslandDew.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandDewTest, "CaptiveSky2.Agent.IslandDew",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandDewTest::RunTest(const FString& Parameters)
{
	FIslandDewInputs Dawn;
	Dawn.SunHeight = 0.2f;
	Dawn.Hour = 7.f;
	Dawn.Wetness = 0.6f;
	Dawn.Rain = 0.f;
	Dawn.CloudCover = 0.2f;
	Dawn.Storm = 0.f;
	TestTrue(TEXT("Clear low morning sun over damp grass glitters"), ComputeDewStrength(Dawn) > 0.7f);

	FIslandDewInputs DryDawn = Dawn;
	DryDawn.Wetness = 0.f;
	TestTrue(TEXT("Dry ground still carries some dew, less than wet"), ComputeDewStrength(DryDawn) > 0.f && ComputeDewStrength(DryDawn) < ComputeDewStrength(Dawn));

	FIslandDewInputs Evening = Dawn;
	Evening.Hour = 17.f;
	TestEqual(TEXT("No dew in the evening"), ComputeDewStrength(Evening), 0.f);

	FIslandDewInputs Noon = Dawn;
	Noon.SunHeight = 1.f;
	Noon.Hour = 11.5f;
	TestEqual(TEXT("The high sun has burnt it off"), ComputeDewStrength(Noon), 0.f);

	FIslandDewInputs Night = Dawn;
	Night.SunHeight = -0.3f;
	Night.Hour = 3.f;
	TestEqual(TEXT("Nothing to catch the light at night"), ComputeDewStrength(Night), 0.f);

	FIslandDewInputs Raining = Dawn;
	Raining.Rain = 0.8f;
	TestEqual(TEXT("No glitter in falling rain"), ComputeDewStrength(Raining), 0.f);

	FIslandDewInputs Grey = Dawn;
	Grey.CloudCover = 1.f;
	TestEqual(TEXT("No glitter under solid cloud"), ComputeDewStrength(Grey), 0.f);

	FIslandDewInputs Stormy = Dawn;
	Stormy.Storm = 1.f;
	TestEqual(TEXT("No glitter in a storm"), ComputeDewStrength(Stormy), 0.f);

	FIslandDewInputs Nonsense;
	Nonsense.SunHeight = 0.2f;
	Nonsense.Hour = 7.f;
	Nonsense.Wetness = 9.f;
	Nonsense.Rain = -3.f;
	Nonsense.CloudCover = -2.f;
	Nonsense.Storm = -1.f;
	const float Clamped = ComputeDewStrength(Nonsense);
	TestTrue(TEXT("Out-of-range inputs stay within 0..1"), Clamped >= 0.f && Clamped <= 1.f);

	TestEqual(TEXT("No strength, no glints"), AIslandDewActor::DesiredCount(0.f), 0);
	TestEqual(TEXT("Full strength fills every slot"), AIslandDewActor::DesiredCount(1.f), AIslandDewActor::MaxGlints);
	TestEqual(TEXT("Strength beyond range is clamped"), AIslandDewActor::DesiredCount(7.f), AIslandDewActor::MaxGlints);
	TestTrue(TEXT("Half strength is about half the glints"), FMath::Abs(AIslandDewActor::DesiredCount(0.5f) - AIslandDewActor::MaxGlints / 2) <= 1);
	TestTrue(TEXT("Far glints are larger than near ones"), AIslandDewActor::GlintSize(1200.f) > AIslandDewActor::GlintSize(100.f));
	TestTrue(TEXT("Glint size stays bounded"), AIslandDewActor::GlintSize(1e6f) <= AIslandDewActor::GlintSize(AIslandDewActor::Radius) + 0.001f);

	TestTrue(TEXT("Nothing is said without dew"), UIslandDewSubsystem::DescribeDew(0.1f).IsEmpty());
	TestTrue(TEXT("A little dew is mentioned"), UIslandDewSubsystem::DescribeDew(0.5f).Contains(TEXT("little dew")));
	TestTrue(TEXT("Heavy dew is mentioned"), UIslandDewSubsystem::DescribeDew(0.9f).Contains(TEXT("glitters")));
	return true;
}
