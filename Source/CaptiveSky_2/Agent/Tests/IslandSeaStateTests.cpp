#include "Misc/AutomationTest.h"
#include "IslandSeaStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GerstnerWaterWaves.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandSeaStateTest, "CaptiveSky2.Agent.IslandSeaState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandSeaStateTest::RunTest(const FString& Parameters)
{
	using Sea = UIslandSeaStateSubsystem;

	TestEqual(TEXT("No weather is a glassy sea"), Sea::TargetSea(0.f, 0.f), 0.f);
	TestEqual(TEXT("A full storm in full wind is capped at 1"), Sea::TargetSea(1.f, 1.f), 1.f);
	TestTrue(TEXT("Storm alone raises the sea"), Sea::TargetSea(1.f, 0.f) > Sea::TargetSea(0.f, 0.f));
	TestTrue(TEXT("Wind alone raises the sea"), Sea::TargetSea(0.f, 0.8f) > Sea::TargetSea(0.f, 0.2f));
	TestEqual(TEXT("Out-of-range inputs are clamped"), Sea::TargetSea(5.f, 5.f), 1.f);

	TestEqual(TEXT("Calm amplitude is the calm scale"), Sea::AmplitudeScale(0.f), Sea::CalmAmplitudeScale);
	TestEqual(TEXT("A full storm uses exactly the authored amplitude"), Sea::AmplitudeScale(1.f), 1.f);
	TestTrue(TEXT("Amplitude never exceeds the authored ceiling"), Sea::AmplitudeScale(3.f) <= 1.f);
	TestTrue(TEXT("Crests sharpen in a storm"), Sea::SteepnessScale(1.f) > Sea::SteepnessScale(0.f));

	const float Rising = Sea::StepSea(0.f, 1.f, 10.f);
	const float Falling = 1.f - Sea::StepSea(1.f, 0.f, 10.f);
	TestTrue(TEXT("The sea moves toward its target"), Rising > 0.f && Rising < 1.f);
	TestTrue(TEXT("Seas build faster than they settle"), Rising > Falling);
	TestEqual(TEXT("No time passing changes nothing"), Sea::StepSea(0.3f, 1.f, 0.f), 0.3f);

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandSeaStateSubsystem* Subsystem = World->GetSubsystem<UIslandSeaStateSubsystem>();
	if (TestNotNull(TEXT("The sea-state subsystem exists in a game world"), Subsystem))
	{
		TestFalse(TEXT("Nothing is bound without an ocean"), Subsystem->IsBound());
		TestFalse(TEXT("A missing wave set is refused"), Subsystem->BindWaves(nullptr));

		UGerstnerWaterWaves* Waves = NewObject<UGerstnerWaterWaves>(GetTransientPackage());
		UGerstnerWaterWaveGeneratorSimple* Generator = NewObject<UGerstnerWaterWaveGeneratorSimple>(Waves);
		Generator->MinAmplitude = 10.f;
		Generator->MaxAmplitude = 100.f;
		Generator->SmallWaveSteepness = 0.5f;
		Generator->LargeWaveSteepness = 0.9f;
		Generator->NumWaves = 8;
		Waves->GerstnerWaveGenerator = Generator;

		if (TestTrue(TEXT("A simple Gerstner generator binds"), Subsystem->BindWaves(Waves)))
		{
			Subsystem->ApplySea(0.f);
			TestEqual(TEXT("Calm lowers the maximum amplitude"), Generator->MaxAmplitude, 100.f * Sea::CalmAmplitudeScale);
			TestEqual(TEXT("Calm lowers the minimum amplitude"), Generator->MinAmplitude, 10.f * Sea::CalmAmplitudeScale);
			const float CalmHeight = Waves->GetMaxWaveHeight();
			Subsystem->ApplySea(1.f);
			TestEqual(TEXT("A storm restores the authored amplitude"), Generator->MaxAmplitude, 100.f);
			TestTrue(TEXT("Steepness stays within 0..1"), Generator->LargeWaveSteepness <= 1.f && Generator->SmallWaveSteepness <= 1.f);
			TestTrue(TEXT("A storm sharpens the small waves"), Generator->SmallWaveSteepness > 0.5f);
			TestTrue(TEXT("The recomputed waves are taller in a storm"), Waves->GetMaxWaveHeight() > CalmHeight);
			Subsystem->RestoreWaves();
			TestFalse(TEXT("Restoring unbinds"), Subsystem->IsBound());
			TestEqual(TEXT("The authored steepness comes back"), Generator->LargeWaveSteepness, 0.9f);
			TestEqual(TEXT("The authored amplitude comes back"), Generator->MinAmplitude, 10.f);
		}
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
