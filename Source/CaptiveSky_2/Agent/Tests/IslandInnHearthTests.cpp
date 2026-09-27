#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "IslandInnHearthSubsystem.h"
#include "IslandInteractionUtility.h"
#include "RavenAgentAIController.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandInnHearthTest, "CaptiveSky2.Agent.IslandInnHearth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandInnHearthTest::RunTest(const FString& Parameters)
{
	// This fixture has no active resident decision loop, memory store, or model request.
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Hearth fixture world created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APointLight* LightActor = World->SpawnActor<APointLight>(FVector(250.f, 0.f, 80.f), FRotator::ZeroRotator, Spawn);
	AActor* HearthMarker = World->SpawnActor<AActor>(FVector(250.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	AActor* Visitor = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Tagged hearth light spawned"), LightActor) || !TestNotNull(TEXT("Hearth marker spawned"), HearthMarker) ||
		!TestNotNull(TEXT("Observer spawned"), Visitor))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	USceneComponent* HearthRoot = NewObject<USceneComponent>(HearthMarker);
	HearthMarker->SetRootComponent(HearthRoot);
	HearthRoot->RegisterComponent();
	HearthMarker->SetActorLocation(FVector(250.f, 0.f, 0.f));
	LightActor->Tags.Add(TEXT("InnHearthLight"));
	HearthMarker->Tags = {TEXT("IslandInn"), TEXT("InnHearth")};
	const float BaseIntensity = LightActor->PointLightComponent->Intensity;

	TestEqual(TEXT("The tagged inn hearth resolves to one stable target"), IslandInteractionUtility::GetTargetTag(HearthMarker), FName(TEXT("InnHearth")));
	TestTrue(TEXT("A nearby visitor can see and select the hearth"), IslandInteractionUtility::CanInteract(Visitor, HearthMarker) &&
		IslandInteractionUtility::FindNearestVisibleTarget(Visitor, World) == HearthMarker);
	AActor* UntaggedHearth = World->SpawnActor<AActor>(FVector(200.f, 300.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Untagged hearth decoy spawned"), UntaggedHearth))
	{
		UntaggedHearth->Tags.Add(TEXT("InnHearth"));
		TestTrue(TEXT("A hearth tag without the inn ownership marker is not interactive"), IslandInteractionUtility::GetTargetTag(UntaggedHearth).IsNone());
	}
	AActor* Occluder = World->SpawnActor<AActor>(FVector(125.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	UBoxComponent* OcclusionBox = nullptr;
	if (TestNotNull(TEXT("Opaque perception blocker spawned"), Occluder))
	{
		OcclusionBox = NewObject<UBoxComponent>(Occluder);
		Occluder->SetRootComponent(OcclusionBox);
		OcclusionBox->SetBoxExtent(FVector(24.f, 120.f, 200.f));
		OcclusionBox->SetCollisionProfileName(TEXT("BlockAll"));
		OcclusionBox->RegisterComponent();
		Occluder->SetActorLocation(FVector(125.f, 0.f, 0.f));
	}

	World->BeginPlay();
	UIslandInnHearthSubsystem* Hearth = World->GetSubsystem<UIslandInnHearthSubsystem>();
	if (!TestNotNull(TEXT("Session hearth subsystem is active"), Hearth))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	TestFalse(TEXT("The hearth begins banked"), Hearth->IsLit());
	TestEqual(TEXT("A banked hearth has no remaining burn time"), Hearth->GetSecondsRemaining(), 0.f);
	TestEqual(TEXT("The saved light starts off for this play session"), LightActor->PointLightComponent->Intensity, 0.f);
	TestEqual(TEXT("Three session-only flame meshes are prepared at the tagged hearth"), Hearth->FlameMeshes.Num(), 3);
	for (const TWeakObjectPtr<UStaticMeshComponent>& Flame : Hearth->FlameMeshes)
		TestTrue(TEXT("A banked stylized flame is invisible and has no collision"), Flame.IsValid() && !Flame->IsVisible() && Flame->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Perception offers an optional, accurately described tending choice"), Hearth->DescribeHearth().Contains(TEXT("dark and banked")) && Hearth->DescribeHearth().Contains(TEXT("if you choose")));

	FString Fact;
	TestTrue(TEXT("A close interaction kindles the hearth"), IslandInteractionUtility::Perform(Visitor, HearthMarker, Fact));
	TestTrue(TEXT("Kindling truthfully distinguishes its local warmth cue from body temperature, shelter, and persistent state"),
		Fact.Contains(TEXT("warmth cue")) && Fact.Contains(TEXT("does not model body temperature")) && Fact.Contains(TEXT("shelter")) && Fact.Contains(TEXT("lasting change")));
	TestTrue(TEXT("Kindling enables only the tagged light for five minutes of play"), Hearth->IsLit() && Hearth->GetSecondsRemaining() == UIslandInnHearthSubsystem::BurnDurationSeconds && LightActor->PointLightComponent->Intensity > 0.f);
	for (const TWeakObjectPtr<UStaticMeshComponent>& Flame : Hearth->FlameMeshes)
		TestTrue(TEXT("Kindling shows the visible, nonblocking flame mesh"), Flame.IsValid() && Flame->IsVisible() && Flame->GetStaticMesh() && Flame->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("A lit hearth is honestly perceived with its remaining time and bank option"), Hearth->DescribeHearth().Contains(TEXT("is lit")) && Hearth->DescribeHearth().Contains(TEXT("bank it early")));
	TestTrue(TEXT("Simulated hearth warmth falls with distance and stops at its explicit local radius"),
		Hearth->GetWarmthFactorAt(HearthMarker->GetActorLocation() + FVector(100.f, 0.f, 0.f)) > 0.f &&
		Hearth->GetWarmthFactorAt(HearthMarker->GetActorLocation() + FVector(UIslandInnHearthSubsystem::WarmthCueRadius, 0.f, 0.f)) == 0.f &&
		Hearth->DescribeWarmthAt(HearthMarker->GetActorLocation()).Contains(TEXT("simulated warmth cue")) &&
		Hearth->DescribeWarmthAt(HearthMarker->GetActorLocation() + FVector(UIslandInnHearthSubsystem::WarmthCueRadius, 0.f, 0.f)).IsEmpty());
	ACharacter* ObserverBody = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Resident perception body spawned without a controller"), ObserverBody))
	{
		UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(ObserverBody);
		ObserverBody->AddInstanceComponent(Brain);
		Brain->RegisterComponent();
		TestFalse(TEXT("Residents are not told about an occluded hearth"),
			Brain->BuildSituationSummary(FAgentConversationContext()).Contains(TEXT("move_to/interact target: InnHearth")));
		if (IsValid(Occluder)) Occluder->Destroy();
		const FString Situation = Brain->BuildSituationSummary(FAgentConversationContext());
		TestTrue(TEXT("A resident sees the nearby optional hearth target again when its view clears"),
			Situation.Contains(TEXT("move_to/interact target: InnHearth")) && Situation.Contains(TEXT("is lit")) && Situation.Contains(TEXT("bank it early")) && Situation.Contains(TEXT("simulated warmth cue")));
	}
	Hearth->Tick(0.17f);
	const float FlickeredIntensity = LightActor->PointLightComponent->Intensity;
	TestTrue(TEXT("The light flickers gently within its authored baseline"), FlickeredIntensity > BaseIntensity * 0.77f && FlickeredIntensity < BaseIntensity * 1.09f);
	TestTrue(TEXT("The reversible interaction banks the hearth again"), IslandInteractionUtility::Perform(Visitor, HearthMarker, Fact));
	TestTrue(TEXT("Banking switches it off without saving a permanent change"), !Hearth->IsLit() && LightActor->PointLightComponent->Intensity == 0.f && Fact.Contains(TEXT("No permanent change")));
	for (const TWeakObjectPtr<UStaticMeshComponent>& Flame : Hearth->FlameMeshes)
		TestTrue(TEXT("Banking hides all three transient flame meshes"), Flame.IsValid() && !Flame->IsVisible());
	TestEqual(TEXT("Banking immediately removes the local warmth cue"), Hearth->GetWarmthFactorAt(HearthMarker->GetActorLocation()), 0.f);
	TestTrue(TEXT("Someone can kindle it again after banking"), IslandInteractionUtility::Perform(Visitor, HearthMarker, Fact) && Hearth->IsLit());
	Hearth->Tick(UIslandInnHearthSubsystem::BurnDurationSeconds);
	TestTrue(TEXT("The light naturally banks when its short burn ends"), !Hearth->IsLit() && Hearth->GetSecondsRemaining() == 0.f && LightActor->PointLightComponent->Intensity == 0.f);

	ACharacter* Resident = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>(Spawn);
	if (TestNotNull(TEXT("Resident body spawned without a decision brain"), Resident) && TestNotNull(TEXT("Resident interaction controller spawned"), Controller))
	{
		Controller->Possess(Resident);
		Controller->InspectTarget(TEXT("InnHearth"));
		TestTrue(TEXT("A resident can choose the same hearth interaction and receives its factual result"),
			Hearth->IsLit() && Controller->DescribeActionState().Contains(TEXT("kindled three small, stylized flames")));
		const double* HearthCooldown = Controller->InspectedUntil.Find(FName(TEXT("InnHearth")));
		TestTrue(TEXT("The reversible hearth can be tended again well before its five-minute burn ends"),
			HearthCooldown && *HearthCooldown - FPlatformTime::Seconds() > 0.0 && *HearthCooldown - FPlatformTime::Seconds() <= 60.0);
		Controller->UnPossess();
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
