#include "RavenAgentAIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "IslandWeather.h"

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
	CruiseTarget = Destination;
	bApproachingPerch = false;
	MovementTarget = GetPawn()->GetActorLocation() + FVector(0.f, 0.f, TakeoffHeight);
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
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query)) return false;
	float HalfHeight = 45.f;
	if (const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn()))
		HalfHeight = RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	OutGroundLocation = Hit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
	return true;
}

void ARavenAgentAIController::BeginLanding(const FVector& DesiredLocation)
{
	if (!TraceGround(DesiredLocation, MovementTarget)) return;
	bHasMovementTarget = true;
	bTargetIsPerch = false;
	LocomotionState = ERavenLocomotionState::Landing;
	bApproachingPerch = false;
	SetFlyingMovement(true);
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
	AActor* BestPerch = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("RavenPerch"))) continue;
		const float Distance = FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation());
		if (Distance < BestDistance) { BestDistance = Distance; BestPerch = *It; }
	}
	return BeginPerchAt(BestPerch);
}

bool ARavenAgentAIController::RequestPerch(FName PerchTag)
{
	if (!GetPawn() || PerchTag.IsNone()) return false;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->ActorHasTag(PerchTag) && It->ActorHasTag(TEXT("RavenPerch"))) return BeginPerchAt(*It);
	return false;
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
	bHasMovementTarget = false;
	bTargetIsPerch = false;
	bApproachingPerch = false;
	LocomotionState = ERavenLocomotionState::Grounded;
	SetFlyingMovement(false);
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
		bHasMovementTarget = false;
		bTargetIsPerch = false;
		bApproachingPerch = false;
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
		if (LocomotionState == ERavenLocomotionState::TakingOff || LocomotionState == ERavenLocomotionState::Landing || LocomotionState == ERavenLocomotionState::Hopping)
		{
			LocomotionState = ERavenLocomotionState::Flying;
			SetFlyingMovement(true);
		}
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
			if (Choice < 0.18f) BeginLanding(GetPawn()->GetActorLocation());
			else if (Choice < 0.32f && BeginPerch()) {}
			else { MovementTarget = MakeCruiseTarget(); bHasMovementTarget = true; bApproachingPerch = bTargetIsPerch = false; }
		}
		return;
	}

	if (Decision.ActionType == EAgentActionType::MoveTo)
	{
		const FName TargetTag(*Decision.ActionTarget);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TargetTag)) continue;
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
			else { MovementTarget = Destination; bHasMovementTarget = true; bApproachingPerch = bTargetIsPerch = false; LocomotionState = ERavenLocomotionState::Flying; SetFlyingMovement(true); }
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
	if (LocomotionState == ERavenLocomotionState::TakingOff)
	{
		MovementTarget = CruiseTarget;
		bHasMovementTarget = true;
		LocomotionState = ERavenLocomotionState::Flying;
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
		else SetGrounded();
	}
	else if (LocomotionState == ERavenLocomotionState::Flying) ReportAction(TEXT("Reached the flight destination. No further movement is needed to arrive."));
}
