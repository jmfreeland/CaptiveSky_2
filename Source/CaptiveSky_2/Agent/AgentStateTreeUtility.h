// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AgentLLMTypes.h"

#include "AgentStateTreeUtility.generated.h"

class AAutonomousAgentCharacter;

/**
 *  Instance data for the Agent Decide StateTree task.
 */
USTRUCT()
struct FStateTreeAgentDecideInstanceData
{
	GENERATED_BODY()

	/** Agent whose brain will be asked to decide. */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAutonomousAgentCharacter> Agent;

	/** Optional line of dialogue from the player to react to. Leave empty if no one is speaking. */
	UPROPERTY(EditAnywhere, Category = "Input")
	FString PlayerUtterance;
};

/**
 *  StateTree task that runs one full agent think-cycle (memory retrieval + first-person
 *  snapshot + LLM call) via UAgentBrainComponent::RequestDecision, following this project's
 *  established async-task idiom (see FStateTreeComboAttackTask in CombatStateTreeUtility):
 *  EnterState kicks off the async work and returns Running, a lambda bound via
 *  Context.MakeWeakExecutionContext() calls FinishTask() when the brain component's completion
 *  delegate fires, and ExitState unbinds it.
 *
 *  The resulting decision is NOT written into this task's instance data -- it's read from
 *  Agent.Brain.LastDecision via ordinary StateTree property binding, since writing into
 *  per-task instance data from an async completion callback would be unsafe. Downstream
 *  states/transitions should bind their conditions to Agent.Brain.LastDecision.ActionType (etc).
 */
USTRUCT(meta = (DisplayName = "Agent Decide", Category = "Agent"))
struct FStateTreeAgentDecideTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FStateTreeAgentDecideTask()
	{
		bShouldCallTick = false;
		bShouldStateChangeOnReselect = false;
	}

	using FInstanceDataType = FStateTreeAgentDecideInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};
