#include "Misc/AutomationTest.h"
#include "RavenAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentRestPresentationComponent.h"
#include "AgentSocialComponent.h"
#include "IslandInnkeeperSubsystem.h"
#include "IslandWeather.h"
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
