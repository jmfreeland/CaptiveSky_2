#include "Agent/IslandCommonFrog.h"
#include "Agent/IslandTidepoolMinnows.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandCommonFrogMinnowTests,
	"CaptiveSky2.Agent.Ecology.CommonFrogMinnowSurfaceBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandCommonFrogMinnowTests::RunTest(const FString& Parameters)
{
	using Frog = AIslandCommonFrog;
	TestTrue(TEXT("A nearby surface break is within the frog's local visual range"),
		Frog::ShouldWatchMinnowSurfaceBreak(1100.f, 500.f));
	TestFalse(TEXT("A distant surface break is ignored"), Frog::ShouldWatchMinnowSurfaceBreak(1101.f, 0.f));
	TestFalse(TEXT("A surface break outside the vertical band is ignored"), Frog::ShouldWatchMinnowSurfaceBreak(400.f, 501.f));
	TestFalse(TEXT("A non-finite surface-break distance is ignored"),
		Frog::ShouldWatchMinnowSurfaceBreak(std::numeric_limits<float>::infinity(), 0.f));

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Frog/minnow fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for frog/minnow fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector FrogStart(1000.f, 2000.f, 0.f);
	AIslandCommonFrog* FrogActor = World->SpawnActor<AIslandCommonFrog>(
		FrogStart, FRotator::ZeroRotator, Spawn);
	AIslandTidepoolMinnows* School = World->SpawnActor<AIslandTidepoolMinnows>(
		FrogStart + FVector(0.f, 350.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Common Frog spawned"), FrogActor) ||
		!TestNotNull(TEXT("Minnow school spawned"), School))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();

	School->SurfaceBreakRemaining = 0.f;
	School->TryCreateSurfaceBreak(0.f);
	TestTrue(TEXT("The fixture creates an actual visible minnow surface break"), School->IsMakingSurfaceBreak());
	const FVector SurfaceLocation = School->GetSurfaceBreakWorldLocation();
	AActor* Blocker = World->SpawnActor<AActor>(FMath::Lerp(FrogStart, SurfaceLocation, 0.5f),
		FRotator::ZeroRotator, Spawn);
	UBoxComponent* SightBlocker = NewObject<UBoxComponent>(Blocker, TEXT("MinnowSightBlocker"));
	if (!TestNotNull(TEXT("A transient visibility blocker is created"), SightBlocker))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Blocker->SetRootComponent(SightBlocker);
	SightBlocker->SetBoxExtent(FVector(120.f));
	SightBlocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SightBlocker->SetCollisionObjectType(ECC_WorldStatic);
	SightBlocker->SetCollisionResponseToAllChannels(ECR_Ignore);
	SightBlocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SightBlocker->RegisterComponent();
	SightBlocker->SetWorldLocation(FMath::Lerp(FrogStart, SurfaceLocation, 0.5f));
	FHitResult BlockerHit;
	TestTrue(TEXT("The fixture's opaque box intercepts the fish's visibility trace"),
		World->LineTraceSingleByChannel(BlockerHit, FrogStart + FVector(0.f, 0.f, 8.f),
			SurfaceLocation, ECC_Visibility));
	FrogActor->CheckForNearbyMinnowSurfaceBreak();
	TestTrue(TEXT("An opaque nearby obstruction prevents the attention cue"),
		FMath::IsNearlyZero(FrogActor->WildlifeAttentionRemaining));
	Blocker->Destroy();
	FrogActor->CheckForNearbyMinnowSurfaceBreak();
	TestTrue(TEXT("The frog latches one nearby surface-break cue"),
		FrogActor->MinnowPresenceLatch.Get() == School && FrogActor->WildlifeAttentionRemaining > 1.7f);
	TestTrue(TEXT("The attention cue stores the fish's real surface-break location"),
		FrogActor->WildlifeAttentionLocation.Equals(SurfaceLocation, 0.1f));
	TestTrue(TEXT("Watching the fish does not move the frog"), FrogActor->GetActorLocation().Equals(FrogStart, 0.1f));

	FrogActor->Tick(0.5f);
	const FVector ToFish = (SurfaceLocation - FrogStart).GetSafeNormal2D();
	TestTrue(TEXT("The frog turns toward the breaching fish"),
		FVector::DotProduct(FrogActor->GetActorForwardVector().GetSafeNormal2D(), ToFish) > 0.9f);
	TestTrue(TEXT("Attention remains readable after the turn begins"), FrogActor->WildlifeAttentionRemaining > 1.2f);

	FrogActor->CheckForNearbyMinnowSurfaceBreak();
	TestTrue(TEXT("Repeated sampling does not restart or multiply the same cue"),
		FMath::IsNearlyEqual(FrogActor->WildlifeAttentionRemaining, 1.3f, 0.05f));
	School->SurfaceBreakHopRemaining = 0.f;
	FrogActor->WildlifeAttentionRemaining = 0.f;
	FrogActor->CheckForNearbyMinnowSurfaceBreak();
	TestNull(TEXT("The school latch clears after the brief surface-break event ends"),
		FrogActor->MinnowPresenceLatch.Get());

	School->SurfaceBreakHopRemaining = AIslandTidepoolMinnows::SurfaceBreakHopDurationSeconds;
	FrogActor->CheckForNearbyMinnowSurfaceBreak();
	TestTrue(TEXT("The cooldown prevents an immediate repeat during the same nearby visit"),
		FMath::IsNearlyZero(FrogActor->WildlifeAttentionRemaining));
	FrogActor->WildlifeAttentionCooldownRemaining = 0.f;
	FrogActor->CheckForNearbyMinnowSurfaceBreak();
	TestTrue(TEXT("A later eligible surface break can be noticed again"),
		FrogActor->MinnowPresenceLatch.Get() == School && FrogActor->WildlifeAttentionRemaining > 1.7f);
	TestTrue(TEXT("The later cue also leaves the frog in place"), FrogActor->GetActorLocation().Equals(FrogStart, 0.1f));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
