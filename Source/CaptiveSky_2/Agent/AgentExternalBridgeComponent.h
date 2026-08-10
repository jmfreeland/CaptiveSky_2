// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentExternalTypes.h"
#include "AgentLLMTypes.h"
#include "AgentExternalBridgeComponent.generated.h"

class UAgentBrainComponent;
class UAgentMemoryComponent;

/**
 * File-backed bridge between an embodied Unreal agent and external channel adapters.
 *
 * A short-lived heartbeat makes this body authoritative. The standalone gateway then places
 * channel-neutral turns in the inbox instead of running the headless responder. This component
 * claims one turn at a time, asks the normal agent brain, and writes speech to the outbox.
 */
UCLASS(ClassGroup = (Agent), meta = (BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentExternalBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentExternalBridgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent External", meta = (ClampMin = "0.1"))
	float HeartbeatIntervalSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent External", meta = (ClampMin = "1.0"))
	float HeartbeatLifetimeSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent External", meta = (ClampMin = "0.1"))
	float InboxPollIntervalSeconds = 0.5f;

	/** True while a valid headless gateway turn owns this consciousness. */
	UFUNCTION(BlueprintCallable, Category = "Agent External")
	bool IsHeadlessTurnActive() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FString SessionId;
	double NextHeartbeatAt = 0.0;
	double NextInboxPollAt = 0.0;
	bool bOwnsEmbodimentAuthority = false;
	bool bProcessingTurn = false;
	FAgentExternalUtterance CurrentTurn;
	FString CurrentProcessingPath;

	UAgentMemoryComponent* GetMemory() const;
	UAgentBrainComponent* GetBrain() const;
	FString GetGatewayDirectory() const;
	void ClaimOrRefreshHeartbeat();
	bool PresenceBelongsToThisSession() const;
	void RemoveOwnHeartbeat();
	void RecoverOrphanedTurns();
	void TryProcessNextTurn();
	void PreserveCurrentTurnForNextEmbodiment();
	void QuarantineInvalidTurn(const FString& ProcessingPath) const;

	UFUNCTION()
	void HandleDecisionReady(const FAgentDecision& Decision);

	static bool WriteFileAtomically(const FString& Destination, const FString& Contents);
};
