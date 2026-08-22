// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentLLMProvider.h"
#include "AgentMemoryTypes.h"
#include "AgentConsolidationComponent.generated.h"

UENUM(BlueprintType)
enum class EAgentConsciousState : uint8
{
	Awake,
	Resting,
	Consolidating
};

USTRUCT(BlueprintType)
struct FAgentPersonalityTendency
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Agent Personality")
	FString Name;

	/** Signed, deliberately slow-moving strength in the range -1..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Agent Personality")
	float Strength = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Personality")
	FString LastReason;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Personality")
	TArray<FString> EvidenceMemoryIds;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAgentConsciousStateChanged,
	EAgentConsciousState, PreviousState, EAgentConsciousState, NewState);

/**
 * Reusable sleep lifecycle for conscious agents. Sleep pauses ordinary thought, then asks the
 * configured provider to reflect only on durable lived memories. Small personality adjustments
 * are stored as a derived overlay plus an append-only history; identity.md and personality.md
 * remain untouched, making every change inspectable and reversible.
 */
UCLASS(ClassGroup = (Agent), meta = (BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentConsolidationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentConsolidationComponent();
	virtual ~UAgentConsolidationComponent() override;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Sleep")
	EAgentConsciousState ConsciousState = EAgentConsciousState::Awake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Sleep", meta = (ClampMin = "0.0"))
	float DefaultRestDurationSeconds = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Sleep", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaximumMemoriesPerConsolidation = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Sleep", meta = (ClampMin = "0.001", ClampMax = "0.05"))
	float MaximumAdjustmentPerSleep = 0.03f;

	UPROPERTY(BlueprintAssignable, Category = "Agent Sleep")
	FOnAgentConsciousStateChanged OnConsciousStateChanged;

	/** Begins rest if the agent is awake and has no ordinary thought request in flight. */
	UFUNCTION(BlueprintCallable, Category = "Agent Sleep")
	bool BeginSleep(float RestDurationSeconds = -1.f);

	/** Wakes immediately. A consolidation HTTP request already in flight is allowed to finish but is ignored. */
	UFUNCTION(BlueprintCallable, Category = "Agent Sleep")
	void WakeUp();

	UFUNCTION(BlueprintPure, Category = "Agent Sleep")
	bool IsAwake() const { return ConsciousState == EAgentConsciousState::Awake; }

	UFUNCTION(BlueprintCallable, Category = "Agent Personality")
	TArray<FAgentPersonalityTendency> GetEvolvingTendencies() const;

	FString GetPersonalityStatePath() const;
	FString GetPersonalityHistoryPath() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TUniquePtr<IAgentLLMProvider> Provider;
	FTimerHandle RestTimer;
	int32 SleepGeneration = 0;
	FDateTime LastConsolidatedAt = FDateTime::MinValue();
	int32 Revision = 0;
	TArray<FAgentPersonalityTendency> Tendencies;
	bool bStateLoaded = false;

	void SetConsciousState(EAgentConsciousState NewState);
	void StartConsolidation();
	void FinishSleep();
	void EnsureStateLoaded();
	bool ApplyConsolidationResponse(const FString& ResponseText, const TArray<FAgentMemoryRecord>& Memories);
	bool SaveState() const;
	void AppendHistory(const FString& TraitName, float PreviousStrength, float NewStrength,
		const FString& Reason, const TArray<FString>& EvidenceIds) const;
	FString BuildConsolidationPrompt(const TArray<FAgentMemoryRecord>& Memories) const;
};
