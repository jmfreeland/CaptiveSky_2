#include "Agent/IslandCommonFrog.h"
#include "Agent/IslandTideglassDragonfly.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCommonFrogDragonflyTests,
	"CaptiveSky2.Agent.Ecology.CommonFrogDragonflyAttention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandCommonFrogDragonflyTests::RunTest(const FString& Parameters)
{
	using Frog = AIslandCommonFrog;
	TestTrue(TEXT("A low nearby dragonfly is within the frog's attention range"),
		Frog::ShouldWatchLowDragonfly(280.f, 45.f));
	TestFalse(TEXT("A dragonfly outside the horizontal attention range is ignored"),
		Frog::ShouldWatchLowDragonfly(281.f, 100.f));
	TestFalse(TEXT("A dragonfly below eye level is not treated as a hovering cue"),
		Frog::ShouldWatchLowDragonfly(100.f, 44.f));
	TestFalse(TEXT("A high-flying dragonfly remains outside the frog's attention band"),
		Frog::ShouldWatchLowDragonfly(100.f, 221.f));
	TestFalse(TEXT("Invalid distances are ignored"),
		Frog::ShouldWatchLowDragonfly(std::numeric_limits<float>::quiet_NaN(), 100.f));

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Frog/dragonfly fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for frog/dragonfly fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandCommonFrog* FrogActor = World->SpawnActor<AIslandCommonFrog>(
		FVector(1000.f, 2000.f, 300.f), FRotator::ZeroRotator, Spawn);
	AIslandTideglassDragonfly* Dragonfly = World->SpawnActor<AIslandTideglassDragonfly>(
		FVector(1000.f, 2200.f, 400.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Common Frog spawned"), FrogActor) ||
		!TestNotNull(TEXT("Tideglass dragonfly spawned"), Dragonfly))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();

	FrogActor->DragonflyCheckRemaining = 0.f;
	FrogActor->Tick(0.25f);
	TestTrue(TEXT("A nearby low-hovering dragonfly triggers one brief attention cue"),
		FrogActor->DragonflyAttentionRemaining > 1.f && FrogActor->DragonflyPresenceLatch.Get() == Dragonfly);
	TestTrue(TEXT("The frog turns toward the passing dragonfly"), FrogActor->GetActorRotation().Yaw > 30.f);
	FrogActor->Tick(1.5f);
	TestTrue(TEXT("The frog returns to its resting orientation after the cue"),
		FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(FrogActor->GetActorRotation().Yaw, 0.f)));
	TestTrue(TEXT("The same pass cannot trigger another attention cue"),
		FMath::IsNearlyZero(FrogActor->DragonflyAttentionRemaining) && FrogActor->DragonflyAttentionCooldownRemaining > 0.f);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
