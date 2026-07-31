// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentLLMTypes.h"
#include "AgentMemoryTypes.h"
#include "AgentLLMProvider.h"
#include "AgentBrainComponent.generated.h"

class UAgentMemoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAgentDecisionReady, const FAgentDecision&, Decision);

// Plain (non-dynamic) single-cast delegate, bound/unbound by a StateTree task's EnterState/ExitState --
// mirrors this project's existing convention (see ACombatEnemy::OnAttackCompleted) for hooking async
// work into StateTree via Context.MakeWeakExecutionContext() + FinishTask(), which dynamic multicast
// delegates can't do since they can't bind a raw lambda.
DECLARE_DELEGATE_OneParam(FOnAgentDecisionCompleteDelegate, const FAgentDecision& /*Decision*/);

/**
 * Ties memory retrieval, the first-person snapshot, and the configured LLM
 * provider together into a single "think" cycle. This is the component a
 * StateTree task calls into (see FStateTreeAgentDecideTask) -- StateTree
 * itself never talks to memory or the LLM directly.
 */
UCLASS(ClassGroup = (Agent), meta = (BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentBrainComponent();
	// Declared (and defined in the .cpp, where IAgentLLMProvider is a complete type) so the
	// implicit destruction of TUniquePtr<IAgentLLMProvider> below doesn't need Provider's full
	// definition here in the header.
	virtual ~UAgentBrainComponent() override;

	// Stable identity/personality seed for this agent. Combined with retrieved memories to form the system prompt.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Brain")
	FString Persona = TEXT("You are an autonomous creature living in a game world. Decide what to do next.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Brain")
	int32 MemoryContextTokenBudget = 800;

	// Broadcast on the game thread exactly once per RequestDecision call (Decision.bValid is false on failure).
	UPROPERTY(BlueprintAssignable, Category = "Agent Brain")
	FOnAgentDecisionReady OnDecisionReady;

	// For the StateTree task hookup: bind in EnterState, unbind in ExitState (see FStateTreeAgentDecideTask).
	FOnAgentDecisionCompleteDelegate OnDecisionCompleteForStateTree;

	// True while a request is in flight. StateTree tasks should not start a second request while this is true.
	UPROPERTY(BlueprintReadOnly, Category = "Agent Brain")
	bool bRequestInFlight = false;

	// The most recent decision. StateTree branches on this via property binding to
	// Agent.Brain.LastDecision rather than via task instance-data output, since that avoids writing
	// into per-task instance data from an async completion callback.
	UPROPERTY(BlueprintReadOnly, Category = "Agent Brain")
	FAgentDecision LastDecision;

	/**
	 * Runs one full think-cycle: retrieves relevant memory, captures a
	 * first-person snapshot (if the owner supports it), calls the LLM, and
	 * parses the structured decision -- storing any new memories the model
	 * chose to write along the way.
	 */
	UFUNCTION(BlueprintCallable, Category = "Agent Brain")
	void RequestDecision(const FString& PlayerUtterance);

protected:
	virtual void BeginPlay() override;

private:
	TUniquePtr<IAgentLLMProvider> Provider;

	FString BuildSituationSummary(const FString& PlayerUtterance) const;
	FString BuildSystemPrompt(const TArray<FAgentMemoryRecord>& RelevantMemories) const;

	// Parses the model's raw response text into a decision, appending any "new_memories" entries via MemoryComp along the way.
	static FAgentDecision ParseDecisionAndStoreMemories(const FString& ResponseText, UAgentMemoryComponent* MemoryComp);
};
