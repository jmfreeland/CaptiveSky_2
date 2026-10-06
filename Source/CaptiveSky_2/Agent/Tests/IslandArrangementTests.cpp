#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentDataPaths.h"
#include "AgentMemoryComponent.h"
#include "IslandFirefly.h"
#include "IslandTidepoolCrab.h"
#include "IslandTidepoolMinnows.h"
#include "IslandArrangement.h"
#include "IslandDayNight.h"
#include "CaptiveSkyArrangementWidget.h"
#include "IslandInteractionTestPlayerController.h"
#include "IslandInteractionUtility.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SWindow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandArrangementTest, "CaptiveSky2.Agent.IslandArrangement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandArrangementInspectionTest, "CaptiveSky2.Agent.IslandArrangementInspection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	struct FScopedArrangementAgentData
	{
		TArray<FString> Directories;

		void Track(const FString& AgentId)
		{
			Directories.Add(CaptiveSkyDataPaths::ResolveProjectDataPath(FString::Printf(TEXT("Agents/%s"), *AgentId)));
		}

		~FScopedArrangementAgentData()
		{
			for (const FString& Directory : Directories)
				IFileManager::Get().DeleteDirectory(*Directory, false, true);
		}
	};

	UWorld* CreateArrangementWorld(const FString& StateFile)
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->GetSubsystem<UIslandWorldStateSubsystem>()->StorageFileOverride = StateFile;
		AActor* Ground = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Ground);
		Ground->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(8000, 8000, 50));
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Ground->SetActorLocation(FVector(0, 0, -50));
		ATargetPoint* Stones = World->SpawnActor<ATargetPoint>(FVector(0, 0, 150), FRotator::ZeroRotator);
		Stones->Tags = {TEXT("ListeningStones"), TEXT("IslandLandmark")};
		return World;
	}

	void DestroyArrangementWorld(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	AIslandArrangement* FindArrangementActor(UWorld* World, FName Id)
	{
		for (TActorIterator<AIslandArrangement> It(World); It; ++It)
			if (It->ActorHasTag(Id)) return *It;
		return nullptr;
	}
}

bool FIslandArrangementTest::RunTest(const FString& Parameters)
{
	// No gateway or model requests. Isolated, unique identities keep the place-memory regression
	// from reading or overwriting any actual resident's data.
	const FString TestSuffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString BystanderAgentId = TEXT("ArrangementBystander_") + TestSuffix;
	const FString ArrangerAgentId = TEXT("ArrangementMaker_") + TestSuffix;
	const FString EmptySiteObserverAgentId = TEXT("EmptyArrangementObserver_") + TestSuffix;
	const FString LineageObserverAgentId = TEXT("LineageObserver_") + TestSuffix;
	FScopedArrangementAgentData AgentDataCleanup;
	AgentDataCleanup.Track(BystanderAgentId);
	AgentDataCleanup.Track(ArrangerAgentId);
	AgentDataCleanup.Track(EmptySiteObserverAgentId);
	AgentDataCleanup.Track(LineageObserverAgentId);
	auto DescribeFrom = [](UWorld* World, const FVector& Where, const FString& AgentId)
	{
		ACharacter* Body = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		if (!AgentId.IsEmpty())
		{
			UAgentMemoryComponent* Memory = NewObject<UAgentMemoryComponent>(Body);
			Memory->AgentId = AgentId;
			Body->AddInstanceComponent(Memory);
			Memory->RegisterComponent();
		}
		UAgentBrainComponent* Brain = NewObject<UAgentBrainComponent>(Body);
		Body->AddInstanceComponent(Brain);
		Brain->RegisterComponent();
		const FString View = Brain->BuildSituationSummary(FAgentConversationContext());
		Body->Destroy();
		return View;
	};
	auto Arrange = [](ARavenAgentAIController* Controller, const FString& Site, const FString& Form, const FString& Title, const FString& Intent)
	{
		FAgentDecision Decision;
		Decision.bValid = true;
		Decision.ActionType = EAgentActionType::Build;
		Decision.ActionTarget = Site;
		Decision.Form = Form;
		Decision.Title = Title;
		Decision.Intent = Intent;
		Controller->ActOnDecision(Decision);
		return Controller->DescribeActionState();
	};

	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandArrangement") / TEXT("WorldState.json"));
	IFileManager::Get().Delete(*StateFile, false, true, true);
	UWorld* World = CreateArrangementWorld(StateFile);
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();

	ACharacter* WildlifeObserver = World->SpawnActor<ACharacter>(FVector(5000.f, 5000.f, 150.f), FRotator::ZeroRotator);
	UAgentBrainComponent* WildlifeBrain = WildlifeObserver ? NewObject<UAgentBrainComponent>(WildlifeObserver) : nullptr;
	if (WildlifeBrain)
	{
		WildlifeObserver->AddInstanceComponent(WildlifeBrain);
		WildlifeBrain->RegisterComponent();
	}
	for (int32 Index = 0; Index < 4; ++Index)
	{
		AIslandFirefly* Firefly = World->SpawnActor<AIslandFirefly>(FVector(5100.f + 100.f * Index, 5000.f, 150.f), FRotator::ZeroRotator);
		if (Firefly) Firefly->Tags.Append({TEXT("IslandLife"), TEXT("Firefly")});
	}
	AIslandTidepoolCrab* WildlifeCrab = World->SpawnActor<AIslandTidepoolCrab>(FVector(5300.f, 5000.f, 150.f), FRotator::ZeroRotator);
	AIslandTidepoolMinnows* WildlifeMinnows = World->SpawnActor<AIslandTidepoolMinnows>(FVector(5400.f, 5000.f, 150.f), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Wildlife perception observer spawned"), WildlifeObserver) && TestNotNull(TEXT("Wildlife perception brain registered"), WildlifeBrain))
	{
		const FString WildlifeView = WildlifeBrain->BuildSituationSummary(FAgentConversationContext());
		AddInfo(FString::Printf(TEXT("Wildlife perception fixture summary: %s"), *WildlifeView));
		int32 FireflyDescriptionCount = 0;
		int32 SearchAt = 0;
		while ((SearchAt = WildlifeView.Find(TEXT("A small firefly glow"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchAt)) != INDEX_NONE)
		{
			++FireflyDescriptionCount;
			++SearchAt;
		}
		TestEqual(TEXT("Several visible fireflies produce one species-level observation"), FireflyDescriptionCount, 1);
		TestTrue(TEXT("The nearest firefly, not an arbitrary member of the species, is described"), WildlifeView.Contains(TEXT("firefly glow is drifting independently nearby, about 1 metres away")));
		TestTrue(TEXT("A larger firefly group does not crowd the shore crab out of perception"), WildlifeView.Contains(TEXT("A small shore crab is scuttling independently")));
		TestTrue(TEXT("A larger firefly group does not crowd the minnows out of perception"), WildlifeView.Contains(TEXT("A small school of minnows is circling")));
	}

	const TArray<FIslandArrangementSite>& Sites = State->GetArrangementSites();
	if (!TestEqual(TEXT("Four arranging grounds are placed near the ListeningStones"), Sites.Num(), 4)) { DestroyArrangementWorld(World); return false; }
	for (const FIslandArrangementSite& Site : Sites)
	{
		TestTrue(*FString::Printf(TEXT("%s is a short walk from the stones"), *Site.Id.ToString()), FVector::Dist2D(Site.Location, FVector::ZeroVector) <= 1250.f);
		TestFalse(*FString::Printf(TEXT("%s starts empty"), *Site.Id.ToString()), Site.bHasWork);
		TestNotNull(*FString::Printf(TEXT("%s is visible as soon as the sites are first placed"), *Site.Id.ToString()), FindArrangementActor(World, Site.Id));
		for (const FIslandCurioRecord& Curio : State->GetCurios())
			TestTrue(*FString::Printf(TEXT("%s keeps clear of %s"), *Site.Id.ToString(), *Curio.Id.ToString()), FVector::Dist2D(Site.Location, Curio.Location) >= 399.f);
	}
	const FIslandArrangementSite First = Sites[0];
	TestTrue(TEXT("Empty ground is offered as a build target up close"), DescribeFrom(World, First.Location + FVector(150, 0, 100), FString()).Contains(TEXT("build target: ArrangingGround_1")));
	DescribeFrom(World, First.Location + FVector(0.f, 200.f, 100.f), EmptySiteObserverAgentId);
	const FString EmptyGroundReturn = DescribeFrom(World, First.Location + FVector(2000.f, 0.f, 100.f), EmptySiteObserverAgentId);
	TestFalse(TEXT("An empty arranging ground is not remembered as a discovered work"),
		EmptyGroundReturn.Contains(TEXT("move_to target: ArrangingGround_1")));

	ACharacter* Maker = World->SpawnActor<ACharacter>(First.Location + FVector(3000, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* MakerController = World->SpawnActor<ARavenAgentAIController>();
	MakerController->Possess(Maker);
	TestTrue(TEXT("Arranging from afar is refused"), Arrange(MakerController, TEXT("ArrangingGround_1"), TEXT("ring"), TEXT("Far"), TEXT("")).Contains(TEXT("too far")));
	Maker->SetActorLocation(First.Location + FVector(120, 0, 100));
	TestTrue(TEXT("An unknown form is refused"), Arrange(MakerController, TEXT("ArrangingGround_1"), TEXT("pyramid"), TEXT("X"), TEXT("")).Contains(TEXT("ring, line, spiral, or pair")));
	const FString Made = Arrange(MakerController, TEXT("ArrangingGround_1"), TEXT(" Ring "), TEXT("Evening \"watch\"\nfor the tide, which is a title far longer than sixty characters allow"), TEXT("To mark where the chime\tcarries farthest."));
	const FIslandArrangementSite* Work = State->FindArrangementSite(TEXT("ArrangingGround_1"));
	TestTrue(TEXT("A resident makes a lasting ring"), Work && Work->bHasWork && Work->Form == EIslandArrangementForm::Ring && Made.Contains(TEXT("stays in the world")));
	TestTrue(TEXT("Titles are single-line, bounded, and quote-free"), Work && Work->Title.Len() <= 60 && !Work->Title.Contains(TEXT("\n")) && !Work->Title.Contains(TEXT("\"")) && Work->Title.StartsWith(TEXT("Evening watch for the tide")));
	TestTrue(TEXT("Intent is kept in the maker's words"), Work && Work->Intent == TEXT("To mark where the chime carries farthest"));
	AIslandArrangement* Visible = FindArrangementActor(World, TEXT("ArrangingGround_1"));
	TestTrue(TEXT("The ring is visible with its stones"), Visible && Visible->GetVisibleStoneCount() == AIslandArrangement::StoneCountFor(EIslandArrangementForm::Ring));
	TestEqual(TEXT("A new work carries one small, visible three-stone signature"), Visible ? Visible->GetVisibleMotifCount() : 0, 3);
	Maker->SetActorLocation(Sites[1].Location + FVector(120, 0, 100));
	TestTrue(TEXT("One arrangement per resident per Island day"), Arrange(MakerController, TEXT("ArrangingGround_2"), TEXT("line"), TEXT("Again"), TEXT("")).Contains(TEXT("already arranged stones today")));

	// Others see the shape and age, never the maker, title, or intent.
	const FString Bystander = DescribeFrom(World, First.Location + FVector(0, 200, 100), BystanderAgentId);
	TestTrue(*FString::Printf(TEXT("A bystander sees a fresh ring (%s)"), *Bystander), Bystander.Contains(TEXT("arranged 9 freshly placed stones here into a ring")));
	TestTrue(TEXT("A first unobstructed sighting becomes bounded, persisted cultural evidence"),
		State->FindArrangementSite(TEXT("ArrangingGround_1"))->ObservedBy.Contains(BystanderAgentId));
	TestFalse(TEXT("The title and intent stay private"), Bystander.Contains(TEXT("Evening")) || Bystander.Contains(TEXT("chime carries")));
	TestTrue(TEXT("A bystander is invited to respond"), Bystander.Contains(TEXT("respond by setting a few small stones")));
	const FString RememberedWork = DescribeFrom(World, First.Location + FVector(2000.f, 0.f, 100.f), BystanderAgentId);
	TestTrue(TEXT("A bystander privately remembers a completed arrangement after walking out of sight"),
		RememberedWork.Contains(TEXT("You remember a stone ring")) && RememberedWork.Contains(TEXT("move_to target: ArrangingGround_1")));
	TestFalse(TEXT("A remembered public work still reveals neither its private title nor intent"),
		RememberedWork.Contains(TEXT("Evening")) || RememberedWork.Contains(TEXT("chime carries")));
	bool bInfluenceChanged = true;
	const FString UnseenInfluence = State->ArrangeStones(TEXT("ArrangingGround_4"), TEXT("spiral"), TEXT("Borrowed shape"), TEXT(""),
		EmptySiteObserverAgentId, -10, bInfluenceChanged, TEXT("ArrangingGround_1"));
	TestFalse(TEXT("An unobserved public work cannot be claimed as inspiration"), bInfluenceChanged);
	TestTrue(TEXT("A rejected inspiration leaves the destination empty"), !State->FindArrangementSite(TEXT("ArrangingGround_4"))->bHasWork);
	TestTrue(TEXT("The rejection explains why the source cannot be cited"), UnseenInfluence.Contains(TEXT("visibly encountered")));
	const FString Untransformed = State->ArrangeStones(TEXT("ArrangingGround_4"), TEXT("ring"), TEXT("Same shape"), TEXT(""),
		BystanderAgentId, -10, bInfluenceChanged, TEXT("ArrangingGround_1"));
	TestFalse(TEXT("A lineage must transform rather than duplicate the source form"), bInfluenceChanged);
	TestTrue(TEXT("The unchanged form is rejected clearly"), Untransformed.Contains(TEXT("different form")));
	const FString Descendant = State->ArrangeStones(TEXT("ArrangingGround_4"), TEXT("spiral"), TEXT("A turning echo"), TEXT("For another return"),
		BystanderAgentId, -10, bInfluenceChanged, TEXT("ArrangingGround_1"));
	const FIslandArrangementSite* DescendantWork = State->FindArrangementSite(TEXT("ArrangingGround_4"));
	TestTrue(TEXT("A resident can make a different-form descendant from remembered sight"), bInfluenceChanged && Descendant.Contains(TEXT("transformed echo")) &&
		DescendantWork && DescendantWork->InfluenceSiteId == FName(TEXT("ArrangingGround_1")) && DescendantWork->Form == EIslandArrangementForm::Spiral);
	const AIslandArrangement* DescendantActor = FindArrangementActor(World, TEXT("ArrangingGround_4"));
	bool bSharedMotifTransformsMatch = Visible && DescendantActor && Visible->MotifStones && DescendantActor->MotifStones &&
		Visible->MotifStones->GetInstanceCount() == 3 && DescendantActor->MotifStones->GetInstanceCount() == 3;
	if (bSharedMotifTransformsMatch)
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FTransform SourceMark, DescendantMark;
			Visible->MotifStones->GetInstanceTransform(Index, SourceMark, false);
			DescendantActor->MotifStones->GetInstanceTransform(Index, DescendantMark, false);
			bSharedMotifTransformsMatch &= SourceMark.Equals(DescendantMark, 0.01f);
		}
	}
	TestTrue(TEXT("A transformed descendant visibly repeats its source's stable motif in the new arrangement"),
		DescendantWork && DescendantWork->MotifSeed == Work->MotifSeed && bSharedMotifTransformsMatch);
	const FString LineageView = DescendantWork
		? DescribeFrom(World, DescendantWork->Location + FVector(150.f, 0.f, 100.f), LineageObserverAgentId)
		: FString();
	TestTrue(TEXT("A later observer can follow the public motif lineage without learning private intent"),
		LineageView.Contains(TEXT("transformation of the ring at ArrangingGround_1")) &&
		!LineageView.Contains(TEXT("For another return")) && !LineageView.Contains(TEXT("A turning echo")));

	// A second resident answers it.
	ACharacter* Responder = World->SpawnActor<ACharacter>(First.Location + FVector(-120, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* ResponderController = World->SpawnActor<ARavenAgentAIController>();
	ResponderController->Possess(Responder);
	TestTrue(TEXT("Another resident responds"), Arrange(ResponderController, TEXT("ArrangingGround_1"), FString(), FString(), TEXT("An answer from the other side.")).Contains(TEXT("as your response")));
	TestEqual(TEXT("The response is recorded"), State->FindArrangementSite(TEXT("ArrangingGround_1"))->Responses.Num(), 1);
	TestEqual(TEXT("The response adds visible stones"), FindArrangementActor(World, TEXT("ArrangingGround_1"))->GetVisibleStoneCount(), 9 + AIslandArrangement::StonesPerResponse);
	bool bChanged = true;
	State->ArrangeStones(TEXT("ArrangingGround_1"), FString(), FString(), TEXT("Once more"), Responder->GetName(), 2, bChanged);
	TestFalse(TEXT("One response per resident per work"), bChanged);
	State->ArrangeStones(TEXT("ArrangingGround_1"), FString(), FString(), TEXT("My own"), Maker->GetName(), 2, bChanged);
	TestFalse(TEXT("Makers do not respond to their own work"), bChanged);

	// A remembered maker recognizes their own work and its private meaning.
	State->ArrangeStones(TEXT("ArrangingGround_3"), TEXT("spiral"), TEXT("Slow turning"), TEXT("For the raven's return"), ArrangerAgentId, 1, bChanged);
	TestTrue(TEXT("A spiral is made directly"), bChanged);
	const FIslandArrangementSite* Third = State->FindArrangementSite(TEXT("ArrangingGround_3"));
	const FString MakerView = DescribeFrom(World, Third->Location + FVector(150, 0, 100), ArrangerAgentId);
	TestTrue(TEXT("The maker recognizes their own work and remembers its meaning"), MakerView.Contains(TEXT("your own stone spiral, \"Slow turning\"")) && MakerView.Contains(TEXT("For the raven's return")));

	// Weathering: a week of Island days darkens the stones toward moss.
	FIslandArrangementSite Aged = *State->FindArrangementSite(TEXT("ArrangingGround_1"));
	Visible = FindArrangementActor(World, TEXT("ArrangingGround_1"));
	const FLinearColor Fresh = Visible->GetCurrentTint();
	Visible->ShowSite(Aged, Aged.Day + AIslandArrangement::DaysToWeather);
	TestTrue(TEXT("Old work looks weathered"), !Visible->GetCurrentTint().Equals(Fresh) && Visible->GetCurrentTint().Equals(AIslandArrangement::WeatheredTint(AIslandArrangement::DaysToWeather)));
	TestTrue(TEXT("Perception describes old work as mossy"), [&]() { State->ArrangeStones(TEXT("ArrangingGround_4"), TEXT("pair"), TEXT("Old"), FString(), TEXT("Someone"), -10, bChanged); return DescribeFrom(World, State->FindArrangementSite(TEXT("ArrangingGround_4"))->Location + FVector(150, 0, 100), FString()).Contains(TEXT("mossy and settled")); }());

	// Visitors can make the same bounded, lasting contribution through the local E interaction.
	AIslandArrangement* VisitorSite = FindArrangementActor(World, TEXT("ArrangingGround_2"));
	ACharacter* VisitorBody = World->SpawnActor<ACharacter>(Sites[1].Location + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
	AIslandInteractionTestPlayerController* VisitorController = World->SpawnActor<AIslandInteractionTestPlayerController>();
	UCaptiveSkyArrangementWidget* ArrangementWidget = VisitorController
		? NewObject<UCaptiveSkyArrangementWidget>(World, NAME_None, RF_Transient)
		: nullptr;
	TestTrue(TEXT("Slate is initialized for keyboard-focus assertions"), FSlateApplication::IsInitialized());
	TSharedPtr<SWindow> ArrangementSlateWindow;
	if (ArrangementWidget && FSlateApplication::IsInitialized())
	{
		const bool bWidgetInitialized = ArrangementWidget->Initialize();
		TestTrue(TEXT("Visitor arrangement Slate widget initializes"), bWidgetInitialized);
		if (bWidgetInitialized)
		{
			// A real Slate window gives SetKeyboardFocus a valid widget path without using the game viewport.
			ArrangementSlateWindow = SNew(SWindow)
				.Title(FText::FromString(TEXT("Arrangement focus fixture")))
				.ClientSize(FVector2D(900.f, 360.f))
				.SupportsMaximize(false);
			ArrangementSlateWindow->SetContent(ArrangementWidget->TakeWidget());
			FSlateApplication::Get().AddWindow(ArrangementSlateWindow.ToSharedRef(), true);
		}
	}
	if (TestNotNull(TEXT("Visitor's open arranging site has its stable id"), VisitorSite) &&
		TestEqual(TEXT("The arranging site is an ordinary visible E target"), IslandInteractionUtility::GetTargetTag(VisitorSite), FName(TEXT("IslandArrangement"))) &&
		TestNotNull(TEXT("Visitor body created beside open ground"), VisitorBody) &&
		TestNotNull(TEXT("Visitor controller created for the bound E path"), VisitorController) &&
		TestNotNull(TEXT("Visitor arrangement panel created"), ArrangementWidget))
	{
		VisitorController->SetFixturePawn(VisitorBody);
		VisitorController->SetArrangementWidget(ArrangementWidget);
		VisitorController->BindFixtureInput();
		TestTrue(TEXT("Pressing E opens the nearby arranging-ground panel"), VisitorController->PressBoundE() && VisitorController->IsArrangementPanelOpenForTest());
		if (FSlateApplication::IsInitialized())
			TestTrue(TEXT("A new arrangement focuses its visible title field"), ArrangementWidget->TitleBox.IsValid() && ArrangementWidget->TitleBox->HasKeyboardFocus());
		TestTrue(TEXT("The panel states the durable/public shape and private intent policy"),
			ArrangementWidget->GetDisplayedContent().Contains(TEXT("private unless you share")) && ArrangementWidget->GetDisplayedContent().Contains(TEXT("persists with the Island")));
		VisitorController->SubmitVisitorArrangement(TEXT("spiral"), TEXT("A visitor's turning mark"), TEXT("A small hello to the raven"));
		const FIslandArrangementSite* VisitorWork = State->FindArrangementSite(TEXT("ArrangingGround_2"));
		TestTrue(TEXT("A visitor can create a lasting spiral at an empty site"), VisitorWork && VisitorWork->bHasWork &&
			VisitorWork->MakerAgentId == TEXT("Visitor") && VisitorWork->Form == EIslandArrangementForm::Spiral &&
			VisitorWork->Title == TEXT("A visitor's turning mark") && VisitorWork->Intent == TEXT("A small hello to the raven"));
		TestTrue(TEXT("A visitor's contribution counts against their one-per-Island-day limit"), State->HasArrangedStonesToday(TEXT("Visitor"), 1));
		TestTrue(TEXT("The persistent actor picks up the visitor's stones"), VisitorSite->GetVisibleStoneCount() == AIslandArrangement::StoneCountFor(EIslandArrangementForm::Spiral));
		TestTrue(TEXT("The visitor receives an honest save result"), ArrangementWidget->GetDisplayedContent().Contains(TEXT("stays in the world")));
		TestFalse(TEXT("The visitor has used today's one-contribution limit"), ArrangementWidget->CanContribute());
		const FString VisitorBystanderView = DescribeFrom(World, VisitorWork->Location + FVector(0, 200, 100), BystanderAgentId);
		TestTrue(TEXT("Other residents see the visitor's form but not their title or intent"),
			VisitorBystanderView.Contains(TEXT("spiral")) && !VisitorBystanderView.Contains(TEXT("turning mark")));
		TestTrue(TEXT("Escape closes the arrangement panel"), VisitorController->PressBoundEscape() && !VisitorController->IsArrangementPanelOpenForTest());

		// The same visitor may respond on a later Island day, but still only once that day.
		VisitorBody->SetActorLocation(Sites[0].Location + FVector(0.f, 0.f, 100.f));
		AIslandDayNight* TestClock = World->SpawnActor<AIslandDayNight>();
		if (TestClock) TestClock->DayNumber = 2;
		TestTrue(TEXT("An existing arrangement is an ordinary visible E target on the next Island day"),
			VisitorController->PressBoundE() && VisitorController->IsArrangementPanelOpenForTest());
		if (FSlateApplication::IsInitialized())
			TestTrue(TEXT("Responding to existing work focuses the visible intent field"), ArrangementWidget->IntentBox.IsValid() && ArrangementWidget->IntentBox->HasKeyboardFocus());
		VisitorController->SubmitVisitorArrangementResponse(TEXT("Another morning, another answer"));
		const FIslandArrangementSite* RespondedWork = State->FindArrangementSite(TEXT("ArrangingGround_1"));
		TestTrue(TEXT("A visitor can add a response on the following Island day"), RespondedWork &&
			RespondedWork->Responses.ContainsByPredicate([](const FIslandArrangementResponse& Response)
				{ return Response.AgentId == TEXT("Visitor") && Response.Day == 2 && Response.Intent == TEXT("Another morning, another answer"); }));
		TestTrue(TEXT("The visible arrangement grows by three response stones"),
			FindArrangementActor(World, TEXT("ArrangingGround_1"))->GetVisibleStoneCount() == 9 + 2 * AIslandArrangement::StonesPerResponse);
		TestFalse(TEXT("The visitor cannot make a second contribution on the new Island day"), ArrangementWidget->CanContribute());
	}
	if (ArrangementSlateWindow.IsValid() && FSlateApplication::IsInitialized())
		FSlateApplication::Get().RequestDestroyWindow(ArrangementSlateWindow.ToSharedRef());
	if (VisitorController) VisitorController->Destroy();
	MakerController->UnPossess();
	// The fixture world is destroyed below; explicit unpossess can touch a torn-down brain in commandlet runs.
	DestroyArrangementWorld(World);

	// A later session restores every site, work, and response.
	World = CreateArrangementWorld(StateFile);
	State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	World->BeginPlay();
	TestEqual(TEXT("Sites are not placed a second time"), State->GetArrangementSites().Num(), 4);
	TestTrue(TEXT("Sites stay where they were"), State->FindArrangementSite(TEXT("ArrangingGround_1")) && State->FindArrangementSite(TEXT("ArrangingGround_1"))->Location.Equals(First.Location, 0.5f));
	const FIslandArrangementSite* Restored = State->FindArrangementSite(TEXT("ArrangingGround_1"));
	TestTrue(TEXT("The ring and both responses persist"), Restored && Restored->bHasWork && Restored->Responses.Num() == 2 && Restored->Intent == TEXT("To mark where the chime carries farthest"));
	const FIslandArrangementSite* RestoredVisitorWork = State->FindArrangementSite(TEXT("ArrangingGround_2"));
	TestTrue(TEXT("The visitor's work survives a new Island session"), RestoredVisitorWork && RestoredVisitorWork->MakerAgentId == TEXT("Visitor") &&
		RestoredVisitorWork->Form == EIslandArrangementForm::Spiral && RestoredVisitorWork->Title == TEXT("A visitor's turning mark"));
	const FIslandArrangementSite* RestoredDescendant = State->FindArrangementSite(TEXT("ArrangingGround_4"));
	TestTrue(TEXT("The observed-source evidence and transformed lineage survive a new Island session"),
		Restored && Restored->ObservedBy.Contains(BystanderAgentId) && RestoredDescendant &&
		RestoredDescendant->InfluenceSiteId == FName(TEXT("ArrangingGround_1")) && RestoredDescendant->MakerAgentId == BystanderAgentId);
	const AIslandArrangement* RestoredSourceActor = FindArrangementActor(World, TEXT("ArrangingGround_1"));
	const AIslandArrangement* RestoredDescendantActor = FindArrangementActor(World, TEXT("ArrangingGround_4"));
	bool bRestoredMotifsMatch = RestoredSourceActor && RestoredDescendantActor &&
		RestoredSourceActor->MotifStones && RestoredDescendantActor->MotifStones &&
		RestoredSourceActor->MotifStones->GetInstanceCount() == 3 && RestoredDescendantActor->MotifStones->GetInstanceCount() == 3;
	if (bRestoredMotifsMatch)
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FTransform SourceMark, DescendantMark;
			RestoredSourceActor->MotifStones->GetInstanceTransform(Index, SourceMark, false);
			RestoredDescendantActor->MotifStones->GetInstanceTransform(Index, DescendantMark, false);
			bRestoredMotifsMatch &= SourceMark.Equals(DescendantMark, 0.01f);
		}
	}
	TestTrue(TEXT("The lineage's shared visual motif survives serialization and materialization"),
		Restored && RestoredDescendant && Restored->MotifSeed == RestoredDescendant->MotifSeed && bRestoredMotifsMatch);
	TestTrue(TEXT("The visitor's daily limit follows their persistent contribution"), State->HasArrangedStonesToday(TEXT("Visitor"), 1));
	TestTrue(TEXT("The restored work is visible"), FindArrangementActor(World, TEXT("ArrangingGround_1")) && FindArrangementActor(World, TEXT("ArrangingGround_1"))->GetVisibleStoneCount() == 9 + 2 * AIslandArrangement::StonesPerResponse);
	TestTrue(TEXT("Developer reset forgets arrangements"), State->ForgetArrangements() && State->GetArrangementSites().Num() == 0 && !FindArrangementActor(World, TEXT("ArrangingGround_1")));
	DestroyArrangementWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);

	// On the saved Island: sites must exist on level walkable ground clear of the curios.
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Island = Context.World();
		if (Context.WorldType != EWorldType::Editor || !Island || Island->GetMapName() != TEXT("Island")) continue;
		TArray<FIslandCurioRecord> Curios;
		TArray<FVector> Avoid;
		if (UIslandWorldStateSubsystem::BuildCurioLayout(Island, Curios))
			for (const FIslandCurioRecord& Curio : Curios) Avoid.Add(Curio.Location);
		TArray<FIslandArrangementSite> IslandSites;
		if (!TestTrue(TEXT("The saved Island has level open ground for four arranging sites"), UIslandWorldStateSubsystem::BuildArrangementSiteLayout(Island, Avoid, IslandSites))) continue;
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Island);
		FVector Start = FVector::ZeroVector;
		for (TActorIterator<AActor> It(Island); It; ++It)
			if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"))) Start = It->GetActorLocation();
		FNavLocation StartOnNav;
		const bool bStartWalkable = Navigation && Navigation->ProjectPointToNavigation(Start, StartOnNav, FVector(250, 250, 1000));
		for (const FIslandArrangementSite& Site : IslandSites)
		{
			const UNavigationPath* Path = bStartWalkable ? Navigation->FindPathToLocationSynchronously(Island, StartOnNav.Location, Site.Location) : nullptr;
			const bool bReachable = Path && Path->IsValid() && !Path->IsPartial();
			AddInfo(FString::Printf(TEXT("%s at %s, %.1f m from the ListeningStones, walk path %s"), *Site.Id.ToString(), *Site.Location.ToString(),
				FVector::Dist2D(Site.Location, Start) / 100.f, bReachable ? *FString::Printf(TEXT("%.1f m"), Path->GetPathLength() / 100.f) : TEXT("missing")));
			TestTrue(*FString::Printf(TEXT("Grounded residents can walk to %s"), *Site.Id.ToString()), bReachable);
		}
	}
	return true;
}

bool FIslandArrangementInspectionTest::RunTest(const FString& Parameters)
{
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() /
		TEXT("Automation") / TEXT("IslandArrangementInspection") / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json")));
	IFileManager::Get().Delete(*StateFile, false, true, true);
	UWorld* World = CreateArrangementWorld(StateFile);
	UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!World || !State)
	{
		AddError(TEXT("Could not create an isolated arrangement-inspection world."));
		if (World) DestroyArrangementWorld(World);
		IFileManager::Get().Delete(*StateFile, false, true, true);
		return false;
	}
	World->BeginPlay();
	const FIslandArrangementSite* EmptySite = State->FindArrangementSite(TEXT("ArrangingGround_1"));
	AIslandArrangement* VisibleWork = FindArrangementActor(World, TEXT("ArrangingGround_1"));
	ACharacter* Observer = EmptySite ? World->SpawnActor<ACharacter>(EmptySite->Location + FVector(0.f, 200.f, 100.f), FRotator::ZeroRotator) : nullptr;
	const FString ObserverId = TEXT("DirectArrangementInspector");
	if (Observer)
	{
		UAgentMemoryComponent* Memory = NewObject<UAgentMemoryComponent>(Observer);
		Memory->AgentId = ObserverId;
		Observer->AddInstanceComponent(Memory);
		Memory->RegisterComponent();
	}
	FString Fact;
	TestTrue(TEXT("An observer can inspect an empty arranging site"), EmptySite && VisibleWork && Observer &&
		IslandInteractionUtility::Perform(Observer, VisibleWork, Fact));
	TestTrue(TEXT("Empty-site inspection offers the separate build affordance without changing the site"),
		Fact.Contains(TEXT("has no arrangement yet")) && Fact.Contains(TEXT("changes nothing")) && EmptySite && !EmptySite->bHasWork);

	bool bChanged = false;
	State->ArrangeStones(TEXT("ArrangingGround_1"), TEXT("spiral"), TEXT("Private title"), TEXT("Private intent"), TEXT("InspectionMaker"), 1, bChanged);
	TestTrue(TEXT("Inspection fixture creates its sample work"), bChanged);
	const FIslandArrangementSite* Work = State->FindArrangementSite(TEXT("ArrangingGround_1"));
	VisibleWork = FindArrangementActor(World, TEXT("ArrangingGround_1"));
	Fact.Reset();
	const bool bInspectedWork = Work && VisibleWork && Observer && IslandInteractionUtility::CanInspect(Observer, VisibleWork) &&
		IslandInteractionUtility::Perform(Observer, VisibleWork, Fact);
	TestTrue(TEXT("An observer can inspect visible completed work"), bInspectedWork);
	TestTrue(TEXT("Inspection reports only public form, weathering, and response count"),
		Fact.Contains(TEXT("spiral")) && Fact.Contains(TEXT("freshly placed")) && Fact.Contains(TEXT("0 small arcs")) &&
		Fact.Contains(TEXT("Looking changes nothing")));
	Work = State->FindArrangementSite(TEXT("ArrangingGround_1"));
	TestTrue(TEXT("A resident's explicit visible inspection records the work as encountered"),
		Work && Work->ObservedBy.Contains(ObserverId));
	TestFalse(TEXT("Inspection withholds the maker, title, and private intent"),
		Fact.Contains(TEXT("InspectionMaker")) || Fact.Contains(TEXT("Private title")) || Fact.Contains(TEXT("Private intent")));
	TestTrue(TEXT("Read-only inspection adds no persistent response"), Work && Work->Responses.IsEmpty());

	bool bDescendantCreated = false;
	State->ArrangeStones(TEXT("ArrangingGround_2"), TEXT("line"), TEXT("Private descendant title"), TEXT("Private descendant intent"),
		ObserverId, 1, bDescendantCreated, TEXT("ArrangingGround_1"));
	TestTrue(TEXT("The resident can transform the source only after its explicit inspection persisted a sighting"), bDescendantCreated);
	AIslandArrangement* DescendantActor = FindArrangementActor(World, TEXT("ArrangingGround_2"));
	const FIslandArrangementSite* Descendant = State->FindArrangementSite(TEXT("ArrangingGround_2"));
	if (Observer && DescendantActor) Observer->SetActorLocation(DescendantActor->GetActorLocation() + FVector(0.f, 200.f, 100.f));
	Fact.Reset();
	const bool bInspectedDescendant = Descendant && DescendantActor && Observer && IslandInteractionUtility::CanInspect(Observer, DescendantActor) &&
		IslandInteractionUtility::Perform(Observer, DescendantActor, Fact);
	TestTrue(TEXT("The resident can inspect the transformed work directly"), bInspectedDescendant);
	TestTrue(TEXT("Inspection explains its public source lineage"),
		Fact.Contains(TEXT("public three-stone mark")) && Fact.Contains(TEXT("spiral at ArrangingGround_1")));
	TestFalse(TEXT("Public lineage inspection still withholds its maker and private meaning"),
		Fact.Contains(ObserverId) || Fact.Contains(TEXT("Private descendant title")) || Fact.Contains(TEXT("Private descendant intent")));
	Descendant = State->FindArrangementSite(TEXT("ArrangingGround_2"));
	TestTrue(TEXT("Direct inspection records cultural evidence for the descendant too"),
		Descendant && Descendant->ObservedBy.Contains(ObserverId));

	DestroyArrangementWorld(World);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	return true;
}
