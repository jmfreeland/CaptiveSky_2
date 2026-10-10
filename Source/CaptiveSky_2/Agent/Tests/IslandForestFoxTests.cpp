#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IslandForestFox.h"
#include "IslandInteractionUtility.h"
#include "Components/SkeletalMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandForestFoxTest, "CaptiveSky2.Agent.WoodlandFox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandForestFoxTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Woodland fox fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for woodland fox fixture"), GEngine))
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

	AIslandForestFox* Fox = World->SpawnActor<AIslandForestFox>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AActor* Observer = World->SpawnActor<AActor>(FVector(-300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("One bounded woodland fox can spawn"), Fox) ||
		!TestNotNull(TEXT("A resident can observe the fox"), Observer))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Observer);
	Observer->AddInstanceComponent(Brain);
	Brain->RegisterComponent();
	TestTrue(TEXT("Nearby residents can perceive the fox as optional independent wildlife"),
		Brain->BuildSituationSummary(FAgentConversationContext()).Contains(TEXT("Interact with target WoodlandFox")));

	TestTrue(TEXT("The free imported fox mesh resolves"), Fox->GetFoxMesh() && Fox->GetFoxMesh()->GetSkeletalMeshAsset());
	TestTrue(TEXT("Breathe, look, walk, run, sleep, and wake clips resolve"),
		Fox->IdleAnimation && Fox->LookAroundAnimation && Fox->WalkAnimation && Fox->RunAnimation &&
		Fox->SleepAnimation && Fox->WakeAnimation);
	if (USkeletalMesh* Mesh = Fox->GetFoxMesh() ? Fox->GetFoxMesh()->GetSkeletalMeshAsset() : nullptr)
	{
		USkeleton* Skeleton = Mesh->GetSkeleton();
		TestTrue(TEXT("Each used animation targets the imported fox skeleton"), Skeleton &&
			Fox->IdleAnimation->GetSkeleton() == Skeleton && Fox->LookAroundAnimation->GetSkeleton() == Skeleton &&
			Fox->WalkAnimation->GetSkeleton() == Skeleton && Fox->RunAnimation->GetSkeleton() == Skeleton &&
			Fox->SleepAnimation->GetSkeleton() == Skeleton && Fox->WakeAnimation->GetSkeleton() == Skeleton);
	}
	if (Fox->IdleAnimation && Fox->LookAroundAnimation && Fox->WalkAnimation && Fox->RunAnimation &&
		Fox->SleepAnimation && Fox->WakeAnimation)
	{
		TestFalse(TEXT("The fox stays actor-driven; its ecology animations do not apply root motion"),
			Fox->IdleAnimation->HasRootMotion() || Fox->LookAroundAnimation->HasRootMotion() ||
			Fox->WalkAnimation->HasRootMotion() || Fox->RunAnimation->HasRootMotion() ||
			Fox->SleepAnimation->HasRootMotion() || Fox->WakeAnimation->HasRootMotion());
	}
	TestTrue(TEXT("The visible fox is nonblocking"), Fox->GetFoxMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("The fox is transient wildlife, not a conscious resident or landmark"),
		Fox->ActorHasTag(TEXT("IslandLife")) && Fox->ActorHasTag(TEXT("WoodlandFox")) &&
		!Fox->ActorHasTag(TEXT("IslandLandmark")) && !Fox->FindComponentByClass<UAgentBrainComponent>());
	TestEqual(TEXT("The interaction helper offers a distinct fox observation"),
		IslandInteractionUtility::GetTargetTag(Fox), FName(TEXT("WoodlandFox")));
	TestFalse(TEXT("Residents cannot use a wild fox as a movement destination"),
		IslandInteractionUtility::IsMovementTargetAllowed(Fox));
	TestTrue(TEXT("A nearby observer can inspect without touching the fox"),
		IslandInteractionUtility::CanInspect(Observer, Fox, 400.f));

	Fox->SetResting(true);
	FString Fact;
	TestTrue(TEXT("Quietly observing a resting fox is a safe inspection"),
		IslandInteractionUtility::Perform(Observer, Fox, Fact));
	TestTrue(TEXT("Daytime observation leaves a resting fox undisturbed"), Fact.Contains(TEXT("left it undisturbed")) && Fox->IsResting());
	Fox->SetResting(false);
	TestTrue(TEXT("Dusk starts the fox's existing wake animation"), Fox->bWaking);
	Fox->bWaking = false;
	TestTrue(TEXT("An awake fox can respond to a quiet observation"),
		Fox->RespondToQuietObservation(Observer->GetActorLocation()));
	TestTrue(TEXT("An awake fox pauses to look before retreating"), Fox->bNoticing && !Fox->bMoving && Fox->NoticeRemaining > 0.f);
	const float HomeRadius = FVector::Dist2D(Fox->HomeLocation, Fox->TargetLocation);
	TestTrue(TEXT("The response target remains inside its 6.5 m home patch"), HomeRadius <= 650.1f);
	Fox->bNoticing = false;
	Fox->bMoving = true;
	Fox->TargetLocation = Fox->GetActorLocation() + FVector(50.f, 0.f, 0.f);
	TestTrue(TEXT("A quiet resident can redirect a fox already on a short forage step"),
		Fox->RespondToQuietObservation(Observer->GetActorLocation()) && Fox->bNoticing && !Fox->bMoving);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
