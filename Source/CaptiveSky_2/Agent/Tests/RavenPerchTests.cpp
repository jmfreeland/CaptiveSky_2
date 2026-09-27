#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentRestPresentationComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRavenPerchTest, "CaptiveSky2.Agent.RavenPerch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRavenPerchTest::RunTest(const FString& Parameters)
{
	// No gateway, model requests, or autobiographical memory in this fixture.
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	// CreateWorld already initializes the world; do not initialize WorldSettings twice.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACharacter* Raven = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator);
	ARavenAgentAIController* Controller = World->SpawnActor<ARavenAgentAIController>();
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
		for (const FName Tag : {FName(TEXT("Roost_West")), FName(TEXT("Roost_East"))})
		{
			AActor* Marker = nullptr;
			for (TActorIterator<AActor> It(Island); It; ++It) if (It->ActorHasTag(Tag)) { Marker = *It; break; }
			if (!TestNotNull(*FString::Printf(TEXT("Island marker %s exists"), *Tag.ToString()), Marker)) continue;
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Probe = Island->SpawnActor<ACharacter>(FVector(-100100, 100960, 3816.3), FRotator::ZeroRotator, Spawn);
			Probe->SetActorScale3D(FVector(0.65f));
			for (TActorIterator<APawn> It(Island); It; ++It)
				if (*It != Probe) Probe->GetCapsuleComponent()->IgnoreActorWhenMoving(*It, true);
			ARavenAgentAIController* Pilot = Island->SpawnActor<ARavenAgentAIController>(Spawn);
			Pilot->Possess(Probe);
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
			Pilot->BeginTakeoff(Probe->GetActorLocation() + FVector(-400, 0, 350));
			for (int32 I = 0; I < 300; ++I) Pilot->Tick(1.f / 60.f);
			TestTrue(*FString::Printf(TEXT("Actual %s departure"), *Tag.ToString()), Pilot->LocomotionState == ERavenLocomotionState::Flying && Probe->GetActorLocation().Z > Marker->GetActorLocation().Z + 300);
			Pilot->UnPossess();
			Pilot->Destroy();
			Probe->Destroy();
		}
	}
	return true;
}
