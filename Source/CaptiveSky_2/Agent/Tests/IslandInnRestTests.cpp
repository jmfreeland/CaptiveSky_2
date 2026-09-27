#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentMemoryComponent.h"
#include "IslandEnvironmentSubsystem.h"
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
	ACharacter* Resident = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	if (!TestNotNull(TEXT("Tagged inn bed spawned"), Bed) || !TestNotNull(TEXT("Resident body spawned"), Resident) ||
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

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	IFileManager::Get().DeleteDirectory(*TestMemoryDirectory, false, true);
	return true;
}
