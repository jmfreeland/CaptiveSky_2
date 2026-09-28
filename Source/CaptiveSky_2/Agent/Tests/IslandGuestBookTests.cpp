#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "IslandWorldStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandGuestBookTest, "CaptiveSky2.Agent.IslandGuestBook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGuestBookTest::RunTest(const FString& Parameters)
{
	const FString Scratch = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandGuestBook"));
	IFileManager::Get().MakeDirectory(*Scratch, true);
	const FString StateFile = Scratch / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));

	auto CreateWorld = [&StateFile]()
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
			.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		if (!World) return static_cast<UWorld*>(nullptr);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->GetSubsystem<UIslandWorldStateSubsystem>()->StorageFileOverride = StateFile;
		AActor* Counter = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		Counter->Tags = { TEXT("IslandInn"), TEXT("InnCounter") };
		World->BeginPlay();
		return World;
	};
	auto DestroyWorld = [](UWorld* World)
	{
		if (!World) return;
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Isolated guest-book fixture created"), World)) return false;
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	ACharacter* Writer = World->SpawnActor<ACharacter>(FVector(400.f, 0.f, 0.f), FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	if (!TestNotNull(TEXT("Resident body created"), Writer) || !TestNotNull(TEXT("Resident controller created"), Controller))
	{
		DestroyWorld(World);
		return false;
	}
	Controller->Possess(Writer);

	FAgentDecision Decision;
	Decision.bValid = true;
	Decision.ActionType = EAgentActionType::Build;
	Decision.ActionTarget = TEXT("GuestBook");
	Decision.Intent = TEXT("  Hello,\tIsland\n\"friends\".  ");
	Controller->ActOnDecision(Decision);
	TestEqual(TEXT("Writing from more than two and a half metres away is refused"), State->GetGuestBookEntries().Num(), 0);
	Writer->SetActorLocation(FVector(100.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Controller->ActOnDecision(Decision);
	const FString WriterId = Writer->GetName();
	TestTrue(TEXT("A nearby resident can leave one durable signed line"),
		State->GetGuestBookEntries().Num() == 1 && State->GetGuestBookEntries()[0].AgentId == WriterId &&
		State->GetGuestBookEntries()[0].Day == 1 && State->GetGuestBookEntries()[0].Line == TEXT("Hello, Island friends"));
	Controller->ActOnDecision(Decision);
	TestEqual(TEXT("The same resident cannot write twice in one Island day"), State->GetGuestBookEntries().Num(), 1);

	UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Writer);
	Writer->AddInstanceComponent(Brain);
	Brain->RegisterComponent();
	const FString Situation = Brain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("A resident near the counter can read the signed guest-book line"),
		Situation.Contains(TEXT("build target: GuestBook")) && Situation.Contains(WriterId) && Situation.Contains(TEXT("Hello, Island friends")));

	bool bChanged = false;
	TestTrue(TEXT("The resident may write again on a later Island day"),
		State->WriteGuestBook(WriterId, TEXT("A second morning."), 2, bChanged).Contains(TEXT("leave this line")) && bChanged);
	TestTrue(TEXT("A different resident may write on the same day"),
		State->WriteGuestBook(TEXT("Guest_Bystander"), TEXT("The pool was silver."), 2, bChanged).Contains(TEXT("leave this line")) && bChanged);
	const int32 BeforeBlankLine = State->GetGuestBookEntries().Num();
	TestTrue(TEXT("An empty cleaned line changes nothing"),
		State->WriteGuestBook(TEXT("Empty_Writer"), TEXT(" \n\"\" "), 2, bChanged).Contains(TEXT("non-empty line")) && !bChanged && State->GetGuestBookEntries().Num() == BeforeBlankLine);
	for (int32 Index = 0; Index < UIslandWorldStateSubsystem::MaxGuestBookEntries + 3; ++Index)
		State->WriteGuestBook(FString::Printf(TEXT("Visitor_%d"), Index), TEXT("A bounded note."), 3 + Index, bChanged);
	TestEqual(TEXT("The guest book retains only its fixed number of recent entries"), State->GetGuestBookEntries().Num(), UIslandWorldStateSubsystem::MaxGuestBookEntries);
	TestFalse(TEXT("The oldest line rolls off when the book is full"),
		State->GetGuestBookEntries().ContainsByPredicate([&WriterId](const FIslandGuestBookEntry& Entry) { return Entry.AgentId == WriterId && Entry.Day == 1; }));
	Controller->UnPossess();
	DestroyWorld(World);

	World = CreateWorld();
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	TestEqual(TEXT("The capped guest book survives a new world session"), State->GetGuestBookEntries().Num(), UIslandWorldStateSubsystem::MaxGuestBookEntries);
	TestTrue(TEXT("The latest signed line is restored from world state"),
		State->GetGuestBookEntries().Last().Line == TEXT("A bounded note") && State->GetGuestBookEntries().Last().AgentId == TEXT("Visitor_26"));
	TestTrue(TEXT("The developer reversal clears all guest-book lines"), State->ForgetGuestBook() && State->GetGuestBookEntries().Num() == 0);
	DestroyWorld(World);
	World = CreateWorld();
	TestEqual(TEXT("Forgotten entries stay gone after a new session"), World->GetSubsystem<UIslandWorldStateSubsystem>()->GetGuestBookEntries().Num(), 0);
	DestroyWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	return true;
}
