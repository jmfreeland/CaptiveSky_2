#include "RavenAgentAIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

ARavenAgentAIController::ARavenAgentAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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
		Movement->GravityScale = bFlying ? 0.f : 1.f;
		Movement->SetMovementMode(bFlying ? MOVE_Flying : MOVE_Walking);
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
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility)) return false;
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
	if (!BestPerch) return false;
	MovementTarget = BestPerch->GetActorLocation();
	bHasMovementTarget = true;
	bTargetIsPerch = true;
	LocomotionState = ERavenLocomotionState::Landing;
	SetFlyingMovement(true);
	return true;
}

void ARavenAgentAIController::SetGrounded()
{
	bHasMovementTarget = false;
	bTargetIsPerch = false;
	LocomotionState = ERavenLocomotionState::Grounded;
	SetFlyingMovement(false);
}

bool ARavenAgentAIController::AdvanceTowardsTarget(float DeltaSeconds)
{
	APawn* Raven = GetPawn();
	const FVector Delta = MovementTarget - Raven->GetActorLocation();
	if (Delta.SizeSquared() < FMath::Square(35.f)) return true;
	const FVector Direction = Delta.GetSafeNormal();
	FHitResult Hit;
	Raven->SetActorLocation(Raven->GetActorLocation() + Direction * FlightSpeed * DeltaSeconds, true, &Hit);
	Raven->SetActorRotation(Direction.Rotation());
	return Hit.bBlockingHit;
}

void ARavenAgentAIController::ActOnDecision(const FAgentDecision& Decision)
{
	if (!GetPawn()) return;

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
			else { MovementTarget = MakeCruiseTarget(); bHasMovementTarget = true; }
		}
		return;
	}

	if (Decision.ActionType == EAgentActionType::MoveTo)
	{
		const FName TargetTag(*Decision.ActionTarget);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TargetTag)) continue;
			const FVector Destination = It->GetActorLocation() + FVector(0.f, 0.f, 180.f);
			if (LocomotionState == ERavenLocomotionState::Grounded || LocomotionState == ERavenLocomotionState::Perched)
				BeginTakeoff(Destination);
			else { MovementTarget = Destination; bHasMovementTarget = true; LocomotionState = ERavenLocomotionState::Flying; }
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
	else if (LocomotionState == ERavenLocomotionState::Landing)
	{
		if (bTargetIsPerch)
		{
			LocomotionState = ERavenLocomotionState::Perched;
			SetFlyingMovement(true);
		}
		else SetGrounded();
	}
}
