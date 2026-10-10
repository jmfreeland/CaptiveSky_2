// Copyright Epic Games, Inc. All Rights Reserved.

#include "Agent/IslandAssetRequestQueue.h"
#include "Agent/AgentDataPaths.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace IslandAssetRequestQueue
{
	namespace
	{
		constexpr int32 MaxDescriptionCharacters = 240;
		constexpr int32 MaxPendingRequests = 64;

		bool ReadPendingRequests(const FString& Path, int32& OutCount, FString& OutError,
			const FString& Requester, const FString& Description)
		{
			OutCount = 0;
			FString Existing;
			if (!FFileHelper::LoadFileToString(Existing, *Path))
			{
				if (!IFileManager::Get().FileExists(*Path))
				{
					return true;
				}
				OutError = TEXT("The asset request inbox could not be read; no request was added.");
				return false;
			}

			TArray<FString> Lines;
			Existing.ParseIntoArrayLines(Lines, false);
			for (const FString& Line : Lines)
			{
				TSharedPtr<FJsonObject> Record;
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);
				if (!FJsonSerializer::Deserialize(Reader, Record) || !Record.IsValid())
				{
					OutError = TEXT("The asset request inbox contains an unreadable record; no request was added.");
					return false;
				}

				FString Status;
				Record->TryGetStringField(TEXT("status"), Status);
				if (Status != TEXT("pending_review"))
				{
					continue;
				}
				++OutCount;

				FString ExistingRequester;
				FString ExistingDescription;
				Record->TryGetStringField(TEXT("requester"), ExistingRequester);
				Record->TryGetStringField(TEXT("description"), ExistingDescription);
				if (ExistingRequester == Requester && ExistingDescription.TrimStartAndEnd().Equals(Description, ESearchCase::IgnoreCase))
				{
					OutError = TEXT("You already have this object request awaiting review; nothing was added.");
					return false;
				}
			}
			return true;
		}
	}

	bool AppendRequest(const FString& Requester, const FString& Description,
		FString& OutRequestId, FString& OutError, const FString& InboxPathOverride)
	{
		OutRequestId.Reset();
		OutError.Reset();

		const FString CleanDescription = Description.TrimStartAndEnd();
		if (Requester.IsEmpty() || CleanDescription.IsEmpty() || CleanDescription.Len() > MaxDescriptionCharacters)
		{
			OutError = FString::Printf(TEXT("An object request needs a description of 1 to %d characters."), MaxDescriptionCharacters);
			return false;
		}
		for (const TCHAR Character : CleanDescription)
		{
			if (FChar::IsControl(Character))
			{
				OutError = TEXT("Object requests must be a single line of plain text.");
				return false;
			}
		}

		const FString Path = InboxPathOverride.IsEmpty()
			? CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl"))
			: InboxPathOverride;
		const FString Directory = FPaths::GetPath(Path);
		if (!IFileManager::Get().MakeDirectory(*Directory, true))
		{
			OutError = TEXT("The asset request inbox could not be created; no request was added.");
			return false;
		}

		int32 PendingCount = 0;
		if (!ReadPendingRequests(Path, PendingCount, OutError, Requester, CleanDescription))
		{
			return false;
		}
		if (PendingCount >= MaxPendingRequests)
		{
			OutError = TEXT("The human-review asset request inbox is full; no request was added.");
			return false;
		}

		OutRequestId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
		TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
		Record->SetStringField(TEXT("id"), OutRequestId);
		Record->SetStringField(TEXT("created_at_utc"), FDateTime::UtcNow().ToIso8601());
		Record->SetStringField(TEXT("requester"), Requester);
		Record->SetStringField(TEXT("description"), CleanDescription);
		Record->SetStringField(TEXT("pipeline"), TEXT("ComfyBlender"));
		Record->SetStringField(TEXT("status"), TEXT("pending_review"));

		FString JsonLine;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonLine);
		if (!FJsonSerializer::Serialize(Record, Writer))
		{
			OutError = TEXT("The object request could not be serialized; no request was added.");
			return false;
		}
		JsonLine.AppendChar(TEXT('\n'));
		if (!FFileHelper::SaveStringToFile(JsonLine, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), EFileWrite::FILEWRITE_Append))
		{
			OutRequestId.Reset();
			OutError = TEXT("The object request could not be written to the review inbox.");
			return false;
		}
		return true;
	}
}
