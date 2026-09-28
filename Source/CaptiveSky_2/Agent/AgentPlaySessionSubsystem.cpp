#include "AgentPlaySessionSubsystem.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAgentSession, Log, All);

double UAgentPlaySessionSubsystem::ClampDuration(double Requested)
{
	return FMath::IsFinite(Requested) ? FMath::Clamp(Requested, 1.0, 1800.0) : 1800.0;
}

int32 UAgentPlaySessionSubsystem::ClampRequestLimit(int32 Requested)
{
	return FMath::Clamp(Requested, 1, 120);
}

void UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(const FString& CommandLine, float& InOutSeconds, int32& InOutRequests)
{
	const float ConfiguredSeconds = static_cast<float>(ClampDuration(InOutSeconds));
	const int32 ConfiguredRequests = ClampRequestLimit(InOutRequests);
	float RequestedSeconds = ConfiguredSeconds;
	int32 RequestedRequests = ConfiguredRequests;
	FParse::Value(*CommandLine, TEXT("CaptiveSkyMaxRealtimeSeconds="), RequestedSeconds);
	FParse::Value(*CommandLine, TEXT("CaptiveSkyMaxModelRequests="), RequestedRequests);
	InOutSeconds = FMath::Min(ConfiguredSeconds, static_cast<float>(ClampDuration(RequestedSeconds)));
	InOutRequests = FMath::Min(ConfiguredRequests, ClampRequestLimit(RequestedRequests));
}

void UAgentPlaySessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ApplyCommandLineOverrides(FCommandLine::Get(), MaxRealtimeSeconds, MaxModelRequests);
	StartedAt = FPlatformTime::Seconds();
	ModelRequests = 0;
	bStopRequested = false;
	if (FParse::Param(FCommandLine::Get(), TEXT("CaptiveSkyContinuous"))) bContinuousPlay = true;
	if (bContinuousPlay) BeginContinuous();
	Watchdog = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAgentPlaySessionSubsystem::CheckDeadline), 0.25f);
	if (bContinuousPlay)
		UE_LOG(LogAgentSession, Log, TEXT("Continuous play: no end time; %.0f model requests per hour, bursts of %d, at most %d per day (%d used today)."),
			ContinuousRequestsPerHour, ContinuousBurst, ContinuousDailyRequests, LedgerRequests)
	else
		UE_LOG(LogAgentSession, Log, TEXT("Play safety active: %.0f real seconds, at most %d model requests."), MaxRealtimeSeconds, MaxModelRequests);
}

bool UAgentPlaySessionSubsystem::IsExpired() const
{
	if (bContinuousPlay) return bStopRequested;
	return bStopRequested || FPlatformTime::Seconds() - StartedAt >= ClampDuration(MaxRealtimeSeconds) || ModelRequests >= ClampRequestLimit(MaxModelRequests);
}

bool UAgentPlaySessionSubsystem::TryReserveModelRequest(const FString& AgentId)
{
	if (IsExpired()) return false;
	if (bContinuousPlay)
	{
		RefillAllowance();
		if (TodayUtc() != LedgerDate) { LedgerDate = TodayUtc(); LedgerRequests = 0; }
		if (LedgerRequests >= FMath::Clamp(ContinuousDailyRequests, 1, 20000)) return false;
		const double Time = Now();
		if (!AgentId.IsEmpty())
			if (const double* Last = LastRequestByAgent.Find(AgentId); Last && Time - *Last < ContinuousAgentSpacingSeconds) return false;
		if (Allowance < 1.0) return false;
		Allowance -= 1.0;
		++LedgerRequests;
		if (!AgentId.IsEmpty()) LastRequestByAgent.Add(AgentId, Time);
		SaveLedger();
	}
	++ModelRequests;
	return true;
}

double UAgentPlaySessionSubsystem::Now() const
{
	return NowOverride >= 0.0 ? NowOverride : FPlatformTime::Seconds();
}

void UAgentPlaySessionSubsystem::BeginContinuous()
{
	// Start half full, so a fresh launch can't spend a whole burst in its first seconds.
	Allowance = FMath::Clamp(ContinuousBurst, 1, 60) * 0.5;
	AllowanceUpdatedAt = Now();
	LastRequestByAgent.Reset();
	LoadLedger();
}

void UAgentPlaySessionSubsystem::RefillAllowance()
{
	const double Time = Now();
	const double PerSecond = FMath::Clamp(ContinuousRequestsPerHour, 1.f, 600.f) / 3600.0;
	Allowance = FMath::Min<double>(FMath::Clamp(ContinuousBurst, 1, 60), Allowance + FMath::Max(0.0, Time - AllowanceUpdatedAt) * PerSecond);
	AllowanceUpdatedAt = Time;
}

FString UAgentPlaySessionSubsystem::TodayUtc()
{
	return FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d"));
}

FString UAgentPlaySessionSubsystem::LedgerPath() const
{
	return LedgerPathOverride.IsEmpty() ? FPaths::ProjectSavedDir() / TEXT("CaptiveSky") / TEXT("ModelBudget.json") : LedgerPathOverride;
}

void UAgentPlaySessionSubsystem::LoadLedger()
{
	LedgerDate = TodayUtc();
	LedgerRequests = 0;
	FString Json;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Json, *LedgerPath()) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) return;
	FString Date;
	int32 Requests = 0;
	if (Root->TryGetStringField(TEXT("date"), Date) && Date == LedgerDate && Root->TryGetNumberField(TEXT("requests"), Requests))
		LedgerRequests = FMath::Max(0, Requests);
}

void UAgentPlaySessionSubsystem::SaveLedger() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("date"), LedgerDate);
	Root->SetNumberField(TEXT("requests"), LedgerRequests);
	FString Json;
	FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *LedgerPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool UAgentPlaySessionSubsystem::CheckDeadline(float DeltaSeconds)
{
	if (!IsExpired() || bStopRequested) return true;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return true;
	bStopRequested = true;
	UE_LOG(LogAgentSession, Warning, TEXT("Ending play: %.1f real seconds elapsed, %d model requests. No automatic restart."), FPlatformTime::Seconds() - StartedAt, ModelRequests);
#if WITH_EDITOR
	if (World->WorldType == EWorldType::PIE && GEditor)
	{
		GEditor->RequestEndPlayMap();
		return true;
	}
#endif
	FPlatformMisc::RequestExit(false);
	return true;
}

void UAgentPlaySessionSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(Watchdog);
	bStopRequested = true;
	UE_LOG(LogAgentSession, Log, TEXT("Play ended after %.1f real seconds with %d model requests."), FPlatformTime::Seconds() - StartedAt, ModelRequests);
	Super::Deinitialize();
}
