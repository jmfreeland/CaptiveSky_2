#include "Misc/AutomationTest.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandInteractionTestPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherObservationTest, "CaptiveSky2.Agent.VisitorWeatherObservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherObservationTest::RunTest(const FString& Parameters)
{
	const FString Clear = UIslandEnvironmentSubsystem::DescribeWeather(0.f, 0.f, FVector::ZeroVector, 0.f);
	TestTrue(TEXT("Calm, dry, clear weather has no generic observation"), Clear.IsEmpty());
	TestTrue(TEXT("Light rain is distinguished"), UIslandEnvironmentSubsystem::DescribeWeather(0.2f, 0.3f,
		FVector::ZeroVector, 0.f).Contains(TEXT("light rain")));
	TestTrue(TEXT("Strong rain is reported as steady"), UIslandEnvironmentSubsystem::DescribeWeather(0.8f, 0.8f,
		FVector::ZeroVector, 0.f).Contains(TEXT("Steady rain")));
	TestTrue(TEXT("Wind is only reported above the breeze threshold"), UIslandEnvironmentSubsystem::DescribeWeather(0.f, 0.f,
		FVector(120.f, 0.f, 0.f), 0.f).Contains(TEXT("light breeze")));
	TestTrue(TEXT("Dampness lingers after rain passes"), UIslandEnvironmentSubsystem::DescribeWeather(0.f, 0.4f,
		FVector::ZeroVector, 0.f).Contains(TEXT("damp from earlier rain")));
	TestTrue(TEXT("Mist is included when visible"), UIslandEnvironmentSubsystem::DescribeWeather(0.f, 0.f,
		FVector::ZeroVector, 0.4f).Contains(TEXT("thin mist")));

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic visitor weather world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>();
	TestNotNull(TEXT("Synthetic world owns the environment subsystem"), Environment);
	if (!Environment)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();
	Environment->RainIntensity = 0.8f;
	Environment->Wind = FVector(400.f, 0.f, 0.f);

	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 90.f), FRotator::ZeroRotator);
	UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
	AIslandInteractionTestPlayerController* Controller = World->SpawnActor<AIslandInteractionTestPlayerController>();
	TestNotNull(TEXT("Visitor pawn created for weather observation"), Visitor);
	TestNotNull(TEXT("Caption widget created for weather observation"), Caption);
	TestNotNull(TEXT("Visitor controller created for weather observation"), Controller);
	if (Visitor && Caption && Controller)
	{
		Controller->SetFixturePawn(Visitor);
		Controller->SetCaptionWidget(Caption);
		Controller->BindFixtureInput();
		Controller->RefreshInteractionHintForTest();
		TestEqual(TEXT("Active outdoor weather offers a local interaction"),
			Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: feel the weather")));
		TestTrue(TEXT("E reports current rain and wind without a model call"), Controller->PressBoundE() &&
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("Steady rain")) &&
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("steady wind")));

		Environment->Indoors = 1.f;
		Controller->RefreshInteractionHintForTest();
		TestTrue(TEXT("Outdoor weather prompt is suppressed under the inn roof"),
			Caption->GetDisplayedInteractionHint().IsEmpty());
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
