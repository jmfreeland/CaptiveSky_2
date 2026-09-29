// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentModelTier.h"
#include "AgentConversationTypes.h"
#include "AgentLLMSettings.h"
#include "AutonomousAgentCharacter.h"
#include "RavenAgentAIController.h"
#include "IslandWorldStateSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace AgentModelTier
{
	bool NeedsFullModel(const FTurnFacts& Facts)
	{
		return Facts.bSomeoneAddressedResident || Facts.bHasBuildOptions
			|| Facts.NearestBeingCm <= BeingRangeCm || Facts.NearestAffordanceCm <= AffordanceRangeCm;
	}

	FTurnFacts GatherTurnFacts(const AActor* Owner, const FAgentConversationContext& Context)
	{
		FTurnFacts Facts;
		Facts.bSomeoneAddressedResident = !Context.Text.IsEmpty() || Context.bExternal || Context.bAgentToAgent;
		UWorld* World = Owner ? Owner->GetWorld() : nullptr;
		if (!Owner || !World) return Facts;

		const FVector Location = Owner->GetActorLocation();
		const ARavenAgentAIController* Raven = nullptr;
		if (const APawn* Body = Cast<APawn>(Owner)) Raven = Cast<ARavenAgentAIController>(Body->GetController());
		if (Raven) Facts.bHasBuildOptions = !Raven->DescribeBuildOptions().IsEmpty();

		auto Consider = [&Location, &Facts](const FVector& Where)
		{
			Facts.NearestAffordanceCm = FMath::Min(Facts.NearestAffordanceCm, static_cast<float>(FVector::Dist(Location, Where)));
		};
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const AActor* Other = *It;
			if (Other == Owner) continue;
			if (Cast<AAutonomousAgentCharacter>(Other))
			{
				Facts.NearestBeingCm = FMath::Min(Facts.NearestBeingCm, static_cast<float>(FVector::Dist(Location, Other->GetActorLocation())));
				continue;
			}
			if (Other->ActorHasTag(TEXT("IslandLandmark")) || Other->ActorHasTag(TEXT("InnHearth")) || Other->ActorHasTag(TEXT("InnCounter"))
				|| (Other->ActorHasTag(TEXT("IslandLife")) && !Other->IsHidden()))
				Consider(Other->GetActorLocation());
		}
		if (const UIslandWorldStateSubsystem* WorldState = World->GetSubsystem<UIslandWorldStateSubsystem>())
		{
			for (const FIslandCurioRecord& Curio : WorldState->GetCurios()) Consider(Curio.Location);
			for (const FIslandArrangementSite& Site : WorldState->GetArrangementSites()) Consider(Site.Location);
		}
		return Facts;
	}

	FString LightModelName()
	{
		FString Name;
		if (FParse::Value(FCommandLine::Get(), TEXT("CaptiveSkyLightModel="), Name) && !Name.IsEmpty()) return Name.TrimQuotes();
		const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
		return Settings ? Settings->LightModel.TrimStartAndEnd() : FString();
	}

	FString LightReasoningEffort()
	{
		const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
		return Settings ? Settings->LightReasoningEffort.TrimStartAndEnd() : FString();
	}

	bool IsLightAction(EAgentActionType Type)
	{
		return Type == EAgentActionType::Idle || Type == EAgentActionType::MoveTo
			|| Type == EAgentActionType::Wander || Type == EAgentActionType::Sleep;
	}

	FString BuildLightSystemPrompt(const FString& Identity, const FString& MemoryLines)
	{
		FString Prompt;
		if (!Identity.IsEmpty()) Prompt += TEXT("Identity:\n") + Identity + TEXT("\n\n");
		Prompt += TEXT(
			"You are deciding only where to be right now; nobody is speaking to you and nothing is within reach to act on. "
			"Reply with ONLY one JSON object, no other text:\n"
			"{\"thought\": \"<a few words>\", \"action\": {\"type\": \"idle|move_to|wander|sleep\", \"target\": \"<optional move_to or sleep target>\"}}\n"
			"Idle is quiet waiting and is a valid choice. Wander is a short walk (or flight) to a nearby place you have not chosen precisely; "
			"it is the easiest way to come across things you cannot yet see. move_to goes to a target the situation lists (\"move_to target: ...\"), "
			"including remembered places that are out of sight. Sleep only after settling in a resting place. "
			"Only these four actions exist this turn. Do not repeat a walk you have just finished, and do not stay in one spot out of habit.");
		if (!MemoryLines.IsEmpty()) Prompt += TEXT("\n\nRecent memories:\n") + MemoryLines;
		return Prompt;
	}

	FString CompactTimestamp(const FDateTime& Time)
	{
		return Time.ToString(TEXT("%Y-%m-%d %H:%M"));
	}
}
