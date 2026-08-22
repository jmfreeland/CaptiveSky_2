// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentRelationshipComponent.h"
#include "AgentMemoryComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentRelationship, Log, All);

UAgentRelationshipComponent::UAgentRelationshipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FString UAgentRelationshipComponent::GetRelationshipsFilePath() const
{
	if (const AActor* Owner = GetOwner())
	{
		if (const UAgentMemoryComponent* Memory = Owner->FindComponentByClass<UAgentMemoryComponent>())
		{
			return Memory->GetAgentDirectory() / TEXT("relationships.json");
		}
	}
	return FString();
}

void UAgentRelationshipComponent::EnsureLoaded() const
{
	if (bLoaded)
	{
		return;
	}
	bLoaded = true;
	Relationships.Reset();

	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *GetRelationshipsFilePath()))
	{
		return;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogAgentRelationship, Warning, TEXT("Could not parse relationship state: %s"), *GetRelationshipsFilePath());
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("relationships"), Entries) || !Entries)
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Entries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
		{
			continue;
		}
		FAgentRelationshipRecord Record;
		(*Object)->TryGetStringField(TEXT("agent_id"), Record.AgentId);
		(*Object)->TryGetStringField(TEXT("display_name"), Record.DisplayName);
		double InteractionCount = 0.0;
		(*Object)->TryGetNumberField(TEXT("interaction_count"), InteractionCount);
		Record.InteractionCount = static_cast<int32>(InteractionCount);
		double Familiarity = 0.0;
		(*Object)->TryGetNumberField(TEXT("familiarity"), Familiarity);
		Record.Familiarity = static_cast<float>(Familiarity);
		FString Timestamp;
		if ((*Object)->TryGetStringField(TEXT("first_interaction_at"), Timestamp))
		{
			FDateTime::ParseIso8601(*Timestamp, Record.FirstInteractionAt);
		}
		if ((*Object)->TryGetStringField(TEXT("last_interaction_at"), Timestamp))
		{
			FDateTime::ParseIso8601(*Timestamp, Record.LastInteractionAt);
		}
		const TArray<TSharedPtr<FJsonValue>>* Evidence = nullptr;
		if ((*Object)->TryGetArrayField(TEXT("recent_evidence"), Evidence) && Evidence)
		{
			for (const TSharedPtr<FJsonValue>& EvidenceValue : *Evidence)
			{
				FString EvidenceText;
				if (EvidenceValue.IsValid() && EvidenceValue->TryGetString(EvidenceText))
				{
					Record.RecentEvidence.Add(EvidenceText);
				}
			}
		}
		if (!Record.AgentId.IsEmpty())
		{
			Relationships.Add(Record.AgentId, MoveTemp(Record));
		}
	}
}

void UAgentRelationshipComponent::RecordInteraction(const FString& OtherAgentId, const FString& OtherDisplayName,
	const FString& Evidence, const FString& ConversationId)
{
	if (OtherAgentId.IsEmpty() || Evidence.IsEmpty())
	{
		return;
	}
	EnsureLoaded();
	FAgentRelationshipRecord& Record = Relationships.FindOrAdd(OtherAgentId);
	const FDateTime Now = FDateTime::UtcNow();
	if (Record.InteractionCount == 0)
	{
		Record.AgentId = OtherAgentId;
		Record.FirstInteractionAt = Now;
	}
	if (!OtherDisplayName.IsEmpty())
	{
		Record.DisplayName = OtherDisplayName;
	}
	++Record.InteractionCount;
	// Familiarity means exposure only, not liking or trust. It approaches 1 slowly and reversibly.
	Record.Familiarity = 1.f - FMath::Exp(-static_cast<float>(Record.InteractionCount) / 12.f);
	Record.LastInteractionAt = Now;
	Record.RecentEvidence.Add(FString::Printf(TEXT("[%s] %s (conversation %s)"),
		*Now.ToIso8601(), *Evidence.Left(500), ConversationId.IsEmpty() ? TEXT("unknown") : *ConversationId));
	while (Record.RecentEvidence.Num() > MaximumRecentEvidencePerBeing)
	{
		Record.RecentEvidence.RemoveAt(0);
	}
	Save();
}

bool UAgentRelationshipComponent::GetRelationship(const FString& OtherAgentId, FAgentRelationshipRecord& OutRelationship) const
{
	EnsureLoaded();
	if (const FAgentRelationshipRecord* Found = Relationships.Find(OtherAgentId))
	{
		OutRelationship = *Found;
		return true;
	}
	return false;
}

FString UAgentRelationshipComponent::BuildPromptSummary() const
{
	EnsureLoaded();
	if (Relationships.IsEmpty())
	{
		return TEXT("(no relationships have formed yet)");
	}
	TArray<FString> AgentIds;
	Relationships.GetKeys(AgentIds);
	AgentIds.Sort();
	FString Summary;
	for (const FString& AgentId : AgentIds)
	{
		const FAgentRelationshipRecord& Record = Relationships[AgentId];
		Summary += FString::Printf(TEXT("- %s (%s): %d recorded interaction(s), familiarity %.2f.\n"),
			Record.DisplayName.IsEmpty() ? *AgentId : *Record.DisplayName, *AgentId,
			Record.InteractionCount, Record.Familiarity);
		const int32 FirstEvidenceIndex = FMath::Max(0, Record.RecentEvidence.Num() - 3);
		for (int32 EvidenceIndex = FirstEvidenceIndex; EvidenceIndex < Record.RecentEvidence.Num(); ++EvidenceIndex)
		{
			Summary += TEXT("  - ") + Record.RecentEvidence[EvidenceIndex] + TEXT("\n");
		}
	}
	return Summary;
}

void UAgentRelationshipComponent::Save() const
{
	const FString Destination = GetRelationshipsFilePath();
	if (Destination.IsEmpty())
	{
		return;
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Destination), true);
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schema_version"), 1);
	TArray<TSharedPtr<FJsonValue>> Entries;
	TArray<FString> AgentIds;
	Relationships.GetKeys(AgentIds);
	AgentIds.Sort();
	for (const FString& AgentId : AgentIds)
	{
		const FAgentRelationshipRecord& Record = Relationships[AgentId];
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("agent_id"), Record.AgentId);
		Object->SetStringField(TEXT("display_name"), Record.DisplayName);
		Object->SetNumberField(TEXT("interaction_count"), Record.InteractionCount);
		Object->SetNumberField(TEXT("familiarity"), Record.Familiarity);
		Object->SetStringField(TEXT("first_interaction_at"), Record.FirstInteractionAt.ToIso8601());
		Object->SetStringField(TEXT("last_interaction_at"), Record.LastInteractionAt.ToIso8601());
		TArray<TSharedPtr<FJsonValue>> Evidence;
		for (const FString& Text : Record.RecentEvidence)
		{
			Evidence.Add(MakeShared<FJsonValueString>(Text));
		}
		Object->SetArrayField(TEXT("recent_evidence"), Evidence);
		Entries.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("relationships"), Entries);

	FString Json;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);
	const FString Temporary = Destination + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Json, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
		!IFileManager::Get().Move(*Destination, *Temporary, true, true, false, true))
	{
		UE_LOG(LogAgentRelationship, Error, TEXT("Could not save relationship state: %s"), *Destination);
		IFileManager::Get().Delete(*Temporary, false, true);
	}
}
