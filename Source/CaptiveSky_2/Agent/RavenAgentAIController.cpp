#include "RavenAgentAIController.h"
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
	HomeAltitude = InPawn ? InPawn->GetActorLocation().Z + 250.f : 0.f;
	if (ACharacter* RavenCharacter = Cast<ACharacter>(InPawn))
	{
		RavenCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		RavenCharacter->GetCharacterMovement()->GravityScale = 0.f;
	}
}

void ARavenAgentAIController::ActOnDecision(const FAgentDecision& Decision)
{
	APawn* Raven = GetPawn();
	if (!Raven)
	{
		return;
	}

	if (Decision.ActionType == EAgentActionType::Wander)
	{
		const FVector Origin = Raven->GetActorLocation();
		const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
		FlightTarget = FVector(Origin.X + Offset.X, Origin.Y + Offset.Y,
			FMath::Clamp(HomeAltitude + FMath::FRandRange(-VerticalRange, VerticalRange), HomeAltitude - 100.f, HomeAltitude + VerticalRange));
		bHasFlightTarget = true;
		return;
	}

	if (Decision.ActionType == EAgentActionType::MoveTo)
	{
		const FName TargetTag(*Decision.ActionTarget);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TargetTag))
			{
				FlightTarget = It->GetActorLocation() + FVector(0.f, 0.f, 180.f);
				bHasFlightTarget = true;
				return;
			}
		}
	}

	Super::ActOnDecision(Decision);
}

void ARavenAgentAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Raven = GetPawn();
	if (!Raven || !bHasFlightTarget)
	{
		return;
	}

	const FVector Current = Raven->GetActorLocation();
	const FVector Delta = FlightTarget - Current;
	if (Delta.SizeSquared() < FMath::Square(35.f))
	{
		bHasFlightTarget = false;
		return;
	}

	const FVector Direction = Delta.GetSafeNormal();
	FHitResult Hit;
	Raven->SetActorLocation(Current + Direction * FlightSpeed * DeltaSeconds, true, &Hit);
	Raven->SetActorRotation(Direction.Rotation());
	if (Hit.bBlockingHit)
	{
		bHasFlightTarget = false;
	}
}
