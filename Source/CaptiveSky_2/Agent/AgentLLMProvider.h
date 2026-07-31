// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgentLLMTypes.h"
#include "AgentLLMSettings.h"

/**
 * Provider-agnostic interface for sending a chat request to an LLM and
 * getting the assistant's raw text back. Concrete providers only differ in
 * how they shape the HTTP request/response for their API -- callers
 * (UAgentBrainComponent) never need to know which one is active.
 */
class CAPTIVESKY_2_API IAgentLLMProvider
{
public:
	virtual ~IAgentLLMProvider() = default;

	// Fires OnComplete exactly once, always on the game thread.
	virtual void SendRequest(const FAgentLLMRequest& Request, FOnAgentLLMComplete OnComplete) = 0;
};

/** Anthropic Messages API (https://api.anthropic.com/v1/messages). */
class CAPTIVESKY_2_API FAnthropicLLMProvider : public IAgentLLMProvider
{
public:
	virtual void SendRequest(const FAgentLLMRequest& Request, FOnAgentLLMComplete OnComplete) override;
};

/** OpenAI chat-completions API shape; also covers local OpenAI-compatible servers (Ollama, LM Studio, ...). */
class CAPTIVESKY_2_API FOpenAICompatibleLLMProvider : public IAgentLLMProvider
{
public:
	virtual void SendRequest(const FAgentLLMRequest& Request, FOnAgentLLMComplete OnComplete) override;
};

/** Builds the provider indicated by UAgentLLMSettings. */
CAPTIVESKY_2_API TUniquePtr<IAgentLLMProvider> CreateAgentLLMProvider();
