// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentLLMTypes.h"
#include "AgentSocialComponent.generated.h"

class AAutonomousAgentCharacter;
class UAgentBrainComponent;

USTRUCT(BlueprintType)
struct FAgentSocialUtterance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Agent Social")
	FString ConversationId;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Social")
	FString SenderAgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Social")
	FString SenderDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Social")
	FString Speech;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Social")
	int32 TurnIndex = 1;
};

/** Bounded, optional agent-to-agent conversation layered over the shared brain. */
UCLASS(ClassGroup = (Agent), meta = (BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentSocialComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentSocialComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Social", meta = (ClampMin = "100.0"))
	float SpeakingRadius = 2500.f;

	/** Total spoken lines allowed in one automatic exchange, including the opening line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Social", meta = (ClampMin = "1", ClampMax = "12"))
	int32 MaximumConversationTurns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Social", meta = (ClampMin = "0.0"))
	float ConversationCooldownSeconds = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Social", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaximumPendingUtterances = 4;

	void ReceiveUtterance(const FAgentSocialUtterance& Utterance);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TArray<FAgentSocialUtterance> PendingUtterances;
	FAgentSocialUtterance ActiveUtterance;
	bool bHandlingUtterance = false;
	TMap<FString, double> CooldownUntilByAgentId;

	UAgentBrainComponent* GetBrain() const;
	AAutonomousAgentCharacter* GetAgentOwner() const;
	AAutonomousAgentCharacter* FindAgent(const FString& IdOrName) const;
	bool IsWithinSpeakingRange(const AAutonomousAgentCharacter* Other) const;
	bool IsCoolingDownWith(const FString& OtherAgentId) const;
	void ApplyMutualCooldown(AAutonomousAgentCharacter* Other);
	void TryBeginPendingUtterance();
	void TryBeginSpontaneousConversation(const FAgentDecision& Decision);
	void DeliverSpeech(AAutonomousAgentCharacter* Recipient, const FString& Speech,
		const FString& ConversationId, int32 TurnIndex);

	UFUNCTION()
	void HandleDecisionReady(const FAgentDecision& Decision);
};
