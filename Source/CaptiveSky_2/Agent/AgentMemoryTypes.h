// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgentMemoryTypes.generated.h"

/**
 * Broad category for a memory record. Kept small and generic on purpose --
 * this schema is meant to remain readable and portable outside of Unreal.
 */
UENUM(BlueprintType)
enum class EAgentMemoryType : uint8
{
	Observation,
	Conversation,
	Decision,
	Reflection
};

/**
 * A single long-term memory entry for an autonomous agent.
 *
 * This struct is deliberately plain: every field maps 1:1 onto a JSON value
 * with no Unreal-specific types in the serialized form, so the on-disk
 * memory store (see UAgentMemoryComponent) can be read, migrated, or
 * re-implemented by an entirely different tech stack later without needing
 * to understand Unreal's reflection system.
 */
USTRUCT(BlueprintType)
struct FAgentMemoryRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	FString Id;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	EAgentMemoryType Type = EAgentMemoryType::Observation;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	FString Text;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	TArray<FString> Tags;

	// 0..1, author-assigned significance. Higher survives context-filtering longer.
	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	float Importance = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Agent Memory")
	FVector Location = FVector::ZeroVector;

	// Scratch field: set by GetRelevantContext for the current query, not persisted.
	float RelevanceScore = 0.f;

	FString ToJsonLine() const;
	static bool FromJsonLine(const FString& Line, FAgentMemoryRecord& OutRecord);

	static FString TypeToString(EAgentMemoryType InType);
	static EAgentMemoryType TypeFromString(const FString& InString);
};
