#include "RavenAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentRestPresentationComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "AgentMemoryComponent.h"
#include "HAL/PlatformTime.h"

ARavenAgentAIController::ARavenAgentAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

bool ARavenAgentAIController::IsActionInProgress() const
{
	return bHasMovementTarget || LocomotionState == ERavenLocomotionState::Hopping;
}
bool ARavenAgentAIController::CanRest() const
{
	return !bHasMovementTarget && (LocomotionState == ERavenLocomotionState::Perched || (LocomotionState == ERavenLocomotionState::Grounded && Super::CanRest()));
}

void ARavenAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (AAutonomousAgentCharacter* Agent = Cast<AAutonomousAgentCharacter>(InPawn))
		if (Agent->RestPresentation) Agent->RestPresentation->SetRestPosture(EAgentRestPosture::PerchedBird);
	HomeAltitude = InPawn ? InPawn->GetActorLocation().Z + TakeoffHeight : 0.f;
	SetGrounded();
}

void ARavenAgentAIController::SetFlyingMovement(bool bFlying) const
{
	if (const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn()))
	{
		UCharacterMovementComponent* Movement = RavenCharacter->GetCharacterMovement();
		Movement->StopMovementImmediately();
		Movement->GravityScale = bFlying ? 0.f : 1.f;
		Movement->SetMovementMode(bFlying ? MOVE_Flying : MOVE_Walking);
		FRotator Facing = RavenCharacter->GetActorRotation();
		Facing.Pitch = Facing.Roll = 0.f;
		GetPawn()->SetActorRotation(Facing);
	}
}

FVector ARavenAgentAIController::MakeCruiseTarget() const
{
	const FVector Origin = GetPawn()->GetActorLocation();
	const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
	return FVector(Origin.X + Offset.X, Origin.Y + Offset.Y,
		FMath::Clamp(HomeAltitude + FMath::FRandRange(-VerticalRange, VerticalRange), HomeAltitude - 100.f, HomeAltitude + VerticalRange));
}

void ARavenAgentAIController::BeginTakeoff(const FVector& Destination)
{
	APawn* Raven = GetPawn();
	if (!Raven) return;
	CruiseTarget = Destination;
	bApproachingPerch = false;
	FlightWaypoints.Reset();
	bHasTakeoffEscapeTarget = false;
	const FVector Origin = Raven->GetActorLocation();
	MovementTarget = Origin + FVector(0.f, 0.f, TakeoffHeight);
	const ACharacter* RavenCharacter = Cast<ACharacter>(Raven);
	const float Radius = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 30.f;
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenTakeoffExit), false, Raven);
	FHitResult Hit;
	const bool bVerticalExitBlocked = GetWorld() &&
		GetWorld()->SweepSingleByChannel(Hit, Origin, MovementTarget, FQuat::Identity, ECC_WorldStatic, Body, Query);
	if (bVerticalExitBlocked)
	{
		// A perch under a branch or roof may have no vertical exit. Search for a
		// collision-clear lateral move beneath the obstruction, then climb at its edge.
		const float SearchStep = FMath::Max(90.f, Radius + 50.f);
		FVector2D PreferredDirection(Destination.X - Origin.X, Destination.Y - Origin.Y);
		PreferredDirection = PreferredDirection.GetSafeNormal();
		if (PreferredDirection.IsNearlyZero()) PreferredDirection = FVector2D(1.f, 0.f);
		const float PreferredAngle = FMath::Atan2(PreferredDirection.Y, PreferredDirection.X);
		static constexpr int32 AngleOffsetsDegrees[] = { 0, 45, -45, 90, -90, 135, -135, 180 };
		for (float Distance = SearchStep; Distance <= 700.f && !bHasTakeoffEscapeTarget; Distance += SearchStep)
		{
			for (const int32 OffsetDegrees : AngleOffsetsDegrees)
			{
				const float Angle = PreferredAngle + FMath::DegreesToRadians(static_cast<float>(OffsetDegrees));
				const FVector Side = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;
				const FVector SideUp = Side + FVector(0.f, 0.f, TakeoffHeight);
				if (GetWorld()->SweepSingleByChannel(Hit, Origin, Side, FQuat::Identity, ECC_WorldStatic, Body, Query)) continue;
				if (GetWorld()->SweepSingleByChannel(Hit, Side, SideUp, FQuat::Identity, ECC_WorldStatic, Body, Query)) continue;
				MovementTarget = Side;
				TakeoffEscapeTarget = SideUp;
				bHasTakeoffEscapeTarget = true;
				break;
			}
		}
	}
	bHasMovementTarget = true;
	bTargetIsPerch = false;
	LocomotionState = ERavenLocomotionState::TakingOff;
	SetFlyingMovement(true);
}

bool ARavenAgentAIController::TraceGround(const FVector& DesiredLocation, FVector& OutGroundLocation) const
{
	FHitResult Hit;
	const FVector Start(DesiredLocation.X, DesiredLocation.Y, DesiredLocation.Z + 1000.f);
	const FVector End(DesiredLocation.X, DesiredLocation.Y, DesiredLocation.Z - 5000.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenGround), false, GetPawn());
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query) || Hit.ImpactNormal.Z < 0.7f) return false;
	float HalfHeight = 45.f;
	if (const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn()))
		HalfHeight = RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	OutGroundLocation = Hit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
	return true;
}

bool ARavenAgentAIController::BeginLanding(const FVector& DesiredLocation)
{
	if (!TraceGround(DesiredLocation, MovementTarget)) return false;
	bHasMovementTarget = true;
	bTargetIsPerch = false;
	LocomotionState = ERavenLocomotionState::Landing;
	bApproachingPerch = false;
	SetFlyingMovement(true);
	return true;
}

void ARavenAgentAIController::BeginGroundLandingAt(FName SiteTag)
{
	if (LocomotionState == ERavenLocomotionState::Grounded)
	{
		ReportAction(TEXT("Already grounded; you may gather fallen twigs here if you choose."));
		return;
	}
	if (bHasMovementTarget || LocomotionState == ERavenLocomotionState::TakingOff || LocomotionState == ERavenLocomotionState::Landing || LocomotionState == ERavenLocomotionState::Hopping)
	{
		ReportAction(TEXT("Finish the current movement before choosing a landing site."));
		return;
	}
	const UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!WorldState || !SiteTag.ToString().StartsWith(TEXT("ArrangingGround_")))
	{
		ReportAction(TEXT("Landing needs an exact listed ArrangingGround target; no lasting change occurred."));
		return;
	}
	const FIslandArrangementSite* Site = nullptr;
	int32 VisibleSites = 0;
	for (const FIslandArrangementSite& Candidate : WorldState->GetArrangementSites())
	{
		if (VisibleSites >= 3) break;
		const FVector View = Candidate.Location + FVector(0.f, 0.f, 30.f);
		if (FVector::DistSquared(GetPawn()->GetActorLocation(), View) > FMath::Square(1200.f)) continue;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenLandingSiteVisibility), false, GetPawn());
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetPawn()->GetActorLocation(), View, ECC_Visibility, Query)) continue;
		++VisibleSites;
		if (Candidate.Id == SiteTag) { Site = &Candidate; break; }
	}
	if (!Site)
	{
		ReportAction(TEXT("That exact open-ground target is not currently visible. Choose one of the listed sites."));
		return;
	}

	ArrangementLandingTarget = Site->Location;
	bLandingAtArrangementSite = true;
	const FVector FlightTarget = ArrangementLandingTarget + FVector(0.f, 0.f, 180.f);
	if (LocomotionState == ERavenLocomotionState::Perched)
	{
		BeginTakeoff(FlightTarget);
	}
	else
	{
		FlightWaypoints.Reset();
		CruiseTarget = FlightTarget;
		MovementTarget = PlanFlightLeg(GetPawn()->GetActorLocation(), FlightTarget);
		bHasMovementTarget = true;
		bApproachingPerch = false;
		bTargetIsPerch = false;
		LocomotionState = ERavenLocomotionState::Flying;
		SetFlyingMovement(true);
	}
	ReportAction(TEXT("Flight to the listed open-ground site started; the descent will be confirmed at arrival."));
}

void ARavenAgentAIController::BeginHop()
{
	APawn* Raven = GetPawn();
	HopStart = Raven->GetActorLocation();
	const FVector2D Offset = FMath::RandPointInCircle(HopDistance);
	FVector Desired = HopStart + FVector(Offset.X, Offset.Y, 0.f);
	if (!TraceGround(Desired, HopEnd)) HopEnd = Desired;
	HopElapsed = 0.f;
	LocomotionState = ERavenLocomotionState::Hopping;
	SetFlyingMovement(true);
}

bool ARavenAgentAIController::BeginPerch()
{
	TArray<AActor*> CandidatePerches;
	TArray<FVector> CandidateLocations;
	TArray<float> CandidateWindSpeeds;
	AIslandWeather* Weather = nullptr;
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("RavenPerch")) || !HasSuitablePerchSupport(*It)) continue;
		CandidatePerches.Add(*It);
		CandidateLocations.Add(It->GetActorLocation());
		CandidateWindSpeeds.Add(Weather ? Weather->GetLocalWind(It->GetActorLocation(), GetPawn()).Size() : 0.f);
	}
	const float CurrentWindSpeed = Weather ? Weather->GetLocalWind(GetPawn()->GetActorLocation(), GetPawn()).Size() : 0.f;
	const int32 PreferredIndex = SelectWindAwarePerch(GetPawn()->GetActorLocation(), CurrentWindSpeed,
		CandidateLocations, CandidateWindSpeeds);
	AActor* BestPerch = CandidatePerches.IsValidIndex(PreferredIndex) ? CandidatePerches[PreferredIndex] : nullptr;
	return BeginPerchAt(BestPerch);
}

int32 ARavenAgentAIController::SelectWindAwarePerch(const FVector& Origin, float CurrentWindSpeed,
	const TArray<FVector>& PerchLocations, const TArray<float>& PerchWindSpeeds)
{
	if (PerchLocations.IsEmpty() || PerchLocations.Num() != PerchWindSpeeds.Num()) return INDEX_NONE;

	int32 NearestIndex = INDEX_NONE;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < PerchLocations.Num(); ++Index)
	{
		const float DistanceSquared = FVector::DistSquared(Origin, PerchLocations[Index]);
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestIndex = Index;
		}
	}

	// In light air, preserve the ordinary nearest-roost choice. In stronger wind,
	// trade travel distance against the wind measured at each physically supported site.
	static constexpr float StrongWindThreshold = 85.f;
	static constexpr float TravelCostPerCentimeter = 0.008f;
	static constexpr float MinimumUsefulShelterGain = 15.f;
	if (CurrentWindSpeed < StrongWindThreshold) return NearestIndex;

	int32 CalmestIndex = NearestIndex;
	float NearestScore = PerchWindSpeeds[NearestIndex] + FMath::Sqrt(NearestDistanceSquared) * TravelCostPerCentimeter;
	float CalmestScore = NearestScore;
	for (int32 Index = 0; Index < PerchLocations.Num(); ++Index)
	{
		const float Distance = FVector::Distance(Origin, PerchLocations[Index]);
		const float Score = PerchWindSpeeds[Index] + Distance * TravelCostPerCentimeter;
		if (Score < CalmestScore)
		{
			CalmestScore = Score;
			CalmestIndex = Index;
		}
	}
	return CalmestScore + MinimumUsefulShelterGain < NearestScore ? CalmestIndex : NearestIndex;
}

bool ARavenAgentAIController::HasSuitablePerchSupport(const AActor* Site, FHitResult* OutSupport) const
{
	if (!Site || !GetWorld() || !GetPawn()) return false;
	const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn());
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenRoostAssessment), false, GetPawn());
	Query.AddIgnoredActor(Site);
	FHitResult Support;
	const FVector SiteLocation = Site->GetActorLocation();
	const bool bSupportHit = GetWorld()->LineTraceSingleByChannel(Support, SiteLocation,
		SiteLocation - FVector(0.f, 0.f, HalfHeight + 12.f), ECC_Visibility, Query);
	if (OutSupport) *OutSupport = Support;
	return bSupportHit && Support.ImpactNormal.Z >= 0.5f;
}

bool ARavenAgentAIController::RequestPerch(FName PerchTag)
{
	if (!GetPawn() || PerchTag.IsNone()) return false;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->ActorHasTag(PerchTag) && It->ActorHasTag(TEXT("RavenPerch"))) return BeginPerchAt(*It);
	return false;
}

FString ARavenAgentAIController::AssessRoostSite(const AActor* Site) const
{
	if (!Site || !GetWorld() || !GetPawn()) return TEXT("Roost conditions cannot be assessed without a visible site and embodied raven.");
	const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn());
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	const float CapsuleRadius = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 30.f;
	const FVector SiteLocation = Site->GetActorLocation();

	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenRoostAssessment), false, GetPawn());
	Query.AddIgnoredActor(Site);
	FHitResult Support;
	const bool bHasSuitableSupport = HasSuitablePerchSupport(Site, &Support);

	const float ProbeSpread = FMath::Min(CapsuleRadius * 0.65f, 30.f);
	const FVector HeadHeight = SiteLocation + FVector(0.f, 0.f, HalfHeight + 5.f);
	const FVector ProbeOffsets[] = {
		FVector::ZeroVector,
		FVector(ProbeSpread, 0.f, 0.f), FVector(-ProbeSpread, 0.f, 0.f),
		FVector(0.f, ProbeSpread, 0.f), FVector(0.f, -ProbeSpread, 0.f)
	};
	int32 OverheadBlockCount = 0;
	for (const FVector& Offset : ProbeOffsets)
	{
		FHitResult Overhead;
		if (GetWorld()->LineTraceSingleByChannel(Overhead, HeadHeight + Offset,
			HeadHeight + Offset + FVector(0.f, 0.f, 300.f), ECC_Visibility, Query))
			++OverheadBlockCount;
	}

	return FString::Printf(TEXT("Read-only site check: %s. %d of 5 short vertical visibility probes above the raven's head found solid overhead geometry; this is only a local rain-cover clue, not proof of waterproof shelter. The probe does not establish branch strength, nest suitability, ownership, or a home. A physical perch approach must still confirm upward-facing support at arrival."),
		bHasSuitableSupport ? TEXT("an upward-facing support surface is currently beneath the marker") : TEXT("suitable upward-facing support was not confirmed beneath the marker"),
		OverheadBlockCount);
}

AActor* ARavenAgentAIController::FindPerchedNestSite() const
{
	if (LocomotionState != ERavenLocomotionState::Perched || !GetPawn()) return nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->ActorHasTag(TEXT("RavenNestSite")) && It->Tags.Num() > 0 &&
			FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation()) < FMath::Square(30.f))
			return *It;
	return nullptr;
}

FString ARavenAgentAIController::DescribeBuildOptions() const
{
	if (!GetPawn() || !GetWorld()) return FString();
	if (LocomotionState == ERavenLocomotionState::Grounded && !bCarryingTwigs)
		return TEXT(" Fallen twigs lie on the ground around you; you may gather a small bundle in your beak (build target: GatherTwigs). Carrying them does not oblige you to build anything.");
	FString Result = bCarryingTwigs ? TEXT(" You are carrying a small bundle of fallen twigs.") : FString();
	const AActor* Site = FindPerchedNestSite();
	if (!Site) return Result;
	const UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
	const FIslandNestRecord* Nest = WorldState ? WorldState->FindNest(Site->Tags[0]) : nullptr;
	if (!WorldState) return Result;
	if (Nest && Nest->Layers >= UIslandWorldStateSubsystem::MaxNestLayers)
		return Result + TEXT(" The nest at this roost is complete; there is no room to weave in more.");
	if (const double* Until = WovenUntil.Find(Site->Tags[0]); Until && *Until > FPlatformTime::Seconds())
		return Result + TEXT(" The layer you just wove here is still settling; more weaving is not possible yet.");
	if (!bCarryingTwigs)
		return Result + TEXT(" To weave a nest here you would first need twigs gathered from the ground. None lie up on this perch. If you wish to forage, use land with a listed ArrangingGround target to fly to that open-ground site and descend; after landing, build with GatherTwigs. You can then move_to a listed roost and weave. This is optional.");
	return Result + FString::Printf(TEXT(" While perched here you may weave them into %s (build target: %s). This is a small lasting change that stays after this session."),
		Nest ? TEXT("the nest at this roost") : TEXT("the start of a nest"), *Site->Tags[0].ToString());
}

void ARavenAgentAIController::Build(FName Target)
{
	if (Target == FName(TEXT("GatherTwigs")))
	{
		if (LocomotionState != ERavenLocomotionState::Grounded) { ReportAction(TEXT("Twigs can only be gathered while standing on the ground. Nothing was gathered.")); return; }
		if (bCarryingTwigs) { ReportAction(TEXT("You are already carrying a bundle of twigs; there is no room in your beak for more.")); return; }
		bCarryingTwigs = true;
		ReportAction(TEXT("GatherTwigs: You picked up a small bundle of fallen twigs in your beak. Nothing else was found, and nothing has been built yet."));
		return;
	}
	AActor* Site = FindPerchedNestSite();
	if (!Site || !Site->ActorHasTag(Target))
	{
		ReportAction(TEXT("Weaving is only possible while perched at a roost site offered as a build target; move_to that roost first. Nothing changed."));
		return;
	}
	if (!bCarryingTwigs) { ReportAction(TEXT("You have no twigs to weave; gather some from the ground first. Nothing changed.")); return; }
	if (const double* Until = WovenUntil.Find(Target); Until && *Until > FPlatformTime::Seconds())
	{
		ReportAction(TEXT("The last layer here is still settling; weaving again is not possible yet. Nothing changed."));
		return;
	}
	UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!WorldState) { ReportAction(TEXT("Nothing lasting can be built in this world. Nothing changed.")); return; }
	if (const FIslandNestRecord* Existing = WorldState->FindNest(Target); Existing && Existing->Layers >= UIslandWorldStateSubsystem::MaxNestLayers)
	{
		ReportAction(TEXT("The nest here is already complete; there is no room to weave in more. You are still carrying your twigs."));
		return;
	}
	// The woven material rests on the support beneath the perched body, not on the marker in the air.
	const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn());
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	FVector SupportLocation = GetPawn()->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
	FHitResult Support;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenNestSupport), false, GetPawn());
	if (GetWorld()->LineTraceSingleByChannel(Support, GetPawn()->GetActorLocation(), SupportLocation - FVector(0.f, 0.f, 12.f), ECC_Visibility, Query))
		SupportLocation = Support.ImpactPoint;
	const UAgentMemoryComponent* Memory = GetPawn()->FindComponentByClass<UAgentMemoryComponent>();
	const int32 NewLayers = WorldState->AddNestLayer(Target, SupportLocation, Memory ? Memory->GetResolvedAgentId() : GetPawn()->GetName());
	if (NewLayers == 0) { ReportAction(TEXT("Weaving failed: the change could not be kept, so nothing lasting occurred. You are still carrying your twigs.")); return; }
	bCarryingTwigs = false;
	WovenUntil.Add(Target, FPlatformTime::Seconds() + 240);
	const FString Fact = FString::Printf(TEXT("%s: You wove your twigs into %s; it now has %d of %d layers. This change stays in the world after this session. It is a nest you made, not an assigned home, and it does not change how you rest."),
		*Target.ToString(), NewLayers == 1 ? TEXT("the first ring of a new nest") : TEXT("the nest"), NewLayers, UIslandWorldStateSubsystem::MaxNestLayers);
	ReportAction(Fact);
	if (UAgentMemoryComponent* Writable = GetPawn()->FindComponentByClass<UAgentMemoryComponent>())
		Writable->AppendMemory(Writable->MakeMemory(EAgentMemoryType::Observation, Fact, 0.6f, {TEXT("action-result"), TEXT("nest"), Target.ToString()}));
}

bool ARavenAgentAIController::BeginPerchAt(AActor* Perch)
{
	if (!Perch || !GetPawn()) return false;
	if (LocomotionState == ERavenLocomotionState::Perched && FVector::DistSquared(GetPawn()->GetActorLocation(), Perch->GetActorLocation()) < FMath::Square(15.f))
	{
		ReportAction(TEXT("Already perched at this site. Arrival is complete; you can rest, inspect once, or depart."));
		return true;
	}
	PerchTarget = Perch->GetActorLocation();
	// Rise vertically, cross above the landing point, then descend. This is a
	// simple approach, not obstacle pathfinding; a blocked segment safely aborts.
	const float ApproachZ = FMath::Max(GetPawn()->GetActorLocation().Z, PerchTarget.Z) + FMath::Max(100.f, TakeoffHeight);
	CruiseTarget = FVector(PerchTarget.X, PerchTarget.Y, ApproachZ);
	BeginTakeoff(CruiseTarget);
	MovementTarget.Z = ApproachZ;
	bApproachingPerch = true;
	bHasMovementTarget = true;
	bTargetIsPerch = true;
	ReportAction(TEXT("Roost approach started; arrival is not yet complete."));
	return true;
}

void ARavenAgentAIController::SetGrounded()
{
	FlightWaypoints.Reset();
	bHasTakeoffEscapeTarget = false;
	bHasMovementTarget = false;
	bTargetIsPerch = false;
	bApproachingPerch = false;
	bLandingAtArrangementSite = false;
	LocomotionState = ERavenLocomotionState::Grounded;
	SetFlyingMovement(false);
}

FVector ARavenAgentAIController::PlanFlightLeg(const FVector& From, const FVector& To)
{
	FlightWaypoints.Reset();
	APawn* Raven = GetPawn();
	if (!Raven || !GetWorld()) return To;
	const ACharacter* RavenCharacter = Cast<ACharacter>(Raven);
	const float Radius = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 30.f;
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenFlightPlan), false, Raven);
	FHitResult Hit;
	if (!GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_WorldStatic, Body, Query)) return To;

	// Something is in the way: find the highest solid surface along the route and cross above it.
	float Highest = FMath::Max(From.Z, To.Z);
	const int32 Samples = FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(From, To) / 250.f), 2, 64);
	for (int32 Index = 0; Index <= Samples; ++Index)
	{
		const FVector Probe = FMath::Lerp(From, To, Index / static_cast<float>(Samples));
		FHitResult Top;
		if (GetWorld()->LineTraceSingleByChannel(Top, FVector(Probe.X, Probe.Y, Highest + 20000.f), FVector(Probe.X, Probe.Y, Probe.Z - 1000.f), ECC_WorldStatic, Query))
			Highest = FMath::Max(Highest, static_cast<float>(Top.ImpactPoint.Z));
	}
	const float Cruise = Highest + HalfHeight + 250.f;
	const FVector Up(From.X, From.Y, FMath::Max(Cruise, From.Z));
	const FVector Over(To.X, To.Y, Up.Z);
	// If even the climb is blocked (a roof overhead), fly as before and let the obstruction be reported.
	if (GetWorld()->SweepSingleByChannel(Hit, From, Up, FQuat::Identity, ECC_WorldStatic, Body, Query)) return To;
	FlightWaypoints = { Over, To };
	return Up;
}

bool ARavenAgentAIController::AdvanceTowardsTarget(float DeltaSeconds)
{
	APawn* Raven = GetPawn();
	const FVector Delta = MovementTarget - Raven->GetActorLocation();
	const float ArrivalRadius = LocomotionState == ERavenLocomotionState::Landing ? 2.f : 15.f;
	if (Delta.SizeSquared() < FMath::Square(ArrivalRadius)) return true;
	const FVector Direction = Delta.GetSafeNormal();
	FHitResult Hit;
	FVector Wind = FVector::ZeroVector;
	if (LocomotionState == ERavenLocomotionState::Flying)
	{
		for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
		{
			Wind = It->GetLocalWind(Raven->GetActorLocation(), Raven);
			break;
		}
	}
	// Fade drift near arrival and clamp steps so low frame rates cannot overshoot a target.
	const FVector Velocity = Direction * FMath::Max(0.f, FlightSpeed) + Wind * FMath::Clamp(Delta.Size() / 300.f, 0.f, 1.f);
	const FVector Step = (Velocity * FMath::Max(0.f, DeltaSeconds)).GetClampedToMaxSize(Delta.Size());
	Raven->SetActorLocation(Raven->GetActorLocation() + Step, true, &Hit);
	if (!Direction.IsNearlyZero()) Raven->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	if (Hit.bBlockingHit)
	{
		// Obstruction is not a successful landing/perch. Stop and allow another decision.
		FlightWaypoints.Reset();
		bHasTakeoffEscapeTarget = false;
		bHasMovementTarget = false;
		bTargetIsPerch = false;
		bApproachingPerch = false;
		bLandingAtArrangementSite = false;
		LocomotionState = ERavenLocomotionState::Flying;
		ReportAction(TEXT("Flight was blocked by geometry; this is not a successful arrival. Choose a different approach."));
		return false;
	}
	return FVector::DistSquared(Raven->GetActorLocation(), MovementTarget) < FMath::Square(ArrivalRadius);
}

void ARavenAgentAIController::ActOnDecision(const FAgentDecision& Decision)
{
	if (!GetPawn()) return;
	if (IsResting()) return;
	if (Decision.ActionType == EAgentActionType::Idle)
	{
		bHasMovementTarget = bApproachingPerch = bTargetIsPerch = false;
		bLandingAtArrangementSite = false;
		bHasTakeoffEscapeTarget = false;
		FlightWaypoints.Reset();
		if (LocomotionState == ERavenLocomotionState::TakingOff || LocomotionState == ERavenLocomotionState::Landing || LocomotionState == ERavenLocomotionState::Hopping)
		{
			LocomotionState = ERavenLocomotionState::Flying;
			SetFlyingMovement(true);
		}
	}
	if (Decision.ActionType == EAgentActionType::Land)
	{
		BeginGroundLandingAt(FName(*Decision.ActionTarget));
		return;
	}

	if (Decision.ActionType == EAgentActionType::Build && !Decision.ActionTarget.StartsWith(TEXT("ArrangingGround")))
	{
		if (Decision.ActionTarget == TEXT("GuestBook"))
		{
			Super::ActOnDecision(Decision);
			return;
		}
		Build(FName(*Decision.ActionTarget));
		return;
	}

	if (Decision.ActionType == EAgentActionType::Wander)
	{
		if (LocomotionState == ERavenLocomotionState::Grounded)
		{
			if (FMath::FRand() < 0.4f) BeginHop(); else BeginTakeoff(MakeCruiseTarget());
		}
		else if (LocomotionState == ERavenLocomotionState::Perched)
		{
			BeginTakeoff(MakeCruiseTarget());
		}
		else if (LocomotionState == ERavenLocomotionState::Flying)
		{
			const float Choice = FMath::FRand();
			if (Choice < 0.18f)
			{
				if (!BeginLanding(GetPawn()->GetActorLocation())) ReportAction(TEXT("No safe ground was found below; choose another flight or landing site."));
			}
			else if (Choice < 0.32f && BeginPerch()) {}
			else { MovementTarget = PlanFlightLeg(GetPawn()->GetActorLocation(), MakeCruiseTarget()); bHasMovementTarget = true; bApproachingPerch = bTargetIsPerch = false; }
		}
		return;
	}

	if (Decision.ActionType == EAgentActionType::MoveTo)
	{
		const FName TargetTag(*Decision.ActionTarget);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TargetTag)) continue;
			if (AAutonomousAgentCharacter* OtherResident = Cast<AAutonomousAgentCharacter>(*It); OtherResident && OtherResident != GetPawn())
			{
				const FVector RavenLocation = GetPawn()->GetActorLocation();
				if (FVector::Dist2D(RavenLocation, OtherResident->GetActorLocation()) <= ResidentApproachStandOffDistance)
				{
					ReportAction(TEXT("Already near the other resident. Speaking remains optional; perch, observe, or choose another activity."));
					return;
				}
				const FVector Destination = BuildResidentApproachPoint(RavenLocation, OtherResident->GetActorLocation());
				ReportAction(TEXT("Flight toward the other resident's conversational space started; arriving does not begin a conversation."));
				if (LocomotionState == ERavenLocomotionState::Grounded || LocomotionState == ERavenLocomotionState::Perched)
				{
					BeginTakeoff(Destination);
				}
				else
				{
					MovementTarget = PlanFlightLeg(RavenLocation, Destination);
					bHasMovementTarget = true;
					bApproachingPerch = bTargetIsPerch = false;
					LocomotionState = ERavenLocomotionState::Flying;
					SetFlyingMovement(true);
				}
				return;
			}
			if (It->ActorHasTag(TEXT("RavenPerch")))
			{
				BeginPerchAt(*It);
				return;
			}
			const FVector Destination = It->GetActorLocation() + FVector(0.f, 0.f, 180.f);
			if (FVector::DistSquared(GetPawn()->GetActorLocation(), Destination) < FMath::Square(35.f)) { ReportAction(TEXT("Already at this landmark. Movement is complete; inspect once, wait, or choose a different destination.")); return; }
			ReportAction(TEXT("Flight to the landmark started; arrival is not yet complete."));
			if (LocomotionState == ERavenLocomotionState::Grounded || LocomotionState == ERavenLocomotionState::Perched)
				BeginTakeoff(Destination);
			else { MovementTarget = PlanFlightLeg(GetPawn()->GetActorLocation(), Destination); bHasMovementTarget = true; bApproachingPerch = bTargetIsPerch = false; LocomotionState = ERavenLocomotionState::Flying; SetFlyingMovement(true); }
			return;
		}
	}

	Super::ActOnDecision(Decision);
}

void ARavenAgentAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Raven = GetPawn();
	if (!Raven) return;
	if (IsResting()) return;

	if (LocomotionState == ERavenLocomotionState::Hopping)
	{
		HopElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(HopElapsed / HopDuration, 0.f, 1.f);
		FVector Position = FMath::Lerp(HopStart, HopEnd, Alpha);
		Position.Z += FMath::Sin(Alpha * PI) * HopHeight;
		Raven->SetActorLocation(Position, true);
		if (Alpha >= 1.f) SetGrounded();
		return;
	}

	if (!bHasMovementTarget) return;
	if (!AdvanceTowardsTarget(DeltaSeconds)) return;

	bHasMovementTarget = false;
	// A planned flight continues leg by leg before any arrival handling.
	if (LocomotionState == ERavenLocomotionState::Flying && FlightWaypoints.Num() > 0)
	{
		MovementTarget = FlightWaypoints[0];
		FlightWaypoints.RemoveAt(0);
		bHasMovementTarget = true;
		return;
	}
	if (LocomotionState == ERavenLocomotionState::TakingOff)
	{
		if (bHasTakeoffEscapeTarget)
		{
			MovementTarget = TakeoffEscapeTarget;
			bHasTakeoffEscapeTarget = false;
		}
		else
		{
			LocomotionState = ERavenLocomotionState::Flying;
			MovementTarget = PlanFlightLeg(Raven->GetActorLocation(), CruiseTarget);
		}
		bHasMovementTarget = true;
	}
	else if (LocomotionState == ERavenLocomotionState::Flying && bApproachingPerch)
	{
		MovementTarget = PerchTarget;
		bHasMovementTarget = true;
		bApproachingPerch = false;
		LocomotionState = ERavenLocomotionState::Landing;
	}
	else if (LocomotionState == ERavenLocomotionState::Landing)
	{
		if (bTargetIsPerch)
		{
			// A marker in empty air is not a perch. Verify close support below.
			const ACharacter* PerchingCharacter = Cast<ACharacter>(Raven);
			const float HalfHeight = PerchingCharacter ? PerchingCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
			FHitResult Support;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenPerchSupport), false, Raven);
			if (!GetWorld()->LineTraceSingleByChannel(Support, Raven->GetActorLocation(), Raven->GetActorLocation() - FVector(0.f, 0.f, HalfHeight + 12.f), ECC_Visibility, Query) || Support.ImpactNormal.Z < 0.5f)
			{
				bTargetIsPerch = false;
				LocomotionState = ERavenLocomotionState::Flying;
				ReportAction(TEXT("Perch rejected: no suitable support below the landing point."));
				return;
			}
			LocomotionState = ERavenLocomotionState::Perched;
			SetFlyingMovement(true);
			ReportAction(TEXT("Landed and perched on solid support. Arrival is complete; you may rest here or choose to depart."));
		}
		else
		{
			const bool bLandedAtArrangementSite = bLandingAtArrangementSite;
			SetGrounded();
			if (bLandedAtArrangementSite)
				ReportAction(TEXT("Landed on the verified open-ground site. You are now grounded; gathering twigs is available if you choose."));
		}
	}
	else if (LocomotionState == ERavenLocomotionState::Flying && bLandingAtArrangementSite)
	{
		if (!BeginLanding(ArrangementLandingTarget))
		{
			bLandingAtArrangementSite = false;
			ReportAction(TEXT("The open-ground site had no safe landing surface on arrival. You remain in flight; choose another visible site."));
		}
		else ReportAction(TEXT("Reached the open-ground site; descending to the surface now."));
	}
	else if (LocomotionState == ERavenLocomotionState::Flying) ReportAction(TEXT("Reached the flight destination. No further movement is needed to arrive."));
}
