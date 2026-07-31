// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AgentLLMSettings.generated.h"

UENUM(BlueprintType)
enum class EAgentLLMProviderType : uint8
{
	// Anthropic Messages API.
	Anthropic,
	// OpenAI chat-completions API shape -- also covers local OpenAI-compatible
	// servers (Ollama, LM Studio, etc.) via EndpointOverride.
	OpenAICompatible
};

/**
 * Project-wide default configuration for the autonomous agents' LLM calls.
 * Never stores an API key -- keys are read from an environment variable at
 * runtime (see ApiKeyEnvVar) so they never touch disk in this project.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Agent LLM Settings"))
class CAPTIVESKY_2_API UAgentLLMSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAgentLLMSettings();

	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	EAgentLLMProviderType Provider = EAgentLLMProviderType::Anthropic;

	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	FString Model = TEXT("claude-sonnet-5");

	// Empty = provider's default public endpoint. Set this to target a local server (e.g. Ollama/LM Studio) for OpenAICompatible.
	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	FString EndpointOverride;

	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	int32 DefaultMaxTokens = 1024;

	// Newer OpenAI reasoning-class models (o1, gpt-5 family) reject 'max_tokens' and require
	// 'max_completion_tokens' instead. Local OpenAI-compatible servers (Ollama, LM Studio) usually
	// still expect 'max_tokens'. Only used by the OpenAICompatible provider.
	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	bool bUseMaxCompletionTokensParam = true;

	UPROPERTY(EditAnywhere, config, Category = "Agent LLM", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Temperature = 1.0f;

	// Name of the OS environment variable holding the API key. Leave the corresponding env var unset for keyless local servers.
	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	FString ApiKeyEnvVar = TEXT("ANTHROPIC_API_KEY");

	// How long to wait for a response before treating the request as failed.
	UPROPERTY(EditAnywhere, config, Category = "Agent LLM")
	float RequestTimeoutSeconds = 30.f;
};
