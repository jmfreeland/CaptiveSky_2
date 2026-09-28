#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "IslandWeather.h"
#include "IslandWeatherTraces.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherTracesTest, "CaptiveSky2.Agent.WeatherTraces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherTracesTest::RunTest(const FString& Parameters)
{
	// No model requests; a scratch world-state file holds a tall cairn and two nests.
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("WeatherTraces") / TEXT("WorldState.json"));
	FFileHelper::SaveStringToFile(TEXT(R"({"version": 1,
		"nests": [{"site": "Roost_A", "layers": 3, "location": [300, 0, 0], "builders": ["Raven"]}, {"site": "Roost_B", "layers": 1, "location": [-300, 0, 0], "builders": ["Raven"]}],
		"curios": [{"id": "Cairn", "kind": "Cairn", "location": [0, 300, 0], "state": 5, "last_changed_day": -1, "contributors": []}],
		"arrangement_sites": []})"), *StateFile);

	auto OpenWorld = [&StateFile]()
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->GetSubsystem<UIslandWorldStateSubsystem>()->StorageFileOverride = StateFile;
		AActor* Ground = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Ground);
		Ground->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(5000, 5000, 50));
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Ground->SetActorLocation(FVector(0, 0, -50));
		World->SpawnActor<AIslandWeather>();
		World->BeginPlay();
		return World;
	};
	auto CloseWorld = [](UWorld* World) { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto Describe = [](UWorld* World, const FVector& Where)
	{
		ACharacter* Body = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Body);
		Body->AddInstanceComponent(Brain);
		Brain->RegisterComponent();
		const FString View = Brain->BuildSituationSummary(FAgentConversationContext());
		Body->Destroy();
		return View;
	};

	UWorld* World = OpenWorld();
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	UIslandWeatherTracesSubsystem* Traces = World->GetSubsystem<UIslandWeatherTracesSubsystem>();
	AIslandWeather* Weather = nullptr;
	for (TActorIterator<AIslandWeather> It(World); It; ++It) Weather = *It;
	if (!TestTrue(TEXT("Fixture has weather, world state, and traces"), Weather && State && Traces)) { CloseWorld(World); return false; }
	const double Now = World->GetTimeSeconds();

	// Sky signs: find a calm moment with a storm due within the quarter hour, and put the weather there.
	double Before = -1.0;
	for (double T = 0.0; T < 200000.0 && Before < 0.0; T += 30.0)
		if (Weather->SampleStormIntensity(T) < 0.2f && Weather->SampleStormIntensity(T + 600.0) > 0.5f) Before = T;
	if (TestTrue(TEXT("The weather has calm moments just before storms"), Before >= 0.0))
	{
		Weather->WeatherTimeOffset = Before - Now;
		TestTrue(TEXT("Residents read the signs of a coming storm"), Describe(World, FVector(0, 0, 100)).Contains(TEXT("a storm may be coming")));
		Weather->WeatherTimeOffset = 0.0;
	}

	Weather->ForcedStormUntil = -1.0;
	const bool bCalmMarks = Weather->SampleStormIntensity(Now) < UIslandWeatherTracesSubsystem::MarkingStorm && Traces->EvaluateStorm(Now);
	TestFalse(TEXT("Ordinary weather leaves no marks"), bCalmMarks);
	Weather->ForcedStormUntil = Now + 600.0;
	TestTrue(TEXT("A full storm leaves its marks"), Traces->EvaluateStorm(Now));
	const FIslandCurioRecord* Cairn = State->FindCurio(TEXT("Cairn"));
	TestTrue(TEXT("The cairn loses its top stone"), Cairn && Cairn->State == 4 && Cairn->StormDamagedDay == UIslandWorldStateSubsystem::CurrentIslandDay(World));
	TestTrue(TEXT("A layered nest loses its outer layer"), State->FindNest(TEXT("Roost_A")) && State->FindNest(TEXT("Roost_A"))->Layers == 2);
	TestTrue(TEXT("A single-layer nest is spared"), State->FindNest(TEXT("Roost_B")) && State->FindNest(TEXT("Roost_B"))->Layers == 1);
	TestFalse(TEXT("One storm marks things only once"), Traces->EvaluateStorm(Now + 30.0));
	TestTrue(TEXT("Residents see the fallen cairn stone"), Describe(World, FVector(0, 400, 100)).Contains(TEXT("top stone was blown down")));
	TestTrue(TEXT("Residents see the torn nest"), Describe(World, FVector(400, 0, 100)).Contains(TEXT("torn loose in a recent storm")));
	CloseWorld(World);

	// A restart during the same storm does not mark things again.
	World = OpenWorld();
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	Traces = World->GetSubsystem<UIslandWeatherTracesSubsystem>();
	for (TActorIterator<AIslandWeather> It(World); It; ++It) It->ForcedStormUntil = World->GetTimeSeconds() + 600.0;
	TestTrue(TEXT("Storm marks are saved"), State->FindCurio(TEXT("Cairn")) && State->FindCurio(TEXT("Cairn"))->State == 4 && State->FindNest(TEXT("Roost_A"))->Layers == 2);
	TestFalse(TEXT("A restart mid-storm does not mark things twice"), Traces->EvaluateStorm(World->GetTimeSeconds()));
	CloseWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	return true;
}
