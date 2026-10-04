#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "IslandForestStag.h"
#include "IslandInteractionUtility.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandForestStagTest, "CaptiveSky2.Agent.WoodlandDeer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandForestStagTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Woodland ecology fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for woodland fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(FVector(0.f, 0.f, -2.f), FRotator::ZeroRotator, Spawn);
	if (Ground)
	{
		if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			Ground->GetStaticMeshComponent()->SetStaticMesh(Plane);
			Ground->SetActorScale3D(FVector(20.f, 20.f, 1.f));
		}
	}

	AIslandForestStag* Deer = World->SpawnActor<AIslandForestStag>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AActor* Observer = World->SpawnActor<AActor>(FVector(-300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("One bounded woodland stag can spawn"), Deer) ||
		!TestNotNull(TEXT("A resident can observe the stag"), Observer))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	TestTrue(TEXT("The imported stag mesh is assigned"), Deer->GetDeerMesh() && Deer->GetDeerMesh()->GetSkeletalMeshAsset());
	TestTrue(TEXT("The imported graze, walk, run, and sleep animations resolve"),
		Deer->GrazeAnimation && Deer->WalkAnimation && Deer->RunAnimation && Deer->SleepAnimation);
	if (USkeletalMesh* StagMesh = Deer->GetDeerMesh() ? Deer->GetDeerMesh()->GetSkeletalMeshAsset() : nullptr)
	{
		USkeleton* StagSkeleton = StagMesh->GetSkeleton();
		TestTrue(TEXT("Every assigned stag animation targets the mesh skeleton"), StagSkeleton &&
			Deer->GrazeAnimation->GetSkeleton() == StagSkeleton && Deer->WalkAnimation->GetSkeleton() == StagSkeleton &&
			Deer->RunAnimation->GetSkeleton() == StagSkeleton && Deer->SleepAnimation->GetSkeleton() == StagSkeleton);
	}
	if (Deer->GrazeAnimation && Deer->WalkAnimation && Deer->RunAnimation && Deer->SleepAnimation)
		TestFalse(TEXT("Stag locomotion animations have no root motion; bounded movement stays actor-driven"),
			Deer->GrazeAnimation->HasRootMotion() || Deer->WalkAnimation->HasRootMotion() ||
			Deer->RunAnimation->HasRootMotion() || Deer->SleepAnimation->HasRootMotion());
	TestTrue(TEXT("The visible mesh is nonblocking"), Deer->GetDeerMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	Deer->HomeLocation = Deer->GetActorLocation();
	TestTrue(TEXT("The stag is wild life, not a conscious agent or landmark"),
		Deer->ActorHasTag(TEXT("IslandLife")) && Deer->ActorHasTag(TEXT("WoodlandDeer")) &&
		!Deer->ActorHasTag(TEXT("IslandLandmark")) && !Deer->FindComponentByClass<UAgentBrainComponent>());
	TestEqual(TEXT("The interaction helper exposes the correct optional target"),
		IslandInteractionUtility::GetTargetTag(Deer), FName(TEXT("WoodlandDeer")));
	TestFalse(TEXT("Wild animals can be observed but are never movement destinations"),
		IslandInteractionUtility::IsMovementTargetAllowed(Deer));
	AActor* Landmark = World->SpawnActor<AActor>(FVector(900.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (Landmark) Landmark->Tags.Add(TEXT("IslandLandmark"));
	TestTrue(TEXT("Ordinary world destinations remain available"), IslandInteractionUtility::IsMovementTargetAllowed(Landmark));
	TestTrue(TEXT("A nearby clear observer can inspect without touching the animal"),
		IslandInteractionUtility::CanInspect(Observer, Deer, 400.f));

	FString Fact;
	TestTrue(TEXT("Quiet observation is an implemented reversible interaction"),
		IslandInteractionUtility::Perform(Observer, Deer, Fact));
	TestTrue(TEXT("The animal briefly startles instead of becoming owned or following"),
		Deer->IsStartled() && !Deer->IsResting() && Fact.Contains(TEXT("nothing persistent changed")));
	TestTrue(TEXT("A startle response remains within the stag's 7 m home radius"),
		FVector::Dist2D(Deer->HomeLocation, Deer->TargetLocation) <= 700.f);
	Deer->SetResting(true);
	TestTrue(TEXT("Night resting is reversible"), Deer->IsResting());
	Deer->RespondToQuietObservation(Observer->GetActorLocation());
	TestFalse(TEXT("A resting wild animal is not forced into a new action"), Deer->IsStartled());
	Deer->SetResting(false);
	TestFalse(TEXT("Daylight returns the stag to its ordinary routine"), Deer->IsResting());

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
