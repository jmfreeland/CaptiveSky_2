// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgentConversationTypes.generated.h"

/**
 * Channel-neutral description of someone addressing an agent.
 *
 * The same brain entry point handles a nearby being, the local player, Discord, and future
 * channels. Medium is explicit so a remote message is never mistaken for embodied perception.
 */
USTRUCT(BlueprintType)
struct FAgentConversationContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	FString Text;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	FString Source;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	FString ConversationId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	FString MessageId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	FString ParticipantId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	FString ParticipantName;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	bool bExternal = false;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	bool bAgentToAgent = false;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Conversation")
	int32 TurnIndex = 0;
};
