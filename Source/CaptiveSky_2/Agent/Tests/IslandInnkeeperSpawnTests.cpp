#include "Misc/AutomationTest.h"
#include "IslandInnkeeperSubsystem.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandInnkeeperSpawnTest, "CaptiveSky2.Agent.IslandInnkeeperSpawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandInnkeeperSpawnTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The Island package name is accepted"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("/Game/Maps/Island")));
	TestTrue(TEXT("A PIE-prefixed Island map name is accepted"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("UEDPIE_2_Island")));
	TestFalse(TEXT("A different map does not receive the innkeeper"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("/Game/Maps/Other")));
	TestFalse(TEXT("A similarly named map is not mistaken for Island"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("Island_Test")));

	const UIslandInnkeeperSubsystem* Defaults = GetDefault<UIslandInnkeeperSubsystem>();
	if (!TestNotNull(TEXT("Innkeeper subsystem defaults are available"), Defaults)) return false;
	TestEqual(TEXT("The innkeeper has an independent stable identity"), Defaults->AgentId, FString(TEXT("Agent_Innkeeper_01")));
	UClass* BodyClass = Defaults->BodyClass.LoadSynchronous();
	if (!TestNotNull(TEXT("The configured shared placeholder body is loadable"), BodyClass)) return false;
	TestTrue(TEXT("The configured body uses the shared autonomous-agent architecture"), BodyClass->IsChildOf(AAutonomousAgentCharacter::StaticClass()));

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Isolated innkeeper fixture world created"), World)) return false;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAutonomousAgentCharacter* Resident = World->SpawnActor<AAutonomousAgentCharacter>(BodyClass, FTransform::Identity, Spawn);
	if (!TestNotNull(TEXT("Placeholder body spawns without starting a world or making a model request"), Resident))
	{
		World->DestroyWorld(false);
		return false;
	}

	UIslandInnkeeperSubsystem::InitializeInnkeeper(Resident, Defaults->AgentId);
	if (!TestNotNull(TEXT("The resident has its own memory component"), Resident->Memory.Get()) ||
		!TestNotNull(TEXT("The resident has its own autonomous brain component"), Resident->Brain.Get()))
	{
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("The independent memory directory identity is assigned before BeginPlay"), Resident->Memory->GetResolvedAgentId(), FString(TEXT("Agent_Innkeeper_01")));
	TestEqual(TEXT("The resident uses its friendly display name"), Resident->GetAgentDisplayName(), FString(TEXT("Innkeeper")));
	Resident->DisplayName.Reset();
	TestEqual(TEXT("Residents without an authored display name retain their actor label"), Resident->GetAgentDisplayName(), Resident->GetActorNameOrLabel());
	Resident->DisplayName = TEXT("Innkeeper");
	TestTrue(TEXT("The resident is clearly identifiable by role"), Resident->ActorHasTag(TEXT("IslandInnkeeper")));
	TestTrue(TEXT("The memory file is isolated in the resident's own agent folder"), Resident->Memory->GetMemoryFilePath().Contains(TEXT("Agent_Innkeeper_01")));
	TestTrue(TEXT("The independent identity document loads from its own agent folder"), Resident->Memory->LoadAgentDocument(TEXT("identity.md")).Contains(TEXT("made a home in the inn")));
	TestTrue(TEXT("The independent personality document loads from its own agent folder"), Resident->Memory->LoadAgentDocument(TEXT("personality.md")).Contains(TEXT("welcoming without insisting on company")));
	TestFalse(TEXT("Fixture tests never dispatch an LLM request"), Resident->Brain->bRequestInFlight);

	World->DestroyWorld(false);
	return true;
}
