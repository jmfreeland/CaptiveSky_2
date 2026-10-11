#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "AutonomousAgentCharacter.h"
#include "IslandInnHearthSubsystem.h"
#include "IslandInteractionUtility.h"
#include "IslandSocialTestCharacter.h"
#include "IslandWorldStateSubsystem.h"
#include "RavenAgentAIController.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandInnHearthTest, "CaptiveSky2.Agent.IslandInnHearth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandInnHearthTest::RunTest(const FString& Parameters)
{
	const FString TestSuffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString StateFile = FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("InnHearthPractice") / (TestSuffix + TEXT(".json"));
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
	UIslandWorldStateSubsystem* WorldState = World->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!TestNotNull(TEXT("Island state is available for the practice fixture"), WorldState))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	WorldState->StorageFileOverride = StateFile;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APointLight* LightActor = World->SpawnActor<APointLight>(FVector(250.f, 0.f, 80.f), FRotator::ZeroRotator, Spawn);
	AActor* HearthMarker = World->SpawnActor<AActor>(FVector(250.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	AActor* Visitor = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AIslandSocialTestCharacter* CraftingResident = World->SpawnActor<AIslandSocialTestCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Tagged hearth light spawned"), LightActor) || !TestNotNull(TEXT("Hearth marker spawned"), HearthMarker) ||
		!TestNotNull(TEXT("Observer spawned"), Visitor) || !TestNotNull(TEXT("Crafting resident spawned"), CraftingResident))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	CraftingResident->Memory->AgentId = TEXT("InnHearthPractice_") + TestSuffix;
	const FString ResidentDataDirectory = CraftingResident->Memory->GetAgentDirectory();
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
	ARavenAgentAIController* CraftController = World->SpawnActor<ARavenAgentAIController>(Spawn);
	if (TestNotNull(TEXT("A resident body is available for optional craft practice"), CraftingResident) &&
		TestNotNull(TEXT("A controller can execute the practice choice"), CraftController))
	{
		const FString Response = TEXT("{\"thought\":\"I am curious about a slow rise\",\"action\":{\"type\":\"practice_craft\",\"target\":\"InnHearth\",\"craft_id\":\"HearthLoaf\",\"craft_name\":\"Hearth loaf\",\"craft_source\":\"a remembered warm smell\",\"craft_parameters\":[{\"id\":\"hydration\",\"value\":0.62},{\"id\":\"heat\",\"value\":0.45}]},\"new_memories\":[]}");
		const FAgentDecision Decision = UAgentBrainComponent::ParseDecisionAndStoreMemories(Response, nullptr);
		TestTrue(TEXT("The brain prompt offers the bounded optional practice action"),
			CraftingResident->Brain->BuildSystemPrompt(TArray<FAgentMemoryRecord>()).Contains(TEXT("practice_craft")));
		TestTrue(TEXT("The parser preserves the safe practice ID and two intended settings"),
			Decision.bValid && Decision.ActionType == EAgentActionType::PracticeCraft &&
			Decision.ActionTarget == TEXT("InnHearth") && Decision.CraftId == TEXT("HearthLoaf") &&
			Decision.CraftParameters.Num() == 2 && Decision.CraftParameters[0].Id == TEXT("hydration") &&
			FMath::IsNearlyEqual(Decision.CraftParameters[0].Value, 0.62f));
		CraftController->Possess(CraftingResident);
		static_cast<AAutonomousAgentAIController*>(CraftController)->ActOnDecision(Decision);
		const FIslandCraftTraditionRecord* Tradition = WorldState->FindCraftTradition(TEXT("HearthLoaf"));
		TestTrue(TEXT("The actual lit, reachable hearth action creates an attributed unrated attempt"),
			Tradition && Tradition->OriginAgentId == CraftingResident->Memory->GetResolvedAgentId() &&
			Tradition->Attempts.Num() == 1 && Tradition->Attempts[0].Outcome == EIslandCraftOutcome::Unrated &&
			Tradition->Attempts[0].Parameters.Num() == 2 &&
			CraftingResident->Brain->BuildSituationSummary(FAgentConversationContext()).Contains(TEXT("unrated experiment")));
		TestTrue(TEXT("The result is honestly reported as notes with no generated food"),
			CraftController->DescribeActionState().Contains(TEXT("no loaf, ingredients, or prop was created")) &&
			CraftingResident->Memory->HasMemoryTag(TEXT("craft-practice")));
		TestFalse(TEXT("The resident cannot repeat the same tradition on the same Island day"),
			Hearth->PracticeCraft(CraftingResident, FName(TEXT("HearthLoaf")), TEXT("Ignored rename"), TEXT("Ignored source"),
				Tradition->Attempts[0].Parameters, Fact));
		FAgentDecision WrongTarget = Decision;
		WrongTarget.ActionTarget = TEXT("GuestBook");
		WrongTarget.CraftId = TEXT("WrongSiteLoaf");
		static_cast<AAutonomousAgentAIController*>(CraftController)->ActOnDecision(WrongTarget);
		TestNull(TEXT("A craft action cannot execute at a non-hearth target"), WorldState->FindCraftTradition(TEXT("WrongSiteLoaf")));
		TestTrue(TEXT("The test banks the reversible hearth before checking practice refusal"),
			IslandInteractionUtility::Perform(Visitor, HearthMarker, Fact) && !Hearth->IsLit());
		TestFalse(TEXT("A dark hearth rejects practice without creating its proposed tradition"),
			Hearth->PracticeCraft(CraftingResident, FName(TEXT("ColdHearthLoaf")), TEXT("Cold loaf"), TEXT("a cold guess"),
				Tradition->Attempts[0].Parameters, Fact));
		TestNull(TEXT("The dark-hearth refusal leaves no craft record"), WorldState->FindCraftTradition(TEXT("ColdHearthLoaf")));
		FIslandCraftParameter InvalidSetting;
		InvalidSetting.Id = TEXT("oven_type");
		InvalidSetting.Value = 0.5f;
		TestFalse(TEXT("Only the four offered baking notes can be recorded"),
			Hearth->PracticeCraft(CraftingResident, FName(TEXT("InvalidSettingLoaf")), TEXT("Bad notes"), TEXT("a guess"),
				{ InvalidSetting }, Fact));
		TestTrue(TEXT("A hearth outside interaction range rejects practice"),
			CraftingResident->SetActorLocation(FVector(1000.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics) &&
			!Hearth->PracticeCraft(CraftingResident, FName(TEXT("DistantHearthLoaf")), TEXT("Distant loaf"), TEXT("a guess"),
				Tradition->Attempts[0].Parameters, Fact));
		CraftingResident->SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
		TestTrue(TEXT("The test restores the hearth's lit state for the remaining reversible-light checks"),
			IslandInteractionUtility::Perform(Visitor, HearthMarker, Fact) && Hearth->IsLit());
		CraftController->UnPossess();
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
	IFileManager::Get().Delete(*StateFile, false, true, true);
	IFileManager::Get().DeleteDirectory(*ResidentDataDirectory, false, true);
	return true;
}
