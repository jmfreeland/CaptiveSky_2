// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AgentLLMTypes.h"
#include "AutonomousAgentAIController.generated.h"

/**
 * Drives an AAutonomousAgentCharacter's think/act loop directly in C++: polls
 * UAgentBrainComponent::RequestDecision on a timer and turns the resulting
 * FAgentDecision into a movement/navigation command.
 *
 * This does NOT use StateTree, unlike ACombatAIController/ASideScrollingAIController
 * elsewhere in this project -- FStateTreeAgentDecideTask (see AgentStateTreeUtility.h)
 * exists for a future StateTree-driven version, but building that graph requires the
 * StateTree editor UI. This controller is the interim path to get the agent thinking
 * and moving today.
 */
UCLASS(Abstract)
class CAPTIVESKY_2_API AAutonomousAgentAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAutonomousAgentAIController();

	// How often the agent asks its brain for a new decision while no request is already in flight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float ThinkIntervalSeconds = 15.f;

	// Radius used to pick a random reachable point for the Wander action.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float WanderRadius = 2000.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	FTimerHandle ThinkTimerHandle;

	// Bound to the possessed character's Brain->OnDecisionReady.
	UFUNCTION()
	void HandleDecisionReady(const FAgentDecision& Decision);

	// Timer callback: asks the brain to decide, if it isn't already working on one.
	void Think();

	void ActOnDecision(const FAgentDecision& Decision);
};
