// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentMemoryTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

FString FAgentMemoryRecord::TypeToString(EAgentMemoryType InType)
{
	switch (InType)
	{
	case EAgentMemoryType::Observation: return TEXT("observation");
	case EAgentMemoryType::Conversation: return TEXT("conversation");
	case EAgentMemoryType::Decision: return TEXT("decision");
	case EAgentMemoryType::Reflection: return TEXT("reflection");
	default: return TEXT("observation");
	}
}

EAgentMemoryType FAgentMemoryRecord::TypeFromString(const FString& InString)
{
	if (InString == TEXT("conversation")) return EAgentMemoryType::Conversation;
	if (InString == TEXT("decision")) return EAgentMemoryType::Decision;
	if (InString == TEXT("reflection")) return EAgentMemoryType::Reflection;
	return EAgentMemoryType::Observation;
}

FString FAgentMemoryRecord::ToJsonLine() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("id"), Id);
	Root->SetStringField(TEXT("timestamp"), Timestamp.ToIso8601());
	Root->SetStringField(TEXT("type"), TypeToString(Type));
	Root->SetStringField(TEXT("text"), Text);
	Root->SetNumberField(TEXT("importance"), Importance);

	TArray<TSharedPtr<FJsonValue>> TagValues;
	for (const FString& Tag : Tags)
	{
		TagValues.Add(MakeShared<FJsonValueString>(Tag));
	}
	Root->SetArrayField(TEXT("tags"), TagValues);

	TSharedRef<FJsonObject> LocationObj = MakeShared<FJsonObject>();
	LocationObj->SetNumberField(TEXT("x"), Location.X);
	LocationObj->SetNumberField(TEXT("y"), Location.Y);
	LocationObj->SetNumberField(TEXT("z"), Location.Z);
	Root->SetObjectField(TEXT("location"), LocationObj);

	FString Out;
	// Condensed writer -> single line, which is what the .jsonl format requires.
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool FAgentMemoryRecord::FromJsonLine(const FString& Line, FAgentMemoryRecord& OutRecord)
{
	if (Line.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Line);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	OutRecord.Id = Root->GetStringField(TEXT("id"));

	FString TimestampStr;
	if (Root->TryGetStringField(TEXT("timestamp"), TimestampStr))
	{
		FDateTime::ParseIso8601(*TimestampStr, OutRecord.Timestamp);
	}

	FString TypeStr;
	Root->TryGetStringField(TEXT("type"), TypeStr);
	OutRecord.Type = TypeFromString(TypeStr);

	Root->TryGetStringField(TEXT("text"), OutRecord.Text);

	double ImportanceD = 0.5;
	Root->TryGetNumberField(TEXT("importance"), ImportanceD);
	OutRecord.Importance = static_cast<float>(ImportanceD);

	OutRecord.Tags.Reset();
	const TArray<TSharedPtr<FJsonValue>>* TagsArray = nullptr;
	if (Root->TryGetArrayField(TEXT("tags"), TagsArray) && TagsArray)
	{
		for (const TSharedPtr<FJsonValue>& Value : *TagsArray)
		{
			FString TagStr;
			if (Value.IsValid() && Value->TryGetString(TagStr))
			{
				OutRecord.Tags.Add(TagStr);
			}
		}
	}

	const TSharedPtr<FJsonObject>* LocationObj = nullptr;
	if (Root->TryGetObjectField(TEXT("location"), LocationObj) && LocationObj && LocationObj->IsValid())
	{
		double X = 0, Y = 0, Z = 0;
		(*LocationObj)->TryGetNumberField(TEXT("x"), X);
		(*LocationObj)->TryGetNumberField(TEXT("y"), Y);
		(*LocationObj)->TryGetNumberField(TEXT("z"), Z);
		OutRecord.Location = FVector(X, Y, Z);
	}

	return true;
}
