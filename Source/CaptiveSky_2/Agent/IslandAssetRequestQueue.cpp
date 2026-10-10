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

		bool SplitJsonObjects(const FString& Text, TArray<FString>& OutObjects)
		{
			OutObjects.Reset();
			FString Current;
			int32 BraceDepth = 0;
			bool bInString = false;
			bool bEscaped = false;
			for (const TCHAR Character : Text)
			{
				if (BraceDepth == 0)
				{
					if (FChar::IsWhitespace(Character)) continue;
					if (Character != TEXT('{')) return false;
					Current.Reset();
					Current.AppendChar(Character);
					BraceDepth = 1;
					continue;
				}

				Current.AppendChar(Character);
				if (bInString)
				{
					if (bEscaped) bEscaped = false;
					else if (Character == TEXT('\\')) bEscaped = true;
					else if (Character == TEXT('"')) bInString = false;
					continue;
				}

				if (Character == TEXT('"')) bInString = true;
				else if (Character == TEXT('{')) ++BraceDepth;
				else if (Character == TEXT('}') && --BraceDepth == 0) OutObjects.Add(Current);
			}
			return BraceDepth == 0 && !bInString;
		}

		bool ReadPendingRequests(const FString& Path, int32& OutCount, FString& OutError,
			const FString& Requester, const FString& Description, const FString& RequestType,
			const FString& Target, const FString& UpgradeKind)
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

			TArray<FString> Records;
			if (!SplitJsonObjects(Existing, Records))
			{
				OutError = TEXT("The asset request inbox contains an unreadable record; no request was added.");
				return false;
			}
			for (const FString& Line : Records)
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
				FString ExistingType;
				FString ExistingTarget;
				FString ExistingKind;
				Record->TryGetStringField(TEXT("requester"), ExistingRequester);
				Record->TryGetStringField(TEXT("description"), ExistingDescription);
				Record->TryGetStringField(TEXT("request_type"), ExistingType);
				Record->TryGetStringField(TEXT("target"), ExistingTarget);
				Record->TryGetStringField(TEXT("upgrade_kind"), ExistingKind);
				if (ExistingType.IsEmpty()) ExistingType = TEXT("new_object");
				if (ExistingRequester == Requester && ExistingDescription.TrimStartAndEnd().Equals(Description, ESearchCase::IgnoreCase) &&
					ExistingType == RequestType && ExistingTarget == Target && ExistingKind == UpgradeKind)
				{
					OutError = TEXT("You already have this proposal awaiting review; nothing was added.");
					return false;
				}
			}
			return true;
		}
	}

	bool AppendRequest(const FString& Requester, const FString& Description,
		FString& OutRequestId, FString& OutError, const FString& InboxPathOverride,
		const FString& RequestType, const FString& Target, const FString& UpgradeKind)
	{
		OutRequestId.Reset();
		OutError.Reset();

		const FString CleanDescription = Description.TrimStartAndEnd();
		if (Requester.IsEmpty() || CleanDescription.IsEmpty() || CleanDescription.Len() > MaxDescriptionCharacters)
		{
			OutError = FString::Printf(TEXT("A world proposal needs a description of 1 to %d characters."), MaxDescriptionCharacters);
			return false;
		}
		for (const TCHAR Character : CleanDescription)
		{
			if (FChar::IsControl(Character))
			{
				OutError = TEXT("World proposals must be a single line of plain text.");
				return false;
			}
		}
		if (RequestType != TEXT("new_object") && RequestType != TEXT("upgrade"))
		{
			OutError = TEXT("The proposal type is not supported.");
			return false;
		}
		if (Target.Len() > 96 || Target.Contains(TEXT("\n")) || Target.Contains(TEXT("\r")))
		{
			OutError = TEXT("The target must be a short, single-line identifier.");
			return false;
		}
		if (RequestType == TEXT("upgrade") && UpgradeKind != TEXT("aesthetic") && UpgradeKind != TEXT("variation") && UpgradeKind != TEXT("functionality"))
		{
			OutError = TEXT("An upgrade proposal must be aesthetic, variation, or functionality.");
			return false;
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
		if (!ReadPendingRequests(Path, PendingCount, OutError, Requester, CleanDescription, RequestType, Target, UpgradeKind))
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
		Record->SetStringField(TEXT("request_type"), RequestType);
		if (!Target.IsEmpty()) Record->SetStringField(TEXT("target"), Target);
		if (!UpgradeKind.IsEmpty()) Record->SetStringField(TEXT("upgrade_kind"), UpgradeKind);
		Record->SetStringField(TEXT("pipeline"), RequestType == TEXT("new_object") ? TEXT("ComfyBlender") : TEXT("human_review"));
		Record->SetStringField(TEXT("status"), TEXT("pending_review"));

		FString JsonLine;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonLine);
		if (!FJsonSerializer::Serialize(Record, Writer))
		{
			OutError = TEXT("The proposal could not be serialized; nothing was added.");
			return false;
		}
		JsonLine.AppendChar(TEXT('\n'));
		if (!FFileHelper::SaveStringToFile(JsonLine, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(), EFileWrite::FILEWRITE_Append))
		{
			OutRequestId.Reset();
			OutError = TEXT("The proposal could not be written to the review inbox.");
			return false;
		}
		return true;
	}
}
