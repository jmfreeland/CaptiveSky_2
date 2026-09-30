#include "Misc/AutomationTest.h"
#include "AgentDataPaths.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentDataPathsTest, "CaptiveSky2.Agent.DataPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentDataPathsTest::RunTest(const FString& Parameters)
{
	const FString ProjectDirectory = TEXT("C:/CaptiveSkyDataPathTest/Project/");
	TestEqual(TEXT("Without an override, resident data stays under the project directory"),
		CaptiveSkyDataPaths::ResolveProjectDataPathWithRoot(ProjectDirectory, TEXT(""), TEXT("Agents/Aster/places.json")),
		FPaths::ConvertRelativePathToFull(ProjectDirectory / TEXT("Agents/Aster/places.json")));
	TestEqual(TEXT("A relative override is resolved from the project directory"),
		CaptiveSkyDataPaths::ResolveProjectDataPathWithRoot(ProjectDirectory, TEXT("Saved/Playtests/ReturnCheck"), TEXT("WorldState/Island.json")),
		FPaths::ConvertRelativePathToFull(ProjectDirectory / TEXT("Saved/Playtests/ReturnCheck/WorldState/Island.json")));
	TestEqual(TEXT("An absolute override redirects persistent data to its isolated root"),
		CaptiveSkyDataPaths::ResolveProjectDataPathWithRoot(ProjectDirectory, TEXT("D:/CaptiveSkyTest/"), TEXT("Agents/Raven/places.json")),
		FPaths::ConvertRelativePathToFull(TEXT("D:/CaptiveSkyTest/Agents/Raven/places.json")));
	return true;
}
