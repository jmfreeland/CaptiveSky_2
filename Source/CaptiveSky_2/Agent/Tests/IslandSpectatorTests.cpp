#include "Misc/AutomationTest.h"
#include "IslandSpectator.h"
#include "IslandDayNight.h"
#include "IslandWorldStateSubsystem.h"
#include "Camera/CameraActor.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandSpectatorTest, "CaptiveSky2.Agent.Spectator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandSpectatorTest::RunTest(const FString& Parameters)
{
	const FString SavedDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	const FString ScreenshotRoot = FPaths::Combine(SavedDirectory, TEXT("Automation"), TEXT("SpectatorShots"));
	TestEqual(TEXT("The default screenshot directory stays compatible"),
		AIslandSpectatorDirector::ResolveScreenshotDirectory(FPaths::ProjectSavedDir(), FString()),
		FPaths::Combine(SavedDirectory, TEXT("Screenshots"), TEXT("Spectator")));
	TestEqual(TEXT("A relative screenshot directory is isolated under Saved"),
		AIslandSpectatorDirector::ResolveScreenshotDirectory(FPaths::ProjectSavedDir(), TEXT("Automation/SpectatorShots")),
		ScreenshotRoot);
	TestEqual(TEXT("An absolute screenshot directory is respected"),
		AIslandSpectatorDirector::ResolveScreenshotDirectory(FPaths::ProjectSavedDir(), ScreenshotRoot), ScreenshotRoot);

	// No model requests; the fixture has its own viewpoints and a scratch world-state file.
	const FString Scratch = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("Spectator"));
	const FString Viewpoints = Scratch / TEXT("Viewpoints.json");
	const FString StateFile = Scratch / TEXT("WorldState.json");
	IFileManager::Get().Delete(*StateFile, false, true, true);
	FFileHelper::SaveStringToFile(TEXT(R"({"viewpoints": [
		{"name": "00_Survey", "from": {"at": [0, 0, 9000]}, "look": {"at": [1, 0, 0]}},
		{"name": "01_FirstPlace", "from": {"at": [-1200, 0, 0]}, "look": {"tag": "ViewA", "offset": [0, 0, 50]}},
		{"name": "02b_SecondPlace", "from": {"tag": "ViewB", "offset": [900, 300, 0]}, "look": {"tag": "ViewB"}}]})"), *Viewpoints);

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->GetSubsystem<UIslandWorldStateSubsystem>()->StorageFileOverride = StateFile;
	AActor* Ground = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(6000, 6000, 50));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	Ground->SetActorLocation(FVector(0, 0, -50));
	World->SpawnActor<ATargetPoint>(FVector(0, 0, 100), FRotator::ZeroRotator)->Tags = {TEXT("ViewA")};
	ATargetPoint* ViewB = World->SpawnActor<ATargetPoint>(FVector(2000, 2000, 100), FRotator::ZeroRotator);
	ViewB->Tags = {TEXT("ViewB")};

	APlayerController* Player = World->SpawnActor<APlayerController>();
	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(-3000, -3000, 100), FRotator::ZeroRotator);
	Player->Possess(Visitor);
	World->BeginPlay();
	AIslandDayNight* TestClock = World->SpawnActor<AIslandDayNight>();
	if (!TestNotNull(TEXT("A synthetic Island clock is available for isolated viewpoint-hour testing"), TestClock))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		IFileManager::Get().Delete(*StateFile, false, true, true);
		return false;
	}
	float AppliedHour = 0.f;
	TestTrue(TEXT("A requested viewpoint hour applies after world-state load when data is isolated"),
		UIslandSpectatorSubsystem::ApplyIsolatedStartHourOverride(World,
			TEXT("-CaptiveSkyDataRoot=\"D:/Captive Sky/Saved/Automation/Spectator\" -CaptiveSkyIsolatedStartHour=11.5"), AppliedHour));
	TestTrue(TEXT("The isolated viewpoint clock is set to the requested hour"),
		FMath::IsNearlyEqual(AppliedHour, 11.5f) && FMath::IsNearlyEqual(TestClock->CurrentHour, 11.5f));
	TestFalse(TEXT("The startup override is refused without a separate data root"),
		UIslandSpectatorSubsystem::ApplyIsolatedStartHourOverride(World,
			TEXT("-CaptiveSkyIsolatedStartHour=7"), AppliedHour));
	TestTrue(TEXT("A refused override leaves the active clock unchanged"), FMath::IsNearlyEqual(TestClock->CurrentHour, 11.5f));
	TestFalse(TEXT("A non-finite isolated viewpoint hour is refused"),
		UIslandSpectatorSubsystem::ApplyIsolatedStartHourOverride(World,
			TEXT("-CaptiveSkyDataRoot=Saved/Automation/Spectator -CaptiveSkyIsolatedStartHour=nan"), AppliedHour));

	const FVector VisitorHome = Visitor->GetActorLocation();
	AIslandSpectatorDirector* Director = World->SpawnActor<AIslandSpectatorDirector>();
	Director->ViewpointFileOverride = Viewpoints;
	Director->BeginSpectating(Player);
	TestEqual(TEXT("Journey views load; the overhead survey is skipped"), Director->GetEstablishingCount(), 2);
	TestEqual(TEXT("Spectating opens on the first view, titled from its name"), Director->GetCurrentShot().Title, FString(TEXT("First Place")));
	TestTrue(TEXT("The camera sits at eye height above the ground"), Director->GetCamera() && Director->GetCamera()->GetActorLocation().Z >= 159.f);
	TestTrue(TEXT("The visitor's body is hidden and inert"), Visitor->IsHidden() && !Visitor->GetActorEnableCollision());
	TestTrue(TEXT("The player's input is set aside"), Player->IsMoveInputIgnored() && Player->IsLookInputIgnored());
	if (Player->PlayerCameraManager) TestTrue(TEXT("The player now sees through the director's camera"), Player->GetViewTarget() == Director->GetCamera());
	TestTrue(TEXT("The hidden body travels with the subject, so subtitles and weather follow the shot"), FVector::Dist2D(Visitor->GetActorLocation(), FVector::ZeroVector) < 10.f);

	const FVector Start = Director->GetCamera()->GetActorLocation();
	Director->Tick(AIslandSpectatorDirector::EstablishingSeconds * 0.5f);
	TestTrue(TEXT("Establishing views drift slowly"), !Director->GetCamera()->GetActorLocation().Equals(Start, 1.f) && FVector::Dist(Director->GetCamera()->GetActorLocation(), Start) < 400.f);
	ViewB->AddActorWorldOffset(FVector(0, 0, 150));
	Director->Tick(AIslandSpectatorDirector::EstablishingSeconds * 0.6f);
	TestEqual(TEXT("After its time, the next view follows"), Director->GetCurrentShot().Title, FString(TEXT("Second Place")));
	const FVector ShotLookAtStart = Director->GetCurrentShot().LookAt;
	TestTrue(TEXT("A tag-anchored shot starts at the subject's current location"),
		ShotLookAtStart.Equals(FVector(2000, 2000, 250), 1.f));
	const FVector CameraBeforeAnchorMove = Director->GetCamera()->GetActorLocation();
	const FVector VisitorBeforeAnchorMove = Visitor->GetActorLocation();
	ViewB->AddActorWorldOffset(FVector(0, 0, 75));
	Director->Tick(0.1f);
	TestTrue(TEXT("A shot whose camera and look-at share a tag follows its moving subject"),
		Director->GetCurrentShot().LookAt.Equals(ShotLookAtStart + FVector(0, 0, 75), 1.f));
	TestTrue(TEXT("The camera and hidden visitor travel with the moving viewpoint anchor"),
		Director->GetCamera()->GetActorLocation().Z > CameraBeforeAnchorMove.Z + 50.f &&
		Visitor->GetActorLocation().Z > VisitorBeforeAnchorMove.Z + 50.f);

	// A resident speaks: cut to them, with a clear line of sight even when a wall is in the obvious spot.
	ACharacter* Speaker = World->SpawnActor<ACharacter>(FVector(500, 500, 100), FRotator::ZeroRotator);
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(WallBox);
	WallBox->SetBoxExtent(FVector(20, 400, 400));
	WallBox->SetCollisionProfileName(TEXT("BlockAll"));
	WallBox->RegisterComponent();
	Wall->SetActorLocation(FVector(700, 500, 300)); // blocks the views from in front
	Director->FocusOnSpeaker(Speaker, TEXT("The rain has passed for now."));
	const FIslandShot Speech = Director->GetCurrentShot();
	TestTrue(TEXT("Speech cuts to the speaker"), Speech.Kind == EIslandShotKind::Speech && Speech.Title == Speaker->GetActorNameOrLabel());
	FHitResult Blocked;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(SpectatorTest), false, Speaker);
	TestFalse(TEXT("The speech camera can see the speaker"), World->LineTraceSingleByChannel(Blocked, Speech.LookAt, Speech.From, ECC_Visibility, Query));
	TestTrue(TEXT("Speech shots last long enough to read the line"), Speech.Duration >= 5.f && Speech.Duration <= 12.f);

	// Something lasting is made: the next poll cuts to it.
	World->GetSubsystem<UIslandWorldStateSubsystem>()->AddNestLayer(TEXT("TestRoost"), FVector(-800, 800, 0), TEXT("Agent_Test"));
	Director->Tick(5.1f);
	TestTrue(TEXT("A new nest gets its own shot"), Director->GetCurrentShot().Kind == EIslandShotKind::Made && Director->GetCurrentShot().Title.Contains(TEXT("A nest at TestRoost")));
	Director->Tick(5.1f);
	TestTrue(TEXT("An unchanged world does not cut again"), Director->GetCurrentShot().Title.Contains(TEXT("A nest at TestRoost")));

	Director->EndSpectating();
	TestTrue(TEXT("Leaving spectator mode restores the visitor"), !Visitor->IsHidden() && Visitor->GetActorEnableCollision() && Visitor->GetActorLocation().Equals(VisitorHome, 1.f));
	TestTrue(TEXT("Leaving spectator mode returns control"), !Player->IsMoveInputIgnored() && !Player->IsLookInputIgnored());
	if (Player->PlayerCameraManager) TestTrue(TEXT("The player's view returns to their body"), Player->GetViewTarget() == Visitor);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	return true;
}
