#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentRestPresentationComponent.h"
#include "AgentSocialComponent.h"
#include "IslandInnkeeperSubsystem.h"
#include "IslandArrangement.h"
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
	const FString AsterSituation = AsterBrain->BuildSituationSummary(FAgentConversationContext());
	TestFalse(TEXT("Aster is not offered bird-sized roosts as movement targets"), AsterSituation.Contains(TEXT("move_to target: TestRoost")));
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
	if (bUsingRiggedCrow && RiggedCrow)
	{
		UMaterialInterface* CrowMaterial = RiggedCrow->GetMaterial(0);
		TestNotNull(TEXT("The imported Crow keeps its authored feather material"), CrowMaterial);
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
	Controller->bCarryingTwigs = true;
	Controller->Tick(0.f);
	TestTrue(TEXT("The carried-twig presentation shows a beak bundle while leaving the patch intact"),
		Controller->bCarryingTwigs && ForagePatch->HasForageableTwigs() && ForagePatch->GetVisibleForageTwigCount() == 7 &&
		CarriedTwigs->IsVisible() && !CarriedTwigs->bHiddenInGame &&
		CarriedTwigs->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !CarriedTwigs->CanEverAffectNavigation());
	TestTrue(TEXT("Carried twig screenshot is saved"), SavePose(TEXT("03_CarryingTwigs.png")));
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
	if (bUsingRiggedCrow)
		TestTrue(TEXT("Flight selects the rigged Crow's flight animation"), Controller->CurrentCrowAnimation == Controller->CrowFlyAnimation);
	else
		TestTrue(TEXT("Flight visibly rotates the procedural wing pivots"),
			!LeftWingPivot->GetRelativeRotation().Equals(LeftRest) && !RightWingPivot->GetRelativeRotation().Equals(RightRest));
	TestTrue(TEXT("Wingdown flight screenshot is saved"), SavePose(TEXT("04_FlightStrokeA.png")));
	Controller->Tick(0.15f);
	TestTrue(TEXT("Opposite flight stroke screenshot is saved"), SavePose(TEXT("05_FlightStrokeB.png")));
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
