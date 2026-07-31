// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "AgentLLMTypes.generated.h"

/** One turn in a chat-style LLM request. */
USTRUCT(BlueprintType)
struct FAgentLLMMessage
{
	GENERATED_BODY()

	// "user" or "assistant".
	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString Role = TEXT("user");

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString Text;

	// Optional: base64-encoded PNG attached to this message (e.g. a first-person snapshot). Empty = no image.
	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString ImageBase64PNG;
};

/** A single request to an LLM provider. Provider-agnostic on purpose. */
USTRUCT(BlueprintType)
struct FAgentLLMRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString SystemPrompt;

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	TArray<FAgentLLMMessage> Messages;

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	int32 MaxTokens = 1024;

	// Empty = use UAgentLLMSettings::Model.
	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString ModelOverride;
};

/** Result of an LLM call. ResponseText is the raw assistant text -- decision-JSON parsing happens above this layer. */
struct FAgentLLMResult
{
	bool bSuccess = false;
	FString ResponseText;
	FString ErrorMessage;
};

DECLARE_DELEGATE_OneParam(FOnAgentLLMComplete, const FAgentLLMResult& /*Result*/);

/** What kind of thing the agent has decided to do, as parsed out of the LLM's response. */
UENUM(BlueprintType)
enum class EAgentActionType : uint8
{
	Idle,
	MoveTo,
	Speak,
	Wander,
	Interact
};

/** The agent's decision for this think-cycle, plus any new memories it chose to write down. */
USTRUCT(BlueprintType)
struct FAgentDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	bool bValid = false;

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString Thought;

	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	EAgentActionType ActionType = EAgentActionType::Idle;

	// Free-form target: an actor name/tag for MoveTo/Interact, unused for Idle/Wander.
	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString ActionTarget;

	// What to say, when ActionType == Speak.
	UPROPERTY(BlueprintReadWrite, Category = "Agent LLM")
	FString Speech;
};
