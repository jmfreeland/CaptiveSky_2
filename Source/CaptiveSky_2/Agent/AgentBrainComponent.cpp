// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "AgentExternalBridgeComponent.h"
#include "AgentLLMProvider.h"
#include "AutonomousAgentCharacter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentBrain, Log, All);

UAgentBrainComponent::UAgentBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAgentBrainComponent::~UAgentBrainComponent() = default;

void UAgentBrainComponent::BeginPlay()
{
	Super::BeginPlay();
	Provider = CreateAgentLLMProvider();
}

FString UAgentBrainComponent::BuildSituationSummary(const FAgentExternalUtterance& Utterance) const
{
	const AActor* Owner = GetOwner();
	const FVector Location = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	FString NearbyBeings;
	if (Owner && GetWorld())
	{
		for (TActorIterator<AAutonomousAgentCharacter> It(GetWorld()); It; ++It)
		{
			if (*It == Owner) continue;
			const float Distance = FVector::Dist(Location, It->GetActorLocation());
			if (Distance <= 2500.f)
			{
				NearbyBeings += FString::Printf(TEXT(" %s is %.0f metres away;"), *It->GetActorLabel(), Distance / 100.f);
			}
		}
	}
	if (NearbyBeings.IsEmpty()) NearbyBeings = TEXT(" no other conscious beings are nearby;");

	// v1 world-state summary: intentionally minimal (position + any player speech). A richer
	// perception summary (nearby actors/points of interest) can be layered in here later without
	// changing anything downstream, since callers only ever see the resulting FString.
	if (Utterance.Text.IsEmpty())
	{
		return FString::Printf(TEXT("You are at position (%.0f, %.0f, %.0f). Nearby:%s no one is speaking to you right now. Decide what to do."),
			Location.X, Location.Y, Location.Z, *NearbyBeings);
	}
	if (!Utterance.Source.IsEmpty())
	{
		return FString::Printf(TEXT("You are at position (%.0f, %.0f, %.0f). Nearby:%s Through %s correspondence, %s wrote to you: \"%s\""),
			Location.X, Location.Y, Location.Z, *NearbyBeings, *Utterance.Source,
			Utterance.ParticipantName.IsEmpty() ? TEXT("a correspondent") : *Utterance.ParticipantName, *Utterance.Text);
	}
	return FString::Printf(TEXT("You are at position (%.0f, %.0f, %.0f). Nearby:%s someone just said to you: \"%s\""),
		Location.X, Location.Y, Location.Z, *NearbyBeings, *Utterance.Text);
}

FString UAgentBrainComponent::BuildSystemPrompt(const TArray<FAgentMemoryRecord>& RelevantMemories) const
{
	FString Prompt;
	if (const AActor* Owner = GetOwner())
	{
		if (const UAgentMemoryComponent* MemoryComp = Owner->FindComponentByClass<UAgentMemoryComponent>())
		{
			const FString Identity = MemoryComp->LoadAgentDocument(TEXT("identity.md")).TrimStartAndEnd();
			const FString Personality = MemoryComp->LoadAgentDocument(TEXT("personality.md")).TrimStartAndEnd();
			if (!Identity.IsEmpty())
			{
				Prompt += TEXT("Identity:\n") + Identity;
			}
			if (!Personality.IsEmpty())
			{
				if (!Prompt.IsEmpty())
				{
					Prompt += TEXT("\n\n");
				}
				Prompt += TEXT("Personality:\n") + Personality;
			}
		}
	}
	if (Prompt.IsEmpty())
	{
		Prompt = Persona;
	}
	Prompt += TEXT("\n\nRelevant memories:\n");

	if (RelevantMemories.Num() == 0)
	{
		Prompt += TEXT("(none yet)\n");
	}
	else
	{
		for (const FAgentMemoryRecord& Record : RelevantMemories)
		{
			Prompt += FString::Printf(TEXT("- [%s] %s\n"), *Record.Timestamp.ToIso8601(), *Record.Text);
		}
	}

	Prompt += TEXT(
		"\nYou will be shown your current first-person view as an image (if available) and a short "
		"description of the situation. Reply with ONLY a single JSON object, no other text, matching "
		"exactly this shape:\n"
		"{\"thought\": \"<brief reasoning>\", "
		"\"action\": {\"type\": \"idle|move_to|speak|wander|interact\", \"target\": \"<optional target name>\", \"speech\": \"<optional line to say>\"}, "
		"\"new_memories\": [{\"text\": \"<what to remember>\", \"importance\": 0.0, \"tags\": [\"<tag>\"]}]}\n"
		"When someone has just spoken to you, ordinarily answer them using the speak action unless you have a compelling reason not to.\n"
		"Omit new_memories (empty array) if nothing is worth remembering long-term from this moment.");

	return Prompt;
}

void UAgentBrainComponent::RequestDecision(const FString& PlayerUtterance)
{
	FAgentExternalUtterance Utterance;
	Utterance.Text = PlayerUtterance;
	Utterance.ParticipantName = TEXT("a visitor");
	RequestDecisionWithContext(Utterance);
}

void UAgentBrainComponent::RequestExternalDecision(const FAgentExternalUtterance& Utterance)
{
	RequestDecisionWithContext(Utterance);
}

static FString SanitizeMemoryTag(FString Tag)
{
	Tag.ToLowerInline();
	FString Result;
	for (const TCHAR Character : Tag)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('-') || Character == TEXT('_') || Character == TEXT(':'))
		{
			Result.AppendChar(Character);
		}
	}
	return Result;
}

void UAgentBrainComponent::RequestDecisionWithContext(const FAgentExternalUtterance& Utterance)
{
	if (bRequestInFlight)
	{
		UE_LOG(LogAgentBrain, Warning, TEXT("RequestDecision called while a request is already in flight; ignoring."));
		return;
	}

	if (!Provider.IsValid())
	{
		Provider = CreateAgentLLMProvider();
	}

	AActor* Owner = GetOwner();
	UAgentMemoryComponent* MemoryComp = Owner ? Owner->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	if (Owner)
	{
		if (const UAgentExternalBridgeComponent* ExternalBridge = Owner->FindComponentByClass<UAgentExternalBridgeComponent>();
			ExternalBridge && ExternalBridge->IsHeadlessTurnActive())
		{
			UE_LOG(LogAgentBrain, Log, TEXT("Embodied decision deferred while a headless external turn owns this agent."));
			FAgentDecision DeferredDecision;
			LastDecision = DeferredDecision;
			OnDecisionReady.Broadcast(DeferredDecision);
			OnDecisionCompleteForStateTree.ExecuteIfBound(DeferredDecision);
			return;
		}
	}
	if (MemoryComp && !Utterance.Text.IsEmpty())
	{
		TArray<FString> Tags = { TEXT("conversation"), TEXT("visitor") };
		FString MemoryText = FString::Printf(TEXT("A visitor said to me: \"%s\""), *Utterance.Text);
		if (!Utterance.Source.IsEmpty())
		{
			Tags = { TEXT("conversation"), TEXT("external"), SanitizeMemoryTag(Utterance.Source),
				TEXT("visitor"), TEXT("participant:") + SanitizeMemoryTag(Utterance.ParticipantId) };
			MemoryText = FString::Printf(TEXT("%s wrote to me via %s: \"%s\""),
				Utterance.ParticipantName.IsEmpty() ? TEXT("A correspondent") : *Utterance.ParticipantName,
				*Utterance.Source, *Utterance.Text);
		}
		MemoryComp->AppendMemory(MemoryComp->MakeMemory(EAgentMemoryType::Conversation,
			MemoryText, 0.5f, Tags));
	}

	const FString Situation = BuildSituationSummary(Utterance);

	TArray<FAgentMemoryRecord> RelevantMemories;
	if (MemoryComp)
	{
		RelevantMemories = MemoryComp->GetRelevantContext(MemoryContextTokenBudget, Situation);
	}

	FString SnapshotBase64;
	if (const AAutonomousAgentCharacter* AgentCharacter = Cast<AAutonomousAgentCharacter>(Owner))
	{
		SnapshotBase64 = AgentCharacter->CaptureFirstPersonSnapshot();
	}

	FAgentLLMRequest Request;
	Request.SystemPrompt = BuildSystemPrompt(RelevantMemories);

	FAgentLLMMessage UserMessage;
	UserMessage.Role = TEXT("user");
	UserMessage.Text = Situation;
	UserMessage.ImageBase64PNG = SnapshotBase64;
	Request.Messages.Add(UserMessage);

	bRequestInFlight = true;

	TWeakObjectPtr<UAgentBrainComponent> WeakThis(this);
	TWeakObjectPtr<UAgentMemoryComponent> WeakMemory(MemoryComp);

	Provider->SendRequest(Request, FOnAgentLLMComplete::CreateLambda([WeakThis, WeakMemory, Utterance](const FAgentLLMResult& Result)
	{
		UAgentBrainComponent* StrongThis = WeakThis.Get();
		if (!StrongThis)
		{
			return;
		}
		StrongThis->bRequestInFlight = false;

		FAgentDecision Decision;
		if (Result.bSuccess)
		{
			Decision = ParseDecisionAndStoreMemories(Result.ResponseText, WeakMemory.Get());
			if (Decision.bValid && !Decision.Speech.IsEmpty() && WeakMemory.IsValid())
			{
				TArray<FString> Tags = { TEXT("conversation"), TEXT("speech") };
				FString MemoryText = FString::Printf(TEXT("I replied: \"%s\""), *Decision.Speech);
				if (!Utterance.Source.IsEmpty())
				{
					Tags = { TEXT("conversation"), TEXT("external"), SanitizeMemoryTag(Utterance.Source),
						TEXT("speech"), TEXT("participant:") + SanitizeMemoryTag(Utterance.ParticipantId) };
					MemoryText = FString::Printf(TEXT("I replied to %s via %s: \"%s\""),
						Utterance.ParticipantName.IsEmpty() ? TEXT("a correspondent") : *Utterance.ParticipantName,
						*Utterance.Source, *Decision.Speech);
				}
				WeakMemory->AppendMemory(WeakMemory->MakeMemory(EAgentMemoryType::Conversation,
					MemoryText, 0.5f, Tags));
			}
			if (!Decision.bValid)
			{
				UE_LOG(LogAgentBrain, Warning, TEXT("Could not parse a decision from the LLM response: %s"), *Result.ResponseText);
			}
		}
		else
		{
			UE_LOG(LogAgentBrain, Warning, TEXT("Agent decision request failed: %s"), *Result.ErrorMessage);
		}

		StrongThis->LastDecision = Decision;
		StrongThis->OnDecisionReady.Broadcast(Decision);
		StrongThis->OnDecisionCompleteForStateTree.ExecuteIfBound(Decision);
	}));
}

static FString StripMarkdownCodeFence(const FString& In)
{
	FString Trimmed = In;
	Trimmed.TrimStartAndEndInline();
	if (Trimmed.StartsWith(TEXT("```")))
	{
		int32 FirstNewline = INDEX_NONE;
		Trimmed.FindChar(TEXT('\n'), FirstNewline);
		if (FirstNewline != INDEX_NONE)
		{
			Trimmed = Trimmed.Mid(FirstNewline + 1);
		}
		int32 ClosingFence = Trimmed.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (ClosingFence != INDEX_NONE)
		{
			Trimmed = Trimmed.Left(ClosingFence);
		}
		Trimmed.TrimStartAndEndInline();
	}
	return Trimmed;
}

static EAgentActionType ActionTypeFromString(const FString& InString)
{
	if (InString == TEXT("move_to")) return EAgentActionType::MoveTo;
	if (InString == TEXT("speak")) return EAgentActionType::Speak;
	if (InString == TEXT("wander")) return EAgentActionType::Wander;
	if (InString == TEXT("interact")) return EAgentActionType::Interact;
	return EAgentActionType::Idle;
}

FAgentDecision UAgentBrainComponent::ParseDecisionAndStoreMemories(const FString& ResponseText, UAgentMemoryComponent* MemoryComp)
{
	FAgentDecision Decision;

	const FString CleanedText = StripMarkdownCodeFence(ResponseText);

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(CleanedText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return Decision; // bValid stays false
	}

	Root->TryGetStringField(TEXT("thought"), Decision.Thought);

	const TSharedPtr<FJsonObject>* ActionObj = nullptr;
	if (Root->TryGetObjectField(TEXT("action"), ActionObj) && ActionObj && ActionObj->IsValid())
	{
		FString TypeStr;
		(*ActionObj)->TryGetStringField(TEXT("type"), TypeStr);
		Decision.ActionType = ActionTypeFromString(TypeStr);
		(*ActionObj)->TryGetStringField(TEXT("target"), Decision.ActionTarget);
		(*ActionObj)->TryGetStringField(TEXT("speech"), Decision.Speech);
	}

	Decision.bValid = true;

	if (MemoryComp)
	{
		const TArray<TSharedPtr<FJsonValue>>* NewMemoriesArray = nullptr;
		if (Root->TryGetArrayField(TEXT("new_memories"), NewMemoriesArray) && NewMemoriesArray)
		{
			for (const TSharedPtr<FJsonValue>& Value : *NewMemoriesArray)
			{
				const TSharedPtr<FJsonObject>* MemObj = nullptr;
				if (!Value.IsValid() || !Value->TryGetObject(MemObj) || !MemObj || !MemObj->IsValid())
				{
					continue;
				}

				FString Text;
				(*MemObj)->TryGetStringField(TEXT("text"), Text);
				if (Text.IsEmpty())
				{
					continue;
				}

				double Importance = 0.5;
				(*MemObj)->TryGetNumberField(TEXT("importance"), Importance);

				TArray<FString> Tags;
				const TArray<TSharedPtr<FJsonValue>>* TagsArray = nullptr;
				if ((*MemObj)->TryGetArrayField(TEXT("tags"), TagsArray) && TagsArray)
				{
					for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
					{
						FString Tag;
						if (TagValue.IsValid() && TagValue->TryGetString(Tag))
						{
							Tags.Add(Tag);
						}
					}
				}

				MemoryComp->AppendMemory(MemoryComp->MakeMemory(EAgentMemoryType::Reflection, Text, static_cast<float>(Importance), Tags));
			}
		}
	}

	return Decision;
}
