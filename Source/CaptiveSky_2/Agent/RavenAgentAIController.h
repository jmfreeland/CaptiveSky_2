#pragma once

#include "CoreMinimal.h"
#include "AutonomousAgentAIController.h"
#include "RavenAgentAIController.generated.h"

/** Asset-independent prototype flight controller for raven consciousnesses. */
UCLASS()
class CAPTIVESKY_2_API ARavenAgentAIController : public AAutonomousAgentAIController
{
	GENERATED_BODY()

public:
	ARavenAgentAIController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Flight")
	float FlightSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Flight")
	float VerticalRange = 600.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ActOnDecision(const FAgentDecision& Decision) override;

private:
	FVector FlightTarget = FVector::ZeroVector;
	float HomeAltitude = 0.f;
	bool bHasFlightTarget = false;
};
