#include "IslandChronicle.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool UIslandChronicleSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UIslandChronicleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandChronicleSubsystem, STATGROUP_Tickables);
}

FString UIslandChronicleSubsystem::GetChroniclePath() const
{
	if (!ChronicleFileOverride.IsEmpty()) return ChronicleFileOverride;
	// Worlds created in code (test fixtures) get no chronicle, like they get no lasting state.
	if (!GetWorld() || GetWorld()->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"))) return FString();
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("WorldState") / TEXT("chronicle.jsonl"));
}

FString UIslandChronicleSubsystem::FormatEntry(const FDateTime& Utc, int32 Day, const FString& Clock, const FString& Type,
	const FString& Agent, const FString& Text, const TMap<FString, FString>& Extra)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Field : Extra) Root->SetStringField(Field.Key, Field.Value);
	Root->SetStringField(TEXT("t"), Utc.ToIso8601());
	Root->SetNumberField(TEXT("day"), Day);
	Root->SetStringField(TEXT("clock"), Clock);
	Root->SetStringField(TEXT("type"), Type);
	if (!Agent.IsEmpty()) Root->SetStringField(TEXT("agent"), Agent);
	Root->SetStringField(TEXT("text"), Text);
	FString Line;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Line);
	FJsonSerializer::Serialize(Root, Writer);
	return Line;
}

bool UIslandChronicleSubsystem::AppendLine(const FString& Line) const
{
	const FString Path = GetChroniclePath();
	if (Path.IsEmpty()) return false;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	return FFileHelper::SaveStringToFile(Line + LINE_TERMINATOR, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

void UIslandChronicleSubsystem::RefreshClock()
{
	const UWorld* World = GetWorld();
	if (!World) return;
	for (TActorIterator<AIslandDayNight> It(World); It; ++It)
	{
		LastDay = It->DayNumber;
		LastClock = FString::Printf(TEXT("%02d:%02d"), FMath::FloorToInt(It->CurrentHour), FMath::FloorToInt(FMath::Frac(It->CurrentHour) * 60.f));
		return;
	}
}

void UIslandChronicleSubsystem::RecordEntry(const FString& Type, const FString& Agent, const FString& Text, const TMap<FString, FString>& Extra)
{
	if (GetChroniclePath().IsEmpty()) return;
	if (!bClosing) RefreshClock();
	AppendLine(FormatEntry(FDateTime::UtcNow(), LastDay, LastClock, Type, Agent, Text, Extra));
}

void UIslandChronicleSubsystem::Record(const UWorld* World, const FString& Type, const FString& Agent, const FString& Text,
	const TMap<FString, FString>& Extra)
{
	if (UIslandChronicleSubsystem* Chronicle = World ? World->GetSubsystem<UIslandChronicleSubsystem>() : nullptr)
		Chronicle->RecordEntry(Type, Agent, Text, Extra);
}

void UIslandChronicleSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The session opens on the first tick, once the Island clock has restored the saved day and hour.
	SinceCheck = 5.f;
}

void UIslandChronicleSubsystem::Deinitialize()
{
	bClosing = true;
	if (bSessionOpened) RecordEntry(TEXT("session"), FString(), TEXT("The session ends."), { { TEXT("event"), TEXT("end") } });
	Super::Deinitialize();
}

void UIslandChronicleSubsystem::Tick(float DeltaTime)
{
	SinceCheck += DeltaTime;
	if (SinceCheck < 5.f) return;
	SinceCheck = 0.f;
	WatchSky();
}

void UIslandChronicleSubsystem::WatchSky()
{
	const UWorld* World = GetWorld();
	if (!World) return;
	const int32 PreviousDay = LastDay;
	RefreshClock();
	if (!bSessionOpened)
	{
		if (LastDay <= 0) return;
		bSessionOpened = true;
		RecordEntry(TEXT("session"), FString(), TEXT("The Island wakes; a session begins."), { { TEXT("event"), TEXT("start") } });
	}
	else if (LastDay != PreviousDay)
		RecordEntry(TEXT("day"), FString(), FString::Printf(TEXT("A new day begins on the Island: day %d."), LastDay), {});
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		const float Storm = It->SampleStormIntensity(World->GetTimeSeconds());
		const bool bWasActive = bStormActive;
		if (Storm >= StormArrives) bStormActive = true;
		else if (Storm < StormPasses) bStormActive = false;
		// A storm already raging when the session opens is not news; only changes after that are.
		if (bStormSeen && bStormActive != bWasActive)
			RecordEntry(TEXT("weather"), FString(), bStormActive ? TEXT("A storm arrives over the Island.") : TEXT("The storm passes."),
				{ { TEXT("event"), bStormActive ? TEXT("storm_start") : TEXT("storm_end") } });
		bStormSeen = true;
		return;
	}
}
