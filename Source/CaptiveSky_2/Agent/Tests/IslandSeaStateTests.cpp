#include "Misc/AutomationTest.h"
#include "IslandSeaStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GerstnerWaterWaves.h"
#include "IslandDayNight.h"
#include "WaterBodyOceanActor.h"
#include "WaterBodyOceanComponent.h"

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
	TestEqual(TEXT("A non-finite storm sample is treated as calm"), Sea::TargetSea(NAN, 0.f), 0.f);
	TestEqual(TEXT("A non-finite wind sample is treated as calm"), Sea::TargetSea(0.f, INFINITY), 0.f);

	TestTrue(TEXT("A glassy sea needs no remark"), Sea::DescribeSea(0.f).IsEmpty() && Sea::DescribeSea(NAN).IsEmpty());
	TestTrue(TEXT("A middling sea is described as running"), Sea::DescribeSea(0.5f).Contains(TEXT("white-tipped")));
	TestTrue(TEXT("A storm sea is described as violent"), Sea::DescribeSea(1.f).Contains(TEXT("violent")));

	TestEqual(TEXT("Calm amplitude is the calm scale"), Sea::AmplitudeScale(0.f), Sea::CalmAmplitudeScale);
	TestEqual(TEXT("A full storm uses exactly the authored amplitude"), Sea::AmplitudeScale(1.f), 1.f);
	TestTrue(TEXT("Amplitude never exceeds the authored ceiling"), Sea::AmplitudeScale(3.f) <= 1.f);
	TestTrue(TEXT("Crests sharpen in a storm"), Sea::SteepnessScale(1.f) > Sea::SteepnessScale(0.f));

	const float Rising = Sea::StepSea(0.f, 1.f, 10.f);
	const float Falling = 1.f - Sea::StepSea(1.f, 0.f, 10.f);
	TestTrue(TEXT("The sea moves toward its target"), Rising > 0.f && Rising < 1.f);
	TestTrue(TEXT("Seas build faster than they settle"), Rising > Falling);
	TestEqual(TEXT("No time passing changes nothing"), Sea::StepSea(0.3f, 1.f, 0.f), 0.3f);
	TestTrue(TEXT("A non-finite current sea recovers from the calm baseline before advancing"),
		FMath::IsNearlyEqual(Sea::StepSea(NAN, 1.f, 10.f), Sea::StepSea(0.f, 1.f, 10.f), 0.0001f));
	TestEqual(TEXT("A non-finite target preserves the finite current sea"), Sea::StepSea(0.4f, NAN, 10.f), 0.4f);
	TestEqual(TEXT("A non-finite timestep cannot poison the sea"), Sea::StepSea(0.4f, 1.f, INFINITY), 0.4f);
	TestEqual(TEXT("Negative current sea is clamped before transition"), Sea::StepSea(-1.f, 0.f, 10.f), 0.f);
	TestTrue(TEXT("Out-of-range sea targets remain bounded"), Sea::StepSea(0.2f, 4.f, 10.f) <= 1.f);
	TestEqual(TEXT("Invalid amplitude input resolves to the calm scale"), Sea::AmplitudeScale(NAN), Sea::CalmAmplitudeScale);
	TestEqual(TEXT("Invalid steepness input resolves to the calm scale"), Sea::SteepnessScale(INFINITY), Sea::CalmSteepnessScale);

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandSeaStateSubsystem* Subsystem = World->GetSubsystem<UIslandSeaStateSubsystem>();
	if (TestNotNull(TEXT("The sea-state subsystem exists in a game world"), Subsystem))
	{
		TestFalse(TEXT("Nothing is bound without an ocean"), Subsystem->IsBound());
		TestFalse(TEXT("No ocean tide target is bound without an ocean"), Subsystem->IsOceanBound());
		TestFalse(TEXT("A missing wave set is refused"), Subsystem->BindWaves(nullptr));

		AWaterBodyOcean* OceanActor = World->SpawnActor<AWaterBodyOcean>();
		AIslandDayNight* IslandClock = World->SpawnActor<AIslandDayNight>();
		UWaterBodyOceanComponent* OceanComponent = OceanActor
			? Cast<UWaterBodyOceanComponent>(OceanActor->GetWaterBodyComponent()) : nullptr;
		TestNotNull(TEXT("The ocean tide fixture has a WaterBodyOcean component"), OceanComponent);
		TestNotNull(TEXT("The ocean tide fixture has an Island clock"), IslandClock);
		if (OceanComponent && IslandClock)
		{
			OceanComponent->SetHeightOffset(4.5f);
			Subsystem->IslandClock = IslandClock;
			const FTransform AuthoredOceanTransform = OceanActor->GetActorTransform();
			const float AuthoredSurfaceZ = AuthoredOceanTransform.GetLocation().Z + OceanComponent->GetHeightOffset();
			IslandClock->CurrentHour = 3.105f;
			IslandClock->DayNumber = 1;
			TestTrue(TEXT("The sea-state subsystem binds the ocean"), Subsystem->BindOcean(OceanActor));
			TestTrue(TEXT("Spring high water offsets, rather than replaces, the authored ocean level"),
				FMath::IsNearlyEqual(OceanActor->GetActorLocation().Z + OceanComponent->GetHeightOffset(),
					AuthoredSurfaceZ + 14.f * AIslandDayNight::TideLevel(1, 3.105f), 0.01f));

			IslandClock->CurrentHour = 9.315f;
			Subsystem->ApplyOceanTideOffset();
			const float ExpectedLowWaterOffset = 4.5f + 14.f * AIslandDayNight::TideLevel(1, 9.315f);
			TestTrue(*FString::Printf(TEXT("Low water uses the shared semidiurnal tide curve (actual %.3f cm, expected %.3f cm)"),
				OceanActor->GetActorLocation().Z + OceanComponent->GetHeightOffset() - AuthoredOceanTransform.GetLocation().Z,
				ExpectedLowWaterOffset),
				FMath::IsNearlyEqual(OceanActor->GetActorLocation().Z + OceanComponent->GetHeightOffset(),
					AuthoredSurfaceZ + 14.f * AIslandDayNight::TideLevel(1, 9.315f), 0.01f));

			Subsystem->RestoreOceanTideOffset();
			TestEqual(TEXT("World teardown restores the authored ocean height offset"), OceanComponent->GetHeightOffset(), 4.5f);
			TestTrue(TEXT("World teardown restores the authored ocean transform"), OceanActor->GetActorTransform().Equals(AuthoredOceanTransform, 0.001f));
		}

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
