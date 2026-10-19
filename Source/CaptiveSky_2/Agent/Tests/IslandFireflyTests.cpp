#include "Misc/AutomationTest.h"
#include "Agent/IslandDayNight.h"
#include "Agent/IslandFirefly.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFireflyGlowColorVariationTest,
	"CaptiveSky2.Agent.FireflyGlowColorVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFireflyGlowColorVariationTest::RunTest(const FString& Parameters)
{
	const FLinearColor First = AIslandFirefly::GlowColorVariationForSeed(42);
	const FLinearColor SameSeed = AIslandFirefly::GlowColorVariationForSeed(42);
	TestTrue(TEXT("A location seed produces a repeatable glow tint"), First.Equals(SameSeed));
	TestTrue(TEXT("Hue multipliers stay subtle and within a safe light range"),
		First.R >= 0.8f && First.R <= 1.f &&
		First.G >= 0.8f && First.G <= 1.f &&
		First.B >= 0.7f && First.B <= 1.f);

	TArray<FLinearColor> DistinctTints;
	for (int32 Seed = -32; Seed <= 32; ++Seed)
	{
		const FLinearColor Tint = AIslandFirefly::GlowColorVariationForSeed(Seed);
		if (!DistinctTints.ContainsByPredicate([&Tint](const FLinearColor& Existing)
			{ return Existing.Equals(Tint); }))
		{
			DistinctTints.Add(Tint);
		}
	}
	TestTrue(TEXT("A small group of fireflies can receive more than one natural tint"), DistinctTints.Num() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFireflyPulseClustersTest,
	"CaptiveSky2.Agent.FireflyPulseClusters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFireflyPulseClustersTest::RunTest(const FString& Parameters)
{
	const float PairCoupling = AIslandFirefly::PulseCouplingScale(0.f, 0.f, 1);
	const float SmallClusterCoupling = AIslandFirefly::PulseCouplingScale(0.f, 0.f, 2);
	TestTrue(TEXT("A nearby pair develops a restrained pulse coupling on a calm night"),
		PairCoupling > 0.f && PairCoupling < 0.03f);
	TestTrue(TEXT("A few nearby fireflies gently strengthen the shared rhythm"),
		SmallClusterCoupling > PairCoupling && SmallClusterCoupling <= 0.055f);
	TestTrue(TEXT("No neighbors means fully independent pulses"),
		FMath::IsNearlyZero(AIslandFirefly::PulseCouplingScale(0.f, 0.f, 0)));
	TestTrue(TEXT("Daytime and heavy rain suppress pulse coupling"),
		FMath::IsNearlyZero(AIslandFirefly::PulseCouplingScale(12.f, 0.f, 4)) &&
		FMath::IsNearlyZero(AIslandFirefly::PulseCouplingScale(0.f, 1.f, 4)));
	TestTrue(TEXT("Dusk introduces coupling gradually instead of switching it on"),
		AIslandFirefly::PulseCouplingScale(18.5f, 0.f, 3) > 0.f &&
		AIslandFirefly::PulseCouplingScale(18.5f, 0.f, 3) < AIslandFirefly::PulseCouplingScale(0.f, 0.f, 3));
	TestTrue(TEXT("A phase nudge follows the shortest route across the pulse-cycle seam"),
		AIslandFirefly::EvaluatePulsePhaseNudge(6.2f, 0.1f, 0.1f) > 0.f &&
		AIslandFirefly::EvaluatePulsePhaseNudge(0.1f, 6.2f, 0.1f) < 0.f);
	TestTrue(TEXT("Phase nudges remain bounded even for opposite phases or excessive coupling"),
		FMath::IsNearlyEqual(FMath::Abs(AIslandFirefly::EvaluatePulsePhaseNudge(0.f, PI, 1.f)), 0.055f) &&
		FMath::IsNearlyEqual(FMath::Abs(AIslandFirefly::EvaluatePulsePhaseNudge(0.f, PI, 10.f)), 0.055f));
	TestTrue(TEXT("Invalid phases or coupling cannot destabilize the glow"),
		FMath::IsNearlyZero(AIslandFirefly::EvaluatePulsePhaseNudge(
			std::numeric_limits<float>::quiet_NaN(), 0.f, 0.05f)) &&
		FMath::IsNearlyZero(AIslandFirefly::PulseCouplingScale(
			0.f, std::numeric_limits<float>::quiet_NaN(), 3)));

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Firefly pulse fixture world created"), World)) return false;
	if (!TestNotNull(TEXT("Engine is available for the pulse fixture"), GEngine))
	{
		World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AIslandFirefly* First = World->SpawnActor<AIslandFirefly>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
	AIslandFirefly* Second = World->SpawnActor<AIslandFirefly>(FVector(120.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Day-night clock spawned"), Clock) ||
		!TestNotNull(TEXT("First firefly spawned"), First) ||
		!TestNotNull(TEXT("Nearby firefly spawned"), Second))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	Clock->CurrentHour = 0.f;
	First->DayNight = Clock;
	First->GlowPhase = 0.f;
	First->PulseRate = 0.82f;
	First->Phase = 1.25f;
	Second->GlowPhase = 2.f;
	Second->PulseRate = 1.18f;
	const float InitialPhaseDifference = FMath::Abs(FMath::FindDeltaAngleRadians(First->GlowPhase, Second->GlowPhase));
	const float MovementPhaseBefore = First->Phase;
	for (int32 Step = 0; Step < 90; ++Step)
		First->UpdatePulseCluster(0.f, 0.f);
	const float FinalPhaseDifference = FMath::Abs(FMath::FindDeltaAngleRadians(First->GlowPhase, Second->GlowPhase));
	TestTrue(TEXT("A nearby night pair slowly brings their glow phases into closer agreement"),
		FinalPhaseDifference < InitialPhaseDifference);
	TestTrue(TEXT("Pulse entrainment leaves the independent flight path phase unchanged"),
		FMath::IsNearlyEqual(First->Phase, MovementPhaseBefore));
	TestTrue(TEXT("A cluster draws pulse rate toward its nearby members without teleporting frequency"),
		First->PulseRate > 0.82f && First->PulseRate < Second->PulseRate);

	const float PhaseBeforeDawnCheck = First->GlowPhase;
	Clock->CurrentHour = 12.f;
	First->UpdatePulseCluster(0.f, 0.f);
	TestTrue(TEXT("Daylight leaves the accumulated local pulse phase untouched"),
		FMath::IsNearlyEqual(First->GlowPhase, PhaseBeforeDawnCheck));
	Clock->CurrentHour = 0.f;
	First->UpdatePulseCluster(0.f, 1.f);
	TestTrue(TEXT("Heavy rain suspends the coupling rather than forcing a shared display"),
		FMath::IsNearlyEqual(First->GlowPhase, PhaseBeforeDawnCheck));
	Second->SetActorLocation(FVector(1000.f, 0.f, 100.f));
	First->UpdatePulseCluster(0.f, 0.f);
	TestTrue(TEXT("Distant fireflies remain independent"),
		FMath::IsNearlyEqual(First->GlowPhase, PhaseBeforeDawnCheck));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
