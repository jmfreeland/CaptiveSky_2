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
	float ThinkIntervalSeconds = 60.f;

	// Radius used to pick a random reachable point for the Wander action.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float WanderRadius = 2000.f;

	FString DescribeActionState() const;
	static double BackgroundDelay(int32 Repeats, double BaseSeconds);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	virtual void ActOnDecision(const FAgentDecision& Decision);
	virtual bool IsActionInProgress() const;
	virtual bool CanRest() const;
	/** Whether this body is settled enough to arrange stones by hand, beak, or otherwise. */
	virtual bool CanArrangeStones() const;
	void ArrangeStones(const FAgentDecision& Decision);
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
	void ReportAction(const FString& Outcome);
	bool IsResting() const;
	bool TryRest();

private:
	friend class FRavenPerchTest;
	friend class FIslandNightEcologyTest;
	FTimerHandle ThinkTimerHandle;
	FString LastActionOutcome = TEXT("No completed action yet.");
	FString LastActionKey;
	int32 RepeatedActions = 0;
	int32 AutonomousRequests = 0;
	double NextThinkAt = 0;
	double NextRestAt = 0;
	TMap<FName, double> InspectedUntil;
	void InspectTarget(FName Target);

	// Bound to the possessed character's Brain->OnDecisionReady.
	UFUNCTION()
	void HandleDecisionReady(const FAgentDecision& Decision);

	// Timer callback: asks the brain to decide, if it isn't already working on one.
	void Think();

};
