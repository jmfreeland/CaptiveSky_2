#include "IslandChronicle.h"
#include "AgentDataPaths.h"
#include "IslandDayNight.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandLightning.h"
#include "IslandRainbow.h"
#include "IslandSeaStateSubsystem.h"
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
	return CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("WorldState/chronicle.jsonl"));
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

FString UIslandChronicleSubsystem::MoonChangeText(const FString& PreviousPhase, const FString& CurrentPhase)
{
	if (PreviousPhase.IsEmpty() || CurrentPhase.IsEmpty() || PreviousPhase == CurrentPhase) return FString();
	// New and full moons bring the spring tides; the quarters bring the neaps.
	const TCHAR* Tides = CurrentPhase == TEXT("new moon") || CurrentPhase == TEXT("full moon") ? TEXT(" Spring tides are due: the sea will run highest and lowest.") :
		CurrentPhase == TEXT("first quarter") || CurrentPhase == TEXT("last quarter") ? TEXT(" Neap tides are due: little will separate high water from low.") : TEXT("");
	return FString::Printf(TEXT("The moon has moved on from the %s to the %s.%s"), *PreviousPhase, *CurrentPhase, Tides);
}

FString UIslandChronicleSubsystem::StormEndText(int32 Strikes)
{
	if (Strikes <= 0) return TEXT("The storm passes.");
	if (Strikes == 1) return TEXT("The storm passes, having thrown a single bolt of lightning.");
	return FString::Printf(TEXT("The storm passes, having thrown %d bolts of lightning."), Strikes);
}

bool UIslandChronicleSubsystem::ShouldNoteMeteor(bool bMeteorNow, int32 IslandDay, int32 LastNotedDay)
{
	return bMeteorNow && IslandDay > 0 && IslandDay != LastNotedDay;
}

bool UIslandChronicleSubsystem::NextRainbowShowing(bool bShowing, float Strength)
{
	if (!FMath::IsFinite(Strength)) return bShowing;
	if (bShowing) return Strength >= 0.1f;
	return Strength >= 0.3f;
}

bool UIslandChronicleSubsystem::NextThickMist(bool bThick, float Mist)
{
	if (!FMath::IsFinite(Mist)) return bThick;
	return bThick ? Mist >= 0.4f : Mist > 0.65f;
}

bool UIslandChronicleSubsystem::NextHeavySea(bool bHeavy, float Sea)
{
	if (!FMath::IsFinite(Sea)) return bHeavy;
	return bHeavy ? Sea >= 0.5f : Sea >= 0.7f;
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
		LastMoonPhase = AIslandDayNight::LunarPhaseName(AIslandDayNight::LunarPhaseProgress(LastDay, 0.f));
		RecordEntry(TEXT("session"), FString(), TEXT("The Island wakes; a session begins."), { { TEXT("event"), TEXT("start") } });
	}
	else if (LastDay != PreviousDay)
	{
		RecordEntry(TEXT("day"), FString(), FString::Printf(TEXT("A new day begins on the Island: day %d."), LastDay), {});
		const FString MoonNow = AIslandDayNight::LunarPhaseName(AIslandDayNight::LunarPhaseProgress(LastDay, 0.f));
		const FString MoonText = MoonChangeText(LastMoonPhase, MoonNow);
		if (!MoonText.IsEmpty()) RecordEntry(TEXT("sky"), FString(), MoonText, { { TEXT("event"), TEXT("moon_phase") } });
		LastMoonPhase = MoonNow;
	}
	// A strike restarts the lightning actor's clock, so the elapsed time dropping means a new bolt since the last look.
	for (TActorIterator<AIslandLightning> It(World); It; ++It)
	{
		const float Elapsed = It->GetElapsedSeconds();
		if (Elapsed < LastStrikeElapsed && bStormActive) ++StormStrikes;
		LastStrikeElapsed = Elapsed;
		break;
	}
	if (const UIslandRainbowSubsystem* Rainbow = World->GetSubsystem<UIslandRainbowSubsystem>())
	{
		const bool bShowing = NextRainbowShowing(bRainbowShowing, Rainbow->GetStrength());
		// Like the storm, a rainbow already arching when the session opens is not news.
		if (bSessionOpened && bShowing && !bRainbowShowing)
			RecordEntry(TEXT("weather"), FString(), TEXT("A rainbow arches over the Island."), { { TEXT("event"), TEXT("rainbow") } });
		bRainbowShowing = bShowing;
	}
	if (const UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>())
	{
		const bool bThick = NextThickMist(bThickMist, Environment->GetMist());
		if (bSessionOpened && bThickMist != bThick)
			RecordEntry(TEXT("weather"), FString(), bThick ? TEXT("A thick mist settles over the Island.") : TEXT("The mist lifts."),
				{ { TEXT("event"), bThick ? TEXT("mist_start") : TEXT("mist_end") } });
		bThickMist = bThick;
	}
	if (const UIslandSeaStateSubsystem* SeaState = World->GetSubsystem<UIslandSeaStateSubsystem>())
	{
		const bool bHeavy = NextHeavySea(bHeavySea, SeaState->GetSea());
		if (bSessionOpened && bHeavySea != bHeavy)
			RecordEntry(TEXT("weather"), FString(), bHeavy ? TEXT("The sea turns heavy, breaking steeply on the shore.") : TEXT("The sea settles."),
				{ { TEXT("event"), bHeavy ? TEXT("sea_heavy") : TEXT("sea_settled") } });
		bHeavySea = bHeavy;
	}
	// A shooting star is the Island's rarest sky event: note the first one each night.
	for (TActorIterator<AIslandDayNight> It(World); It; ++It)
	{
		float Cloud = 0.f;
		for (TActorIterator<AIslandWeather> Weather(World); Weather; ++Weather)
		{
			Cloud = Weather->SampleCloudCover(World->GetTimeSeconds());
			break;
		}
		const bool bMeteor = AIslandDayNight::IsShootingStarSlot(It->DayNumber, It->CurrentHour, Cloud, AIslandDayNight::LunarIllumination(It->DayNumber, It->CurrentHour));
		if (bSessionOpened && ShouldNoteMeteor(bMeteor, It->DayNumber, LastMeteorDay))
		{
			LastMeteorDay = It->DayNumber;
			RecordEntry(TEXT("sky"), FString(), TEXT("A shooting star streaks across the night sky."), { { TEXT("event"), TEXT("meteor") } });
		}
		break;
	}
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		const float Storm = It->SampleStormIntensity(World->GetTimeSeconds());
		const bool bWasActive = bStormActive;
		if (Storm >= StormArrives) bStormActive = true;
		else if (Storm < StormPasses) bStormActive = false;
		// A storm already raging when the session opens is not news; only changes after that are.
		if (bStormSeen && bStormActive != bWasActive)
		{
			RecordEntry(TEXT("weather"), FString(), bStormActive ? TEXT("A storm arrives over the Island.") : StormEndText(StormStrikes),
				{ { TEXT("event"), bStormActive ? TEXT("storm_start") : TEXT("storm_end") } });
			StormStrikes = 0;
		}
		bStormSeen = true;
		return;
	}
}
