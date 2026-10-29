#include "Misc/AutomationTest.h"
#include "IslandTidepoolPeriwinkle.h"
#include "IslandTidepoolMinnows.h"
#include "IslandTideglassSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleWildlifeAttentionTest,
	"CaptiveSky2.Agent.TidepoolPeriwinkleWildlifeAttention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleWildlifeAttentionTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	TestTrue(TEXT("A nearby frog or dragonfly can attract a quiet glance"),
		Periwinkle::ShouldAttendWildlife(180.f, 45.f));
	TestTrue(TEXT("The attention radius boundary is inclusive"),
		Periwinkle::ShouldAttendWildlife(220.f, 0.f));
	TestFalse(TEXT("Distant wildlife does not draw the snail's gaze"),
		Periwinkle::ShouldAttendWildlife(220.1f, 0.f));
	TestFalse(TEXT("Wildlife far below the tidepool rock is ignored"),
		Periwinkle::ShouldAttendWildlife(120.f, -80.1f));
	TestTrue(TEXT("A low-flying dragonfly remains within the gaze band"),
		Periwinkle::ShouldAttendWildlife(120.f, 220.f));
	TestFalse(TEXT("Wildlife above the small gaze band is ignored"),
		Periwinkle::ShouldAttendWildlife(120.f, 220.1f));
	TestFalse(TEXT("Invalid distances cannot draw the gaze"),
		Periwinkle::ShouldAttendWildlife(std::numeric_limits<float>::infinity(), 0.f));
	TestFalse(TEXT("Invalid heights cannot draw the gaze"),
		Periwinkle::ShouldAttendWildlife(120.f, std::numeric_limits<float>::quiet_NaN()));

	Periwinkle* Snail = NewObject<Periwinkle>(GetTransientPackage());
	if (!Snail) return false;
	const FVector Origin = Snail->GetActorLocation();
	const float FrontLeftYaw = Snail->CalculateAttentionYaw(Origin + FVector(120.f, -120.f, 0.f));
	TestTrue(TEXT("The eye stalks turn toward a nearby animal to the left"), FrontLeftYaw < -20.f);
	const float BehindYaw = Snail->CalculateAttentionYaw(Origin + FVector(-120.f, 30.f, 0.f));
	TestEqual(TEXT("A nearby animal behind the shell does not cause an impossible backward gaze"), BehindYaw, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleMinnowAttentionTest,
	"CaptiveSky2.Agent.TidepoolPeriwinkleMinnowAttention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleMinnowAttentionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Periwinkle-minnow attention fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for the attention fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandTidepoolPeriwinkle* Snail = World->SpawnActor<AIslandTidepoolPeriwinkle>(
		FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator, Spawn);
	AIslandTidepoolMinnows* Minnows = World->SpawnActor<AIslandTidepoolMinnows>(
		FVector(120.f, -120.f, 40.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Periwinkle spawned beside the tidepool"), Snail) ||
		!TestNotNull(TEXT("Minnow school spawned in the tidepool"), Minnows) || Minnows->Fish.IsEmpty())
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();
	Minnows->Fish[0]->SetWorldLocation(FVector(120.f, -120.f, 48.f));
	Snail->RavenCheckRemaining = 0.f;
	Snail->Tick(0.2f);
	TestFalse(TEXT("A calm, submerged minnow school does not attract an arbitrary gaze"),
		Snail->WildlifeAttentionTarget.IsValid());

	Minnows->SurfaceBreakHopFishIndex = 0;
	Minnows->SurfaceBreakHopRemaining = 0.54f;
	Snail->RavenCheckRemaining = 0.f;
	Snail->Tick(0.2f);
	TestTrue(TEXT("A real active surface break attracts the snail's eye stalks"), Snail->AttentionYaw < 0.f);
	TestTrue(TEXT("The glance is aimed at the breaking fish, not the school actor's center"),
		Snail->WildlifeAttentionLocation.Equals(Minnows->GetSurfaceBreakWorldLocation(), 0.01f));
	TestTrue(TEXT("The glance lingers briefly after the hop ends"),
		Snail->MinnowAttentionRemaining > 0.f && Snail->MinnowAttentionRemaining <= Snail->MinnowAttentionHoldSeconds);

	Minnows->SurfaceBreakHopRemaining = 0.f;
	Minnows->SurfaceBreakHopFishIndex = INDEX_NONE;
	Snail->RavenCheckRemaining = 0.f;
	Snail->Tick(0.2f);
	TestTrue(TEXT("An ended hop receives only the bounded attention linger"), Snail->MinnowAttentionRemaining > 0.f);
	Snail->RavenCheckRemaining = 0.f;
	Snail->Tick(Snail->MinnowAttentionHoldSeconds + 0.2f);
	TestEqual(TEXT("The minnow glance expires without lingering as stale attention"), Snail->MinnowAttentionRemaining, 0.f);
	TestTrue(TEXT("Watching the school never carries the snail off its small home patch"),
		FVector::Dist2D(Snail->GetActorLocation(), Snail->HomeLocation) <= Snail->GrazingMaxExcursionCm + 0.1f);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleCrabCueTest, "CaptiveSky2.Agent.TidepoolPeriwinkleCrabCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleCrabCueTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A nearby scurrying crab briefly startles the periwinkle"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 120.f, 20.f));
	TestTrue(TEXT("The edge of the threat radius remains inclusive"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 175.f, 0.f));
	TestFalse(TEXT("A calm crab does not repeatedly disturb its neighbor"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(false, 60.f, 0.f));
	TestFalse(TEXT("A crab outside close range does not trigger a tuck"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 176.f, 0.f));
	TestFalse(TEXT("A crab far above or below the shoreline does not trigger a tuck"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, 100.f, 101.f));
	TestFalse(TEXT("Non-finite crab distance is ignored"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, std::numeric_limits<float>::infinity(), 0.f));
	TestFalse(TEXT("Negative crab distance is ignored"),
		AIslandTidepoolPeriwinkle::ShouldTuckForCrab(true, -1.f, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleRatCueTest, "CaptiveSky2.Agent.TidepoolPeriwinkleRatCue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleRatCueTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	TestTrue(TEXT("A close rat approach briefly startles the snail"),
		Periwinkle::ShouldTuckForRat(130.f, 15.f));
	TestTrue(TEXT("The rat cue radius edge is inclusive"),
		Periwinkle::ShouldTuckForRat(190.f, 0.f));
	TestFalse(TEXT("A rat still on the broader shore does not startle the snail"),
		Periwinkle::ShouldTuckForRat(191.f, 0.f));
	TestFalse(TEXT("A rat far above or below the rock does not trigger the cue"),
		Periwinkle::ShouldTuckForRat(120.f, 121.f));
	TestFalse(TEXT("Negative distance is rejected"),
		Periwinkle::ShouldTuckForRat(-1.f, 0.f));
	TestFalse(TEXT("Non-finite distance is rejected"),
		Periwinkle::ShouldTuckForRat(std::numeric_limits<float>::infinity(), 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleGrazingPathTest, "CaptiveSky2.Agent.TidepoolPeriwinkleGrazingPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleGrazingPathTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	const FVector2D AtStart = Periwinkle::GrazingOffset(0.f, 0.f);
	TestTrue(TEXT("The grazing path starts at its home point"), AtStart.IsNearlyZero());

	const FVector2D MidCycle = Periwinkle::GrazingOffset(30.f, 0.f);
	TestTrue(TEXT("The snail progresses across its patch while active"), MidCycle.Size() > 5.f);
	TestTrue(TEXT("Mid-cycle movement remains local"), MidCycle.Size() <= Periwinkle::GrazingMaxExcursionCm);

	const FVector2D FullCycle = Periwinkle::GrazingOffset(120.f, 0.f);
	TestTrue(TEXT("The closed path returns to its starting point"), FullCycle.Size() < 0.01f);

	bool bAllSamplesBounded = true;
	for (int32 Sample = 0; Sample <= 240; ++Sample)
	{
		const FVector2D Offset = Periwinkle::GrazingOffset(Sample * 0.5f, 1.37f);
		bAllSamplesBounded &= Offset.Size() <= Periwinkle::GrazingMaxExcursionCm + 0.001f;
	}
	TestTrue(TEXT("Every sampled step stays inside the bounded home patch"), bAllSamplesBounded);

	const FVector2D Invalid = Periwinkle::GrazingOffset(std::numeric_limits<float>::quiet_NaN(), 0.f);
	TestTrue(TEXT("Non-finite time cannot move the snail"), Invalid.IsNearlyZero());
	TestEqual(TEXT("Active grazing advances at full openness"), Periwinkle::AdvanceGrazingTime(4.f, 2.f, 0.f), 6.f);
	TestEqual(TEXT("A fully tucked snail pauses its graze path"), Periwinkle::AdvanceGrazingTime(4.f, 2.f, 1.f), 4.f);
	TestEqual(TEXT("Partial shelter proportionally slows the path"), Periwinkle::AdvanceGrazingTime(4.f, 2.f, 0.5f), 5.f);
	TestEqual(TEXT("Negative frame time cannot reverse the path"), Periwinkle::AdvanceGrazingTime(4.f, -2.f, 0.f), 4.f);
	TestEqual(TEXT("Invalid rest state resets safely"),
		Periwinkle::AdvanceGrazingTime(4.f, 2.f, std::numeric_limits<float>::quiet_NaN()), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleNocturnalGrazingTest,
	"CaptiveSky2.Agent.TidepoolPeriwinkleNocturnalGrazing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleNocturnalGrazingTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	const float MiddayActivity = Periwinkle::GrazingActivityForHour(12.f);
	const float NightActivity = Periwinkle::GrazingActivityForHour(0.f);
	const float DuskActivity = Periwinkle::GrazingActivityForHour(18.5f);

	TestTrue(TEXT("The snail still grazes during daytime"), MiddayActivity > 0.f);
	TestTrue(TEXT("Nighttime grazing is more active than midday"), NightActivity > MiddayActivity);
	TestTrue(TEXT("Twilight activity transitions smoothly between day and night"),
		DuskActivity > MiddayActivity && DuskActivity < NightActivity);
	TestTrue(TEXT("Activity respects the clock's wrapped hour"),
		FMath::IsNearlyEqual(Periwinkle::GrazingActivityForHour(24.f), NightActivity));
	TestTrue(TEXT("Invalid clock time safely preserves full grazing activity"),
		FMath::IsNearlyEqual(Periwinkle::GrazingActivityForHour(std::numeric_limits<float>::quiet_NaN()), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleIntertidalZonationTest,
	"CaptiveSky2.Agent.TidepoolPeriwinkleIntertidalZonation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleIntertidalZonationTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	constexpr float SeaLevel = 1000.f;
	const float Ebb = -UIslandTideglassSubsystem::MaximumTideOffsetCm;
	const float SameLevel = Periwinkle::LocalWaterlineOffsetCm(Ebb, SeaLevel, SeaLevel);
	const float HighRock = Periwinkle::LocalWaterlineOffsetCm(Ebb, SeaLevel + 20.f, SeaLevel);
	const float LowRock = Periwinkle::LocalWaterlineOffsetCm(Ebb, SeaLevel - 20.f, SeaLevel);
	TestEqual(TEXT("A snail at mean water level keeps the existing tide response"), SameLevel, Ebb);
	TestTrue(TEXT("A higher rock experiences a more exposed local ebb"), HighRock < SameLevel);
	TestTrue(TEXT("A lower rock remains wetter through the same ebb"), LowRock > SameLevel);
	TestTrue(TEXT("The higher snail tucks more than the lower snail"),
		Periwinkle::ExposureRestAmount(HighRock) > Periwinkle::ExposureRestAmount(LowRock));
	TestTrue(TEXT("The lower snail keeps a wetter shell than the higher snail"),
		Periwinkle::WetShellAmount(LowRock, 0.f) > Periwinkle::WetShellAmount(HighRock, 0.f));
	TestEqual(TEXT("Missing shore height falls back to the global tide offset"),
		Periwinkle::LocalWaterlineOffsetCm(Ebb, std::numeric_limits<float>::quiet_NaN(), SeaLevel), Ebb);
	TestEqual(TEXT("A non-finite tide offset resolves safely"),
		Periwinkle::LocalWaterlineOffsetCm(std::numeric_limits<float>::infinity(), SeaLevel, SeaLevel), 0.f);
	TestTrue(TEXT("Extreme finite heights remain bounded to a finite response"),
		FMath::IsFinite(Periwinkle::LocalWaterlineOffsetCm(Ebb, -std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandPeriwinkleWindExposureTest,
	"CaptiveSky2.Agent.TidepoolPeriwinkleWindExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandPeriwinkleWindExposureTest::RunTest(const FString& Parameters)
{
	using Periwinkle = AIslandTidepoolPeriwinkle;
	TestEqual(TEXT("Still air leaves the snail fully open"), Periwinkle::WindRestAmount(0.f), 0.f);
	TestEqual(TEXT("Negative wind samples cannot create an exposure response"), Periwinkle::WindRestAmount(-40.f), 0.f);
	TestTrue(TEXT("An ordinary Island breeze only produces a subtle tuck"),
		Periwinkle::WindRestAmount(180.f) > 0.f && Periwinkle::WindRestAmount(180.f) < 0.2f);
	TestTrue(TEXT("Storm-strength wind causes a stronger but still partial tuck"),
		Periwinkle::WindRestAmount(330.f) > 0.35f && Periwinkle::WindRestAmount(330.f) < 0.4f);
	TestEqual(TEXT("The wind response saturates instead of over-tucking"), Periwinkle::WindRestAmount(1000.f), 0.38f);
	TestEqual(TEXT("Non-finite wind cannot produce a non-finite pose"),
		Periwinkle::WindRestAmount(std::numeric_limits<float>::infinity()), 0.f);
	return true;
}
