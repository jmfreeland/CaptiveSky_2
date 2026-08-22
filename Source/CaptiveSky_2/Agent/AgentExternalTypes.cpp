// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentExternalTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FAgentExternalUtterance::FromJson(const FString& Json, FAgentExternalUtterance& OutUtterance)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	double SchemaVersion = 0.0;
	if (!Root->TryGetNumberField(TEXT("schema_version"), SchemaVersion) || static_cast<int32>(SchemaVersion) != 1)
	{
		return false;
	}

	Root->TryGetStringField(TEXT("turn_id"), OutUtterance.TurnId);
	Root->TryGetStringField(TEXT("agent_id"), OutUtterance.AgentId);
	Root->TryGetStringField(TEXT("source"), OutUtterance.Source);
	Root->TryGetStringField(TEXT("conversation_id"), OutUtterance.ConversationId);
	Root->TryGetStringField(TEXT("message_id"), OutUtterance.MessageId);
	Root->TryGetStringField(TEXT("text"), OutUtterance.Text);

	FString ReceivedAtString;
	if (Root->TryGetStringField(TEXT("received_at"), ReceivedAtString))
	{
		FDateTime::ParseIso8601(*ReceivedAtString, OutUtterance.ReceivedAt);
	}

	const TSharedPtr<FJsonObject>* Sender = nullptr;
	if (Root->TryGetObjectField(TEXT("sender"), Sender) && Sender && Sender->IsValid())
	{
		(*Sender)->TryGetStringField(TEXT("id"), OutUtterance.ParticipantId);
		(*Sender)->TryGetStringField(TEXT("display_name"), OutUtterance.ParticipantName);
	}

	return !OutUtterance.TurnId.IsEmpty() && !OutUtterance.AgentId.IsEmpty() &&
		!OutUtterance.Source.IsEmpty() && !OutUtterance.Text.IsEmpty();
}

FAgentConversationContext FAgentExternalUtterance::ToConversationContext() const
{
	FAgentConversationContext Context;
	Context.Text = Text;
	Context.Source = Source;
	Context.ConversationId = ConversationId;
	Context.MessageId = MessageId;
	Context.ParticipantId = ParticipantId;
	Context.ParticipantName = ParticipantName;
	Context.bExternal = true;
	return Context;
}

FString FAgentExternalResponse::ToJson() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schema_version"), 1);
	Root->SetStringField(TEXT("turn_id"), TurnId);
	Root->SetStringField(TEXT("agent_id"), AgentId);
	Root->SetStringField(TEXT("status"), Status);
	Root->SetStringField(TEXT("speech"), Speech);
	Root->SetStringField(TEXT("error"), Error);
	Root->SetStringField(TEXT("completed_at"), CompletedAt.ToIso8601());

	FString Json;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);
	return Json;
}
