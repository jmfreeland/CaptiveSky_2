// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentRelationshipComponent.generated.h"

/** Neutral social history. Evaluative qualities are left for evidence-based consolidation. */
USTRUCT(BlueprintType)
struct FAgentRelationshipRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	FString AgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	int32 InteractionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	float Familiarity = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	FDateTime FirstInteractionAt;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	FDateTime LastInteractionAt;

	UPROPERTY(BlueprintReadOnly, Category = "Agent Relationship")
	TArray<FString> RecentEvidence;
};

/** Persists factual relationship evidence without inventing trust, affection, or friendship. */
UCLASS(ClassGroup = (Agent), meta = (BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentRelationshipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentRelationshipComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent Relationship", meta = (ClampMin = "1"))
	int32 MaximumRecentEvidencePerBeing = 12;

	void RecordInteraction(const FString& OtherAgentId, const FString& OtherDisplayName,
		const FString& Evidence, const FString& ConversationId);

	UFUNCTION(BlueprintCallable, Category = "Agent Relationship")
	bool GetRelationship(const FString& OtherAgentId, FAgentRelationshipRecord& OutRelationship) const;

	/** Human-readable, bounded context for the agent's system prompt. */
	FString BuildPromptSummary() const;

	FString GetRelationshipsFilePath() const;

private:
	mutable bool bLoaded = false;
	mutable TMap<FString, FAgentRelationshipRecord> Relationships;

	void EnsureLoaded() const;
	void Save() const;
};
