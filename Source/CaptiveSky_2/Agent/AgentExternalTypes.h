// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgentConversationTypes.h"
#include "AgentExternalTypes.generated.h"

/** Channel-neutral correspondence delivered by the standalone agent gateway. */
USTRUCT(BlueprintType)
struct FAgentExternalUtterance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString TurnId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString AgentId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString Source;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString ConversationId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString MessageId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString ParticipantId;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString ParticipantName;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FString Text;

	UPROPERTY(BlueprintReadWrite, Category = "Agent External")
	FDateTime ReceivedAt;

	static bool FromJson(const FString& Json, FAgentExternalUtterance& OutUtterance);
	FAgentConversationContext ToConversationContext() const;
};

/** Result returned through the gateway outbox after an embodied turn. */
USTRUCT()
struct FAgentExternalResponse
{
	GENERATED_BODY()

	FString TurnId;
	FString AgentId;
	FString Status;
	FString Speech;
	FString Error;
	FDateTime CompletedAt;

	FString ToJson() const;
};
