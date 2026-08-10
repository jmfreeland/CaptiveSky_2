// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentMemoryTypes.h"
#include "AgentMemoryComponent.generated.h"

/**
 * Long-term memory store for an autonomous agent.
 *
 * Storage: plain-text, append-only JSON Lines under
 * <ProjectDir>/Agents/<AgentId>/memory.jsonl -- deliberately outside
 * Unreal's Saved/ directory (which is disposable/gitignored) so this data
 * survives engine cleanup and can be read, migrated, or re-implemented by
 * different technology later without needing Unreal at all.
 *
 * Retrieval: GetRelevantContext is the "pre-filter for a context budget"
 * function. It is intentionally a simple, explainable heuristic (recency +
 * importance + keyword overlap) rather than an embedding/vector search, so
 * it has zero extra dependencies today -- but every caller only ever talks
 * to this one function, so the scoring can be swapped for real embedding
 * retrieval later without touching storage or call sites.
 */
UCLASS(ClassGroup = (Agent), meta = (BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentMemoryComponent();

	// Identifies this agent's memory folder on disk. Falls back to the owning actor's name if left empty.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Memory")
	FString AgentId;

	// Approximate half-life, in hours, used by the recency component of GetRelevantContext's scoring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Memory")
	float RecencyHalfLifeHours = 24.f;

	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	FAgentMemoryRecord MakeMemory(EAgentMemoryType Type, const FString& Text, float Importance, const TArray<FString>& Tags) const;

	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	void AppendMemory(const FAgentMemoryRecord& Record);

	/**
	 * Returns memories most relevant to Situation, greedily selected by
	 * score until an approximate token budget is reached (~4 chars/token).
	 */
	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	TArray<FAgentMemoryRecord> GetRelevantContext(int32 MaxTokens, const FString& Situation) const;

	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	FString GetMemoryFilePath() const;

	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	FString GetAgentDirectory() const;

	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	FString GetResolvedAgentId() const;

	/** Loads a UTF-8 document from this agent's directory (for example identity.md). */
	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	FString LoadAgentDocument(const FString& FileName) const;

	UFUNCTION(BlueprintCallable, Category = "Agent Memory")
	int32 GetMemoryCount() const;

protected:
	virtual void BeginPlay() override;

private:
	mutable TArray<FAgentMemoryRecord> Cache;
	mutable bool bLoaded = false;

	void EnsureLoaded() const;
	FString ResolveAgentId() const;
	FString GetLegacyMemoryFilePath() const;

	static float ScoreRecord(const FAgentMemoryRecord& Record, const TArray<FString>& SituationWords, float HalfLifeHours);
};
