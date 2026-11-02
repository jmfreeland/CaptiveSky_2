#include "Misc/AutomationTest.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "IslandDayNight.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandInteractionUtility.h"
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
			Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: notice your surroundings")));
		TestTrue(TEXT("E reports current rain and wind without a model call"), Controller->PressBoundE() &&
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("Steady rain")) &&
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("steady wind")));

		Environment->Indoors = 1.f;
		Controller->RefreshInteractionHintForTest();
		TestTrue(TEXT("Environmental observation prompt is suppressed under the inn roof"),
			Caption->GetDisplayedInteractionHint().IsEmpty());
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideObservationTest, "CaptiveSky2.Agent.VisitorTideObservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideObservationTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic Tideglass visitor world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->BeginPlay();

	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>();
	ATargetPoint* Tideglass = World->SpawnActor<ATargetPoint>(FVector(700.f, 0.f, 90.f), FRotator::ZeroRotator);
	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 90.f), FRotator::ZeroRotator);
	UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
	AIslandInteractionTestPlayerController* Controller = World->SpawnActor<AIslandInteractionTestPlayerController>();
	TestNotNull(TEXT("Island clock created for tide observation"), Clock);
	TestNotNull(TEXT("Tideglass landmark created for proximity gating"), Tideglass);
	TestNotNull(TEXT("Visitor created for tide observation"), Visitor);
	TestNotNull(TEXT("Caption widget created for tide observation"), Caption);
	TestNotNull(TEXT("Visitor controller created for tide observation"), Controller);
	if (Clock && Tideglass && Visitor && Caption && Controller)
	{
		Clock->bAdvanceTime = false;
		Clock->DayNumber = 1;
		Clock->CurrentHour = 2.5f;
		Tideglass->Tags.Add(TEXT("TideglassPool"));
		Controller->SetFixturePawn(Visitor);
		Controller->SetCaptionWidget(Caption);
		Controller->BindFixtureInput();
		Controller->RefreshInteractionHintForTest();
		TestEqual(TEXT("A clear, dry visitor still discovers the environmental observation action"),
			Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: notice your surroundings")));
		TestTrue(TEXT("A visitor can read the live tide near Tideglass without an interaction target"),
			Controller->PressBoundE() && Caption->GetDisplayedCaption().ToString().Contains(TEXT("You pause beside Tideglass")) &&
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("tide is rising")));

		Visitor->SetActorLocation(FVector(3000.f, 0.f, 90.f));
		Controller->RefreshInteractionHintForTest();
		TestTrue(TEXT("Tideglass is not offered as a local observation from far away"),
			Controller->PressBoundE() && !Caption->GetDisplayedCaption().ToString().Contains(TEXT("tide is rising")));
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandSkyObservationTest, "CaptiveSky2.Agent.VisitorSkyObservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandSkyObservationTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic sky-observation world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->BeginPlay();

	UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>();
	TestNotNull(TEXT("Synthetic sky-observation world owns the environment subsystem"), Environment);
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>();
	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 90.f), FRotator::ZeroRotator);
	UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
	AIslandInteractionTestPlayerController* Controller = World->SpawnActor<AIslandInteractionTestPlayerController>();
	TestNotNull(TEXT("Island clock created for sky observation"), Clock);
	TestNotNull(TEXT("Visitor created for sky observation"), Visitor);
	TestNotNull(TEXT("Caption widget created for sky observation"), Caption);
	TestNotNull(TEXT("Visitor controller created for sky observation"), Controller);
	if (Environment && Clock && Visitor && Caption && Controller)
	{
		Clock->bAdvanceTime = false;
		Clock->DayNumber = 100;
		Clock->CurrentHour = 23.5f;
		Controller->SetFixturePawn(Visitor);
		Controller->SetCaptionWidget(Caption);
		Controller->BindFixtureInput();
		TestTrue(TEXT("A visitor can notice the actual night-sky conditions on demand"),
			Controller->PressBoundE() && Caption->GetDisplayedCaption().ToString().Contains(TEXT("You pause to read the sky")) &&
			(Caption->GetDisplayedCaption().ToString().Contains(TEXT("stars")) ||
				Caption->GetDisplayedCaption().ToString().Contains(TEXT("No stars"))));

		Environment->Indoors = 1.f;
		TestTrue(TEXT("The visitor does not receive an outdoor sky observation under the inn roof"),
			Controller->PressBoundE() && !Caption->GetDisplayedCaption().ToString().Contains(TEXT("read the sky")));

		Environment->Indoors = 0.f;
		Clock->CurrentHour = AIslandDayNight::SunriseHour(Clock->DayNumber) + 0.1f;
		TestTrue(TEXT("A visitor can notice the actual seasonal sunrise and horizon color"),
			Controller->PressBoundE() && Caption->GetDisplayedCaption().ToString().Contains(TEXT("sun has just cleared the horizon")) &&
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("horizon glows")));

		Clock->CurrentHour = 12.f;
		TestTrue(TEXT("A midday visitor is not told that stars are visible"),
			Controller->PressBoundE() && !Caption->GetDisplayedCaption().ToString().Contains(TEXT("read the sky")) &&
			!Caption->GetDisplayedCaption().ToString().Contains(TEXT("stars")));
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWildlifeObservationTest, "CaptiveSky2.Agent.VisitorWildlifeObservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWildlifeObservationTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic visitor wildlife world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->BeginPlay();

	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 90.f), FRotator::ZeroRotator);
	ATargetPoint* Stag = World->SpawnActor<ATargetPoint>(FVector(1000.f, 0.f, 90.f), FRotator::ZeroRotator);
	UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
	AIslandInteractionTestPlayerController* Controller = World->SpawnActor<AIslandInteractionTestPlayerController>();
	TestNotNull(TEXT("Visitor pawn created for wildlife observation"), Visitor);
	TestNotNull(TEXT("Wildlife actor created for wildlife observation"), Stag);
	TestNotNull(TEXT("Caption widget created for wildlife observation"), Caption);
	TestNotNull(TEXT("Visitor controller created for wildlife observation"), Controller);
	if (Visitor && Stag && Caption && Controller)
	{
		Stag->Tags.Add(TEXT("IslandLife"));
		Stag->Tags.Add(TEXT("WoodlandDeer"));
		Stag->SetActorHiddenInGame(false);
		TestEqual(TEXT("The fixture is classified as a stag"), IslandInteractionUtility::GetTargetTag(Stag), FName(TEXT("WoodlandDeer")));
		TestTrue(TEXT("The visible fixture passes the same visibility gate as direct interactions"),
			IslandInteractionUtility::CanInteract(Visitor, Stag, 1800.f));
		Controller->SetFixturePawn(Visitor);
		Controller->SetCaptionWidget(Caption);
		Controller->BindFixtureInput();
		TestTrue(TEXT("The wildlife observation helper resolves the nearest visible species"),
			Controller->DescribeNearbyWildlife().Contains(TEXT("stag browsing")));
		bool bSawWildlife = false;
		FString CaptionsSeen;
		for (int32 Attempt = 0; Attempt < 8 && !bSawWildlife; ++Attempt)
		{
			TestTrue(TEXT("E remains bound during the observation cycle"), Controller->PressBoundE());
			const FString CaptionText = Caption->GetDisplayedCaption().ToString();
			CaptionsSeen += FString::Printf(TEXT("[%d] %s "), Attempt, *CaptionText);
			bSawWildlife = CaptionText.Contains(TEXT("stag browsing")) && CaptionText.Contains(TEXT("leave it undisturbed"));
		}
		TestTrue(FString::Printf(TEXT("The on-demand observation cycle includes visible wildlife beyond direct interaction range without disturbing it. Captions: %s"), *CaptionsSeen), bSawWildlife);

		Stag->SetActorHiddenInGame(true);
		bool bHiddenWildlifeWasNamed = false;
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			Controller->PressBoundE();
			bHiddenWildlifeWasNamed |= Caption->GetDisplayedCaption().ToString().Contains(TEXT("stag browsing"));
		}
		TestFalse(TEXT("Hidden wildlife is not described as visible"), bHiddenWildlifeWasNamed);

		Stag->SetActorHiddenInGame(false);
		Visitor->SetActorLocation(FVector(3000.f, 0.f, 90.f));
		bool bDistantWildlifeWasNamed = false;
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			Controller->PressBoundE();
			bDistantWildlifeWasNamed |= Caption->GetDisplayedCaption().ToString().Contains(TEXT("stag browsing"));
		}
		TestFalse(TEXT("Distant wildlife is not described as local"), bDistantWildlifeWasNamed);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
