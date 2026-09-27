#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "IslandDayNight.h"
#include "IslandInteractionUtility.h"
#include "IslandTidepoolMinnows.h"
#include "IslandWeather.h"
#include "RavenAgentAIController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandMinnowTest, "CaptiveSky2.Agent.TidepoolMinnows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandMinnowTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Calm shallows allow ordinary school movement"), FMath::IsNearlyEqual(AIslandTidepoolMinnows::RainMovementScale(0.f), 1.f));
	TestTrue(TEXT("Heavy rain gently tightens, but does not stop, the school"),
		AIslandTidepoolMinnows::RainMovementScale(1.f) > 0.f && AIslandTidepoolMinnows::RainMovementScale(1.f) < 1.f);
	TestTrue(TEXT("The rain response changes smoothly and monotonically"),
		AIslandTidepoolMinnows::RainMovementScale(0.8f) < AIslandTidepoolMinnows::RainMovementScale(0.4f));

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Minnow ecology fixture world created"), World) || !TestNotNull(TEXT("Engine is available for minnow fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AActor* Habitat = World->SpawnActor<AActor>(FVector(1000.f, 2000.f, 300.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Day-night clock spawned"), Clock) || !TestNotNull(TEXT("Weather actor spawned"), Weather) || !TestNotNull(TEXT("Tideglass habitat marker spawned"), Habitat))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Clock->CurrentHour = 12.f;
	Habitat->Tags.Add(TEXT("TideglassPool"));
	Weather->RefreshNightEcology();
	AIslandTidepoolMinnows* School = Weather->DayMinnowSchool.Get();
	if (!TestNotNull(TEXT("One daytime school appears at Tideglass"), School))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	School->Weather = Weather;
	Weather->RefreshNightEcology();
	TestTrue(TEXT("Repeated ecology refresh reuses rather than duplicates the school"), Weather->DayMinnowSchool.Get() == School);
	TestTrue(TEXT("The school is wild life, not a landmark, pet, or nest site"),
		School->ActorHasTag(TEXT("IslandLife")) && School->ActorHasTag(TEXT("MinnowSchool")) &&
		!School->ActorHasTag(TEXT("IslandLandmark")) && !School->ActorHasTag(TEXT("RavenNestSite")));
	TestEqual(TEXT("The shallow-water school has a small bounded population"), School->Fish.Num(), 5);
	for (UStaticMeshComponent* Minnow : School->Fish)
		TestTrue(TEXT("Minnows are visual-only and nonblocking"), Minnow && Minnow->GetStaticMesh() && Minnow->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	School->Tick(0.35f);
	const FVector BeforeScatter = [&School]()
	{
		FVector Center = FVector::ZeroVector;
		for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) Center += Minnow->GetRelativeLocation();
		return Center / School->Fish.Num();
	}();
	ACharacter* Visitor = World->SpawnActor<ACharacter>(School->GetActorLocation() - FVector(300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Quiet observer spawned near the pool"), Visitor))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Visitors and residents resolve one stable school target"), IslandInteractionUtility::GetTargetTag(School), FName(TEXT("MinnowSchool")));
	TestTrue(TEXT("The nearby school is visible to a visitor"), IslandInteractionUtility::CanInteract(Visitor, School));
	FString VisitorFact;
	TestTrue(TEXT("Quiet observation is a valid visitor response"), IslandInteractionUtility::Perform(Visitor, School, VisitorFact));
	TestTrue(TEXT("The response truthfully describes scattering without capture or persistence"),
		VisitorFact.Contains(TEXT("scattered from your quiet attention")) && VisitorFact.Contains(TEXT("wild and uncaught")) && VisitorFact.Contains(TEXT("nothing persistent changed")));
	School->Tick(0.7f);
	FVector ScatteredCenter = FVector::ZeroVector;
	for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) ScatteredCenter += Minnow->GetRelativeLocation();
	ScatteredCenter /= School->Fish.Num();
	TestTrue(TEXT("The school visibly fans away from a quiet observer"), ScatteredCenter.X > BeforeScatter.X + 80.f);
	School->Tick(2.f);
	FVector RegroupedCenter = FVector::ZeroVector;
	for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) RegroupedCenter += Minnow->GetRelativeLocation();
	RegroupedCenter /= School->Fish.Num();
	TestTrue(TEXT("Minnows naturally regroup within their small local habitat"), RegroupedCenter.Size2D() < 180.f && FMath::IsNearlyZero(School->ScatterRemaining));

	ARavenAgentAIController* ResidentController = World->SpawnActor<ARavenAgentAIController>(Spawn);
	ACharacter* Resident = World->SpawnActor<ACharacter>(School->GetActorLocation() + FVector(0.f, 100.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Resident inspection controller spawned"), ResidentController) && TestNotNull(TEXT("Resident body spawned"), Resident))
	{
		ResidentController->Possess(Resident);
		ResidentController->InspectTarget(TEXT("MinnowSchool"));
		TestTrue(TEXT("A resident can choose to inspect the school through its ordinary wildlife path"),
			ResidentController->DescribeActionState().Contains(TEXT("scattered from your quiet attention")));
		ResidentController->UnPossess();
	}

	Clock->CurrentHour = 19.f;
	Weather->RefreshNightEcology();
	TestTrue(TEXT("The daytime school leaves the habitat after dusk"), School->IsActorBeingDestroyed() && !Weather->DayMinnowSchool.IsValid());
	Clock->CurrentHour = 6.f;
	Weather->RefreshNightEcology();
	TestTrue(TEXT("One school returns at dawn without duplicating"), Weather->DayMinnowSchool.IsValid());
	AIslandTidepoolMinnows* DawnSchool = Weather->DayMinnowSchool.Get();
	Weather->RefreshNightEcology();
	TestTrue(TEXT("Dawn refresh preserves the same single school"), Weather->DayMinnowSchool.Get() == DawnSchool);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
