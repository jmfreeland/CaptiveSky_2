// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentMemoryComponent.h"
#include "AgentDataPaths.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentMemory, Log, All);

static void TokenizeLower(const FString& In, TArray<FString>& OutWords);

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

FString UAgentMemoryComponent::GetAgentDirectory() const
{
	return CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / ResolveAgentId());
}

FString UAgentMemoryComponent::GetResolvedAgentId() const
{
	return ResolveAgentId();
}

FString UAgentMemoryComponent::GetMemoryFilePath() const
{
	return GetAgentDirectory() / TEXT("memory.jsonl");
}

FString UAgentMemoryComponent::GetLegacyMemoryFilePath() const
{
	return CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("AgentMemory") / ResolveAgentId() / TEXT("memory.jsonl"));
}

FString UAgentMemoryComponent::LoadAgentDocument(const FString& FileName) const
{
	// Agent documents are deliberately flat: reject paths so callers cannot escape the agent home.
	if (FileName.IsEmpty() || FileName.Contains(TEXT("/")) || FileName.Contains(TEXT("\\")) || FileName.Contains(TEXT("..")))
	{
		UE_LOG(LogAgentMemory, Warning, TEXT("Rejected invalid agent document name: %s"), *FileName);
		return FString();
	}

	FString Contents;
	const FString AuthoredAgentDirectory = FPaths::ProjectDir() / TEXT("Agents") / ResolveAgentId();
	FFileHelper::LoadFileToString(Contents, *(AuthoredAgentDirectory / FileName));
	return Contents;
}

void UAgentMemoryComponent::EnsureLoaded() const
{
	if (bLoaded)
	{
		return;
	}
	bLoaded = true;

	FString FilePath = GetMemoryFilePath();
	if (!FPaths::FileExists(FilePath))
	{
		// Read legacy memories in place. The next append writes to the new agent home;
		// operators can then remove the old file after confirming the migration.
		FilePath = GetLegacyMemoryFilePath();
		if (!FPaths::FileExists(FilePath))
		{
			return;
		}
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

	const FString Dir = GetAgentDirectory();
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString Line = Record.ToJsonLine() + LINE_TERMINATOR;
	if (!FFileHelper::SaveStringToFile(Line, *GetMemoryFilePath(),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
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

TArray<FAgentMemoryRecord> UAgentMemoryComponent::GetMemoriesSince(const FDateTime& SinceUtc) const
{
	EnsureLoaded();
	TArray<FAgentMemoryRecord> Result;
	for (const FAgentMemoryRecord& Record : Cache)
	{
		if (Record.Timestamp >= SinceUtc)
		{
			Result.Add(Record);
		}
	}
	Result.Sort([](const FAgentMemoryRecord& A, const FAgentMemoryRecord& B)
	{
		return A.Timestamp < B.Timestamp;
	});
	return Result;
}

bool UAgentMemoryComponent::HasSimilarMemorySince(EAgentMemoryType Type, const FString& Text, const FDateTime& SinceUtc, float MinimumSimilarity) const
{
	if (Text.IsEmpty()) return false;
	EnsureLoaded();
	const TSet<FString> CandidateWords = [&Text]()
	{
		TArray<FString> Words;
		TokenizeLower(Text, Words);
		TSet<FString> Unique;
		for (const FString& Word : Words) Unique.Add(Word);
		return Unique;
	}();
	if (CandidateWords.IsEmpty()) return false;

	for (const FAgentMemoryRecord& Record : Cache)
	{
		if (Record.Type != Type || Record.Timestamp < SinceUtc) continue;
		TArray<FString> Words;
		TokenizeLower(Record.Text, Words);
		TSet<FString> RecordWords;
		for (const FString& Word : Words) RecordWords.Add(Word);
		if (RecordWords.IsEmpty()) continue;

		int32 Intersection = 0;
		for (const FString& Word : CandidateWords)
			if (RecordWords.Contains(Word)) ++Intersection;
		const int32 Union = CandidateWords.Num() + RecordWords.Num() - Intersection;
		const float Similarity = Union > 0 ? static_cast<float>(Intersection) / Union : 0.f;
		if (Similarity >= FMath::Clamp(MinimumSimilarity, 0.f, 1.f)) return true;
	}
	return false;
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

static bool IsMeaningfulContextWord(const FString& Word)
{
	if (Word.Len() < 3) return false;
	static const TSet<FString> StopWords = {
		TEXT("about"), TEXT("after"), TEXT("again"), TEXT("also"), TEXT("among"), TEXT("and"), TEXT("are"), TEXT("because"), TEXT("before"),
		TEXT("being"), TEXT("but"), TEXT("can"), TEXT("could"), TEXT("does"), TEXT("each"), TEXT("for"), TEXT("from"), TEXT("have"), TEXT("into"), TEXT("just"),
		TEXT("most"), TEXT("much"), TEXT("only"), TEXT("other"), TEXT("over"), TEXT("same"), TEXT("some"), TEXT("such"),
		TEXT("than"), TEXT("that"), TEXT("their"), TEXT("them"), TEXT("then"), TEXT("there"), TEXT("these"), TEXT("they"),
		TEXT("this"), TEXT("those"), TEXT("the"), TEXT("through"), TEXT("under"), TEXT("very"), TEXT("was"), TEXT("were"), TEXT("what"), TEXT("when"),
		TEXT("where"), TEXT("which"), TEXT("while"), TEXT("will"), TEXT("with"), TEXT("would"), TEXT("your"), TEXT("you"), TEXT("not")
	};
	return !StopWords.Contains(Word);
}

float UAgentMemoryComponent::ScoreRecord(const FAgentMemoryRecord& Record, const TArray<FString>& SituationWords,
	const TSet<FString>& SituationContentWords, float HalfLifeHours)
{
	// Recency: exponential decay against the configured half-life.
	const double AgeHours = (FDateTime::UtcNow() - Record.Timestamp).GetTotalHours();
	const float RecencyScore = FMath::Exp(-static_cast<float>(FMath::Max(AgeHours, 0.0)) / FMath::Max(HalfLifeHours, 0.01f));
	TArray<FString> RecordContentTokens;
	TokenizeLower(Record.Text, RecordContentTokens);
	TArray<FString> RecordWords = RecordContentTokens;
	for (const FString& Tag : Record.Tags)
	{
		// Preserve the existing keyword-match behavior for tags as whole strings.
		RecordWords.Add(Tag.ToLower());
		TArray<FString> TagWords;
		TokenizeLower(Tag, TagWords);
		RecordContentTokens.Append(TagWords);
	}

	// Keyword overlap: fraction of situation words that appear in this record's text/tags.
	float OverlapScore = 0.f;
	if (SituationWords.Num() > 0)
	{
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

	// Recent dialogue can otherwise crowd out an older, specific memory even when
	// the current scene clearly returns to that topic. A two-term content match is
	// a stronger cue than incidental overlap on articles or common verbs; it earns
	// a recency-independent boost without turning one shared word into a goal.
	TSet<FString> RecordContentWords;
	for (const FString& Word : RecordContentTokens)
		if (IsMeaningfulContextWord(Word)) RecordContentWords.Add(Word);
	int32 DistinctContentMatches = 0;
	for (const FString& Word : SituationContentWords)
		if (RecordContentWords.Contains(Word)) ++DistinctContentMatches;
	const float ContextAnchorBonus = DistinctContentMatches >= 2 ? 0.5f : 0.f;

	// Weights are a deliberately simple, documented starting point -- tune freely.
	return FMath::Min(1.f, 0.4f * RecencyScore + 0.4f * Record.Importance + 0.2f * OverlapScore + ContextAnchorBonus);
}

TArray<FAgentMemoryRecord> UAgentMemoryComponent::GetRelevantContext(int32 MaxTokens, const FString& Situation) const
{
	EnsureLoaded();

	TArray<FString> SituationWords;
	TokenizeLower(Situation, SituationWords);
	TSet<FString> SituationContentWords;
	for (const FString& Word : SituationWords)
		if (IsMeaningfulContextWord(Word)) SituationContentWords.Add(Word);

	TArray<FAgentMemoryRecord> Scored = Cache;
	for (FAgentMemoryRecord& Record : Scored)
	{
		Record.RelevanceScore = ScoreRecord(Record, SituationWords, SituationContentWords, RecencyHalfLifeHours);
	}

	Scored.Sort([](const FAgentMemoryRecord& A, const FAgentMemoryRecord& B)
	{
		return A.RelevanceScore > B.RelevanceScore;
	});

	TArray<FAgentMemoryRecord> NonConversation;
	TArray<FAgentMemoryRecord> Conversations;
	for (const FAgentMemoryRecord& Record : Scored)
	{
		(Record.Type == EAgentMemoryType::Conversation ? Conversations : NonConversation).Add(Record);
	}

	TArray<FAgentMemoryRecord> Result;
	TArray<TSet<FString>> ChosenWordSets;
	int32 RunningChars = 0;
	int32 NonConversationCount = 0;
	int32 ConversationCount = 0;
	const int32 CharBudget = MaxTokens * 4; // rough chars-per-token heuristic, no tokenizer dependency
	const int32 NonConversationBudget = CharBudget - CharBudget / 3; // leave room for a small amount of dialogue

	auto TryAdd = [&](const FAgentMemoryRecord& Record, int32 CharacterLimit)
	{
		TArray<FString> Words;
		TokenizeLower(Record.Text, Words);
		TSet<FString> UniqueWords;
		for (const FString& Word : Words) UniqueWords.Add(Word);
		if (!UniqueWords.IsEmpty())
		{
			for (const TSet<FString>& ExistingWords : ChosenWordSets)
			{
				int32 Intersection = 0;
				for (const FString& Word : UniqueWords)
					if (ExistingWords.Contains(Word)) ++Intersection;
				const int32 Union = UniqueWords.Num() + ExistingWords.Num() - Intersection;
				if (Union > 0 && static_cast<float>(Intersection) / Union >= 0.6f) return false;
			}
		}

		const int32 RecordChars = Record.Text.Len() + 16;
		if (Result.Num() > 0 && RunningChars + RecordChars > CharacterLimit) return false;
		Result.Add(Record);
		ChosenWordSets.Add(MoveTemp(UniqueWords));
		RunningChars += RecordChars;
		return true;
	};

	// Prioritize distinct lived experience, then reserve no more than one dialogue line per two other records.
	for (const FAgentMemoryRecord& Record : NonConversation)
		if (TryAdd(Record, NonConversationBudget)) ++NonConversationCount;

	const int32 ConversationLimit = NonConversationCount / 2;
	for (const FAgentMemoryRecord& Record : Conversations)
	{
		if (ConversationCount >= ConversationLimit) break;
		if (TryAdd(Record, CharBudget)) ++ConversationCount;
	}

	// Reuse spare budget for additional non-dialogue memories when dialogue was sparse or did not fit.
	for (const FAgentMemoryRecord& Record : NonConversation)
	{
		if (RunningChars >= CharBudget) break;
		if (TryAdd(Record, CharBudget)) ++NonConversationCount;
	}

	// Conversation-only histories still need a usable context when no other kind is available.
	if (Result.IsEmpty() && !Conversations.IsEmpty()) TryAdd(Conversations[0], CharBudget);

	Result.Sort([](const FAgentMemoryRecord& A, const FAgentMemoryRecord& B)
	{
		return A.RelevanceScore > B.RelevanceScore;
	});
	return Result;
}
