#include "Agent/IslandTideglassDragonfly.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassDragonflyFrogResponseTests,
	"CaptiveSky2.Agent.Ecology.DragonflyFrogResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassDragonflyFrogResponseTests::RunTest(const FString& Parameters)
{
	using Dragonfly = AIslandTideglassDragonfly;
	TestTrue(TEXT("A nearby hopping frog can startle the dragonfly"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 180.f, 110.f));
	TestTrue(TEXT("The startle radius is inclusive"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 230.f, 0.f));
	TestFalse(TEXT("A settled frog is not a startle cue"),
		Dragonfly::ShouldScatterFromHoppingFrog(false, 100.f, 0.f));
	TestFalse(TEXT("A distant hop is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 231.f, 0.f));
	TestFalse(TEXT("A frog at a much higher elevation is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, 100.f, 251.f));
	TestFalse(TEXT("Invalid distance is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, std::numeric_limits<float>::quiet_NaN(), 0.f));
	TestFalse(TEXT("Negative distance is ignored"),
		Dragonfly::ShouldScatterFromHoppingFrog(true, -1.f, 0.f));
	return true;
}
