#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "IslandForestStag.h"
#include "IslandLightning.h"
#include "IslandListeningStonesChime.h"
#include "IslandInteractionUtility.h"
#include "RavenAgentAIController.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandForestStagTest, "CaptiveSky2.Agent.WoodlandDeer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandForestStagTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Woodland ecology fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for woodland fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(FVector(0.f, 0.f, -2.f), FRotator::ZeroRotator, Spawn);
	if (Ground)
	{
		if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			Ground->GetStaticMeshComponent()->SetStaticMesh(Plane);
			Ground->SetActorScale3D(FVector(20.f, 20.f, 1.f));
		}
	}

	AIslandForestStag* Deer = World->SpawnActor<AIslandForestStag>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AActor* Observer = World->SpawnActor<AActor>(FVector(-300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("One bounded woodland stag can spawn"), Deer) ||
		!TestNotNull(TEXT("A resident can observe the stag"), Observer))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	TestTrue(TEXT("The imported stag mesh is assigned"), Deer->GetDeerMesh() && Deer->GetDeerMesh()->GetSkeletalMeshAsset());
	TestTrue(TEXT("The imported graze, walk, run, sleep, wake, and look-around animations resolve"),
		Deer->GrazeAnimation && Deer->WalkAnimation && Deer->RunAnimation && Deer->SleepAnimation &&
		Deer->WakeAnimation && Deer->LookAroundAnimation);
	if (USkeletalMesh* StagMesh = Deer->GetDeerMesh() ? Deer->GetDeerMesh()->GetSkeletalMeshAsset() : nullptr)
	{
		USkeleton* StagSkeleton = StagMesh->GetSkeleton();
		TestTrue(TEXT("Every assigned stag animation targets the mesh skeleton"), StagSkeleton &&
			Deer->GrazeAnimation->GetSkeleton() == StagSkeleton && Deer->WalkAnimation->GetSkeleton() == StagSkeleton &&
			Deer->RunAnimation->GetSkeleton() == StagSkeleton && Deer->SleepAnimation->GetSkeleton() == StagSkeleton &&
			Deer->WakeAnimation->GetSkeleton() == StagSkeleton && Deer->LookAroundAnimation->GetSkeleton() == StagSkeleton);
	}
	if (Deer->GrazeAnimation && Deer->WalkAnimation && Deer->RunAnimation && Deer->SleepAnimation &&
		Deer->WakeAnimation && Deer->LookAroundAnimation)
		TestFalse(TEXT("Stag response and locomotion animations have no root motion; movement stays actor-driven"),
			Deer->GrazeAnimation->HasRootMotion() || Deer->WalkAnimation->HasRootMotion() ||
			Deer->RunAnimation->HasRootMotion() || Deer->SleepAnimation->HasRootMotion() ||
			Deer->WakeAnimation->HasRootMotion() || Deer->LookAroundAnimation->HasRootMotion());
	TestTrue(TEXT("The visible mesh is nonblocking"), Deer->GetDeerMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	Deer->HomeLocation = Deer->GetActorLocation();
	TestTrue(TEXT("The stag is wild life, not a conscious agent or landmark"),
		Deer->ActorHasTag(TEXT("IslandLife")) && Deer->ActorHasTag(TEXT("WoodlandDeer")) &&
		!Deer->ActorHasTag(TEXT("IslandLandmark")) && !Deer->FindComponentByClass<UAgentBrainComponent>());
	TestEqual(TEXT("The interaction helper exposes the correct optional target"),
		IslandInteractionUtility::GetTargetTag(Deer), FName(TEXT("WoodlandDeer")));
	TestFalse(TEXT("Wild animals can be observed but are never movement destinations"),
		IslandInteractionUtility::IsMovementTargetAllowed(Deer));
	AActor* Landmark = World->SpawnActor<AActor>(FVector(900.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (Landmark) Landmark->Tags.Add(TEXT("IslandLandmark"));
	TestTrue(TEXT("Ordinary world destinations remain available"), IslandInteractionUtility::IsMovementTargetAllowed(Landmark));
	TestTrue(TEXT("A nearby clear observer can inspect without touching the animal"),
		IslandInteractionUtility::CanInspect(Observer, Deer, 400.f));

	AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	TestNotNull(TEXT("A short-lived Listening Stones chime can be heard by nearby wildlife"), Chime);
	if (Chime)
	{
		Chime->BeginChime();
		Deer->BeginGrazing();
		const FVector BeforeListening = Deer->GetActorLocation();
		Deer->CheckForNearbyListeningStonesChime();
		TestTrue(TEXT("A nearby fading stone tone starts one look-around response"), Deer->bListeningToChime);
		TestTrue(TEXT("The imported look-around animation is selected for the response"),
			Deer->LookAroundAnimation && Deer->GetDeerMesh()->GetSingleNodeInstance() &&
			Deer->GetDeerMesh()->GetSingleNodeInstance()->GetAnimationAsset() == Deer->LookAroundAnimation);
		TestTrue(TEXT("Listening does not move the wild stag from its bounded patch"),
			Deer->GetActorLocation().Equals(BeforeListening));
		TestTrue(TEXT("The chime response is temporary and rate-limited"),
			Deer->ListeningRemaining > 0.f && Deer->ListeningStonesCooldownRemaining == 10.f);

		Deer->CheckForNearbyListeningStonesChime();
		TestTrue(TEXT("One chime cannot restart the look-around while it is already playing"), Deer->bListeningToChime);
		Deer->Tick(Deer->ListeningRemaining + 0.1f);
		TestFalse(TEXT("The stag returns to its ordinary routine after looking around"), Deer->bListeningToChime);
		TestTrue(TEXT("The finite response leaves the stag at its original location"),
			Deer->GetActorLocation().Equals(BeforeListening));

		AIslandListeningStonesChime* DistantChime = World->SpawnActor<AIslandListeningStonesChime>(
			FVector(Chime->AudibleRadius + 50.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
		if (DistantChime)
		{
			DistantChime->BeginChime();
			Deer->ListeningStonesCooldownRemaining = 0.f;
			Deer->CheckForNearbyListeningStonesChime();
			TestFalse(TEXT("A chime outside its documented audible radius does not draw the stag's attention"),
				Deer->bListeningToChime);
		}
	}

	// Ambient wildlife notices a calm resident without turning proximity into an
	// interaction, movement, persistent memory, or additional model request.
	ACharacter* Resident = World->SpawnActor<ACharacter>(FVector(-300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	UAgentBrainComponent* ResidentBrain = Resident
		? NewObject<UAgentBrainComponent>(Resident, TEXT("AgentBrain")) : nullptr;
	if (ResidentBrain)
	{
		Resident->AddInstanceComponent(ResidentBrain);
	}
	TestNotNull(TEXT("The ambient visitor is a conscious resident"), ResidentBrain);
	Deer->BeginGrazing();
	if (Resident) Resident->SetActorLocation(Deer->GetActorLocation() + FVector(-300.f, 0.f, 0.f));
	Deer->ResidentPresenceCheckRemaining = 0.f;
	Deer->CheckForNearbyResident();
	TestTrue(TEXT("A nearby, visible, unhurried resident draws one brief look"), Deer->bNoticingResident);
	TestTrue(TEXT("Noticing remains local and does not move or startle the stag"),
		Deer->GetActorLocation().IsNearlyZero() && !Deer->bMoving && !Deer->bStartled);
	TestFalse(TEXT("Passive noticing never starts a model request"), ResidentBrain->bRequestInFlight);
	TestTrue(TEXT("The response is temporary and rate-limited"),
		Deer->ResidentNoticeRemaining > 0.f && Deer->ResidentPresenceCooldownRemaining == 8.f);
	const float NoticeRemaining = Deer->ResidentNoticeRemaining;
	Deer->CheckForNearbyResident();
	TestEqual(TEXT("One resident presence does not restart the same look"), Deer->ResidentNoticeRemaining, NoticeRemaining);
	Deer->Tick(NoticeRemaining + 0.1f);
	TestFalse(TEXT("The stag returns to grazing after its brief notice"), Deer->bNoticingResident || Deer->bMoving);
	Deer->CheckForNearbyResident();
	TestFalse(TEXT("A resident lingering nearby does not cause repeated looks"), Deer->bNoticingResident);
	if (Resident) Resident->SetActorLocation(Deer->GetActorLocation() + FVector(-1000.f, 0.f, 0.f));
	TestTrue(TEXT("The fixture resident has left the stag's rearm radius"),
		Resident && FVector::Dist2D(Resident->GetActorLocation(), Deer->GetActorLocation()) > 850.f);
	Deer->CheckForNearbyResident();
	TestFalse(TEXT("Presence rearms only after the resident leaves the wider local area"), Deer->bResidentPresenceNearby);
	if (Resident) Resident->SetActorLocation(Deer->GetActorLocation() + FVector(-300.f, 0.f, 0.f));
	Deer->CheckForNearbyResident();
	TestFalse(TEXT("The separate cooldown prevents an immediate return look"), Deer->bNoticingResident);
	Deer->ResidentPresenceCooldownRemaining = 0.f;
	Deer->CheckForNearbyResident();
	TestTrue(TEXT("A resident who returns after leaving can be noticed again"), Deer->bNoticingResident);
	Deer->BeginGrazing();
	if (ResidentBrain)
	{
		Resident->RemoveInstanceComponent(ResidentBrain);
		ResidentBrain->DestroyComponent();
	}
	if (Resident) Resident->Destroy();

	FString Fact;
	TestTrue(TEXT("Quiet observation is an implemented reversible interaction"),
		IslandInteractionUtility::Perform(Observer, Deer, Fact));
	TestTrue(TEXT("The animal briefly startles instead of becoming owned or following"),
		Deer->IsStartled() && !Deer->IsResting() && Fact.Contains(TEXT("nothing persistent changed")));
	TestTrue(TEXT("A startle response remains within the stag's 7 m home radius"),
		FVector::Dist2D(Deer->HomeLocation, Deer->TargetLocation) <= 700.f);
	Deer->SetResting(true);
	TestTrue(TEXT("Night resting is reversible"), Deer->IsResting());
	Deer->RespondToQuietObservation(Observer->GetActorLocation());
	TestFalse(TEXT("A resting wild animal is not forced into a new action"), Deer->IsStartled());
	Deer->SetResting(false);
	TestFalse(TEXT("Daylight returns the stag to its ordinary routine"), Deer->IsResting());
	TestTrue(TEXT("Waking plays the sleep-to-standing transition before grazing"), Deer->bWakingUp && Deer->WakeRemaining > 0.f);
	const float WakeDuration = Deer->WakeRemaining;
	Deer->Tick(WakeDuration * 0.5f);
	TestTrue(TEXT("The stag remains in its one-shot wake animation until it completes"), Deer->bWakingUp && Deer->WakeRemaining > 0.f);
	Deer->Tick(WakeDuration);
	TestFalse(TEXT("The stag returns to grazing after standing"), Deer->bWakingUp || Deer->IsResting());

	AIslandLightning* NearbyStrike = World->SpawnActor<AIslandLightning>(Spawn);
	if (!TestNotNull(TEXT("A nearby storm strike can be observed by local ecology"), NearbyStrike))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	const FVector NearbyGround(50000.f, 0.f, 0.f);
	NearbyStrike->Strike(NearbyGround, FVector(150000.f, 0.f, 0.f), 418);
	const float StagThunderDelay = FVector::Dist2D(NearbyGround, Deer->GetActorLocation()) / AIslandLightning::SoundSpeed;
	TestFalse(TEXT("The stag cannot hear sound before its own strike-to-listener travel time"),
		NearbyStrike->HasThunderReached(Deer->GetActorLocation()));
	TestTrue(TEXT("The distant player listener has a longer thunder delay than the nearby stag"),
		NearbyStrike->GetThunderDelay() > StagThunderDelay);
	NearbyStrike->Tick(StagThunderDelay - 0.1f);
	Deer->ThunderCheckRemaining = 0.f;
	Deer->Tick(0.36f);
	TestTrue(TEXT("A nearby stag does not startle before sound reaches its location"), !Deer->IsStartled());
	NearbyStrike->Tick(0.2f);
	TestTrue(TEXT("Thunder has reached the stag before it reaches the more distant player"),
		NearbyStrike->HasThunderReached(Deer->GetActorLocation()) && !NearbyStrike->HasThundered());
	Deer->BeginGrazing();
	Deer->ThunderCheckRemaining = 0.f;
	Deer->Tick(0.36f);
	TestTrue(TEXT("The stag briefly startles from the audible nearby thunder"), Deer->IsStartled());
	const FVector AwayFromStrike = (Deer->GetActorLocation() - NearbyStrike->GetStrikeGroundLocation()).GetSafeNormal2D();
	TestTrue(TEXT("The stag moves away from the nearby strike within its bounded home range"),
		FVector::DotProduct((Deer->TargetLocation - Deer->HomeLocation).GetSafeNormal2D(), AwayFromStrike) > 0.9f &&
		FVector::Dist2D(Deer->HomeLocation, Deer->TargetLocation) <= 700.f);
	const float StartleRemaining = Deer->ActivityRemaining;
	Deer->ThunderCheckRemaining = 0.f;
	Deer->Tick(0.36f);
	TestTrue(TEXT("One lingering thunder actor causes only one brief startle"), Deer->ActivityRemaining < StartleRemaining);

	Deer->BeginGrazing();
	AIslandLightning* DistantStrike = World->SpawnActor<AIslandLightning>(Spawn);
	if (TestNotNull(TEXT("A distant storm strike can be compared independently"), DistantStrike))
	{
		DistantStrike->Strike(FVector(150000.f, 0.f, 0.f), FVector::ZeroVector, 419);
		DistantStrike->Tick(DistantStrike->GetThunderDelay() + 0.1f);
		Deer->ThunderCheckRemaining = 0.f;
		Deer->Tick(0.36f);
		TestTrue(TEXT("Thunder beyond the local audible radius does not startle the stag"), !Deer->IsStartled());
	}
	Deer->SetResting(true);
	AIslandLightning* WakingStrike = World->SpawnActor<AIslandLightning>(Spawn);
	if (TestNotNull(TEXT("A nearby clap can wake a resting stag"), WakingStrike))
	{
		WakingStrike->Strike(FVector(60000.f, 0.f, 0.f), FVector::ZeroVector, 420);
		WakingStrike->Tick(WakingStrike->GetThunderDelay() + 0.1f);
		Deer->ThunderCheckRemaining = 0.f;
		Deer->Tick(0.36f);
		TestTrue(TEXT("An audible close thunderclap gently wakes and startles the stag"), !Deer->IsResting() && Deer->IsStartled());
	}

	ACharacter* RavenPawn = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	UAgentBrainComponent* RavenBrain = RavenPawn
		? NewObject<UAgentBrainComponent>(RavenPawn, TEXT("AgentBrain")) : nullptr;
	if (RavenBrain)
	{
		RavenPawn->AddInstanceComponent(RavenBrain);
		RavenBrain->RegisterComponent();
	}
	ARavenAgentAIController* RavenController = World->SpawnActor<ARavenAgentAIController>(Spawn);
	TestNotNull(TEXT("A raven pawn is available for the local flyby response"), RavenPawn);
	TestNotNull(TEXT("The Raven carries the same resident brain used by nearby-witness sensing"), RavenBrain);
	TestNotNull(TEXT("The raven's locomotion state can qualify a flyby"), RavenController);
	if (RavenPawn && RavenController)
	{
		RavenController->Possess(RavenPawn);
		RavenController->LocomotionState = ERavenLocomotionState::Flying;
		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-300.f, 0.f, 420.f));
		Deer->BeginGrazing();
		Deer->CheckForNearbyRavenFlyby();
		TestTrue(TEXT("A close, low raven pass briefly startles a grazing stag"), Deer->IsStartled());
		const FVector AwayFromRaven = (Deer->GetActorLocation() - RavenPawn->GetActorLocation()).GetSafeNormal2D();
		TestTrue(TEXT("The stag's short run moves away from the raven"),
			FVector::DotProduct((Deer->TargetLocation - Deer->GetActorLocation()).GetSafeNormal2D(), AwayFromRaven) > 0.9f);
		TestTrue(TEXT("The raven response stays within the stag's existing home radius"),
			FVector::Dist2D(Deer->HomeLocation, Deer->TargetLocation) <= 700.f);
		TestEqual(TEXT("One flyby starts a bounded twelve-second cooldown"), Deer->RavenFlybyCooldownRemaining, 12.f);

		Deer->BeginGrazing();
		Deer->RavenFlybyCooldownRemaining = 0.f;
		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-250.f, 0.f, 1100.f));
		Deer->CheckForNearbyRavenFlyby();
		TestFalse(TEXT("A high raven stays outside the stag's disturbance band"), Deer->IsStartled());

		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-900.f, 0.f, 420.f));
		Deer->CheckForNearbyRavenFlyby();
		TestFalse(TEXT("A distant low flight does not startle the stag"), Deer->IsStartled());

		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-250.f, 0.f, 420.f));
		RavenController->LocomotionState = ERavenLocomotionState::Perched;
		Deer->CheckForNearbyRavenFlyby();
		TestFalse(TEXT("A perched raven does not startle the stag"), Deer->IsStartled());
		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-250.f, 0.f, 120.f));
		Deer->BeginGrazing();
		Deer->bResidentPresenceNearby = false;
		Deer->ResidentPresenceCooldownRemaining = 0.f;
		Deer->ResidentPresenceCheckRemaining = 0.f;
		const FVector BeforeQuietRavenNotice = Deer->GetActorLocation();
		Deer->CheckForNearbyResident();
		TestTrue(TEXT("The quiet, nearby perched Raven earns the stag's brief look-around response"), Deer->bNoticingResident);
		TestTrue(TEXT("Quiet Raven presence does not startle, move, or leave the stag's patch"),
			!Deer->bStartled && !Deer->bMoving && Deer->GetActorLocation().Equals(BeforeQuietRavenNotice));
		TestTrue(TEXT("The perched Raven response uses the existing look-around animation"),
			Deer->GetDeerMesh()->GetSingleNodeInstance() &&
			Deer->GetDeerMesh()->GetSingleNodeInstance()->GetAnimationAsset() == Deer->LookAroundAnimation);
		TestTrue(TEXT("The stag exposes only its calm, temporary resident-notice state to companion sensing"),
			Deer->IsQuietlyNoticingResident());
		TestFalse(TEXT("Noticing a perched companion makes no model request"), RavenBrain && RavenBrain->bRequestInFlight);

		// The settled Raven returns one small look while the stag is quietly noticing it.
		// Existing sensory priorities and the already-visited chime must not be displaced.
		RavenController->LastNoticedListeningChime = Chime;
		RavenController->ListeningStoneAttentionRemaining = 0.f;
		RavenController->MinnowRippleAttentionRemaining = 0.f;
		RavenController->RainBasinAttentionRemaining = 0.f;
		RavenController->DewGlintAttentionRemaining = 0.f;
		RavenController->WindMoteAttentionRemaining = 0.f;
		RavenController->CrabScurryAttentionRemaining = 0.f;
		RavenController->ResidentAttentionRemaining = 0.f;
		RavenController->WildlifeAttentionRemaining = 0.f;
		RavenController->NoticedWildlifeInNearbyGroup.Reset();
		RavenController->ListeningStoneCheckRemaining = 0.f;
		const FVector RavenLocationBeforeLook = RavenPawn->GetActorLocation();
		RavenController->Tick(0.3f);
		TestTrue(TEXT("A settled Raven gives one brief directed glance to a stag quietly noticing it"),
			RavenController->IsShowingDirectedAttention() &&
			RavenController->WildlifeAttentionTarget.Get() == Deer);
		TestTrue(TEXT("The reciprocal glance does not move the Raven or start a model request"),
			RavenPawn->GetActorLocation().Equals(RavenLocationBeforeLook) && !RavenBrain->bRequestInFlight);
		RavenController->WildlifeAttentionRemaining = 0.f;
		RavenController->ListeningStoneAttentionRemaining = 0.f;
		RavenController->MinnowRippleAttentionRemaining = 0.f;
		RavenController->RainBasinAttentionRemaining = 0.f;
		RavenController->DewGlintAttentionRemaining = 0.f;
		RavenController->WindMoteAttentionRemaining = 0.f;
		RavenController->CrabScurryAttentionRemaining = 0.f;
		RavenController->ResidentAttentionRemaining = 0.f;
		RavenController->CheckForNearbyWildlifePresence();
		TestTrue(TEXT("The same stag cannot retrigger a glance while the pair remain together"),
			FMath::IsNearlyZero(RavenController->WildlifeAttentionRemaining));
		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-800.f, 0.f, 120.f));
		RavenController->CheckForNearbyWildlifePresence();
		TestTrue(TEXT("Leaving the 7 m group range clears the one-shot nearby-wildlife marker"),
			RavenController->NoticedWildlifeInNearbyGroup.IsEmpty());
		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-250.f, 0.f, 120.f));
		TestTrue(TEXT("The stag is still giving its brief calm look when the Raven returns"),
			Deer->IsQuietlyNoticingResident());
		TestTrue(TEXT("The Raven is still settled and no other cue owns its attention"),
			RavenController->LocomotionState == ERavenLocomotionState::Perched &&
			!RavenController->IsResting() && !RavenController->IsShowingDirectedAttention());
		RavenController->CheckForNearbyWildlifePresence();
		TestTrue(TEXT("Leaving the local group and returning rearms a full-duration wildlife glance"),
			FMath::IsNearlyEqual(RavenController->WildlifeAttentionRemaining, ARavenAgentAIController::WildlifeAttentionDuration));

		RavenController->LocomotionState = ERavenLocomotionState::Flying;
		Deer->SetResting(true);
		Deer->RavenFlybyCooldownRemaining = 0.f;
		Deer->CheckForNearbyRavenFlyby();
		TestTrue(TEXT("A resting stag remains settled during a low raven flyby"), Deer->IsResting() && !Deer->IsStartled());

		Deer->SetResting(false);
		Deer->BeginGrazing();
		Deer->RavenFlybyCooldownRemaining = 0.f;
		Deer->ThunderCheckRemaining = 100.f;
		Deer->RavenCheckRemaining = 0.f;
		RavenController->LocomotionState = ERavenLocomotionState::Flying;
		RavenPawn->SetActorLocation(Deer->GetActorLocation() + FVector(-250.f, 0.f, 420.f));
		Deer->Tick(0.36f);
		TestTrue(TEXT("Periodic stag sensing notices the same close low flyby"), Deer->IsStartled() && Deer->RavenFlybyCooldownRemaining > 0.f);
		RavenController->UnPossess();
	}
	if (RavenBrain)
	{
		RavenPawn->RemoveInstanceComponent(RavenBrain);
		RavenBrain->DestroyComponent();
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
