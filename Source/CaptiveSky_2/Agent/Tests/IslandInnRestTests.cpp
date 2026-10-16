#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWildlifeHome.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandInnRestTest, "CaptiveSky2.Agent.IslandInnRest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandInnRestTest::RunTest(const FString& Parameters)
{
	// No gateway or model requests; the test uses a unique temporary memory store and cleans it up.
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Isolated inn-rest fixture world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	auto SpawnInnBox = [World](const FVector& Centre, const FVector& Extent)
	{
		AActor* Part = World->SpawnActor<AActor>(Centre, FRotator::ZeroRotator);
		if (!Part) return static_cast<AActor*>(nullptr);
		Part->Tags.Add(TEXT("IslandInn"));
		UBoxComponent* Box = NewObject<UBoxComponent>(Part);
		Part->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Part->SetActorLocation(Centre, false, nullptr, ETeleportType::TeleportPhysics);
		return Part;
	};
	SpawnInnBox(FVector(450.f, 0.f, 200.f), FVector(20.f, 500.f, 200.f));
	SpawnInnBox(FVector(-450.f, 0.f, 200.f), FVector(20.f, 500.f, 200.f));
	SpawnInnBox(FVector(0.f, 450.f, 200.f), FVector(500.f, 20.f, 200.f));
	SpawnInnBox(FVector(0.f, -450.f, 200.f), FVector(500.f, 20.f, 200.f));
	AActor* Roof = SpawnInnBox(FVector(0.f, 0.f, 500.f), FVector(500.f, 500.f, 20.f));
	AActor* Bed = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	Bed->Tags = { TEXT("IslandInn"), TEXT("InnBed_1") };
	AActor* RavenPerch = World->SpawnActor<AActor>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (RavenPerch) RavenPerch->Tags = { TEXT("RavenPerch"), TEXT("Roost_East") };
	ACharacter* Resident = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	if (!TestNotNull(TEXT("Tagged inn bed spawned"), Bed) || !TestNotNull(TEXT("Tagged raven perch spawned"), RavenPerch) ||
		!TestNotNull(TEXT("Resident body spawned"), Resident) ||
		!TestNotNull(TEXT("Resident controller spawned"), Controller))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	World->BeginPlay();
	Controller->Possess(Resident);
	Resident->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	UAgentConsolidationComponent* Rest = NewObject<UAgentConsolidationComponent>(Resident);
	Resident->AddInstanceComponent(Rest);
	Rest->RegisterComponent();
	UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Resident);
	Resident->AddInstanceComponent(Brain);
	Brain->RegisterComponent();
	UAgentMemoryComponent* Memory = NewObject<UAgentMemoryComponent>(Resident);
	Resident->AddInstanceComponent(Memory);
	Memory->AgentId = TEXT("Automation_InnRest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TestMemoryDirectory = Memory->GetAgentDirectory();
	IFileManager::Get().DeleteDirectory(*TestMemoryDirectory, false, true);
	Memory->RegisterComponent();
	const FString Situation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A resident can see the optional tagged bed as a concrete movement target"),
		Situation.Contains(TEXT("optional move_to target: InnBed_1")) && Situation.Contains(TEXT("blockout resting place")));
	TestTrue(TEXT("Bed perception sets limits on comfort and recovery"),
		Situation.Contains(TEXT("not a promise of comfort or recovery")) && Situation.Contains(TEXT("does not restore health")));

	Resident->SetActorLocation(FVector(700.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	TestFalse(TEXT("A named bed cannot put a resident to sleep from a distance"), Controller->TryRest(TEXT("InnBed_1")));
	TestTrue(TEXT("Distant rest leaves consciousness unchanged and reports that sleep does not teleport"),
		Rest->IsAwake() && Controller->DescribeActionState().Contains(TEXT("sleep does not teleport")));

	Resident->SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("The actual tagged roof and walls pass the shared interior evidence check"),
		UIslandEnvironmentSubsystem::IsInsideInnAt(World, Resident->GetActorLocation(), Resident));
	TestTrue(TEXT("An arrived resident may rest at the selected bed"), Controller->TryRest(TEXT("InnBed_1")));
	TestFalse(TEXT("Sleep pauses consciousness only after bed arrival"), Rest->IsAwake());
	TestTrue(TEXT("The result reports verified geometry and defers memory until the rest completes"),
		Controller->DescribeActionState().Contains(TEXT("indoor-geometry check")) &&
		Controller->DescribeActionState().Contains(TEXT("will be added to your memory when the rest interval completes")) &&
		Controller->DescribeActionState().Contains(TEXT("does not promise warmth")));
	TestTrue(TEXT("The sheltered-rest memory is queued but is not written before the rest interval ends"),
		!Rest->PendingSleepExperience.IsEmpty() && Memory->GetMemoryCount() == 0);

	Rest->WakeUp();
	TestTrue(TEXT("An early wake cancels the pending sheltered-rest memory"),
		Rest->PendingSleepExperience.IsEmpty() && Memory->GetMemoryCount() == 0);
	Resident->SetActorLocation(RavenPerch->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("A resident may choose sleep after arriving at a tagged raven perch"), Controller->TryRest(TEXT("Roost_East")));
	TestFalse(TEXT("Perch sleep pauses ordinary thought"), Rest->IsAwake());
	TestTrue(TEXT("Open-perch sleep is not misreported as a supported inn bed or sheltered rest"),
		Controller->DescribeActionState().Contains(TEXT("Settled to sleep at the tagged perch")) &&
		Controller->DescribeActionState().Contains(TEXT("no sheltered inn rest was verified")) &&
		Rest->PendingSleepExperience.IsEmpty() && Memory->GetMemoryCount() == 0);
	Rest->WakeUp();
	Resident->SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("The resident may choose the bed again after waking"), Controller->TryRest(TEXT("InnBed_1")));
	Rest->AppendPendingSleepExperience(); // This is the same local append path called when the rest timer ends.
	const TArray<FAgentMemoryRecord> RestMemories = Memory->GetMemoriesSince(FDateTime::MinValue());
	TestTrue(TEXT("Completed verified sheltered rest is written as a durable, bed-specific lived memory"),
		RestMemories.Num() == 1 && RestMemories[0].Text.Contains(TEXT("InnBed_1")) &&
		RestMemories[0].Text.Contains(TEXT("geometric enclosure check")));

	Rest->WakeUp();
	if (Roof)
	{
		Roof->SetActorEnableCollision(false);
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Roof->GetRootComponent()))
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	TestFalse(TEXT("A bed alone cannot establish sheltered rest without a tagged overhead roof"),
		UIslandEnvironmentSubsystem::IsInsideInnAt(World, Resident->GetActorLocation(), Resident));
	TestTrue(TEXT("A bed may still be slept beside when its shelter evidence fails"), Controller->TryRest(TEXT("InnBed_1")));
	TestTrue(TEXT("Failed roof evidence is reported honestly and does not claim a sheltered memory"),
		Controller->DescribeActionState().Contains(TEXT("will not be recorded as sheltered inn rest")));
	TestEqual(TEXT("A bed without verified enclosure creates no additional sheltered-rest memory"), Memory->GetMemoryCount(), 1);
	Rest->WakeUp();

	const FString WorldStateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("WildlifeHomeRest") /
		(TEXT("WorldState_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json")));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(WorldStateFile), true);
	UIslandWorldStateSubsystem* WorldState = World->GetSubsystem<UIslandWorldStateSubsystem>();
	TestNotNull(TEXT("Fixture world has an isolated wildlife-home store"), WorldState);
	if (WorldState)
	{
		WorldState->StorageFileOverride = WorldStateFile;
		FIslandWildlifeHomeRecord Home;
		Home.Id = TEXT("Fenrus_FallenWood");
		Home.Species = TEXT("Rat");
		Home.OccupantAgentId = Memory->GetResolvedAgentId();
		Home.EntranceLocation = FVector(2000.f, 0.f, 0.f);
		Home.ShelterQuality = 4;
		Home.ShelterState = EIslandWildlifeShelterState::Settled;
		Home.LastUsedUtc = FDateTime(2000, 1, 1);
		TestTrue(TEXT("A bounded assigned Rat home is stored outside the real world-state file"), WorldState->StoreWildlifeHome(Home));
		AActor* HomeActor = WorldState->FindWildlifeHomeActor(Home.Id);
		if (!HomeActor)
		{
			HomeActor = World->SpawnActor<AActor>(Home.EntranceLocation, Home.EntranceRotation);
			if (HomeActor)
			{
				HomeActor->Tags.AddUnique(TEXT("IslandWildlifeHome"));
				HomeActor->Tags.AddUnique(FName(*(TEXT("WildlifeHome_") + Home.Id.ToString())));
			}
		}
		TestNotNull(TEXT("Assigned wildlife home has a visible place actor"), HomeActor);
		if (HomeActor)
		{
			Resident->SetActorLocation(Home.EntranceLocation, false, nullptr, ETeleportType::TeleportPhysics);
			const FString HomeSituation = Brain->BuildSituationSummary(FAgentConversationContext());
			TestTrue(TEXT("The resident sees its own nearby home as an exact optional movement target"),
				HomeSituation.Contains(TEXT("optional move_to target: WildlifeHome_Fenrus_FallenWood")) && HomeSituation.Contains(TEXT("Sleep is optional")));
			TestTrue(TEXT("Home perception does not promise shelter, safety, comfort, or recovery"),
				HomeSituation.Contains(TEXT("does not establish shelter, safety, comfort, or recovery")));

			Resident->SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
			TestFalse(TEXT("A requested home cannot teleport a resident into sleep"), Controller->TryRest(TEXT("WildlifeHome_Fenrus_FallenWood")));
			TestTrue(TEXT("A distant home request leaves the resident awake"), Rest->IsAwake());

			Home.OccupantAgentId = TEXT("Agent_Someone_Else");
			TestTrue(TEXT("The ownership fixture can be changed reversibly"), WorldState->StoreWildlifeHome(Home));
			Resident->SetActorLocation(Home.EntranceLocation, false, nullptr, ETeleportType::TeleportPhysics);
			const FString OtherResidentSituation = Brain->BuildSituationSummary(FAgentConversationContext());
			TestFalse(TEXT("Another resident does not receive this home's optional target"),
				OtherResidentSituation.Contains(TEXT("optional move_to target: WildlifeHome_Fenrus_FallenWood")));
			TestFalse(TEXT("A resident cannot sleep at another agent's assigned home"), Controller->TryRest(TEXT("WildlifeHome_Fenrus_FallenWood")));
			TestTrue(TEXT("Rejecting another resident's home leaves consciousness unchanged"), Rest->IsAwake());

			Home.OccupantAgentId = Memory->GetResolvedAgentId();
			TestTrue(TEXT("The assigned home owner is restored"), WorldState->StoreWildlifeHome(Home));
			TestTrue(TEXT("The owner may sleep after arriving at its named home"), Controller->TryRest(TEXT("WildlifeHome_Fenrus_FallenWood")));
			TestFalse(TEXT("Home sleep pauses ordinary thought"), Rest->IsAwake());
			TestTrue(TEXT("Home sleep queues only a cautious lived-place memory"),
				Rest->PendingSleepExperience.Contains(TEXT("Fenrus_FallenWood")) &&
				Rest->PendingSleepExperience.Contains(TEXT("not proof of weather protection")) &&
				!Rest->PendingSleepExperience.Contains(TEXT("sheltered")));
			TestTrue(TEXT("Home sleep never claims verified comfort, safety, or recovery"),
				Controller->DescribeActionState().Contains(TEXT("does not verify weather protection, safety, comfort, or recovery")));
			const FIslandWildlifeHomeRecord* UsedHome = WorldState->FindWildlifeHome(Home.Id);
			TestTrue(TEXT("Using the home updates its persistent last-used time"), UsedHome && UsedHome->LastUsedUtc > FDateTime(2000, 1, 1));

			Rest->WakeUp();
			TestTrue(TEXT("Waking early cancels the pending home memory"), Rest->PendingSleepExperience.IsEmpty());
			TestTrue(TEXT("The owner may also rest without repeating the target after arriving home"), Controller->TryRest());
			Rest->AppendPendingSleepExperience();
			const TArray<FAgentMemoryRecord> MemoriesAfterHomeRest = Memory->GetMemoriesSince(FDateTime::MinValue());
			TestTrue(TEXT("A completed home rest becomes a durable place-specific memory"),
				MemoriesAfterHomeRest.Num() == 2 && MemoriesAfterHomeRest.Last().Text.Contains(TEXT("Fenrus_FallenWood")) &&
				MemoriesAfterHomeRest.Last().Text.Contains(TEXT("not proof of weather protection")));
		}
	}

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	IFileManager::Get().DeleteDirectory(*TestMemoryDirectory, false, true);
	IFileManager::Get().Delete(*WorldStateFile, false, true, true);
	return true;
}
