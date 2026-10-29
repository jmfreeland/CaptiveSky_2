#include "Misc/AutomationTest.h"
#include "IslandTidepoolCrab.h"
#include "IslandWrack.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTidepoolCrabWrackTest, "CaptiveSky2.Agent.TidepoolCrabShoreForage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTidepoolCrabWrackTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("A world is required for the local shoreline interaction"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	AIslandTidepoolCrab* Crab = World->SpawnActor<AIslandTidepoolCrab>(FVector::ZeroVector, FRotator::ZeroRotator);
	AIslandWrack* Wrack = World->SpawnActor<AIslandWrack>(FVector(180.f, 0.f, 30.f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A Tideglass crab can be staged beside the shore"), Crab) ||
		!TestNotNull(TEXT("A storm-wrack find can be staged beside the crab"), Wrack))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	Wrack->Show(1, EIslandWrackKind::Kelp, 17, 0, false);
	Crab->CheckForNearbyWrack();
	TestTrue(TEXT("A nearby shore find draws one brief forage detour"), Crab->IsInvestigatingShoreFind());
	TestTrue(TEXT("The forage target is the nearby wrack rather than a new saved location"),
		Crab->WrackInterestLocation.Equals(Wrack->GetActorLocation()));
	TestFalse(TEXT("Foraging is not misreported as fleeing a disturbance"), Crab->IsScurrying());

	const float FirstInterestRemaining = Crab->WrackInterestRemaining;
	Crab->CheckForNearbyWrack();
	TestEqual(TEXT("A continuously nearby find does not restart its investigation"),
		Crab->WrackInterestRemaining, FirstInterestRemaining);

	Crab->RespondToQuietObservation(Crab->GetActorLocation() - FVector(100.f, 0.f, 0.f));
	TestFalse(TEXT("A visitor's approach interrupts foraging and lets the crab flee"), Crab->IsInvestigatingShoreFind());
	TestTrue(TEXT("The visitor response remains a separate brief scurry"), Crab->IsScurrying());

	Crab->ScurryRemaining = 0.f;
	Crab->WrackInterestCooldownRemaining = 0.f;
	Crab->WrackInterestRemaining = 0.f;
	Crab->CheckForNearbyWrack();
	TestFalse(TEXT("The same nearby find stays latched after an interruption"), Crab->IsInvestigatingShoreFind());

	Crab->SetActorLocation(FVector(700.f, 0.f, 0.f));
	Crab->CheckForNearbyWrack(); // Leaves the wider rearm ring and clears the old find latch.
	Crab->SetActorLocation(FVector::ZeroVector);
	Crab->CheckForNearbyWrack();
	TestTrue(TEXT("Leaving the wider ring rearms a later visit to that find"), Crab->IsInvestigatingShoreFind());
	Crab->SetSheltered(true);
	TestFalse(TEXT("Night shelter ends a forage detour"), Crab->IsInvestigatingShoreFind());
	Crab->SetSheltered(false);
	TestFalse(TEXT("Dawn does not resume a stale forage detour"), Crab->IsInvestigatingShoreFind());

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
