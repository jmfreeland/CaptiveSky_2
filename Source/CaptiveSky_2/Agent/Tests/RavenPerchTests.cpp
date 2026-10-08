#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "Animation/AnimSequence.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentRestPresentationComponent.h"
#include "AgentSocialComponent.h"
#include "IslandInnkeeperSubsystem.h"
#include "IslandArrangement.h"
#include "IslandDew.h"
#include "IslandListeningStonesChime.h"
#include "IslandPoolRippleEffect.h"
#include "IslandRainBasin.h"
#include "IslandTidepoolCrab.h"
#include "IslandWindMoteEffect.h"
#include "IslandWeather.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "NavigationSystem.h"
#include "ContentStreaming.h"
#include "Rendering/SkeletalMeshRenderData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRavenPerchTest, "CaptiveSky2.Agent.RavenPerch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRavenPerchTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> PerchOptions = { FVector(400.f, 0.f, 0.f), FVector(900.f, 0.f, 0.f) };
	const TArray<float> ShelteredWind = { 160.f, 20.f };
	const TArray<float> SlightlyDifferentWind = { 160.f, 155.f };
	const TArray<float> IncompleteWindSamples = { 160.f };
	TestEqual(TEXT("Light wind leaves the raven's ordinary nearest-roost choice intact"),
		ARavenAgentAIController::SelectWindAwarePerch(FVector::ZeroVector, 50.f, PerchOptions, ShelteredWind), 0);
	TestEqual(TEXT("Strong wind can favor a materially calmer nearby supported roost"),
		ARavenAgentAIController::SelectWindAwarePerch(FVector::ZeroVector, 160.f, PerchOptions, ShelteredWind), 1);
	TestEqual(TEXT("Small exposure differences do not outweigh the nearest roost preference"),
		ARavenAgentAIController::SelectWindAwarePerch(FVector::ZeroVector, 160.f, PerchOptions, SlightlyDifferentWind), 0);
	TestEqual(TEXT("Malformed perch samples safely produce no candidate"),
		ARavenAgentAIController::SelectWindAwarePerch(FVector::ZeroVector, 160.f, PerchOptions, IncompleteWindSamples), INDEX_NONE);
	const TArray<int32> OpenPerches = { 0, 0 };
	const TArray<int32> CoveredAlternative = { 0, 4 };
	const TArray<int32> SlightCoverDifference = { 2, 3 };
	const TArray<int32> EquallyCovered = { 4, 4 };
	const TArray<int32> MissingCoverSamples = { 0 };
	TestEqual(TEXT("Heavy rain favors a clearly better overhead-cover clue"),
		ARavenAgentAIController::SelectWeatherAwarePerch(FVector::ZeroVector, 50.f, 0.8f,
			PerchOptions, ShelteredWind, CoveredAlternative), 1);
	TestEqual(TEXT("A light shower preserves the wind-aware choice"),
		ARavenAgentAIController::SelectWeatherAwarePerch(FVector::ZeroVector, 50.f, 0.2f,
			PerchOptions, ShelteredWind, CoveredAlternative), 0);
	TestEqual(TEXT("One extra probe is too weak to override the wind-aware choice"),
		ARavenAgentAIController::SelectWeatherAwarePerch(FVector::ZeroVector, 50.f, 0.8f,
			PerchOptions, ShelteredWind, SlightCoverDifference), 0);
	TestEqual(TEXT("Wind and distance remain decisive among similarly covered roosts"),
		ARavenAgentAIController::SelectWeatherAwarePerch(FVector::ZeroVector, 160.f, 0.8f,
			PerchOptions, ShelteredWind, EquallyCovered), 1);
	TestEqual(TEXT("No cover at any candidate preserves the wind-aware choice"),
		ARavenAgentAIController::SelectWeatherAwarePerch(FVector::ZeroVector, 50.f, 0.8f,
			PerchOptions, ShelteredWind, OpenPerches), 0);
	TestEqual(TEXT("Mismatched overhead samples safely fall back to the wind-aware rule"),
		ARavenAgentAIController::SelectWeatherAwarePerch(FVector::ZeroVector, 50.f, 0.8f,
			PerchOptions, ShelteredWind, MissingCoverSamples), 0);
	const FBox SyntheticSpruceBounds(FVector(-100.f, -100.f, 0.f), FVector(100.f, 100.f, 1200.f));
	const FTransform UprightSpruce(FVector::ZeroVector);
	TestTrue(TEXT("A probe through the mature spruce upper crown provides a cautious cover clue"),
		AIslandWeather::DoesSpruceCrownCoverProbeSegment(SyntheticSpruceBounds, UprightSpruce,
			FVector(0.f, 0.f, 700.f), FVector(0.f, 0.f, 900.f)));
	TestFalse(TEXT("A probe beneath the spruce crown is not counted as overhead cover"),
		AIslandWeather::DoesSpruceCrownCoverProbeSegment(SyntheticSpruceBounds, UprightSpruce,
			FVector(0.f, 0.f, 200.f), FVector(0.f, 0.f, 350.f)));
	TestFalse(TEXT("A probe outside the upper-crown footprint is not covered"),
		AIslandWeather::DoesSpruceCrownCoverProbeSegment(SyntheticSpruceBounds, UprightSpruce,
			FVector(140.f, 0.f, 700.f), FVector(140.f, 0.f, 1000.f)));
	TestTrue(TEXT("A tilted mature spruce crown still covers a segment in its local upper canopy"),
		AIslandWeather::DoesSpruceCrownCoverProbeSegment(SyntheticSpruceBounds,
			FTransform(FRotator(15.f, 25.f, 10.f), FVector(50.f, -20.f, 100.f), FVector(1.2f)),
			FTransform(FRotator(15.f, 25.f, 10.f), FVector(50.f, -20.f, 100.f), FVector(1.2f)).TransformPosition(FVector(0.f, 0.f, 700.f)),
			FTransform(FRotator(15.f, 25.f, 10.f), FVector(50.f, -20.f, 100.f), FVector(1.2f)).TransformPosition(FVector(0.f, 0.f, 1000.f))));
	const FVector Origin(0.f, 0.f, 600.f);
	const TArray<FVector> CruiseCandidates = { FVector(0.f, 0.f, 900.f), FVector(800.f, 0.f, 900.f), FVector(-800.f, 0.f, 900.f) };
	const TArray<FVector> VisibleLandmarks = { FVector(2000.f, 0.f, 900.f) };
	TestTrue(TEXT("Curiosity can select a candidate that approaches a visible landmark"),
		ARavenAgentAIController::SelectWanderCruiseTarget(Origin, CruiseCandidates, VisibleLandmarks, 0, true).Equals(CruiseCandidates[1]));
	TestTrue(TEXT("Ordinary flight wander preserves its random fallback when curiosity is inactive"),
		ARavenAgentAIController::SelectWanderCruiseTarget(Origin, CruiseCandidates, VisibleLandmarks, 2, false).Equals(CruiseCandidates[2]));
	TestTrue(TEXT("No visible landmarks leave flight wander on its random fallback"),
		ARavenAgentAIController::SelectWanderCruiseTarget(Origin, CruiseCandidates, {}, 1, true).Equals(CruiseCandidates[1]));

	// No gateway, model requests, or autobiographical memory in this fixture.
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	// CreateWorld already initializes the world; do not initialize WorldSettings twice.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACharacter* Raven = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	UStaticMeshComponent* LeftWing = NewObject<UStaticMeshComponent>(Raven, TEXT("LeftWing"));
	Raven->AddInstanceComponent(LeftWing);
	LeftWing->SetupAttachment(Raven->GetRootComponent());
	LeftWing->SetRelativeRotation(FRotator(0.f, 0.f, 8.f));
	LeftWing->RegisterComponent();
	UStaticMeshComponent* RightWing = NewObject<UStaticMeshComponent>(Raven, TEXT("RightWing"));
	Raven->AddInstanceComponent(RightWing);
	RightWing->SetupAttachment(Raven->GetRootComponent());
	RightWing->SetRelativeRotation(FRotator(0.f, 0.f, -8.f));
	RightWing->RegisterComponent();
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	UClass* RavenBlueprintClass = LoadClass<ACharacter>(nullptr, TEXT("/Game/Agents/BP_Raven_Placeholder.BP_Raven_Placeholder_C"));
	ACharacter* BlueprintRaven = RavenBlueprintClass
		? World->SpawnActor<ACharacter>(RavenBlueprintClass, FVector(1800.f, 0.f, 100.f), FRotator::ZeroRotator) : nullptr;
	ARavenAgentAIController* BlueprintController = BlueprintRaven ? World->SpawnActor<ARavenAgentAIController>() : nullptr;
	ATargetPoint* Perch = World->SpawnActor<ATargetPoint>(FVector(600, 0, 302), FRotator::ZeroRotator);
	Perch->Tags = {TEXT("RavenPerch"), TEXT("TestRoost")};
	AActor* Support = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Support);
	Support->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(150, 150, 20));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	Support->SetActorLocation(FVector(600, 0, 302 - Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 22));
	World->BeginPlay();
	Controller->Possess(Raven);
	const FRotator LeftWingRest = LeftWing->GetRelativeRotation();
	const FRotator RightWingRest = RightWing->GetRelativeRotation();
	Controller->Tick(0.05f);
	TestTrue(TEXT("Grounded rest preserves the placeholder wing pose"),
		LeftWing->GetRelativeRotation().Equals(LeftWingRest) && RightWing->GetRelativeRotation().Equals(RightWingRest));
	Controller->LocomotionState = ERavenLocomotionState::Flying;
	Controller->Tick(0.05f);
	const float LeftWingStroke = FMath::FindDeltaAngleDegrees(LeftWingRest.Roll, LeftWing->GetRelativeRotation().Roll);
	const float RightWingStroke = FMath::FindDeltaAngleDegrees(RightWingRest.Roll, RightWing->GetRelativeRotation().Roll);
	TestTrue(TEXT("Flying animates the left placeholder wing from its authored baseline"), FMath::Abs(LeftWingStroke) > 1.f);
	TestTrue(TEXT("Flight mirrors the wing strokes instead of rotating both wings in the same direction"),
		FMath::IsNearlyEqual(LeftWingStroke, -RightWingStroke, 0.1f));
	Controller->LocomotionState = ERavenLocomotionState::Hopping;
	Controller->HopStart = Raven->GetActorLocation();
	Controller->HopEnd = Controller->HopStart;
	Controller->HopElapsed = 0.f;
	Controller->HopDuration = 100.f;
	Controller->Tick(0.05f);
	TestTrue(TEXT("A ground hop gets a smaller wing stroke than sustained flight"),
		FMath::Abs(FMath::FindDeltaAngleDegrees(LeftWingRest.Roll, LeftWing->GetRelativeRotation().Roll)) > 1.f &&
		FMath::Abs(FMath::FindDeltaAngleDegrees(LeftWingRest.Roll, LeftWing->GetRelativeRotation().Roll)) < FMath::Abs(LeftWingStroke));
	Raven->SetActorLocation(Controller->HopStart);
	Controller->LocomotionState = ERavenLocomotionState::Grounded;
	Controller->Tick(0.05f);
	Controller->HopDuration = 0.55f;
	TestTrue(TEXT("Settling returns both wings to their exact authored baselines without drift"),
		LeftWing->GetRelativeRotation().Equals(LeftWingRest) && RightWing->GetRelativeRotation().Equals(RightWingRest));
	TestNotNull(TEXT("The real raven placeholder Blueprint is available to the provider-free fixture"), BlueprintRaven);
	if (TestNotNull(TEXT("The real raven placeholder has a controller"), BlueprintController))
	{
		BlueprintController->Possess(BlueprintRaven);
		TestFalse(TEXT("The human mannequin is hidden for the raven's procedural bird body"), BlueprintRaven->GetMesh()->IsVisible());
		TArray<UStaticMeshComponent*> PlaceholderStaticMeshes;
		BlueprintRaven->GetComponents<UStaticMeshComponent>(PlaceholderStaticMeshes);
		bool bAllPlaceholderPartsHidden = !PlaceholderStaticMeshes.IsEmpty();
		for (const UStaticMeshComponent* Part : PlaceholderStaticMeshes)
			bAllPlaceholderPartsHidden &= Part && !Part->IsVisible() && Part->bHiddenInGame;
		TestTrue(TEXT("Every legacy static-mesh placeholder part is hidden behind the procedural raven"), bAllPlaceholderPartsHidden);
		UProceduralMeshComponent* BlueprintBody = nullptr;
		UProceduralMeshComponent* BlueprintHead = nullptr;
		UProceduralMeshComponent* BlueprintLeftFeathers = nullptr;
		UProceduralMeshComponent* BlueprintRightFeathers = nullptr;
		USceneComponent* BlueprintHeadPivot = nullptr;
		USceneComponent* BlueprintLeftWingPivot = nullptr;
		USceneComponent* BlueprintRightWingPivot = nullptr;
		USkeletalMeshComponent* BlueprintRiggedCrow = nullptr;
		TArray<UProceduralMeshComponent*> BlueprintBirdMeshes;
		BlueprintRaven->GetComponents<UProceduralMeshComponent>(BlueprintBirdMeshes);
		for (UProceduralMeshComponent* Component : BlueprintBirdMeshes)
		{
			if (!Component) continue;
			if (Component->GetName() == TEXT("RavenBodyMesh")) BlueprintBody = Component;
			if (Component->GetName() == TEXT("RavenHeadMesh")) BlueprintHead = Component;
			if (Component->GetName() == TEXT("RavenLeftWingFeathers")) BlueprintLeftFeathers = Component;
			if (Component->GetName() == TEXT("RavenRightWingFeathers")) BlueprintRightFeathers = Component;
		}
		TArray<USceneComponent*> BlueprintSceneComponents;
		BlueprintRaven->GetComponents<USceneComponent>(BlueprintSceneComponents);
		for (USceneComponent* Component : BlueprintSceneComponents)
		{
			if (!Component) continue;
			if (Component->GetName() == TEXT("RavenHeadPivot")) BlueprintHeadPivot = Component;
			if (Component->GetName() == TEXT("RavenLeftWingPivot")) BlueprintLeftWingPivot = Component;
			if (Component->GetName() == TEXT("RavenRightWingPivot")) BlueprintRightWingPivot = Component;
		}
		TArray<USkeletalMeshComponent*> BlueprintSkeletalMeshes;
		BlueprintRaven->GetComponents<USkeletalMeshComponent>(BlueprintSkeletalMeshes);
		for (USkeletalMeshComponent* Component : BlueprintSkeletalMeshes)
			if (Component && Component->GetFName() == TEXT("RavenRiggedCrowBody")) { BlueprintRiggedCrow = Component; break; }
		if (BlueprintRiggedCrow)
		{
			TestNotNull(TEXT("The imported Crow has its idle animation"), BlueprintController->CrowIdleAnimation.Get());
			TestNotNull(TEXT("The imported Crow has its flight animation"), BlueprintController->CrowFlyAnimation.Get());
			TestTrue(TEXT("The rigged Crow is visual-only for collision and navigation"),
				BlueprintRiggedCrow->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !BlueprintRiggedCrow->CanEverAffectNavigation());
			TestTrue(TEXT("A grounded rigged Raven selects its idle animation"), BlueprintController->CurrentCrowAnimation == BlueprintController->CrowIdleAnimation);
			BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
			BlueprintController->Tick(0.05f);
			TestTrue(TEXT("Flight selects the imported Crow flight animation"), BlueprintController->CurrentCrowAnimation == BlueprintController->CrowFlyAnimation);
			BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
			BlueprintController->Tick(0.05f);
		}
		else
		{
			TestNotNull(TEXT("The raven body is assembled from procedural bird geometry"), BlueprintBody);
			TestNotNull(TEXT("The raven has a separate procedural head mesh"), BlueprintHead);
			TestNotNull(TEXT("The raven head is mounted on a movable scan pivot"), BlueprintHeadPivot);
			TestNotNull(TEXT("The procedural left wing has actual feather geometry"), BlueprintLeftFeathers);
			TestNotNull(TEXT("The procedural right wing has actual feather geometry"), BlueprintRightFeathers);
			TestNotNull(TEXT("The procedural left wing is mounted on an animated pivot"), BlueprintLeftWingPivot);
			TestNotNull(TEXT("The procedural right wing is mounted on an animated pivot"), BlueprintRightWingPivot);
			if (BlueprintBody && BlueprintHead && BlueprintHeadPivot && BlueprintLeftFeathers && BlueprintRightFeathers && BlueprintLeftWingPivot && BlueprintRightWingPivot)
			{
				const FProcMeshSection* BodySection = BlueprintBody->GetProcMeshSection(0);
				const FProcMeshSection* HeadSection = BlueprintHead->GetProcMeshSection(0);
				const FProcMeshSection* LeftFeatherSection = BlueprintLeftFeathers->GetProcMeshSection(0);
				const FProcMeshSection* RightFeatherSection = BlueprintRightFeathers->GetProcMeshSection(0);
				TestTrue(TEXT("Body, head, and both wings contain nontrivial triangle geometry"),
					BodySection && BodySection->ProcIndexBuffer.Num() > 300 &&
					HeadSection && HeadSection->ProcIndexBuffer.Num() > 300 &&
					LeftFeatherSection && LeftFeatherSection->ProcIndexBuffer.Num() > 60 &&
					RightFeatherSection && RightFeatherSection->ProcIndexBuffer.Num() > 60);
				const FRotator HeadRest = BlueprintHeadPivot->GetRelativeRotation();
				BlueprintController->Tick(1.f);
				const FRotator IdleHeadPose = BlueprintHeadPivot->GetRelativeRotation();
				TestTrue(TEXT("A grounded raven makes a subtle, bounded idle glance"),
					FMath::Abs(IdleHeadPose.Yaw) > 0.1f && FMath::Abs(IdleHeadPose.Yaw) <= 7.f && FMath::Abs(IdleHeadPose.Pitch) <= 2.5f);
				const FRotator BlueprintLeftRest = BlueprintLeftWingPivot->GetRelativeRotation();
				const FRotator BlueprintRightRest = BlueprintRightWingPivot->GetRelativeRotation();
				TestTrue(TEXT("Perched procedural wings fold close to the raven's body"),
					FMath::Abs(BlueprintLeftRest.Yaw) >= 90.f && FMath::Abs(BlueprintLeftRest.Yaw) <= 110.f &&
					FMath::IsNearlyEqual(BlueprintLeftRest.Yaw, -BlueprintRightRest.Yaw, 0.1f));
				const FRotator BlueprintLeftWorldRest = BlueprintLeftFeathers->GetComponentRotation();
				BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
				BlueprintController->Tick(0.05f);
				TestTrue(TEXT("Flight settles the head back to its forward pose"), BlueprintHeadPivot->GetRelativeRotation().Equals(HeadRest));
				const float BlueprintLeftStroke = FMath::FindDeltaAngleDegrees(BlueprintLeftRest.Roll, BlueprintLeftWingPivot->GetRelativeRotation().Roll);
				const float BlueprintRightStroke = FMath::FindDeltaAngleDegrees(BlueprintRightRest.Roll, BlueprintRightWingPivot->GetRelativeRotation().Roll);
				TestTrue(TEXT("Flight animates the rendered procedural wing away from its perched pose"),
					!BlueprintLeftFeathers->GetComponentRotation().Equals(BlueprintLeftWorldRest));
				TestTrue(TEXT("The real Blueprint wings animate in mirrored strokes"),
					FMath::Abs(BlueprintLeftStroke) > 1.f && FMath::IsNearlyEqual(BlueprintLeftStroke, -BlueprintRightStroke, 0.1f));
				BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
				BlueprintController->Tick(0.05f);
				TestTrue(TEXT("The procedural wings return exactly to their perched transforms"),
					BlueprintLeftWingPivot->GetRelativeRotation().Equals(BlueprintLeftRest) && BlueprintRightWingPivot->GetRelativeRotation().Equals(BlueprintRightRest));
			}
		}
		if (BlueprintHeadPivot)
		{
			const FRotator RavenHeadRest = BlueprintHeadPivot->GetRelativeRotation();
			const FVector RavenPositionBeforeChime = BlueprintRaven->GetActorLocation();
			AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(
				RavenPositionBeforeChime + FVector(0.f, 450.f, 0.f), FRotator::ZeroRotator);
			TestNotNull(TEXT("A nearby Listening Stones chime can be created without external services"), Chime);
			if (Chime)
			{
				Chime->BeginChime(120.f);
				BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
				BlueprintController->Tick(0.25f);
				BlueprintController->Tick(0.25f);
				const FRotator ChimeAttentionPose = BlueprintHeadPivot->GetRelativeRotation();
				TestTrue(TEXT("A grounded raven briefly turns toward an audible Listening Stones chime"),
					ChimeAttentionPose.Yaw > 5.f && ChimeAttentionPose.Yaw <= 30.f);
				TestTrue(TEXT("Noticing the chime does not move the raven"), BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforeChime, 0.1f));
				const float AttentionRemainingAfterNotice = BlueprintController->ListeningStoneAttentionRemaining;
				BlueprintController->Tick(0.25f);
				TestTrue(TEXT("Chime attention fades on a short timer instead of retriggering continuously"),
					BlueprintController->ListeningStoneAttentionRemaining < AttentionRemainingAfterNotice);
				BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
				BlueprintController->Tick(0.05f);
				TestTrue(TEXT("Taking flight immediately restores the raven's neutral head pose"),
					BlueprintHeadPivot->GetRelativeRotation().Equals(RavenHeadRest));
				Chime->Destroy();
			}

			BlueprintController->ListeningStoneAttentionRemaining = 0.f;
			BlueprintController->RainBasinAttentionRemaining = 0.f;
			BlueprintController->LastNoticedRainBasin.Reset();
			BlueprintController->MinnowRippleAttentionRemaining = 0.f;
			BlueprintController->DewGlintAttentionRemaining = 0.f;
			BlueprintController->WindMoteAttentionRemaining = 0.f;
			BlueprintController->CrabScurryAttentionRemaining = 0.f;
			BlueprintController->ResidentAttentionRemaining = 0.f;
			BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
			const FVector RavenPositionBeforeRainBasin = BlueprintRaven->GetActorLocation();
			AIslandRainBasin* RainBasin = World->SpawnActor<AIslandRainBasin>(
				RavenPositionBeforeRainBasin + FVector(0.f, 450.f, 0.f), FRotator::ZeroRotator);
			UIslandRainBasinSubsystem* RainBasinSubsystem = World->GetSubsystem<UIslandRainBasinSubsystem>();
			TestNotNull(TEXT("A rain basin can be created for the raven's water-attention fixture"), RainBasin);
			TestNotNull(TEXT("The rain basin's deterministic state is available to the fixture"), RainBasinSubsystem);
			if (RainBasin && RainBasinSubsystem)
			{
				FIslandBasinState& BasinState = RainBasinSubsystem->GetStateMutable();
				BasinState.bPlaced = true;
				BasinState.Location = RainBasin->GetActorLocation() - FVector(0.f, 0.f, AIslandRainBasin::OriginLift);
				BasinState.Water = 0.f;
				RainBasin->Show(BasinState, 1);
				BlueprintController->CheckForNearbyRainBasin();
				TestTrue(TEXT("An empty stone hollow does not attract the raven's attention"),
					FMath::IsNearlyZero(BlueprintController->RainBasinAttentionRemaining));

				BasinState.Water = 0.5f;
				RainBasin->Show(BasinState, 1);
				BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
				BlueprintController->CheckForNearbyRainBasin();
				TestTrue(TEXT("A flying raven does not attend to a ground-level water bowl"),
					FMath::IsNearlyZero(BlueprintController->RainBasinAttentionRemaining));

				BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
				BlueprintController->CheckForNearbyRainBasin();
				TestTrue(TEXT("A settled raven notices the nearby rainwater once it is deep enough to float a leaf"),
					BlueprintController->RainBasinAttentionRemaining > 0.f &&
					BlueprintController->RainBasinLocation.Equals(BasinState.Location +
						FVector(0.f, 0.f, AIslandRainBasin::FloorThickness + AIslandRainBasin::WaterDepth(BasinState.Water)), 0.1f));
				BlueprintController->Tick(0.25f);
				TestFalse(TEXT("The raven briefly turns its head toward the water surface"),
					BlueprintHeadPivot->GetRelativeRotation().Equals(BlueprintController->RavenHeadRestRotation, 0.1f));
				TestTrue(TEXT("Noticing the rain basin leaves the raven in place"),
					BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforeRainBasin, 0.1f));
				BlueprintController->RainBasinAttentionRemaining = 0.f;
				BlueprintController->CheckForNearbyRainBasin();
				TestTrue(TEXT("One continuously wet basin cannot repeatedly restart the glance"),
					FMath::IsNearlyZero(BlueprintController->RainBasinAttentionRemaining));
				BasinState.Water = 0.f;
				BlueprintController->CheckForNearbyRainBasin();
				TestFalse(TEXT("A dry interval rearms rain-basin awareness"),
					BlueprintController->LastNoticedRainBasin.IsValid());
				BasinState.Water = 0.5f;
				BlueprintController->CheckForNearbyRainBasin();
				TestTrue(TEXT("A later wet spell can draw one new glance"),
					BlueprintController->RainBasinAttentionRemaining > 0.f);
				RainBasin->Destroy();
			}

			BlueprintController->ListeningStoneAttentionRemaining = 0.f;
			BlueprintController->RainBasinAttentionRemaining = 0.f;
			BlueprintController->LastNoticedRainBasin.Reset();
			BlueprintController->MinnowRippleAttentionRemaining = 0.f;
			BlueprintController->LastNoticedMinnowRipple.Reset();
			BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
			const FVector RavenPositionBeforeRipple = BlueprintRaven->GetActorLocation();
			AIslandPoolRippleEffect* MinnowRipple = World->SpawnActor<AIslandPoolRippleEffect>(
				RavenPositionBeforeRipple + FVector(0.f, 450.f, 0.f), FRotator::ZeroRotator);
			TestNotNull(TEXT("A transient minnow ripple can be created without external services"), MinnowRipple);
			if (MinnowRipple)
			{
				MinnowRipple->ConfigureAsMinnowImpact();
				BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
				BlueprintController->CheckForNearbyMinnowSurfaceBreak();
				TestTrue(TEXT("An airborne raven does not try to attend a brief surface ripple"),
					FMath::IsNearlyZero(BlueprintController->MinnowRippleAttentionRemaining));
				BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
				BlueprintController->CheckForNearbyMinnowSurfaceBreak();
				BlueprintController->Tick(0.25f);
				TestTrue(TEXT("A grounded raven briefly turns toward a nearby minnow surface break"),
					BlueprintController->MinnowRippleAttentionRemaining > 0.f &&
					FMath::Abs(BlueprintHeadPivot->GetRelativeRotation().Yaw) > 4.f &&
					FMath::Abs(BlueprintHeadPivot->GetRelativeRotation().Yaw) <= 25.f);
				TestTrue(TEXT("Noticing a fish ripple does not move the raven"),
					BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforeRipple, 0.1f));
				const float RippleAttentionAfterNotice = BlueprintController->MinnowRippleAttentionRemaining;
				BlueprintController->Tick(0.25f);
				TestTrue(TEXT("Fish-ripple attention fades instead of holding a fixed pose"),
					BlueprintController->MinnowRippleAttentionRemaining < RippleAttentionAfterNotice);
				BlueprintController->MinnowRippleAttentionRemaining = 0.f;
				BlueprintController->CheckForNearbyMinnowSurfaceBreak();
				TestTrue(TEXT("The same brief ripple cannot restart attention after it has been noticed"),
					FMath::IsNearlyZero(BlueprintController->MinnowRippleAttentionRemaining));
				MinnowRipple->Destroy();
			}

			BlueprintController->ListeningStoneAttentionRemaining = 0.f;
			BlueprintController->MinnowRippleAttentionRemaining = 0.f;
			BlueprintController->DewGlintAttentionRemaining = 0.f;
			BlueprintController->CrabScurryAttentionRemaining = 0.f;
			BlueprintController->ResidentAttentionRemaining = 0.f;
			BlueprintController->LastNoticedDewActor.Reset();
			BlueprintController->ListeningStoneCheckRemaining = 1.f;
			const FVector RavenPositionBeforeDew = BlueprintRaven->GetActorLocation();
			AIslandDewActor* MorningDew = World->SpawnActor<AIslandDewActor>(
				RavenPositionBeforeDew, FRotator::ZeroRotator);
			TestNotNull(TEXT("The transient morning dew scatter can be created without external services"), MorningDew);
			if (MorningDew)
			{
				for (int32 SeatPass = 0; SeatPass < 8; ++SeatPass)
					MorningDew->Advance(RavenPositionBeforeDew, 0.9f);

				FVector NearestDewGlint = FVector::ZeroVector;
				const bool bHasVisibleDewGlint = MorningDew->FindNearestGlint(
					RavenPositionBeforeDew + FVector(0.f, 0.f, 25.f), 850.f, NearestDewGlint);
				if (!MorningDew->HasMaterial())
				{
					TestFalse(TEXT("Missing optional dew material cannot attract the raven"), bHasVisibleDewGlint);
					BlueprintController->CheckForNearbyDewGlint();
					TestTrue(TEXT("Missing optional material produces no attention cue"),
						FMath::IsNearlyZero(BlueprintController->DewGlintAttentionRemaining));
				}
				else
				{
					TestTrue(TEXT("Dew exposes a nearby active glint as an attention target"), bHasVisibleDewGlint);
					BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
					BlueprintController->CheckForNearbyDewGlint();
					TestTrue(TEXT("An airborne raven ignores a brief ground-level dew glint"),
						FMath::IsNearlyZero(BlueprintController->DewGlintAttentionRemaining));
					BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
					BlueprintController->CheckForNearbyDewGlint();
					TestTrue(TEXT("A settled raven gives a visible dew glint one brief look"),
						BlueprintController->DewGlintAttentionRemaining > 0.f &&
						BlueprintController->DewGlintLocation.Equals(NearestDewGlint, 0.1f));
					TestTrue(TEXT("Noticing dew leaves the raven's body where it was"),
						BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforeDew, 0.1f));
					const FRotator HeadPoseBeforeDew = BlueprintHeadPivot->GetRelativeRotation();
					const float DewAttentionAfterNotice = BlueprintController->DewGlintAttentionRemaining;
					BlueprintController->Tick(0.25f);
					TestTrue(TEXT("Dew attention fades rather than holding the raven's head fixed"),
						BlueprintController->DewGlintAttentionRemaining < DewAttentionAfterNotice);
					TestFalse(TEXT("The raven's head actually turns toward its dew-glint cue"),
						BlueprintHeadPivot->GetRelativeRotation().Equals(HeadPoseBeforeDew, 0.1f));
					BlueprintController->DewGlintAttentionRemaining = 0.f;
					BlueprintController->CheckForNearbyDewGlint();
					TestTrue(TEXT("One morning's dew scatter cannot repeatedly retrigger the glance"),
						FMath::IsNearlyZero(BlueprintController->DewGlintAttentionRemaining));
					MorningDew->Advance(RavenPositionBeforeDew, 0.f);
					BlueprintController->CheckForNearbyDewGlint();
					TestFalse(TEXT("Fading dew rearms the brief attention cue for a later morning"),
						BlueprintController->LastNoticedDewActor.IsValid());
				}
			}
			if (MorningDew) MorningDew->Destroy();

			BlueprintController->ListeningStoneAttentionRemaining = 0.f;
			BlueprintController->DewGlintAttentionRemaining = 0.f;
			BlueprintController->WindMoteAttentionRemaining = 0.f;
			BlueprintController->MinnowRippleAttentionRemaining = 0.f;
			BlueprintController->CrabScurryAttentionRemaining = 0.f;
			BlueprintController->ResidentAttentionRemaining = 0.f;
			BlueprintController->LastNoticedWindMote.Reset();
			BlueprintController->HeadScanTime = 0.f;
			BlueprintHeadPivot->SetRelativeRotation(BlueprintController->RavenHeadRestRotation);
			BlueprintController->ListeningStoneCheckRemaining = 1.f;
			const FVector RavenPositionBeforeWindMotes = BlueprintRaven->GetActorLocation();
			AIslandWindMoteEffect* WindMotes = World->SpawnActor<AIslandWindMoteEffect>(
				RavenPositionBeforeWindMotes + FVector(0.f, 450.f, 0.f), FRotator::ZeroRotator);
			TestNotNull(TEXT("A transient Wind Arch gust can be created without external services"), WindMotes);
			if (WindMotes)
			{
				WindMotes->InitializeGust(FVector::ForwardVector, 800.f, 8.f);
				FVector NearestMoteLocation = FVector::ZeroVector;
				TestTrue(TEXT("The moving gust exposes a nearby visible mote"),
					WindMotes->FindNearestVisibleMote(RavenPositionBeforeWindMotes + FVector(0.f, 0.f, 25.f),
						1400.f, NearestMoteLocation));
				FVector OutOfRangeMoteLocation = FVector::ZeroVector;
				TestFalse(TEXT("The gust does not expose motes beyond the listener's range"),
					WindMotes->FindNearestVisibleMote(RavenPositionBeforeWindMotes + FVector(6000.f, 0.f, 25.f),
						1400.f, OutOfRangeMoteLocation));
				BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
				BlueprintController->CheckForNearbyWindMote();
				TestTrue(TEXT("An airborne raven ignores the brief Wind Arch cue"),
					FMath::IsNearlyZero(BlueprintController->WindMoteAttentionRemaining));
				BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
				BlueprintController->CheckForNearbyWindMote();
				TestTrue(TEXT("A settled raven gives a visible nearby gust mote one brief look"),
					BlueprintController->WindMoteAttentionRemaining > 0.f &&
					BlueprintController->WindMoteLocation.Equals(NearestMoteLocation, 0.1f));
				TestTrue(TEXT("Noticing the moving gust leaves the raven in place"),
					BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforeWindMotes, 0.1f));
				const FRotator HeadPoseBeforeWindMotes = BlueprintHeadPivot->GetRelativeRotation();
				const float WindAttentionAfterNotice = BlueprintController->WindMoteAttentionRemaining;
				BlueprintController->Tick(0.3f);
				TestTrue(TEXT("Wind-mote attention fades smoothly instead of holding"),
					BlueprintController->WindMoteAttentionRemaining < WindAttentionAfterNotice);
				TestFalse(TEXT("The raven's head turns toward the drifting gust lights"),
					BlueprintHeadPivot->GetRelativeRotation().Equals(HeadPoseBeforeWindMotes, 0.1f));
				BlueprintController->WindMoteAttentionRemaining = 0.f;
				BlueprintController->CheckForNearbyWindMote();
				TestTrue(TEXT("One Wind Arch gust cannot repeatedly restart the same glance"),
					FMath::IsNearlyZero(BlueprintController->WindMoteAttentionRemaining));
				WindMotes->Destroy();
			}

			BlueprintController->ListeningStoneAttentionRemaining = 0.f;
			BlueprintController->DewGlintAttentionRemaining = 0.f;
			BlueprintController->WindMoteAttentionRemaining = 0.f;
			BlueprintController->MinnowRippleAttentionRemaining = 0.f;
			BlueprintController->CrabScurryAttentionRemaining = 0.f;
			BlueprintController->ResidentAttentionRemaining = 0.f;
			BlueprintController->LastNoticedScurryingCrab.Reset();
			BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
			const FVector RavenPositionBeforeCrab = BlueprintRaven->GetActorLocation();
			AIslandTidepoolCrab* ScurryingCrab = World->SpawnActor<AIslandTidepoolCrab>(
				RavenPositionBeforeCrab + FVector(0.f, 450.f, 0.f), FRotator::ZeroRotator);
			TestNotNull(TEXT("A shore crab can be spawned without external services"), ScurryingCrab);
			if (ScurryingCrab)
			{
				ScurryingCrab->RespondToQuietObservation(RavenPositionBeforeCrab);
				TestTrue(TEXT("A crab exposes its brief disturbance state to nearby wildlife"), ScurryingCrab->IsScurrying());
				BlueprintController->CheckForNearbyCrabScurry();
				BlueprintController->Tick(0.25f);
				TestTrue(TEXT("A grounded raven briefly looks toward a nearby scurrying crab"),
					BlueprintController->CrabScurryAttentionRemaining > 0.f &&
					FMath::Abs(BlueprintHeadPivot->GetRelativeRotation().Yaw) > 4.f &&
					FMath::Abs(BlueprintHeadPivot->GetRelativeRotation().Yaw) <= 25.f);
				TestTrue(TEXT("Noticing the crab does not move the raven"),
					BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforeCrab, 0.1f));
				const float CrabAttentionAfterNotice = BlueprintController->CrabScurryAttentionRemaining;
				BlueprintController->Tick(0.25f);
				TestTrue(TEXT("Crab attention fades on a short timer"),
					BlueprintController->CrabScurryAttentionRemaining < CrabAttentionAfterNotice);
				BlueprintController->CrabScurryAttentionRemaining = 0.f;
				BlueprintController->CheckForNearbyCrabScurry();
				TestTrue(TEXT("One scurry cannot repeatedly restart the same glance"),
					FMath::IsNearlyZero(BlueprintController->CrabScurryAttentionRemaining));
				ScurryingCrab->ScurryRemaining = 0.f;
				TestFalse(TEXT("A crab's brief scurry naturally ends"), ScurryingCrab->IsScurrying());
				BlueprintController->CheckForNearbyCrabScurry();
				ScurryingCrab->RespondToQuietObservation(RavenPositionBeforeCrab);
				BlueprintController->CheckForNearbyCrabScurry();
				TestTrue(TEXT("A later scurry can draw the raven's attention again"),
					BlueprintController->CrabScurryAttentionRemaining > 0.f);
				ScurryingCrab->Destroy();
			}

			UClass* ConsciousResidentClass = LoadClass<AAutonomousAgentCharacter>(nullptr,
				TEXT("/Game/Agents/BP_Agent_Placeholder.BP_Agent_Placeholder_C"));
			TestNotNull(TEXT("Aster's grounded resident Blueprint is available to the quiet-presence fixture"), ConsciousResidentClass);
			if (ConsciousResidentClass)
			{
				const FTransform AsterTransform(FRotator::ZeroRotator,
					BlueprintRaven->GetActorLocation() + FVector(0.f, 450.f, 0.f));
				AAutonomousAgentCharacter* Aster = World->SpawnActorDeferred<AAutonomousAgentCharacter>(
					ConsciousResidentClass, AsterTransform, nullptr, nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (Aster)
				{
					Aster->AutoPossessAI = EAutoPossessAI::Disabled;
					Aster->FinishSpawning(AsterTransform);
				}
				TestNotNull(TEXT("Aster spawns without an AI controller or external service"), Aster);
				if (Aster)
				{
					BlueprintController->ListeningStoneAttentionRemaining = 0.f;
					BlueprintController->MinnowRippleAttentionRemaining = 0.f;
					BlueprintController->ResidentAttentionRemaining = 0.f;
					BlueprintController->NoticedResidentsInNearbyGroup.Reset();
					BlueprintController->LocomotionState = ERavenLocomotionState::Flying;
					BlueprintController->CheckForNearbyResidentPresence();
					TestTrue(TEXT("An airborne raven does not attend nearby resident presence"),
						FMath::IsNearlyZero(BlueprintController->ResidentAttentionRemaining));
					BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
					Aster->GetCharacterMovement()->Velocity = FVector(250.f, 0.f, 0.f);
					BlueprintController->CheckForNearbyResidentPresence();
					TestTrue(TEXT("A fast-moving resident does not demand the raven's attention"),
						FMath::IsNearlyZero(BlueprintController->ResidentAttentionRemaining));
					Aster->Consolidation->ConsciousState = EAgentConsciousState::Resting;
					BlueprintController->CheckForNearbyResidentPresence();
					TestTrue(TEXT("A sleeping resident does not draw the raven's attention"),
						FMath::IsNearlyZero(BlueprintController->ResidentAttentionRemaining));
					Aster->Consolidation->ConsciousState = EAgentConsciousState::Awake;
					Aster->GetCharacterMovement()->Velocity = FVector::ZeroVector;
					const FVector RavenPositionBeforePresence = BlueprintRaven->GetActorLocation();
					BlueprintController->HeadScanTime = 0.f;
					BlueprintHeadPivot->SetRelativeRotation(BlueprintController->RavenHeadRestRotation);
					BlueprintController->CheckForNearbyResidentPresence();
					TestTrue(TEXT("A nearby, unhurried resident gets one short acknowledgment"),
						BlueprintController->ResidentAttentionRemaining > 1.7f &&
						BlueprintController->ResidentAttentionRemaining <= ARavenAgentAIController::ResidentAttentionDuration);
					Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(0.f, 400.f, 20.f));
					BlueprintController->Tick(0.5f);
					TestTrue(TEXT("The perched raven gently turns its head toward Aster"),
						BlueprintHeadPivot->GetRelativeRotation().Yaw > 5.f && BlueprintHeadPivot->GetRelativeRotation().Yaw <= 25.f);
					TestTrue(TEXT("The glance tracks Aster's updated position while he remains calm and nearby"),
						BlueprintController->ResidentAttentionLocation.Equals(
							Aster->GetActorLocation() + FVector(0.f, 0.f, 90.f), 0.1f));
					TestTrue(TEXT("Acknowledging Aster does not move the raven"),
						BlueprintRaven->GetActorLocation().Equals(RavenPositionBeforePresence, 0.1f));
					const float PresenceAttentionAfterNotice = BlueprintController->ResidentAttentionRemaining;
					BlueprintController->Tick(0.25f);
					TestTrue(TEXT("Resident attention fades instead of holding a fixed pose"),
						BlueprintController->ResidentAttentionRemaining < PresenceAttentionAfterNotice);
					BlueprintController->ResidentAttentionRemaining = 0.f;
					BlueprintController->CheckForNearbyResidentPresence();
					TestTrue(TEXT("A resident who remains nearby does not retrigger the glance"),
						FMath::IsNearlyZero(BlueprintController->ResidentAttentionRemaining));
					const FTransform SecondResidentTransform(FRotator::ZeroRotator,
						BlueprintRaven->GetActorLocation() + FVector(0.f, -450.f, 0.f));
					AAutonomousAgentCharacter* SecondResident = World->SpawnActorDeferred<AAutonomousAgentCharacter>(
						ConsciousResidentClass, SecondResidentTransform, nullptr, nullptr,
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					if (SecondResident)
					{
						SecondResident->AutoPossessAI = EAutoPossessAI::Disabled;
						SecondResident->FinishSpawning(SecondResidentTransform);
					}
					TestNotNull(TEXT("A second nearby resident can be added to the quiet-presence fixture"), SecondResident);
					if (SecondResident)
					{
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("A second resident can receive their own brief acknowledgment"),
							BlueprintController->ResidentAttentionRemaining > 0.f);
						BlueprintController->ResidentAttentionRemaining = 0.f;
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("The raven does not alternate back to either resident while both remain nearby"),
							FMath::IsNearlyZero(BlueprintController->ResidentAttentionRemaining));
						BlueprintController->NoticedResidentsInNearbyGroup.Reset();
						Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(0.f, 450.f, 0.f));
						SecondResident->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(0.f, -300.f, 0.f));
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("When two residents are eligible, the raven notices the closer one first"),
							BlueprintController->ResidentAttentionLocation.Equals(
								SecondResident->GetActorLocation() + FVector(0.f, 0.f, 90.f), 0.1f));
						BlueprintController->ResidentAttentionRemaining = 0.f;
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("The next unacknowledged resident can then receive attention"),
							BlueprintController->ResidentAttentionRemaining > 0.f);
						BlueprintController->ResidentAttentionRemaining = 0.f;
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("Neither member of the nearby group is repeatedly selected"),
							FMath::IsNearlyZero(BlueprintController->ResidentAttentionRemaining));
						Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(800.f, 0.f, 0.f));
						SecondResident->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(800.f, 0.f, 0.f));
						BlueprintController->CheckForNearbyResidentPresence();
						Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(0.f, 450.f, 0.f));
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("Leaving as a group and calmly returning allows a fresh acknowledgment"),
							BlueprintController->ResidentAttentionRemaining > 0.f);
						SecondResident->Destroy();
					}
					else
					{
						Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(800.f, 0.f, 0.f));
						BlueprintController->CheckForNearbyResidentPresence();
						Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(0.f, 450.f, 0.f));
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("Leaving and calmly returning allows a fresh acknowledgment"),
							BlueprintController->ResidentAttentionRemaining > 0.f);
					}
					Aster->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(800.f, 0.f, 0.f));
					BlueprintController->Tick(0.1f);
					TestTrue(TEXT("The raven releases attention soon after Aster leaves"),
						BlueprintController->ResidentAttentionRemaining <= 0.31f);
					BlueprintController->Tick(0.4f);
					TestFalse(TEXT("A completed glance releases its weak attention target"),
						BlueprintController->ResidentAttentionTarget.IsValid());
					Aster->Destroy();
			}
			BlueprintController->ResidentAttentionRemaining = 0.f;
			if (BlueprintRiggedCrow)
			{
				UClass* RiggedResidentClass = LoadClass<AAutonomousAgentCharacter>(nullptr,
					TEXT("/Game/Agents/BP_Agent_Placeholder.BP_Agent_Placeholder_C"));
				TestNotNull(TEXT("Aster's resident Blueprint is available to the rigged Crow attention fixture"), RiggedResidentClass);
				if (RiggedResidentClass)
				{
					const FTransform ResidentTransform(FRotator::ZeroRotator,
						BlueprintRaven->GetActorLocation() + FVector(0.f, 450.f, 0.f));
					AAutonomousAgentCharacter* Resident = World->SpawnActorDeferred<AAutonomousAgentCharacter>(
						RiggedResidentClass, ResidentTransform, nullptr, nullptr,
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					if (Resident)
					{
						Resident->AutoPossessAI = EAutoPossessAI::Disabled;
						Resident->FinishSpawning(ResidentTransform);
						if (Resident->Consolidation)
							Resident->Consolidation->ConsciousState = EAgentConsciousState::Awake;
						Resident->GetCharacterMovement()->Velocity = FVector::ZeroVector;
					}
					TestNotNull(TEXT("Aster spawns without an AI controller for the rigged Crow cue"), Resident);
					if (Resident)
					{
						BlueprintController->LocomotionState = ERavenLocomotionState::Grounded;
						BlueprintController->ListeningStoneAttentionRemaining = 0.f;
						BlueprintController->MinnowRippleAttentionRemaining = 0.f;
						BlueprintController->ResidentAttentionRemaining = 0.f;
						BlueprintController->NoticedResidentsInNearbyGroup.Reset();
						BlueprintController->ListeningStoneCheckRemaining = 1.f;
						BlueprintController->CheckForNearbyResidentPresence();
						TestTrue(TEXT("The rigged Crow selects a calm nearby resident"),
							BlueprintController->ResidentAttentionTarget.Get() == Resident &&
							BlueprintController->ResidentAttentionRemaining > 1.7f);
						const FVector RavenLocationBeforeGlance = BlueprintRaven->GetActorLocation();
						const FRotator CrowRest = BlueprintController->RiggedCrowRestRotation;
						BlueprintController->Tick(0.25f);
						const float GlanceYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
							CrowRest.Yaw, BlueprintRiggedCrow->GetRelativeRotation().Yaw));
						TestTrue(TEXT("The rigged Crow gives a small visible turn toward the calm resident"),
							GlanceYaw > 3.f && GlanceYaw <= 12.f);
						TestTrue(TEXT("Rigged-Crow attention does not move Raven"),
							BlueprintRaven->GetActorLocation().Equals(RavenLocationBeforeGlance, 0.1f));
						Resident->SetActorLocation(BlueprintRaven->GetActorLocation() + FVector(800.f, 0.f, 0.f));
						BlueprintController->Tick(0.1f);
						TestTrue(TEXT("The rigged Crow releases attention soon after the resident leaves"),
							BlueprintController->ResidentAttentionRemaining <= 0.31f);
						BlueprintController->Tick(0.4f);
						TestFalse(TEXT("The rigged Crow releases its completed attention target"),
							BlueprintController->ResidentAttentionTarget.IsValid());
						TestTrue(TEXT("The rigged Crow returns to its authored orientation after the glance"),
							BlueprintRiggedCrow->GetRelativeRotation().Equals(CrowRest, 1.f));
						Resident->Destroy();
					}
				}
			}
		}
		}
		BlueprintController->UnPossess();
		BlueprintController->Destroy();
		BlueprintRaven->Destroy();
	}
	UAgentConsolidationComponent* RavenRest = NewObject<UAgentConsolidationComponent>(Raven);
	Raven->AddInstanceComponent(RavenRest);
	RavenRest->RegisterComponent();
	UAgentRestPresentationComponent* RavenPresentation = NewObject<UAgentRestPresentationComponent>(Raven);
	Raven->AddInstanceComponent(RavenPresentation);
	RavenPresentation->RegisterComponent();
	RavenPresentation->SetRestPosture(EAgentRestPosture::PerchedBird);
	RavenPresentation->BindToConsciousness(RavenRest);
	const FTransform RavenAwakeMeshPose = Raven->GetMesh()->GetRelativeTransform();
	TestFalse(TEXT("Unknown marker rejected"), Controller->RequestPerch(TEXT("Missing")));
	TestTrue(TEXT("Known marker accepted"), Controller->RequestPerch(TEXT("TestRoost")));
	for (int32 I = 0; I < 600; ++I) Controller->Tick(1.f / 60.f);
	TestTrue(TEXT("Arrived perched"), Controller->LocomotionState == ERavenLocomotionState::Perched);
	TestTrue(TEXT("Reached marker"), Raven->GetActorLocation().Equals(Perch->GetActorLocation(), 2.f));
	TestTrue(TEXT("Capsule remains upright"), FMath::IsNearlyZero(Raven->GetActorRotation().Pitch));
	TestTrue(TEXT("Arrival produces physical feedback"), Controller->DescribeActionState().Contains(TEXT("perch")));
	TestTrue(TEXT("Repeat perch request is accepted without takeoff"), Controller->RequestPerch(TEXT("TestRoost")));
	TestTrue(TEXT("Repeat request remains perched"), Controller->LocomotionState == ERavenLocomotionState::Perched);
	Perch->Tags.Add(TEXT("RavenNestSite"));
	const FVector BeforeReadOnlyAssessment = Raven->GetActorLocation();
	const FString SiteAssessment = Controller->AssessRoostSite(Perch);
	TestTrue(TEXT("Read-only roost assessment confirms the same upward-facing support used at arrival"), SiteAssessment.Contains(TEXT("an upward-facing support surface is currently beneath the marker")));
	TestTrue(TEXT("Open fixture has no nearby overhead collision in the sampled probes"), SiteAssessment.Contains(TEXT("0 of 5 short vertical visibility probes")));
	TestTrue(TEXT("Site assessment disclaims branch strength, waterproofing, and ownership"), SiteAssessment.Contains(TEXT("not proof of waterproof shelter")) && SiteAssessment.Contains(TEXT("does not establish branch strength")));
	TestTrue(TEXT("Read-only assessment does not move the perched raven"), Raven->GetActorLocation().Equals(BeforeReadOnlyAssessment) && Controller->LocomotionState == ERavenLocomotionState::Perched);
	AActor* OverheadCover = World->SpawnActor<AActor>();
	if (TestNotNull(TEXT("Overhead cover fixture actor spawned"), OverheadCover))
	{
		UBoxComponent* CoverBox = NewObject<UBoxComponent>(OverheadCover);
		OverheadCover->SetRootComponent(CoverBox);
		CoverBox->SetBoxExtent(FVector(80.f, 80.f, 20.f));
		CoverBox->SetCollisionProfileName(TEXT("BlockAll"));
		CoverBox->RegisterComponent();
		OverheadCover->SetActorLocation(Perch->GetActorLocation() + FVector(0.f, 0.f, Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 35.f));
		TestTrue(TEXT("Solid overhead fixture is found by the cover probes"), Controller->AssessRoostSite(Perch).Contains(TEXT("5 of 5 short vertical visibility probes")));
		OverheadCover->Destroy();
	}
	UAgentBrainComponent* RavenBrain = NewObject<UAgentBrainComponent>(Raven);
	Raven->AddInstanceComponent(RavenBrain);
	RavenBrain->RegisterComponent();
	const FString RavenSituation = RavenBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Raven perception includes nearby roost support and overhead evidence before an interaction"), RavenSituation.Contains(TEXT("Read-only site check")) && RavenSituation.Contains(TEXT("0 of 5 short vertical visibility probes")));
	FAgentDecision Inspect;
	Inspect.bValid = true;
	Inspect.ActionType = EAgentActionType::Interact;
	Inspect.ActionTarget = TEXT("TestRoost");
	Controller->HandleDecisionReady(Inspect);
	TestTrue(TEXT("Inspection reports implemented facts"), Controller->DescribeActionState().Contains(TEXT("No nest, ownership")));
	Controller->HandleDecisionReady(Inspect);
	TestTrue(TEXT("Repeated inspection reports cooldown"), Controller->DescribeActionState().Contains(TEXT("Already inspected")));
	Controller->HandleDecisionReady(Inspect);
	TestTrue(TEXT("Third identical autonomous choice is suppressed"), Controller->DescribeActionState().Contains(TEXT("Repeated action suppressed")));
	FAgentDecision Sleep;
	Sleep.bValid = true;
	Sleep.ActionType = EAgentActionType::Sleep;
	Controller->ActOnDecision(Sleep);
	TestFalse(TEXT("Perched raven can sleep"), RavenRest->IsAwake());
	Controller->Tick(0.05f);
	TestTrue(TEXT("Sleep leaves the wings at their authored rest rotations"),
		LeftWing->GetRelativeRotation().Equals(LeftWingRest) && RightWing->GetRelativeRotation().Equals(RightWingRest));
	TestFalse(TEXT("Raven sleep keeps the flight capsule un-crouched"), Raven->bIsCrouched);
	RavenPresentation->TickComponent(0.5f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Raven adopts its distinctive tucked resting posture"), Raven->GetMesh()->GetRelativeTransform().Equals(RavenAwakeMeshPose));
	FAgentDecision Wander;
	Wander.bValid = true;
	Wander.ActionType = EAgentActionType::Wander;
	const FVector SleepingLocation = Raven->GetActorLocation();
	Controller->ActOnDecision(Wander);
	Controller->Tick(1.f);
	TestTrue(TEXT("Sleep prevents movement and new actions"), Raven->GetActorLocation().Equals(SleepingLocation));
	RavenRest->WakeUp();
	RavenPresentation->TickComponent(0.5f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Wake restores awake state and original raven posture"), RavenRest->IsAwake() && Raven->GetMesh()->GetRelativeTransform().Equals(RavenAwakeMeshPose));
	RavenRest->BeginSleep(0.f);
	TestTrue(TEXT("No-memory consolidation completes without a model call"), RavenRest->IsAwake());

	ACharacter* AsterBody = World->SpawnActor<ACharacter>(FVector(2000.f, 0.f, 100.f), FRotator::ZeroRotator);
	AsterBody->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	AsterBody->SetActorLocation(FVector(0.f, 400.f, 302.f));
	UAgentBrainComponent* AsterBrain = NewObject<UAgentBrainComponent>(AsterBody);
	AsterBody->AddInstanceComponent(AsterBrain);
	AsterBrain->RegisterComponent();
	const FVector BlueprintRavenOriginalLocation = BlueprintRaven ? BlueprintRaven->GetActorLocation() : FVector::ZeroVector;
	if (BlueprintRaven) BlueprintRaven->SetActorLocation(Raven->GetActorLocation() + FVector(5000.f, 0.f, 0.f));
	Controller->bCarryingTwigs = true;
	const FString AsterSituation = AsterBrain->BuildSituationSummary(FAgentConversationContext());
	TestFalse(TEXT("Aster is not offered bird-sized roosts as movement targets"), AsterSituation.Contains(TEXT("move_to target: TestRoost")));
	TestTrue(TEXT("Aster receives the raven's visible perched activity as a transient nearby cue"),
		AsterSituation.Contains(TEXT("In clear view, the raven is perched")) && AsterSituation.Contains(TEXT("about 7 metres away")));
	TestTrue(TEXT("Aster can notice the carried twigs without being told the raven's intent"),
		AsterSituation.Contains(TEXT("bundle of fallen twigs held in the raven's beak")) &&
		AsterSituation.Contains(TEXT("does not tell you what it plans to do")));
	AsterBody->SetActorLocation(Raven->GetActorLocation() + FVector(1600.f, 0.f, 0.f));
	const FString DistantRavenSituation = AsterBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Raven activity remains visible at the broader awareness range"), DistantRavenSituation.Contains(TEXT("In clear view, the raven is perched")));
	TestFalse(TEXT("The finer foraging detail is reserved for close range"), DistantRavenSituation.Contains(TEXT("bundle of fallen twigs")));
	AsterBody->SetActorLocation(FVector(0.f, 400.f, 302.f));
	AActor* RavenOccluder = World->SpawnActor<AActor>();
	if (TestNotNull(TEXT("Aster-raven visibility blocker fixture spawned"), RavenOccluder))
	{
		UBoxComponent* OccluderBox = NewObject<UBoxComponent>(RavenOccluder);
		RavenOccluder->SetRootComponent(OccluderBox);
		OccluderBox->SetBoxExtent(FVector(100.f, 100.f, 150.f));
		OccluderBox->SetCollisionProfileName(TEXT("BlockAll"));
		OccluderBox->RegisterComponent();
		OccluderBox->SetWorldLocation(FVector(300.f, 200.f, 332.f));
		TestFalse(TEXT("Aster does not receive the raven activity cue through solid cover"),
			AsterBrain->BuildSituationSummary(FAgentConversationContext()).Contains(TEXT("In clear view, the raven is perched")));
		TestFalse(TEXT("Aster does not receive the carried-twig cue through solid cover"),
			AsterBrain->BuildSituationSummary(FAgentConversationContext()).Contains(TEXT("bundle of fallen twigs")));
		RavenOccluder->Destroy();
	}
	Controller->bCarryingTwigs = false;
	if (BlueprintRaven) BlueprintRaven->SetActorLocation(BlueprintRavenOriginalLocation);
	UAgentConsolidationComponent* AsterRest = NewObject<UAgentConsolidationComponent>(AsterBody);
	AsterBody->AddInstanceComponent(AsterRest);
	AsterRest->RegisterComponent();
	UAgentRestPresentationComponent* AsterPresentation = NewObject<UAgentRestPresentationComponent>(AsterBody);
	AsterBody->AddInstanceComponent(AsterPresentation);
	AsterPresentation->RegisterComponent();
	AsterPresentation->BindToConsciousness(AsterRest);
	const float AsterAwakeCapsuleHalfHeight = AsterBody->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	TestTrue(TEXT("Aster placeholder enters its sleep state"), AsterRest->BeginSleep(10.f));
	TestTrue(TEXT("Aster sleep requests the native grounded crouch posture"), AsterBody->GetCharacterMovement()->bWantsToCrouch);
	AsterBody->GetCharacterMovement()->Crouch(true);
	TestTrue(TEXT("CharacterMovement applies the lower sleeping capsule"), AsterBody->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() < AsterAwakeCapsuleHalfHeight);
	AsterRest->WakeUp();
	TestFalse(TEXT("Wake clears the pending crouch request"), AsterBody->GetCharacterMovement()->bWantsToCrouch);
	AsterBody->GetCharacterMovement()->UnCrouch(true);
	TestTrue(TEXT("CharacterMovement restores Aster's awake capsule size"), FMath::IsNearlyEqual(AsterBody->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), AsterAwakeCapsuleHalfHeight));
	TestFalse(TEXT("Wake restores Aster's original crouch capability"), AsterBody->GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch);
	Controller->BeginTakeoff(FVector(1000, 0, 700));
	for (int32 I = 0; I < 300; ++I) Controller->Tick(1.f / 60.f);
	TestTrue(TEXT("Departed roost"), Controller->LocomotionState == ERavenLocomotionState::Flying && Raven->GetActorLocation().Z > 650);
	// An unsupported marker must never be reported as a successful perch.
	Perch->SetActorLocation(FVector(1200, 0, 500));
	TestTrue(TEXT("Unsupported candidate is reported before attempting another arrival"), Controller->AssessRoostSite(Perch).Contains(TEXT("suitable upward-facing support was not confirmed")));
	Controller->RequestPerch(TEXT("TestRoost"));
	for (int32 I = 0; I < 600; ++I) Controller->Tick(1.f / 60.f);
	TestTrue(TEXT("Unsupported marker rejected at arrival"), Controller->LocomotionState != ERavenLocomotionState::Perched);
	// A blocked ascent must abort, not teleport or report successful arrival.
	Raven->SetActorLocation(FVector(600, 0, 100));
	Perch->SetActorLocation(FVector(600, 0, 302));
	Controller->RequestPerch(TEXT("TestRoost"));
	for (int32 I = 0; I < 600; ++I) Controller->Tick(1.f / 60.f);
	TestTrue(TEXT("Blocked ascent is not a perch"), Controller->LocomotionState != ERavenLocomotionState::Perched);
	TestTrue(TEXT("Blocking support not crossed"), Raven->GetActorLocation().Z < 200);
	Controller->UnPossess();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	// When Island is open, exercise its real collision geometry as well. These
	// transient fixtures are always destroyed and never acquire a brain/memory.
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Island = Context.World();
		if (Context.WorldType != EWorldType::Editor || !Island || Island->GetMapName() != TEXT("Island")) continue;
		// Grounded agents share elevated landmark tags with the raven. Verify the
		// saved Island has walkable ground beneath both Aster and the WindArch.
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Island);
		FNavLocation Ground;
		TestTrue(TEXT("Aster spawn has navigation"), Navigation && Navigation->ProjectPointToNavigation(FVector(-100400, 100960, 2823), Ground, FVector(250, 250, 1000)));
		TestTrue(TEXT("WindArch has a projected walking goal"), Navigation && Navigation->ProjectPointToNavigation(FVector(-101650, 100200, 2950), Ground, FVector(250, 250, 1000)));
		const UIslandInnkeeperSubsystem* InnkeeperDefaults = GetDefault<UIslandInnkeeperSubsystem>();
		UClass* GroundResidentClass = InnkeeperDefaults ? InnkeeperDefaults->BodyClass.LoadSynchronous() : nullptr;
		const AAutonomousAgentCharacter* GroundResidentDefaults = GroundResidentClass
			? Cast<AAutonomousAgentCharacter>(GroundResidentClass->GetDefaultObject()) : nullptr;
		const UAgentSocialComponent* SocialDefaults = GetDefault<UAgentSocialComponent>();
		const TArray<FVector> GroundResidentStarts = {
			FVector(-100400.f, 100960.f, 2823.f), // Aster's saved spawn used by the existing reachability check.
			FVector(-100039.f, 103308.f, 2778.f)  // Aster's logged indoor position when the raven approach failed.
		};
		const TCHAR* GroundResidentStartLabels[] = { TEXT("Aster spawn"), TEXT("inn common room") };
		AIslandWeather* IslandWeather = nullptr;
		for (TActorIterator<AIslandWeather> It(Island); It; ++It) { IslandWeather = *It; break; }
		TestNotNull(TEXT("Saved Island exposes its spatial weather signal to the raven"), IslandWeather);
		for (const FName Tag : {FName(TEXT("Roost_West")), FName(TEXT("Roost_East"))})
		{
			AActor* Marker = nullptr;
			for (TActorIterator<AActor> It(Island); It; ++It) if (It->ActorHasTag(Tag)) { Marker = *It; break; }
			if (!TestNotNull(*FString::Printf(TEXT("Island marker %s exists"), *Tag.ToString()), Marker)) continue;
			for (int32 StartIndex = 0; StartIndex < GroundResidentStarts.Num(); ++StartIndex)
			{
				FNavLocation ResidentApproachStart;
				FNavLocation ResidentApproachGoal;
				const bool bGroundResidentHasConversationalRoute = Navigation && GroundResidentDefaults && SocialDefaults &&
					AAutonomousAgentAIController::FindGroundedResidentApproachGoal(Navigation,
						GroundResidentStarts[StartIndex], Marker->GetActorLocation(),
						GroundResidentDefaults->GetNavAgentPropertiesRef(),
						GroundResidentDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
						SocialDefaults->SpeakingRadius, ResidentApproachStart, ResidentApproachGoal);
				TestTrue(*FString::Printf(TEXT("A ground resident from %s has a complete, in-range route beside airborne %s"),
					GroundResidentStartLabels[StartIndex], *Tag.ToString()), bGroundResidentHasConversationalRoute);
				if (bGroundResidentHasConversationalRoute)
					AddInfo(FString::Printf(TEXT("Ground resident route from %s to %s: %s -> %s."), GroundResidentStartLabels[StartIndex],
						*Tag.ToString(), *ResidentApproachStart.Location.ToCompactString(), *ResidentApproachGoal.Location.ToCompactString()));
			}
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Probe = Island->SpawnActor<ACharacter>(FVector(-100100, 100960, 3816.3), FRotator::ZeroRotator, Spawn);
			Probe->SetActorScale3D(FVector(0.65f));
			for (TActorIterator<APawn> It(Island); It; ++It)
				if (*It != Probe) Probe->GetCapsuleComponent()->IgnoreActorWhenMoving(*It, true);
			ARavenAgentAIController* Pilot = Island->SpawnActor<ARavenAgentAIController>(Spawn);
			Pilot->Possess(Probe);
			if (Tag == FName(TEXT("Roost_West")) && IslandWeather)
			{
				TArray<FVector> SupportedLocations;
				TArray<float> MeasuredWindSpeeds;
				TArray<FName> SupportedTags;
				for (const FName CandidateTag : {FName(TEXT("Roost_West")), FName(TEXT("Roost_East"))})
				{
					AActor* Candidate = nullptr;
					for (TActorIterator<AActor> It(Island); It; ++It)
						if (It->ActorHasTag(CandidateTag)) { Candidate = *It; break; }
					if (Candidate && Pilot->HasSuitablePerchSupport(Candidate))
					{
						SupportedTags.Add(CandidateTag);
						SupportedLocations.Add(Candidate->GetActorLocation());
						MeasuredWindSpeeds.Add(IslandWeather->GetLocalWind(Candidate->GetActorLocation(), Probe).Size());
					}
				}
				const float CurrentWindSpeed = IslandWeather->GetLocalWind(Probe->GetActorLocation(), Probe).Size();
				const int32 PreferredIndex = ARavenAgentAIController::SelectWindAwarePerch(
					Probe->GetActorLocation(), CurrentWindSpeed, SupportedLocations, MeasuredWindSpeeds);
				TestEqual(TEXT("Both real Island roosts provide supported sites for the wind choice"), SupportedLocations.Num(), 2);
				TestTrue(TEXT("The measured saved-Island conditions select one supported roost"), SupportedTags.IsValidIndex(PreferredIndex));
				if (SupportedTags.Num() == 2 && SupportedTags.IsValidIndex(PreferredIndex))
				{
					int32 StrongWindSamples = 0;
					TArray<int32> SelectionCounts = { 0, 0 };
					TArray<int32> StrongWindSelectionCounts = { 0, 0 };
					const double SampleStart = Island->GetTimeSeconds();
					const double SampleDuration = FMath::Max(1200.0, static_cast<double>(IslandWeather->CycleSeconds) * 2.0);
					for (double Seconds = SampleStart; Seconds <= SampleStart + SampleDuration; Seconds += 30.0)
					{
						const float SampledCurrentWind = IslandWeather->SampleLocalWind(Probe->GetActorLocation(), Seconds, Probe).Size();
						TArray<float> SampledPerchWinds;
						for (const FVector& Location : SupportedLocations)
							SampledPerchWinds.Add(IslandWeather->SampleLocalWind(Location, Seconds, Probe).Size());
						const int32 SampledChoice = ARavenAgentAIController::SelectWindAwarePerch(
							Probe->GetActorLocation(), SampledCurrentWind, SupportedLocations, SampledPerchWinds);
						if (SampledCurrentWind >= 85.f)
						{
							++StrongWindSamples;
							if (StrongWindSelectionCounts.IsValidIndex(SampledChoice)) ++StrongWindSelectionCounts[SampledChoice];
						}
						if (SelectionCounts.IsValidIndex(SampledChoice)) ++SelectionCounts[SampledChoice];
					}
					TestTrue(TEXT("Real strong-wind samples sometimes favor the calmer non-nearest West roost"), StrongWindSelectionCounts[0] > 0);
					AddInfo(FString::Printf(TEXT("Saved Island now: raven wind %.1f cm/s; %s %.1f cm/s, %s %.1f cm/s; preference %s. Across %.0f simulated seconds at 30-second steps: %d strong-wind samples, all-sample preferences %s=%d and %s=%d, strong-wind preferences %s=%d and %s=%d."),
						CurrentWindSpeed, *SupportedTags[0].ToString(), MeasuredWindSpeeds[0], *SupportedTags[1].ToString(), MeasuredWindSpeeds[1],
						*SupportedTags[PreferredIndex].ToString(), SampleDuration, StrongWindSamples,
						*SupportedTags[0].ToString(), SelectionCounts[0], *SupportedTags[1].ToString(), SelectionCounts[1],
						*SupportedTags[0].ToString(), StrongWindSelectionCounts[0], *SupportedTags[1].ToString(), StrongWindSelectionCounts[1]));
				}
			}
			const FString SavedSiteAssessment = Pilot->AssessRoostSite(Marker);
			AddInfo(FString::Printf(TEXT("%s read-only site assessment: %s"), *Tag.ToString(), *SavedSiteAssessment));
			TestTrue(*FString::Printf(TEXT("Actual %s marker reports the support that the landing gate will require"), *Tag.ToString()), SavedSiteAssessment.Contains(TEXT("an upward-facing support surface is currently beneath the marker")));
			Pilot->RequestPerch(Tag);
			for (int32 I = 0; I < 1200; ++I) Pilot->Tick(1.f / 60.f);
			FHitResult GroundHit;
			FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(RoostFixtureGround), false, Probe);
			const bool bSupport = Island->LineTraceSingleByChannel(GroundHit, Probe->GetActorLocation(), Probe->GetActorLocation() - FVector(0, 0, 70), ECC_Visibility, GroundQuery);
			AddInfo(FString::Printf(TEXT("%s: state %d, half height %.2f, support %d, distance %.2f, normal %s"), *Tag.ToString(), static_cast<int32>(Pilot->LocomotionState), Probe->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), bSupport, GroundHit.Distance, *GroundHit.ImpactNormal.ToString()));
			TestTrue(*FString::Printf(TEXT("Actual %s landing at %s"), *Tag.ToString(), *Probe->GetActorLocation().ToString()), Pilot->LocomotionState == ERavenLocomotionState::Perched);
			TestTrue(*FString::Printf(TEXT("Perched raven %s remains held in flying mode so gravity cannot pull it off the branch"), *Tag.ToString()),
				Probe->GetCharacterMovement()->MovementMode == MOVE_Flying);
			TestTrue(*FString::Printf(TEXT("Approach logic still recognizes perched raven %s as an elevated target"), *Tag.ToString()),
				AAutonomousAgentAIController::IsElevatedResidentForApproach(Probe));

			const FName OtherTag = Tag == FName(TEXT("Roost_West")) ? FName(TEXT("Roost_East")) : FName(TEXT("Roost_West"));
			AActor* OtherMarker = nullptr;
			for (TActorIterator<AActor> It(Island); It; ++It)
				if (It->ActorHasTag(OtherTag)) { OtherMarker = *It; break; }
			const bool bOtherRoostExists = TestNotNull(*FString::Printf(TEXT("Opposite Island roost %s exists"), *OtherTag.ToString()), OtherMarker);
			if (bOtherRoostExists)
			{
				TestTrue(*FString::Printf(TEXT("Perched raven can begin the real Island route %s -> %s"), *Tag.ToString(), *OtherTag.ToString()),
					Pilot->RequestPerch(OtherTag));
				for (int32 I = 0; I < 60 * 45 && (Pilot->bHasMovementTarget || Pilot->FlightWaypoints.Num() > 0); ++I)
					Pilot->Tick(1.f / 60.f);
				TestFalse(*FString::Printf(TEXT("The real Island route %s -> %s avoids collision"), *Tag.ToString(), *OtherTag.ToString()),
					Pilot->DescribeActionState().Contains(TEXT("blocked by geometry")));
				const bool bCrossRoostFlightCompleted = Pilot->LocomotionState == ERavenLocomotionState::Perched &&
					Probe->GetActorLocation().Equals(OtherMarker->GetActorLocation(), 2.f);
				TestTrue(*FString::Printf(TEXT("The raven completes the real Island flight %s -> %s"), *Tag.ToString(), *OtherTag.ToString()),
					bCrossRoostFlightCompleted);
				if (bCrossRoostFlightCompleted)
					AddInfo(FString::Printf(TEXT("Island roost flight %s -> %s completed at %s without an obstruction."),
						*Tag.ToString(), *OtherTag.ToString(), *Probe->GetActorLocation().ToCompactString()));
			}
			const float DepartureStartZ = Probe->GetActorLocation().Z;
			Pilot->BeginTakeoff(Probe->GetActorLocation() + FVector(-400, 0, 350));
			for (int32 I = 0; I < 300; ++I) Pilot->Tick(1.f / 60.f);
			TestTrue(*FString::Printf(TEXT("Actual %s departure after the inter-roost route"), *OtherTag.ToString()),
				Pilot->LocomotionState == ERavenLocomotionState::Flying && Probe->GetActorLocation().Z > DepartureStartZ + 300.f);
			Pilot->UnPossess();
			Pilot->Destroy();
			Probe->Destroy();
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandRavenWingCaptureTest, "CaptiveSky2.Visual.RavenWingMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandRavenWingCaptureTest::RunTest(const FString& Parameters)
{
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() && Context.World()->GetMapName() == TEXT("Island"))
		{
			Island = Context.World();
			break;
		}
	if (!TestNotNull(TEXT("The editor Island world is loaded for the raven wing capture"), Island)) return false;

	UClass* RavenClass = LoadClass<ACharacter>(nullptr, TEXT("/Game/Agents/BP_Raven_Placeholder.BP_Raven_Placeholder_C"));
	if (!TestNotNull(TEXT("The raven placeholder Blueprint is available"), RavenClass)) return false;
	AActor* Roost = nullptr;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("RavenPerch")) && It->ActorHasTag(TEXT("Roost_West"))) { Roost = *It; break; }
	if (!TestNotNull(TEXT("Roost_West provides a scenic capture location"), Roost)) return false;

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Raven = Island->SpawnActor<ACharacter>(RavenClass, Roost->GetActorLocation(), FRotator::ZeroRotator, Spawn);
	ARavenAgentAIController* Controller = Island->SpawnActor<ARavenAgentAIController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("A transient raven placeholder was spawned at the west roost"), Raven) ||
		!TestNotNull(TEXT("A transient raven controller was spawned"), Controller))
	{
		if (Raven) Raven->Destroy();
		if (Controller) Controller->Destroy();
		return false;
	}

	Controller->Possess(Raven);
	// A saved/preview Raven may already stand on this authored roost. Temporarily hide
	// nearby pawns so the capture proves which body the transient test actor renders.
	TArray<APawn*> TemporarilyHiddenPawns;
	auto RestoreNearbyPawns = [&TemporarilyHiddenPawns]()
	{
		for (APawn* HiddenPawn : TemporarilyHiddenPawns)
			if (IsValid(HiddenPawn)) HiddenPawn->SetActorHiddenInGame(false);
		TemporarilyHiddenPawns.Reset();
	};
	for (TActorIterator<APawn> It(Island); It; ++It)
		if (*It != Raven && !It->IsHidden() && FVector::DistSquared(It->GetActorLocation(), Raven->GetActorLocation()) <= FMath::Square(650.f))
		{
			TemporarilyHiddenPawns.Add(*It);
			It->SetActorHiddenInGame(true);
		}
	const bool bUsingRiggedCrow = Controller->RiggedCrowBody.IsValid();
	USkeletalMesh* AvailableCrowAsset = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow_CaptiveSky.SK_Crow_CaptiveSky"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	const bool bHasIslandCrowAsset = AvailableCrowAsset != nullptr;
	if (!AvailableCrowAsset)
		AvailableCrowAsset = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow.SK_Crow"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	TestTrue(TEXT("Raven uses the rigged Crow exactly when its optional asset is available"),
		bUsingRiggedCrow == (AvailableCrowAsset != nullptr));
	UProceduralMeshComponent* LeftWing = nullptr;
	UProceduralMeshComponent* RightWing = nullptr;
	USceneComponent* HeadPivot = nullptr;
	USceneComponent* LeftWingPivot = nullptr;
	USceneComponent* RightWingPivot = nullptr;
	UInstancedStaticMeshComponent* CarriedTwigs = nullptr;
	USkeletalMeshComponent* RiggedCrow = Controller->RiggedCrowBody.Get();
	TArray<UProceduralMeshComponent*> WingMeshes;
	Raven->GetComponents<UProceduralMeshComponent>(WingMeshes);
	for (UProceduralMeshComponent* Component : WingMeshes)
	{
		if (!Component) continue;
		if (Component->GetName() == TEXT("RavenLeftWingFeathers")) LeftWing = Component;
		if (Component->GetName() == TEXT("RavenRightWingFeathers")) RightWing = Component;
	}
	TArray<USceneComponent*> RavenComponents;
	Raven->GetComponents<USceneComponent>(RavenComponents);
	for (USceneComponent* Component : RavenComponents)
	{
		if (!Component) continue;
		if (Component->GetName() == TEXT("RavenHeadPivot")) HeadPivot = Component;
		if (Component->GetName() == TEXT("RavenLeftWingPivot")) LeftWingPivot = Component;
		if (Component->GetName() == TEXT("RavenRightWingPivot")) RightWingPivot = Component;
		if (Component->GetName() == TEXT("RavenCarriedTwigs")) CarriedTwigs = Cast<UInstancedStaticMeshComponent>(Component);
	}
	const bool bVisualReady = bUsingRiggedCrow
		? (TestNotNull(TEXT("The optional rigged Crow mesh is being used for Raven"), RiggedCrow) &&
			TestNotNull(TEXT("The rigged Crow idle animation resolved"), Controller->CrowIdleAnimation.Get()) &&
			TestNotNull(TEXT("The rigged Crow flight animation resolved"), Controller->CrowFlyAnimation.Get()) &&
			TestTrue(TEXT("The rigged Crow is visual-only for collision and navigation"),
				RiggedCrow && RiggedCrow->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !RiggedCrow->CanEverAffectNavigation()))
		: (TestNotNull(TEXT("The procedural fallback has a separate idle-scanning head pivot"), HeadPivot) &&
			TestNotNull(TEXT("The procedural fallback has a left wing"), LeftWing) &&
			TestNotNull(TEXT("The procedural fallback has a right wing"), RightWing) &&
			TestNotNull(TEXT("The left procedural wing has an animated pivot"), LeftWingPivot) &&
			TestNotNull(TEXT("The right procedural wing has an animated pivot"), RightWingPivot));
	if (!bVisualReady || !TestNotNull(TEXT("The beak has a hidden, instanced twig bundle ready to show on gather"), CarriedTwigs))
	{
		Controller->UnPossess();
		Raven->Destroy();
		Controller->Destroy();
		RestoreNearbyPawns();
		return false;
	}
	UMaterialInterface* IslandCrowMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/AnimalVarietyPack/Crow/Materials/M_Crow_CaptiveSky.M_Crow_CaptiveSky"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (bUsingRiggedCrow && RiggedCrow)
	{
		TestTrue(TEXT("The active Raven body is the imported Crow asset"), RiggedCrow->GetSkeletalMeshAsset() == AvailableCrowAsset);
		TestTrue(TEXT("The rigged Crow does not layer the procedural fallback wing meshes"), !LeftWing && !RightWing);
		UMaterialInterface* CrowMaterial = RiggedCrow->GetMaterial(0);
		TestNotNull(TEXT("The imported Crow keeps its authored feather material"), CrowMaterial);
		if (bHasIslandCrowAsset && IslandCrowMaterial)
			TestTrue(TEXT("The Island-specific dark-feather material is preferred when installed"), CrowMaterial == IslandCrowMaterial);
		const FSkeletalMeshRenderData* CrowRenderData = AvailableCrowAsset ? AvailableCrowAsset->GetResourceForRendering() : nullptr;
		if (CrowRenderData)
			for (int32 LODIndex = 0; LODIndex < CrowRenderData->LODRenderData.Num(); ++LODIndex)
				for (const FSkelMeshRenderSection& Section : CrowRenderData->LODRenderData[LODIndex].RenderSections)
				{
					AddInfo(FString::Printf(TEXT("Crow render section LOD=%d materialIndex=%u componentMaterial=%s"),
						LODIndex, Section.MaterialIndex, *GetPathNameSafe(RiggedCrow->GetMaterial(Section.MaterialIndex))));
					TestTrue(TEXT("Every imported Crow render section resolves an assigned component material"),
						RiggedCrow->GetMaterial(Section.MaterialIndex) != nullptr);
				}
		TArray<UTexture*> CrowTextures;
		if (CrowMaterial) CrowMaterial->GetUsedTextures(CrowTextures);
		bool bHasBaseColorTexture = false;
		for (const UTexture* Texture : CrowTextures)
			bHasBaseColorTexture |= Texture && Texture->GetFName() == TEXT("T_Crow_BaseColor");
		TestTrue(TEXT("The authored feather material resolves the Crow base-color texture"), bHasBaseColorTexture);
	}
	const FRotator LeftRest = LeftWingPivot ? LeftWingPivot->GetRelativeRotation() : FRotator::ZeroRotator;
	const FRotator RightRest = RightWingPivot ? RightWingPivot->GetRelativeRotation() : FRotator::ZeroRotator;
	TestEqual(TEXT("The beak bundle contains three collisionless twigs"), CarriedTwigs->GetInstanceCount(), 3);
	TestFalse(TEXT("The beak is empty before gathering"), CarriedTwigs->IsVisible());

	const FIntPoint CaptureSize(1280, 720);
	UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	Target->RenderTargetFormat = RTF_RGBA8_SRGB;
	Target->InitAutoFormat(CaptureSize.X, CaptureSize.Y);
	Target->UpdateResourceImmediate(true);
	const FVector LookAt = Raven->GetActorLocation() + FVector(0.f, 0.f, 70.f);
	const FVector CameraLocation = LookAt + FVector(-330.f, -420.f, 150.f);
	ASceneCapture2D* Camera = Island->SpawnActor<ASceneCapture2D>(CameraLocation, (LookAt - CameraLocation).Rotation(), Spawn);
	USceneCaptureComponent2D* Capture = Camera ? Camera->GetCaptureComponent2D() : nullptr;
	if (!TestNotNull(TEXT("The transient real-RHI wing camera was spawned"), Capture))
	{
		Controller->UnPossess();
		Raven->Destroy();
		Controller->Destroy();
		if (Camera) Camera->Destroy();
		RestoreNearbyPawns();
		return false;
	}
	Capture->TextureTarget = Target;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 42.f;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	IStreamingManager::Get().StreamAllResources(2.f);

	const FString CaptureDirectory = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),
		TEXT("Viewpoints"), TEXT("RavenWingMotion_"), FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
	auto SavePose = [this, &Capture, &Target, &CaptureSize, &CaptureDirectory](const TCHAR* FileName)
	{
		Capture->CaptureScene();
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) || Pixels.Num() != CaptureSize.X * CaptureSize.Y)
			return false;
		for (FColor& Pixel : Pixels) Pixel.A = 255;
		const FString Path = CaptureDirectory / FString(FileName);
		if (!FImageUtils::SaveImageByExtension(*Path, FImageView(Pixels.GetData(), CaptureSize.X, CaptureSize.Y))) return false;
		AddInfo(FString::Printf(TEXT("Raven wing pose captured at %s"), *Path));
		return true;
	};

	TestTrue(TEXT("Authored rest-pose screenshot is saved"), SavePose(TEXT("01_Rest.png")));
	const ESceneCapturePrimitiveRenderMode OriginalRenderMode = Capture->PrimitiveRenderMode;
	Capture->ShowOnlyActors.Add(Raven);
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	TestTrue(TEXT("An isolated rest-pose capture proves the transient Raven body is what renders"),
		SavePose(TEXT("00_IsolatedRest.png")));
	Capture->ShowOnlyActors.Reset();
	Capture->PrimitiveRenderMode = OriginalRenderMode;
	Controller->Tick(1.f);
	if (bUsingRiggedCrow)
		TestTrue(TEXT("Grounded Raven selects the rigged Crow's idle-look animation"), Controller->CurrentCrowAnimation == Controller->CrowIdleAnimation);
	else
	{
		const float IdleHeadYaw = HeadPivot->GetRelativeRotation().Yaw;
		TestTrue(TEXT("The grounded head scan remains visibly small and bounded"), FMath::Abs(IdleHeadYaw) > 0.1f && FMath::Abs(IdleHeadYaw) <= 7.f);
	}
	TestTrue(TEXT("Subtle idle-glance screenshot is saved"), SavePose(TEXT("02_IdleGlance.png")));
	// Keep this editor-world capture presentation-only: Editor worlds intentionally have
	// no persistent world-state subsystem. Production gathering is covered by game-world tests.
	FVector ForageGround = FVector::ZeroVector;
	bool bFoundForageGround = false;
	const FVector ForageOffsets[] = { FVector(450.f, 0.f, 0.f), FVector(0.f, 450.f, 0.f),
		FVector(-450.f, 0.f, 0.f), FVector(0.f, -450.f, 0.f), FVector(800.f, 800.f, 0.f) };
	FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(RavenWingCaptureForageGround), false, Raven);
	for (const FVector& Offset : ForageOffsets)
	{
		FHitResult GroundHit;
		const FVector Candidate = Roost->GetActorLocation() + Offset;
		if (Island->LineTraceSingleByChannel(GroundHit, Candidate + FVector(0.f, 0.f, 1600.f),
			Candidate - FVector(0.f, 0.f, 3000.f), ECC_Visibility, GroundQuery) && GroundHit.ImpactNormal.Z >= 0.8f)
		{
			ForageGround = GroundHit.ImpactPoint;
			bFoundForageGround = true;
			break;
		}
	}
	if (!TestTrue(TEXT("Nearby walkable ground is available for the transient forage fixture"), bFoundForageGround))
	{
		Controller->UnPossess(); Raven->Destroy(); Controller->Destroy(); Camera->Destroy(); RestoreNearbyPawns(); return false;
	}
	FIslandArrangementSite ForageSite;
	ForageSite.Id = TEXT("ArrangingGround_RavenWingCapture");
	ForageSite.Location = ForageGround;
	AIslandArrangement* ForagePatch = Island->SpawnActor<AIslandArrangement>(ForageGround, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("A transient twig pile is spawned for the visible gather action"), ForagePatch))
	{
		Controller->UnPossess(); Raven->Destroy(); Controller->Destroy(); Camera->Destroy(); RestoreNearbyPawns(); return false;
	}
	ForagePatch->ShowSite(ForageSite, 1);
	const FVector PerchedLocation = Raven->GetActorLocation();
	const FTransform PerchedCameraTransform = Camera->GetActorTransform();
	Raven->SetActorLocation(ForageGround + FVector(0.f, 0.f, Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	const FVector ForageLookAt = Raven->GetActorLocation() + FVector(0.f, 0.f, 70.f);
	const FVector ForageCameraLocation = ForageLookAt + FVector(-330.f, -420.f, 150.f);
	Camera->SetActorLocationAndRotation(ForageCameraLocation, (ForageLookAt - ForageCameraLocation).Rotation());
	Raven->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Controller->LocomotionState = ERavenLocomotionState::Grounded;
	UClass* AttentionResidentClass = LoadClass<AAutonomousAgentCharacter>(nullptr,
		TEXT("/Game/Agents/BP_Agent_Placeholder.BP_Agent_Placeholder_C"));
	AAutonomousAgentCharacter* AttentionResident = nullptr;
	const FVector AttentionResidentLocation = Raven->GetActorLocation() + FVector(0.f, 300.f, 0.f);
	if (TestNotNull(TEXT("Aster's placeholder is available for the resident-attention capture"), AttentionResidentClass))
	{
		const FTransform ResidentTransform(FRotator(0.f, 180.f, 0.f), AttentionResidentLocation);
		AttentionResident = Island->SpawnActorDeferred<AAutonomousAgentCharacter>(AttentionResidentClass,
			ResidentTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (AttentionResident)
		{
			AttentionResident->AutoPossessAI = EAutoPossessAI::Disabled;
			AttentionResident->FinishSpawning(ResidentTransform);
			if (AttentionResident->Consolidation)
				AttentionResident->Consolidation->ConsciousState = EAgentConsciousState::Awake;
			AttentionResident->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
	}
	if (TestNotNull(TEXT("A transient Aster placeholder was spawned for the attention capture"), AttentionResident))
	{
		TestTrue(TEXT("Aster is conscious for the presentation capture"),
			AttentionResident->Consolidation && AttentionResident->Consolidation->IsAwake());
		Controller->ResidentAttentionRemaining = 0.f;
		Controller->ResidentAttentionTarget.Reset();
		Controller->ListeningStoneAttentionRemaining = 0.f;
		Controller->MinnowRippleAttentionRemaining = 0.f;
		Controller->ListeningStoneCheckRemaining = 1.f;
		Controller->HeadScanTime = 0.f;
		if (HeadPivot)
			HeadPivot->SetRelativeRotation(Controller->RavenHeadRestRotation);
		else if (RiggedCrow)
			RiggedCrow->SetRelativeRotation(Controller->RiggedCrowRestRotation);
		// RavenPerch covers eligibility/one-shot selection; this editor-world capture stages
		// the already-selected cue so the screenshot focuses on presentation and framing.
		Controller->ResidentAttentionTarget = AttentionResident;
		Controller->ResidentAttentionLocation = AttentionResidentLocation + FVector(0.f, 0.f, 90.f);
		Controller->ResidentAttentionRemaining = ARavenAgentAIController::ResidentAttentionDuration;
		Controller->Tick(0.25f);
		if (bUsingRiggedCrow && RiggedCrow)
		{
			const float BodyTurn = FMath::Abs(FMath::FindDeltaAngleDegrees(
				Controller->RiggedCrowRestRotation.Yaw, RiggedCrow->GetRelativeRotation().Yaw));
			TestTrue(TEXT("The imported Crow visibly turns toward Aster without moving its actor"),
				BodyTurn > 3.f && BodyTurn <= 12.f &&
				Raven->GetActorLocation().Equals(ForageGround + FVector(0.f, 0.f,
					Raven->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), 0.1f));
		}
	else if (HeadPivot)
		TestTrue(TEXT("The procedural Raven visibly turns its head toward Aster"),
			FMath::Abs(HeadPivot->GetRelativeRotation().Yaw) > 4.f &&
			FMath::Abs(HeadPivot->GetRelativeRotation().Yaw) <= 25.f);
	ForagePatch->SetActorHiddenInGame(true);
	const FVector AttentionLookAt = (Raven->GetActorLocation() + AttentionResidentLocation) * 0.5f + FVector(0.f, 0.f, 25.f);
	const FVector CameraOffsets[] = { FVector(-660.f, 0.f, 460.f), FVector(660.f, 0.f, 460.f),
		FVector(0.f, 660.f, 460.f), FVector(0.f, -660.f, 460.f) };
	FVector AttentionCameraLocation = AttentionLookAt + CameraOffsets[0];
	FCollisionQueryParams AttentionCameraQuery(SCENE_QUERY_STAT(RavenAttentionCameraVisibility), false, Camera);
	AttentionCameraQuery.AddIgnoredActor(Raven);
	AttentionCameraQuery.AddIgnoredActor(AttentionResident);
	AttentionCameraQuery.AddIgnoredActor(ForagePatch);
	int32 AttentionCameraBlockers = MAX_int32;
	for (const FVector& CameraOffset : CameraOffsets)
	{
		const FVector CandidateCameraLocation = AttentionLookAt + CameraOffset;
		FHitResult RavenOcclusion;
		FHitResult ResidentOcclusion;
		const bool bRavenOccluded = Island->LineTraceSingleByChannel(RavenOcclusion, CandidateCameraLocation,
			Raven->GetActorLocation() + FVector(0.f, 0.f, 25.f), ECC_Visibility, AttentionCameraQuery);
		const bool bResidentOccluded = Island->LineTraceSingleByChannel(ResidentOcclusion, CandidateCameraLocation,
			AttentionResidentLocation, ECC_Visibility, AttentionCameraQuery);
		const int32 BlockerCount = static_cast<int32>(bRavenOccluded) + static_cast<int32>(bResidentOccluded);
		if (BlockerCount < AttentionCameraBlockers)
		{
			AttentionCameraBlockers = BlockerCount;
			AttentionCameraLocation = CandidateCameraLocation;
		}
		if (BlockerCount == 0)
			break;
	}
	AddInfo(FString::Printf(TEXT("Resident-attention selected camera at %s with %d blocked subject traces"),
		*AttentionCameraLocation.ToCompactString(), AttentionCameraBlockers));
	Camera->SetActorLocationAndRotation(AttentionCameraLocation, (AttentionLookAt - AttentionCameraLocation).Rotation());
	TestTrue(TEXT("Calm resident attention screenshot is saved"), SavePose(TEXT("03_ResidentAttention.png")));
	AttentionResident->Destroy();
	Controller->ResidentAttentionRemaining = 0.f;
	Controller->ResidentAttentionTarget.Reset();
	Controller->NoticedResidentsInNearbyGroup.Reset();
	if (HeadPivot)
		HeadPivot->SetRelativeRotation(Controller->RavenHeadRestRotation);
	if (RiggedCrow)
		RiggedCrow->SetRelativeRotation(Controller->RiggedCrowRestRotation);
	ForagePatch->SetActorHiddenInGame(false);
	}
	Camera->SetActorLocationAndRotation(ForageCameraLocation, (ForageLookAt - ForageCameraLocation).Rotation());
	Controller->bCarryingTwigs = true;
	Controller->Tick(0.f);
	TestTrue(TEXT("The carried-twig presentation shows a beak bundle while leaving the patch intact"),
		Controller->bCarryingTwigs && ForagePatch->HasForageableTwigs() && ForagePatch->GetVisibleForageTwigCount() == 7 &&
		CarriedTwigs->IsVisible() && !CarriedTwigs->bHiddenInGame &&
		CarriedTwigs->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !CarriedTwigs->CanEverAffectNavigation());
	if (RiggedCrow)
	{
		FTransform FirstTwigWorld;
		CarriedTwigs->GetInstanceTransform(0, FirstTwigWorld, true);
		const FTransform CarrySocketWorld = RiggedCrow->GetSocketTransform(CarriedTwigs->GetAttachSocketName(), RTS_World);
		const FVector TwigOffsetFromHead = FirstTwigWorld.GetLocation() - CarrySocketWorld.GetLocation();
		TestTrue(TEXT("The carried twig bundle sits ahead of and slightly below the Crow's head"),
			FVector::DotProduct(TwigOffsetFromHead, Raven->GetActorForwardVector()) > 10.f && TwigOffsetFromHead.Z < 0.f);
		AddInfo(FString::Printf(TEXT("Twig anchor socket=%s firstInstanceWorld=%s offsetFromSocket=%s"),
			*CarriedTwigs->GetAttachSocketName().ToString(), *FirstTwigWorld.GetLocation().ToCompactString(),
			*TwigOffsetFromHead.ToCompactString()));
	}
	TestTrue(TEXT("Carried twig screenshot is saved"), SavePose(TEXT("04_CarryingTwigs.png")));
	AAutonomousAgentCharacter* DistantAster = Island->SpawnActor<AAutonomousAgentCharacter>(
		AttentionResidentClass, Raven->GetActorLocation() + FVector(600.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), Spawn);
	if (TestNotNull(TEXT("A second transient Aster is available to scale gameplay-distance twig captures"), DistantAster))
	{
		auto CaptureAtResidentRange = [this, &Raven, &DistantAster, &Camera, &SavePose, &ForagePatch](float ResidentRange,
			const TCHAR* ScreenshotName)
		{
			const FVector RavenLocation = Raven->GetActorLocation();
			const FVector ResidentLocation = RavenLocation + FVector(ResidentRange, 0.f, 0.f);
			DistantAster->SetActorLocation(ResidentLocation);
			const FVector LookAt = (RavenLocation + ResidentLocation) * 0.5f + FVector(0.f, 0.f, 65.f);
			const float CameraDistance = ResidentRange * 2.f + 200.f;
			const float CameraElevation = ResidentRange * 0.65f + 200.f;
			const FVector Offsets[] = {
				FVector(0.f, -CameraDistance, CameraElevation), FVector(0.f, CameraDistance, CameraElevation),
				FVector(-CameraDistance, 0.f, CameraElevation), FVector(CameraDistance, 0.f, CameraElevation)
			};
			FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenCarryDistanceCamera), false, Camera);
			Query.AddIgnoredActor(Raven);
			Query.AddIgnoredActor(DistantAster);
			Query.AddIgnoredActor(ForagePatch);
			FVector CameraLocation = LookAt + Offsets[0];
			int32 BestBlockerCount = MAX_int32;
			for (const FVector& Offset : Offsets)
			{
				const FVector Candidate = LookAt + Offset;
				FHitResult RavenHit;
				FHitResult ResidentHit;
				const bool bRavenBlocked = Raven->GetWorld()->LineTraceSingleByChannel(RavenHit, Candidate,
					RavenLocation + FVector(0.f, 0.f, 35.f), ECC_Visibility, Query);
				const bool bResidentBlocked = Raven->GetWorld()->LineTraceSingleByChannel(ResidentHit, Candidate,
					ResidentLocation + FVector(0.f, 0.f, 80.f), ECC_Visibility, Query);
				const int32 BlockerCount = static_cast<int32>(bRavenBlocked) + static_cast<int32>(bResidentBlocked);
				if (BlockerCount < BestBlockerCount)
				{
					BestBlockerCount = BlockerCount;
					CameraLocation = Candidate;
				}
				if (BlockerCount == 0) break;
			}
			Camera->SetActorLocationAndRotation(CameraLocation, (LookAt - CameraLocation).Rotation());
			return SavePose(ScreenshotName);
		};
		TestTrue(TEXT("Aster and the carried bundle are framed together at 3 m"),
			CaptureAtResidentRange(300.f, TEXT("07_CarryingTwigs_3m.png")));
		TestTrue(TEXT("Aster and the carried bundle are framed together at 6 m"),
			CaptureAtResidentRange(600.f, TEXT("08_CarryingTwigs_6m.png")));
		TestTrue(TEXT("Aster and the carried bundle are framed together at 12 m"),
			CaptureAtResidentRange(1200.f, TEXT("09_CarryingTwigs_12m.png")));
		DistantAster->Destroy();
	}
	// Reset only this transient test actor; a real weave has separate persistence coverage.
	Controller->bCarryingTwigs = false;
	Controller->Tick(0.f);
	TestFalse(TEXT("Weaving or consuming the bundle restores the empty beak"), CarriedTwigs->IsVisible());
	ForagePatch->Destroy();
	Raven->SetActorLocation(PerchedLocation);
	Camera->SetActorTransform(PerchedCameraTransform);
	Raven->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Controller->LocomotionState = ERavenLocomotionState::Perched;
	Controller->LocomotionState = ERavenLocomotionState::Flying;
	Controller->Tick(0.05f);
	TArray<FTransform> FirstCrowFlightPose;
	if (bUsingRiggedCrow)
	{
		TestTrue(TEXT("Flight selects the rigged Crow's flight animation"), Controller->CurrentCrowAnimation == Controller->CrowFlyAnimation);
		if (RiggedCrow && Controller->CrowFlyAnimation)
		{
			const float FlightDuration = Controller->CrowFlyAnimation->GetPlayLength();
			RiggedCrow->SetPosition(FlightDuration * 0.125f, false);
			RiggedCrow->RefreshBoneTransforms();
			for (int32 BoneIndex = 0; BoneIndex < RiggedCrow->GetNumBones(); ++BoneIndex)
				FirstCrowFlightPose.Add(RiggedCrow->GetBoneTransform(BoneIndex));
		}
	}
	else
		TestTrue(TEXT("Flight visibly rotates the procedural wing pivots"),
			!LeftWingPivot->GetRelativeRotation().Equals(LeftRest) && !RightWingPivot->GetRelativeRotation().Equals(RightRest));
	TestTrue(TEXT("Wingdown flight screenshot is saved"), SavePose(TEXT("05_FlightStrokeA.png")));
	Controller->Tick(0.15f);
	if (bUsingRiggedCrow && RiggedCrow && Controller->CrowFlyAnimation)
	{
		const float FlightDuration = Controller->CrowFlyAnimation->GetPlayLength();
		RiggedCrow->SetPosition(FlightDuration * 0.625f, false);
		RiggedCrow->RefreshBoneTransforms();
		bool bFlightPoseChanged = false;
		for (int32 BoneIndex = 0; BoneIndex < FirstCrowFlightPose.Num(); ++BoneIndex)
			bFlightPoseChanged |= !RiggedCrow->GetBoneTransform(BoneIndex).Equals(FirstCrowFlightPose[BoneIndex], 0.1f);
		TestTrue(TEXT("Two captured rigged-Crow flight phases contain distinct rendered skeletal poses"), bFlightPoseChanged);
	}
	TestTrue(TEXT("Opposite flight stroke screenshot is saved"), SavePose(TEXT("06_FlightStrokeB.png")));
	if (!bUsingRiggedCrow)
		TestTrue(TEXT("Flight fully deploys both wings from their folded perch pose"),
			FMath::Abs(LeftWingPivot->GetRelativeRotation().Yaw) < 0.1f && FMath::Abs(RightWingPivot->GetRelativeRotation().Yaw) < 0.1f);
	Controller->LocomotionState = ERavenLocomotionState::Grounded;
	Controller->Tick(0.2f);
	if (!bUsingRiggedCrow)
		TestTrue(TEXT("A captured flight returns both wings to their authored rests"),
			LeftWingPivot->GetRelativeRotation().Equals(LeftRest) && RightWingPivot->GetRelativeRotation().Equals(RightRest));
	Controller->UnPossess();
	RestoreNearbyPawns();
	Raven->Destroy();
	Controller->Destroy();
	Camera->Destroy();
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRavenFlightTest, "CaptiveSky2.Agent.RavenFlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRavenFlightTest::RunTest(const FString& Parameters)
{
	// A wall stands between the raven and a landmark; the raven should climb over it, not stop at it.
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto Block = [World](const FVector& Centre, const FVector& Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Actor->SetActorLocation(Centre);
		return Actor;
	};
	Block(FVector(0, 0, -50), FVector(8000, 8000, 50));    // ground
	Block(FVector(1500, 0, 400), FVector(40, 1500, 400));  // an 8 m wall across the route
	ACharacter* Raven = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
	ATargetPoint* Landmark = World->SpawnActor<ATargetPoint>(FVector(3000, 0, 100), FRotator::ZeroRotator);
	Landmark->Tags = {TEXT("FarLandmark"), TEXT("IslandLandmark")};
	World->BeginPlay();
	Controller->Possess(Raven);

	FAgentDecision Fly;
	Fly.bValid = true;
	Fly.ActionType = EAgentActionType::MoveTo;
	Fly.ActionTarget = TEXT("FarLandmark");
	Controller->ActOnDecision(Fly);
	float HighestZ = 0.f;
	for (int32 Step = 0; Step < 60 * 30 && (Controller->bHasMovementTarget || Controller->FlightWaypoints.Num() > 0); ++Step)
	{
		Controller->Tick(1.f / 60.f);
		HighestZ = FMath::Max(HighestZ, static_cast<float>(Raven->GetActorLocation().Z));
	}
	const FVector Destination = Landmark->GetActorLocation() + FVector(0, 0, 180);
	TestFalse(TEXT("The wall does not stop the flight"), Controller->DescribeActionState().Contains(TEXT("blocked by geometry")));
	TestTrue(TEXT("The raven arrives at the far landmark"), Raven->GetActorLocation().Equals(Destination, 40.f));
	TestTrue(TEXT("It crossed above the wall"), HighestZ > 800.f);

	// A clear route stays a straight flight with no detour.
	TestTrue(TEXT("A clear route is flown directly"), Controller->PlanFlightLeg(FVector(2500, 0, 300), FVector(3500, 0, 300)).Equals(FVector(3500, 0, 300)) && Controller->FlightWaypoints.Num() == 0);

	// A perch below a low canopy must allow a lateral exit before the raven climbs.
	Raven->SetActorLocation(FVector(6000, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
	Block(FVector(6000, 0, 220), FVector(200, 200, 20)); // underside at Z 200; vertical ascent is blocked
	const FVector OpenAirTarget(4500, 0, 300);
	Controller->BeginTakeoff(OpenAirTarget);
	TestTrue(TEXT("A blocked vertical takeoff finds a collision-clear sideways exit"), Controller->bHasTakeoffEscapeTarget);
	TestTrue(TEXT("The first escape leg moves laterally under the canopy"), FVector::Dist2D(Controller->MovementTarget, Raven->GetActorLocation()) > 100.f);
	TestTrue(TEXT("The escape search favors the chosen destination direction"), Controller->MovementTarget.X < Raven->GetActorLocation().X);
	for (int32 Step = 0; Step < 60 * 30 && (Controller->bHasMovementTarget || Controller->FlightWaypoints.Num() > 0); ++Step)
		Controller->Tick(1.f / 60.f);
	TestFalse(TEXT("The canopy does not trap the raven on takeoff"), Controller->DescribeActionState().Contains(TEXT("blocked by geometry")));
	TestTrue(TEXT("After clearing the canopy, the raven continues to its chosen destination"), Raven->GetActorLocation().Equals(OpenAirTarget, 40.f));
	Controller->UnPossess();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
