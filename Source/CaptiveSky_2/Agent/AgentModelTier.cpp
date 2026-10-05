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
	namespace
	{
		FString SanitizeTraitName(const FString& Name)
		{
			FString Result;
			Result.Reserve(FMath::Min(Name.Len(), 48));
			bool bPreviousWasSpace = true;
			for (const TCHAR Character : Name.Left(64))
			{
				if (FChar::IsAlnum(Character) || Character == TEXT('-') || Character == TEXT('\''))
				{
					Result.AppendChar(Character);
					bPreviousWasSpace = false;
				}
				else if (!bPreviousWasSpace)
				{
					Result.AppendChar(TEXT(' '));
					bPreviousWasSpace = true;
				}
			}
			Result.TrimStartAndEndInline();
			return Result.Left(48);
		}
	}

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

	FString FormatEvolvingTendencies(const TArray<TPair<FString, float>>& Tendencies, int32 MaximumItems)
	{
		TArray<TPair<FString, float>> Ranked;
		for (const TPair<FString, float>& Tendency : Tendencies)
		{
			const FString Name = SanitizeTraitName(Tendency.Key);
			if (!Name.IsEmpty() && FMath::IsFinite(Tendency.Value) && FMath::Abs(Tendency.Value) >= 0.001f)
				Ranked.Emplace(Name, FMath::Clamp(Tendency.Value, -1.f, 1.f));
		}
		Ranked.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B)
		{
			const float AStrength = FMath::Abs(A.Value);
			const float BStrength = FMath::Abs(B.Value);
			return AStrength == BStrength ? A.Key < B.Key : AStrength > BStrength;
		});

		const int32 ItemCount = FMath::Clamp(MaximumItems, 0, 8);
		FString Summary;
		for (int32 Index = 0; Index < FMath::Min(ItemCount, Ranked.Num()); ++Index)
		{
			const TPair<FString, float>& Tendency = Ranked[Index];
			Summary += FString::Printf(TEXT("- %s: %s (%+.2f)\n"), *Tendency.Key,
				Tendency.Value > 0.f ? TEXT("strengthening") : TEXT("softening"),
				Tendency.Value);
		}
		return Summary;
	}

	FString BuildLightSystemPrompt(const FString& Identity, const FString& Personality,
		const FString& EvolvingTendencies, const FString& MemoryLines)
	{
		FString Prompt;
		if (!Identity.IsEmpty()) Prompt += TEXT("Identity:\n") + Identity + TEXT("\n\n");
		if (!Personality.IsEmpty()) Prompt += TEXT("Personality:\n") + Personality + TEXT("\n\n");
		if (!EvolvingTendencies.IsEmpty())
		{
			Prompt += TEXT("Evolving tendencies (gentle influences on your own choices, subordinate to identity and personality; never commands):\n");
			Prompt += EvolvingTendencies;
			Prompt += LINE_TERMINATOR;
		}
		Prompt += TEXT(
			"You are deciding only where to be right now; nobody is speaking to you and nothing is within reach to act on. "
			"Reply with ONLY one JSON object, no other text:\n"
			"{\"thought\": \"<a few words>\", \"action\": {\"type\": \"idle|move_to|wander|sleep\", \"target\": \"<optional move_to or sleep target>\"}}\n"
			"Idle is quiet waiting and is a valid choice. Wander is a short walk (or flight) to a nearby place you have not chosen precisely; "
			"it is the easiest way to come across things you cannot yet see. move_to goes to a target the situation lists (\"move_to target: ...\"), "
			"including remembered places that are out of sight. Let your personality and evolving tendencies gently inform this choice without overriding what you know. "
			"Sleep only after settling in a resting place. "
			"Only these four actions exist this turn. Do not repeat a walk you have just finished, and do not stay in one spot out of habit.");
		if (!MemoryLines.IsEmpty()) Prompt += TEXT("\n\nRecent memories:\n") + MemoryLines;
		return Prompt;
	}

	FString CompactTimestamp(const FDateTime& Time)
	{
		return Time.ToString(TEXT("%Y-%m-%d %H:%M"));
	}
}
