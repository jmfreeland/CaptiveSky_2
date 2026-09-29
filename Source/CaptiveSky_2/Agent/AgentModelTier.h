// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AgentLLMTypes.h"

class AActor;
struct FAgentConversationContext;

/**
 * Which model a resident's turn goes to. A light turn is a quiet, unprompted one where the resident only has to
 * decide where to be (idle, wander, move_to, sleep). Anything social, or anything within reach of something it could
 * act on, goes to the full model, which also carries the whole persona, memories and first-person image.
 */
namespace AgentModelTier
{
	/** Another being this close could plausibly be spoken to. */
	static constexpr float BeingRangeCm = 600.f;
	/** Something to interact with or build on this close is an opportunity that deserves the full model. */
	static constexpr float AffordanceRangeCm = 400.f;
	static constexpr float FarCm = 1.0e9f;

	struct FTurnFacts
	{
		bool bSomeoneAddressedResident = false;
		float NearestBeingCm = FarCm;
		float NearestAffordanceCm = FarCm;
		bool bHasBuildOptions = false;
	};

	CAPTIVESKY_2_API bool NeedsFullModel(const FTurnFacts& Facts);
	CAPTIVESKY_2_API FTurnFacts GatherTurnFacts(const AActor* Owner, const FAgentConversationContext& Context);

	/** The light model's id: -CaptiveSkyLightModel=, else UAgentLLMSettings::LightModel. Empty means the tier is off. */
	CAPTIVESKY_2_API FString LightModelName();
	CAPTIVESKY_2_API FString LightReasoningEffort();

	/** The only actions a light turn may take; anything else is dropped as idling. */
	CAPTIVESKY_2_API bool IsLightAction(EAgentActionType Type);

	CAPTIVESKY_2_API FString BuildLightSystemPrompt(const FString& Identity, const FString& MemoryLines);

	/** "2026-09-28 20:51": prompt-friendly time without seconds, milliseconds or zone noise. */
	CAPTIVESKY_2_API FString CompactTimestamp(const FDateTime& Time);
}
