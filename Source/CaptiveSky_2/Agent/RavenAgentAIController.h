#pragma once

#include "CoreMinimal.h"
#include "AutonomousAgentAIController.h"
#include "RavenAgentAIController.generated.h"

UENUM(BlueprintType)
enum class ERavenLocomotionState : uint8
{
	Grounded,
	Hopping,
	TakingOff,
	Flying,
	Landing,
	Perched
};

/** Asset-independent raven locomotion; animation assets can read LocomotionState later. */
UCLASS()
class CAPTIVESKY_2_API ARavenAgentAIController : public AAutonomousAgentAIController
{
	GENERATED_BODY()

public:
	ARavenAgentAIController();

	UPROPERTY(BlueprintReadOnly, Category = "Raven|Locomotion")
	ERavenLocomotionState LocomotionState = ERavenLocomotionState::Grounded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float FlightSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float VerticalRange = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float TakeoffHeight = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float HopDistance = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float HopHeight = 55.f;

	/** Tagged markers specify the centre of the upright raven capsule at rest. */
	UFUNCTION(BlueprintCallable, Category = "Raven|Locomotion")
	bool RequestPerch(FName PerchTag);

	/** Read-only collision assessment for a tagged roost marker; does not begin movement or claim ownership. */
	FString AssessRoostSite(const AActor* Site) const;

protected:
	virtual bool IsActionInProgress() const override;
	virtual bool CanRest() const override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ActOnDecision(const FAgentDecision& Decision) override;

private:
	friend class FRavenPerchTest;
	FVector MovementTarget = FVector::ZeroVector;
	FVector PerchTarget = FVector::ZeroVector;
	FVector CruiseTarget = FVector::ZeroVector;
	FVector HopStart = FVector::ZeroVector;
	FVector HopEnd = FVector::ZeroVector;
	float HomeAltitude = 0.f;
	float HopElapsed = 0.f;
	float HopDuration = 0.55f;
	bool bHasMovementTarget = false;
	bool bTargetIsPerch = false;
	bool bApproachingPerch = false;

	void SetFlyingMovement(bool bFlying) const;
	void BeginTakeoff(const FVector& Destination);
	void BeginLanding(const FVector& DesiredLocation);
	void BeginHop();
	bool BeginPerch();
	bool BeginPerchAt(AActor* Perch);
	void SetGrounded();
	FVector MakeCruiseTarget() const;
	bool TraceGround(const FVector& DesiredLocation, FVector& OutGroundLocation) const;
	bool AdvanceTowardsTarget(float DeltaSeconds);
};
