// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentMemoryComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentMemory, Log, All);

UAgentMemoryComponent::UAgentMemoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAgentMemoryComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureLoaded();
}

FString UAgentMemoryComponent::ResolveAgentId() const
{
	if (!AgentId.IsEmpty())
	{
		return AgentId;
	}
	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetName();
	}
	return TEXT("UnknownAgent");
}

FString UAgentMemoryComponent::GetMemoryDir() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("AgentMemory") / ResolveAgentId());
}

FString UAgentMemoryComponent::GetMemoryFilePath() const
{
	return GetMemoryDir() / TEXT("memory.jsonl");
}

void UAgentMemoryComponent::EnsureLoaded() const
{
	if (bLoaded)
	{
		return;
	}
	bLoaded = true;

	const FString FilePath = GetMemoryFilePath();
	if (!FPaths::FileExists(FilePath))
	{
		return;
	}

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
	{
		UE_LOG(LogAgentMemory, Warning, TEXT("Failed to read memory file: %s"), *FilePath);
		return;
	}

	Cache.Reset();
	for (const FString& Line : Lines)
	{
		FAgentMemoryRecord Record;
		if (FAgentMemoryRecord::FromJsonLine(Line, Record))
		{
			Cache.Add(Record);
		}
	}

	UE_LOG(LogAgentMemory, Log, TEXT("Loaded %d memory record(s) from %s"), Cache.Num(), *FilePath);
}

FAgentMemoryRecord UAgentMemoryComponent::MakeMemory(EAgentMemoryType Type, const FString& Text, float Importance, const TArray<FString>& Tags) const
{
	FAgentMemoryRecord Record;
	Record.Id = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	Record.Timestamp = FDateTime::UtcNow();
	Record.Type = Type;
	Record.Text = Text;
	Record.Importance = FMath::Clamp(Importance, 0.f, 1.f);
	Record.Tags = Tags;
	if (const AActor* Owner = GetOwner())
	{
		Record.Location = Owner->GetActorLocation();
	}
	return Record;
}

void UAgentMemoryComponent::AppendMemory(const FAgentMemoryRecord& Record)
{
	EnsureLoaded();

	const FString Dir = GetMemoryDir();
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString Line = Record.ToJsonLine() + LINE_TERMINATOR;
	if (!FFileHelper::SaveStringToFile(Line, *GetMemoryFilePath(),
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append))
	{
		UE_LOG(LogAgentMemory, Error, TEXT("Failed to append memory record to %s"), *GetMemoryFilePath());
		return;
	}

	Cache.Add(Record);
}

int32 UAgentMemoryComponent::GetMemoryCount() const
{
	EnsureLoaded();
	return Cache.Num();
}

static void TokenizeLower(const FString& In, TArray<FString>& OutWords)
{
	FString Cleaned = In.ToLower();
	for (TCHAR& Ch : Cleaned)
	{
		if (!FChar::IsAlnum(Ch))
		{
			Ch = TEXT(' ');
		}
	}
	Cleaned.ParseIntoArrayWS(OutWords);
}

float UAgentMemoryComponent::ScoreRecord(const FAgentMemoryRecord& Record, const TArray<FString>& SituationWords, float HalfLifeHours)
{
	// Recency: exponential decay against the configured half-life.
	const double AgeHours = (FDateTime::UtcNow() - Record.Timestamp).GetTotalHours();
	const float RecencyScore = FMath::Exp(-static_cast<float>(FMath::Max(AgeHours, 0.0)) / FMath::Max(HalfLifeHours, 0.01f));

	// Keyword overlap: fraction of situation words that appear in this record's text/tags.
	float OverlapScore = 0.f;
	if (SituationWords.Num() > 0)
	{
		TArray<FString> RecordWords;
		TokenizeLower(Record.Text, RecordWords);
		for (const FString& Tag : Record.Tags)
		{
			RecordWords.Add(Tag.ToLower());
		}

		int32 Matches = 0;
		for (const FString& Word : SituationWords)
		{
			if (RecordWords.Contains(Word))
			{
				++Matches;
			}
		}
		OverlapScore = static_cast<float>(Matches) / static_cast<float>(SituationWords.Num());
	}

	// Weights are a deliberately simple, documented starting point -- tune freely.
	return 0.4f * RecencyScore + 0.4f * Record.Importance + 0.2f * OverlapScore;
}

TArray<FAgentMemoryRecord> UAgentMemoryComponent::GetRelevantContext(int32 MaxTokens, const FString& Situation) const
{
	EnsureLoaded();

	TArray<FString> SituationWords;
	TokenizeLower(Situation, SituationWords);

	TArray<FAgentMemoryRecord> Scored = Cache;
	for (FAgentMemoryRecord& Record : Scored)
	{
		Record.RelevanceScore = ScoreRecord(Record, SituationWords, RecencyHalfLifeHours);
	}

	Scored.Sort([](const FAgentMemoryRecord& A, const FAgentMemoryRecord& B)
	{
		return A.RelevanceScore > B.RelevanceScore;
	});

	TArray<FAgentMemoryRecord> Result;
	int32 RunningChars = 0;
	const int32 CharBudget = MaxTokens * 4; // rough chars-per-token heuristic, no tokenizer dependency

	for (const FAgentMemoryRecord& Record : Scored)
	{
		const int32 RecordChars = Record.Text.Len() + 16;
		if (Result.Num() > 0 && RunningChars + RecordChars > CharBudget)
		{
			break;
		}
		Result.Add(Record);
		RunningChars += RecordChars;
	}

	return Result;
}
