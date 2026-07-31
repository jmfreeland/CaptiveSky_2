// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeExecutionTypes.h"
#include "StateTreeAsyncExecutionContext.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"

EStateTreeRunStatus FStateTreeAgentDecideTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.Agent || !InstanceData.Agent->Brain)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.Agent->Brain->bRequestInFlight)
	{
		// Already thinking -- treat as still running rather than starting an overlapping request.
		return EStateTreeRunStatus::Running;
	}

	// bind to the on decision complete delegate
	InstanceData.Agent->Brain->OnDecisionCompleteForStateTree.BindLambda(
		[WeakContext = Context.MakeWeakExecutionContext()](const FAgentDecision& Decision)
		{
			WeakContext.FinishTask(Decision.bValid ? EStateTreeFinishTaskType::Succeeded : EStateTreeFinishTaskType::Failed);
		}
	);

	// kick off the think-cycle
	InstanceData.Agent->Brain->RequestDecision(InstanceData.PlayerUtterance);

	return EStateTreeRunStatus::Running;
}

void FStateTreeAgentDecideTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.Agent && InstanceData.Agent->Brain)
	{
		InstanceData.Agent->Brain->OnDecisionCompleteForStateTree.Unbind();
	}
}

#if WITH_EDITOR
FText FStateTreeAgentDecideTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Agent Decide</b>");
}
#endif // WITH_EDITOR
