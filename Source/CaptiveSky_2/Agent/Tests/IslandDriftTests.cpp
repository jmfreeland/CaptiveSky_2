#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IslandDrift.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandDriftTest, "CaptiveSky2.Agent.IslandDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandDriftTest::RunTest(const FString& Parameters)
{
	using Drift = AIslandDrift;
	TestEqual(TEXT("Dead calm lifts only a few seeds"), Drift::DesiredCount(0.f, 0.f, 1.f), 6);
	TestEqual(TEXT("A gale fills the air"), Drift::DesiredCount(400.f, 0.f, 1.f), Drift::MaxMotes);
	TestTrue(TEXT("More wind lifts more"), Drift::DesiredCount(80.f, 0.f, 1.f) < Drift::DesiredCount(160.f, 0.f, 1.f));
	TestEqual(TEXT("Rain holds the leaves down"), Drift::DesiredCount(400.f, 1.f, 1.f), 0);
	TestEqual(TEXT("Night holds them too"), Drift::DesiredCount(400.f, 0.f, 0.f), 0);
	TestEqual(TEXT("Negative wind is clamped"), Drift::DesiredCount(-50.f, 0.f, 1.f), 6);

	TestTrue(TEXT("Seeds lift before petals"), Drift::LiftWindSpeed(EIslandDriftKind::Seed) < Drift::LiftWindSpeed(EIslandDriftKind::Petal));
	TestTrue(TEXT("Petals lift before leaves"), Drift::LiftWindSpeed(EIslandDriftKind::Petal) < Drift::LiftWindSpeed(EIslandDriftKind::DryLeaf));
	TestTrue(TEXT("Dry leaves sink slower than green"), Drift::SinkSpeed(EIslandDriftKind::DryLeaf) < Drift::SinkSpeed(EIslandDriftKind::GreenLeaf));

	int32 Seen[static_cast<int32>(EIslandDriftKind::Count)] = {};
	for (int32 Slot = 0; Slot < Drift::MaxMotes; ++Slot) ++Seen[static_cast<int32>(Drift::KindForSlot(Slot))];
	for (int32 Kind = 0; Kind < static_cast<int32>(EIslandDriftKind::Count); ++Kind)
		TestTrue(TEXT("Every kind has slots"), Seen[Kind] > 0);

	TestTrue(TEXT("Still air hangs a few seeds"), Drift::DescribeDrift(0.f, 0.f, 1.f).Contains(TEXT("seeds")));
	TestTrue(TEXT("A breeze carries petals"), Drift::DescribeDrift(80.f, 0.f, 1.f).Contains(TEXT("Petals")));
	TestTrue(TEXT("A wind tumbles leaves"), Drift::DescribeDrift(160.f, 0.f, 1.f).Contains(TEXT("tumble")));
	TestTrue(TEXT("A gale flings them"), Drift::DescribeDrift(400.f, 0.f, 1.f).Contains(TEXT("flinging")));
	TestTrue(TEXT("Nothing drifts in rain"), Drift::DescribeDrift(400.f, 1.f, 1.f).IsEmpty());
	TestTrue(TEXT("Nothing is seen at night"), Drift::DescribeDrift(400.f, 0.f, 0.f).IsEmpty());

	const FVector Wind(200.f, 0.f, 0.f);
	FVector Velocity = FVector::ZeroVector;
	for (int32 Step = 0; Step < 600; ++Step) Velocity = Drift::StepVelocity(Velocity, Wind, EIslandDriftKind::DryLeaf, 0.016f);
	TestTrue(TEXT("A leaf comes up to the wind speed"), FMath::Abs(Velocity.X - 200.f) < 2.f);
	TestTrue(TEXT("It keeps sinking"), FMath::IsNearlyEqual(Velocity.Z, -Drift::SinkSpeed(EIslandDriftKind::DryLeaf), 1.f));
	const FVector Gust = Drift::StepVelocity(FVector::ZeroVector, Wind, EIslandDriftKind::GreenLeaf, 0.1f);
	const FVector GustSeed = Drift::StepVelocity(FVector::ZeroVector, Wind, EIslandDriftKind::Seed, 0.1f);
	TestTrue(TEXT("A seed answers a gust faster than a green leaf"), GustSeed.X > Gust.X);

	if (!TestNotNull(TEXT("Engine is available"), GEngine)) return false;
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Drift fixture world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (Drift* Actor = World->SpawnActor<Drift>(Spawn))
	{
		const FVector Viewer(1000.f, -500.f, 300.f);
		const TFunctionRef<FVector(const FVector&)> Breeze = [](const FVector&) { return FVector(180.f, 0.f, 0.f); };
		Actor->Advance(0.016f, Viewer, Breeze, 0.f, 1.f);
		TestTrue(TEXT("A breeze lifts motes on the first frame"), Actor->GetActiveCount() > 6);

		for (int32 Step = 0; Step < 120; ++Step) Actor->Advance(0.016f, Viewer, Breeze, 0.f, 1.f);
		bool bInside = true;
		for (const FIslandDriftMote& Mote : Actor->GetMotes())
			if (Mote.bActive && FVector::Dist2D(Mote.Position, Viewer) > Drift::Radius * 1.2f) bInside = false;
		TestTrue(TEXT("Motes stay around the viewer"), bInside);
		TestTrue(TEXT("The population is held up"), Actor->GetActiveCount() > 6);

		const TFunctionRef<FVector(const FVector&)> Storm = [](const FVector&) { return FVector(400.f, 0.f, 0.f); };
		for (int32 Step = 0; Step < 100; ++Step) Actor->Advance(0.1f, Viewer, Storm, 1.f, 1.f);
		TestEqual(TEXT("Heavy rain clears the air"), Actor->GetActiveCount(), 0);
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
