#include "Agent/IslandTideglassDragonfly.h"
#include "Agent/IslandListeningStonesChime.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassDragonflyListeningStonesTest,
	"CaptiveSky2.Agent.Ecology.DragonflyListeningStonesResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassDragonflyListeningStonesTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic ecology world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	AIslandTideglassDragonfly* Dragonfly = World->SpawnActor<AIslandTideglassDragonfly>(
		FVector(180.f, 0.f, 120.f), FRotator::ZeroRotator);
	AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(
		FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Dragonfly actor spawned"), Dragonfly) ||
		!TestNotNull(TEXT("Listening Stones chime actor spawned"), Chime))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	Dragonfly->SetColorVariant(1);
	World->BeginPlay();
	Chime->BeginChime(0.f);
	Dragonfly->CheckForNearbyListeningStonesChime();
	TestTrue(TEXT("An audible Listening Stones tone draws the dragonfly into a brief hover"),
		Dragonfly->LastHeardListeningStonesChime.Get() == Chime && Dragonfly->ListeningStonesInterestRemaining > 0.f);
	TestTrue(TEXT("The response is finite and temporary"),
		Dragonfly->ListeningStonesInterestRemaining > 2.5f && Dragonfly->ListeningStonesInterestCooldownRemaining == 18.f);
	TestTrue(TEXT("Color morph selects a separated station above the stones"),
		FMath::IsNearlyEqual(Dragonfly->ListeningStonesInterestLocation.Z, 125.f) &&
		FMath::IsNearlyEqual(Dragonfly->ListeningStonesInterestLocation.Size2D(), 58.f, 0.01f));
	TestTrue(TEXT("The visit fades smoothly rather than snapping on or off"),
		Dragonfly->GetListeningStonesInterestAlpha() == 0.f);
	Dragonfly->CheckForNearbyListeningStonesChime();
	TestTrue(TEXT("One chime cannot repeatedly retrigger the same dragonfly"),
		FMath::IsNearlyEqual(Dragonfly->ListeningStonesInterestRemaining, 2.6f));
	const FVector BeforeHover = Dragonfly->GetActorLocation();
	Dragonfly->Tick(0.9f);
	TestTrue(TEXT("The dragonfly's flight path bends toward its temporary station"),
		FVector::Dist(Dragonfly->GetActorLocation(), Dragonfly->ListeningStonesInterestLocation) <
		FVector::Dist(BeforeHover, Dragonfly->ListeningStonesInterestLocation));

	Dragonfly->ListeningStonesInterestRemaining = 0.f;
	Dragonfly->ListeningStonesInterestCooldownRemaining = 0.f;
	Dragonfly->LastHeardListeningStonesChime.Reset();
	Dragonfly->FlowerInterestRemaining = 1.f;
	AIslandListeningStonesChime* CompetingChime = World->SpawnActor<AIslandListeningStonesChime>(
		FVector(20.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (CompetingChime) CompetingChime->BeginChime(0.f);
	TestFalse(TEXT("A deliberate flower visit is not displaced by the landmark tone"),
		Dragonfly->RespondToListeningStonesChime(CompetingChime));
	TestTrue(TEXT("The dragonfly keeps its active flower destination"),
		FMath::IsNearlyEqual(Dragonfly->FlowerInterestRemaining, 1.f));

	Dragonfly->FlowerInterestRemaining = 0.f;
	AIslandListeningStonesChime* DistantChime = World->SpawnActor<AIslandListeningStonesChime>(
		FVector(2000.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (DistantChime) DistantChime->BeginChime(0.f);
	TestFalse(TEXT("A chime beyond the audible radius is ignored"),
		Dragonfly->RespondToListeningStonesChime(DistantChime));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
