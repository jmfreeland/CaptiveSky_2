// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentConsolidationComponent.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "AgentRelationshipComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentConsolidation, Log, All);

namespace AgentConsolidationPrivate
{
	FString JsonToString(const TSharedRef<FJsonObject>& Object, const bool bPretty = false)
	{
		FString Result;
		if (bPretty)
		{
			TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
				TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Result);
			FJsonSerializer::Serialize(Object, Writer);
		}
		else
		{
			TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
				TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result);
			FJsonSerializer::Serialize(Object, Writer);
		}
		return Result;
	}

	FString StripCodeFence(FString Text)
	{
		Text.TrimStartAndEndInline();
		if (Text.StartsWith(TEXT("```")))
		{
			int32 FirstNewline = INDEX_NONE;
			Text.FindChar(TEXT('\n'), FirstNewline);
			if (FirstNewline != INDEX_NONE)
			{
				Text = Text.Mid(FirstNewline + 1);
			}
			const int32 ClosingFence = Text.Find(TEXT("```"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (ClosingFence != INDEX_NONE)
			{
				Text = Text.Left(ClosingFence);
			}
			Text.TrimStartAndEndInline();
		}
		return Text;
	}

	bool SaveAtomically(const FString& Path, const FString& Contents)
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		const FString TemporaryPath = Path + TEXT(".tmp");
		if (!FFileHelper::SaveStringToFile(Contents, *TemporaryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			return false;
		}
		return IFileManager::Get().Move(*Path, *TemporaryPath, true, true);
	}
}

UAgentConsolidationComponent::UAgentConsolidationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAgentConsolidationComponent::~UAgentConsolidationComponent() = default;

void UAgentConsolidationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	++SleepGeneration;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RestTimer);
	}
	Provider.Reset();
	Super::EndPlay(EndPlayReason);
}

void UAgentConsolidationComponent::SetConsciousState(const EAgentConsciousState NewState)
{
	if (ConsciousState == NewState)
	{
		return;
	}
	const EAgentConsciousState PreviousState = ConsciousState;
	ConsciousState = NewState;
	OnConsciousStateChanged.Broadcast(PreviousState, NewState);
}

bool UAgentConsolidationComponent::BeginSleep(float RestDurationSeconds)
{
	if (!IsAwake())
	{
		return false;
	}
	if (const UAgentBrainComponent* Brain = GetOwner() ? GetOwner()->FindComponentByClass<UAgentBrainComponent>() : nullptr;
		Brain && Brain->bRequestInFlight)
	{
		return false;
	}

	EnsureStateLoaded();
	++SleepGeneration;
	SetConsciousState(EAgentConsciousState::Resting);
	const float Duration = RestDurationSeconds < 0.f ? DefaultRestDurationSeconds : RestDurationSeconds;
	if (Duration <= 0.f)
	{
		StartConsolidation();
	}
	else if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(RestTimer, this,
			&UAgentConsolidationComponent::StartConsolidation, Duration, false);
	}
	return true;
}

void UAgentConsolidationComponent::WakeUp()
{
	++SleepGeneration;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RestTimer);
	}
	SetConsciousState(EAgentConsciousState::Awake);
}

void UAgentConsolidationComponent::StartConsolidation()
{
	if (ConsciousState != EAgentConsciousState::Resting)
	{
		return;
	}
	SetConsciousState(EAgentConsciousState::Consolidating);

	UAgentMemoryComponent* Memory = GetOwner() ? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	if (!Memory)
	{
		FinishSleep();
		return;
	}
	TArray<FAgentMemoryRecord> Memories = Memory->GetMemoriesSince(LastConsolidatedAt);
	if (Memories.Num() > MaximumMemoriesPerConsolidation)
	{
		Memories.RemoveAt(0, Memories.Num() - MaximumMemoriesPerConsolidation);
	}
	if (Memories.IsEmpty())
	{
		FinishSleep();
		return;
	}

	if (!Provider.IsValid())
	{
		Provider = CreateAgentLLMProvider();
	}
	FAgentLLMRequest Request;
	Request.SystemPrompt = BuildConsolidationPrompt(Memories);
	Request.MaxTokens = 1000;
	FAgentLLMMessage Message;
	Message.Text = TEXT("Reflect on the supplied memories now. Return only the requested JSON object.");
	Request.Messages.Add(Message);

	const int32 Generation = SleepGeneration;
	TWeakObjectPtr<UAgentConsolidationComponent> WeakThis(this);
	Provider->SendRequest(Request, FOnAgentLLMComplete::CreateLambda(
		[WeakThis, Memories, Generation](const FAgentLLMResult& Result)
		{
			UAgentConsolidationComponent* StrongThis = WeakThis.Get();
			if (!StrongThis || StrongThis->SleepGeneration != Generation ||
				StrongThis->ConsciousState != EAgentConsciousState::Consolidating)
			{
				return;
			}
			if (Result.bSuccess)
			{
				StrongThis->ApplyConsolidationResponse(Result.ResponseText, Memories);
			}
			else
			{
				UE_LOG(LogAgentConsolidation, Warning, TEXT("Sleep consolidation failed: %s"), *Result.ErrorMessage);
			}
			StrongThis->FinishSleep();
		}));
}

void UAgentConsolidationComponent::FinishSleep()
{
	SetConsciousState(EAgentConsciousState::Awake);
}

FString UAgentConsolidationComponent::GetPersonalityStatePath() const
{
	if (const UAgentMemoryComponent* Memory = GetOwner() ? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr)
	{
		return Memory->GetAgentDirectory() / TEXT("personality_evolution.json");
	}
	return FString();
}

FString UAgentConsolidationComponent::GetPersonalityHistoryPath() const
{
	if (const UAgentMemoryComponent* Memory = GetOwner() ? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr)
	{
		return Memory->GetAgentDirectory() / TEXT("personality_history.jsonl");
	}
	return FString();
}

void UAgentConsolidationComponent::EnsureStateLoaded()
{
	if (bStateLoaded)
	{
		return;
	}
	bStateLoaded = true;
	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *GetPersonalityStatePath()))
	{
		return;
	}
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(Contents), Root) || !Root.IsValid())
	{
		return;
	}
	double RevisionNumber = 0;
	Root->TryGetNumberField(TEXT("revision"), RevisionNumber);
	Revision = static_cast<int32>(RevisionNumber);
	FString Timestamp;
	if (Root->TryGetStringField(TEXT("last_consolidated_at"), Timestamp))
	{
		FDateTime::ParseIso8601(*Timestamp, LastConsolidatedAt);
	}
	const TArray<TSharedPtr<FJsonValue>>* TraitArray = nullptr;
	if (Root->TryGetArrayField(TEXT("tendencies"), TraitArray) && TraitArray)
	{
		for (const TSharedPtr<FJsonValue>& Value : *TraitArray)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
			{
				continue;
			}
			FAgentPersonalityTendency Tendency;
			(*Object)->TryGetStringField(TEXT("name"), Tendency.Name);
			double Strength = 0;
			(*Object)->TryGetNumberField(TEXT("strength"), Strength);
			Tendency.Strength = FMath::Clamp(static_cast<float>(Strength), -1.f, 1.f);
			(*Object)->TryGetStringField(TEXT("last_reason"), Tendency.LastReason);
			const TArray<TSharedPtr<FJsonValue>>* Evidence = nullptr;
			if ((*Object)->TryGetArrayField(TEXT("evidence_memory_ids"), Evidence) && Evidence)
			{
				for (const TSharedPtr<FJsonValue>& EvidenceValue : *Evidence)
				{
					FString Id;
					if (EvidenceValue.IsValid() && EvidenceValue->TryGetString(Id)) Tendency.EvidenceMemoryIds.Add(Id);
				}
			}
			if (!Tendency.Name.IsEmpty()) Tendencies.Add(Tendency);
		}
	}
}

TArray<FAgentPersonalityTendency> UAgentConsolidationComponent::GetEvolvingTendencies() const
{
	const_cast<UAgentConsolidationComponent*>(this)->EnsureStateLoaded();
	return Tendencies;
}

FString UAgentConsolidationComponent::BuildConsolidationPrompt(const TArray<FAgentMemoryRecord>& Memories) const
{
	FString Prompt = TEXT(
		"You are performing a sleeping agent's cautious memory consolidation. Foundational identity and personality are immutable here. "
		"Infer no trait without explicit evidence. Changes must be small, gradual, and may be negative. A young agent may remain unchanged.\n\n"
		"Return only JSON: {\"reflection\":\"brief first-person synthesis\",\"personality_adjustments\":["
		"{\"trait\":\"short neutral tendency name\",\"direction\":\"strengthen|soften\",\"amount\":0.0,"
		"\"reason\":\"evidence-based reason\",\"evidence_memory_ids\":[\"exact id\"]}]}\n"
		"Use an empty adjustment array unless repeated or unusually significant lived evidence supports a change.\n\nMemories:\n");
	for (const FAgentMemoryRecord& Memory : Memories)
	{
		Prompt += FString::Printf(TEXT("- id=%s time=%s importance=%.2f: %s\n"),
			*Memory.Id, *Memory.Timestamp.ToIso8601(), Memory.Importance, *Memory.Text);
	}
	if (const UAgentRelationshipComponent* Relationships = GetOwner() ? GetOwner()->FindComponentByClass<UAgentRelationshipComponent>() : nullptr)
	{
		Prompt += TEXT("\nFactual relationship exposure (not trust or affection):\n") + Relationships->BuildPromptSummary();
	}
	return Prompt;
}

bool UAgentConsolidationComponent::ApplyConsolidationResponse(const FString& ResponseText,
	const TArray<FAgentMemoryRecord>& Memories)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(
		AgentConsolidationPrivate::StripCodeFence(ResponseText)), Root) || !Root.IsValid())
	{
		UE_LOG(LogAgentConsolidation, Warning, TEXT("Consolidation response was not valid JSON."));
		return false;
	}
	TSet<FString> ValidEvidenceIds;
	for (const FAgentMemoryRecord& Memory : Memories) ValidEvidenceIds.Add(Memory.Id);

	FString Reflection;
	Root->TryGetStringField(TEXT("reflection"), Reflection);
	int32 AppliedCount = 0;
	const TArray<TSharedPtr<FJsonValue>>* Adjustments = nullptr;
	if (Root->TryGetArrayField(TEXT("personality_adjustments"), Adjustments) && Adjustments)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Adjustments)
		{
			const TSharedPtr<FJsonObject>* Adjustment = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Adjustment) || !Adjustment || !Adjustment->IsValid()) continue;
			FString Trait;
			FString Direction;
			FString Reason;
			double RequestedAmount = 0;
			(*Adjustment)->TryGetStringField(TEXT("trait"), Trait);
			(*Adjustment)->TryGetStringField(TEXT("direction"), Direction);
			(*Adjustment)->TryGetStringField(TEXT("reason"), Reason);
			(*Adjustment)->TryGetNumberField(TEXT("amount"), RequestedAmount);
			Trait.TrimStartAndEndInline();
			Direction.ToLowerInline();
			TArray<FString> EvidenceIds;
			const TArray<TSharedPtr<FJsonValue>>* Evidence = nullptr;
			if ((*Adjustment)->TryGetArrayField(TEXT("evidence_memory_ids"), Evidence) && Evidence)
			{
				for (const TSharedPtr<FJsonValue>& EvidenceValue : *Evidence)
				{
					FString Id;
					if (EvidenceValue.IsValid() && EvidenceValue->TryGetString(Id) && ValidEvidenceIds.Contains(Id)) EvidenceIds.AddUnique(Id);
				}
			}
			if (Trait.IsEmpty() || Trait.Len() > 64 || Reason.IsEmpty() || EvidenceIds.IsEmpty() ||
				(Direction != TEXT("strengthen") && Direction != TEXT("soften")) || RequestedAmount <= 0)
			{
				continue;
			}
			const float SignedAmount = FMath::Min(static_cast<float>(RequestedAmount), MaximumAdjustmentPerSleep) *
				(Direction == TEXT("strengthen") ? 1.f : -1.f);
			FAgentPersonalityTendency* Existing = Tendencies.FindByPredicate([&Trait](const FAgentPersonalityTendency& Item)
			{
				return Item.Name.Equals(Trait, ESearchCase::IgnoreCase);
			});
			if (!Existing)
			{
				FAgentPersonalityTendency NewTendency;
				NewTendency.Name = Trait;
				Tendencies.Add(NewTendency);
				Existing = &Tendencies.Last();
			}
			const float PreviousStrength = Existing->Strength;
			Existing->Strength = FMath::Clamp(PreviousStrength + SignedAmount, -1.f, 1.f);
			Existing->LastReason = Reason;
			Existing->EvidenceMemoryIds = EvidenceIds;
			AppendHistory(Existing->Name, PreviousStrength, Existing->Strength, Reason, EvidenceIds);
			++AppliedCount;
		}
	}

	++Revision;
	if (!Reflection.IsEmpty())
	{
		if (UAgentMemoryComponent* Memory = GetOwner() ? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr)
		{
			Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Reflection, Reflection, 0.65f,
				{ TEXT("sleep"), TEXT("consolidation") }));
		}
	}
	// Advance the watermark after writing the derived reflection. Otherwise that reflection is
	// newer than the watermark and can become the only "new experience" at the next sleep,
	// allowing personality evolution to recursively feed on its own previous synthesis.
	LastConsolidatedAt = FDateTime::UtcNow();
	UE_LOG(LogAgentConsolidation, Log, TEXT("Sleep consolidation applied %d personality adjustment(s)."), AppliedCount);
	return SaveState();
}

bool UAgentConsolidationComponent::SaveState() const
{
	const FString Path = GetPersonalityStatePath();
	if (Path.IsEmpty()) return false;
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schema_version"), 1);
	Root->SetNumberField(TEXT("revision"), Revision);
	Root->SetStringField(TEXT("last_consolidated_at"), LastConsolidatedAt.ToIso8601());
	Root->SetStringField(TEXT("note"), TEXT("Derived evidence-bound tendencies. Foundational identity and personality remain unchanged."));
	TArray<TSharedPtr<FJsonValue>> TraitArray;
	for (const FAgentPersonalityTendency& Tendency : Tendencies)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("name"), Tendency.Name);
		Object->SetNumberField(TEXT("strength"), Tendency.Strength);
		Object->SetStringField(TEXT("last_reason"), Tendency.LastReason);
		TArray<TSharedPtr<FJsonValue>> Evidence;
		for (const FString& Id : Tendency.EvidenceMemoryIds) Evidence.Add(MakeShared<FJsonValueString>(Id));
		Object->SetArrayField(TEXT("evidence_memory_ids"), Evidence);
		TraitArray.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("tendencies"), TraitArray);
	return AgentConsolidationPrivate::SaveAtomically(Path, AgentConsolidationPrivate::JsonToString(Root, true));
}

void UAgentConsolidationComponent::AppendHistory(const FString& TraitName, float PreviousStrength,
	float NewStrength, const FString& Reason, const TArray<FString>& EvidenceIds) const
{
	const FString Path = GetPersonalityHistoryPath();
	if (Path.IsEmpty()) return;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	TSharedRef<FJsonObject> Event = MakeShared<FJsonObject>();
	Event->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());
	Event->SetStringField(TEXT("trait"), TraitName);
	Event->SetNumberField(TEXT("previous_strength"), PreviousStrength);
	Event->SetNumberField(TEXT("new_strength"), NewStrength);
	Event->SetStringField(TEXT("reason"), Reason);
	TArray<TSharedPtr<FJsonValue>> Evidence;
	for (const FString& Id : EvidenceIds) Evidence.Add(MakeShared<FJsonValueString>(Id));
	Event->SetArrayField(TEXT("evidence_memory_ids"), Evidence);
	FFileHelper::SaveStringToFile(AgentConsolidationPrivate::JsonToString(Event) + LINE_TERMINATOR, *Path,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}
