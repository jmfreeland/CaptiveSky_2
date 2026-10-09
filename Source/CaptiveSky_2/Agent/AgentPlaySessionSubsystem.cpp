#include "AgentPlaySessionSubsystem.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
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
	return FMath::Clamp(Requested, 0, 120);
}

void UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(const FString& CommandLine, float& InOutSeconds, int32& InOutRequests,
	bool* bOutHasExplicitTimeCap, bool* bOutHasExplicitRequestCap)
{
	const float ConfiguredSeconds = static_cast<float>(ClampDuration(InOutSeconds));
	const int32 ConfiguredRequests = ClampRequestLimit(InOutRequests);
	float RequestedSeconds = ConfiguredSeconds;
	int32 RequestedRequests = ConfiguredRequests;
	const bool bHasExplicitTimeCap = FParse::Value(*CommandLine, TEXT("CaptiveSkyMaxRealtimeSeconds="), RequestedSeconds);
	const bool bHasExplicitRequestCap = FParse::Value(*CommandLine, TEXT("CaptiveSkyMaxModelRequests="), RequestedRequests);
	if (bOutHasExplicitTimeCap) *bOutHasExplicitTimeCap = bHasExplicitTimeCap;
	if (bOutHasExplicitRequestCap) *bOutHasExplicitRequestCap = bHasExplicitRequestCap;
	InOutSeconds = FMath::Min(ConfiguredSeconds, static_cast<float>(ClampDuration(RequestedSeconds)));
	InOutRequests = FMath::Min(ConfiguredRequests, ClampRequestLimit(RequestedRequests));
}

void UAgentPlaySessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ApplyCommandLineOverrides(FCommandLine::Get(), MaxRealtimeSeconds, MaxModelRequests,
		&bHasExplicitTimeCap, &bHasExplicitRequestCap);
	StartedAt = FPlatformTime::Seconds();
	ModelRequests = 0;
	ModelRequestsInFlight = 0;
	RequestCapReachedAt = -1.0;
	bStopRequested = false;
	if (FParse::Param(FCommandLine::Get(), TEXT("CaptiveSkyContinuous"))) bContinuousPlay = true;
	if (bContinuousPlay) BeginContinuous();
	Watchdog = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAgentPlaySessionSubsystem::CheckDeadline), 0.25f);
	if (bContinuousPlay)
	{
		FString ExplicitCaps = TEXT("no explicit command-line end cap");
		if (bHasExplicitTimeCap)
			ExplicitCaps = FString::Printf(TEXT("time cap %.0f seconds"), MaxRealtimeSeconds);
		if (bHasExplicitRequestCap)
			ExplicitCaps += FString::Printf(TEXT("%srequest cap %d"), bHasExplicitTimeCap ? TEXT(" and ") : TEXT(""), MaxModelRequests);
		UE_LOG(LogAgentSession, Log, TEXT("Continuous play: %s; %.0f model requests per hour, bursts of %d, at most %d per day (%d used today)."),
			*ExplicitCaps, ContinuousRequestsPerHour, ContinuousBurst, ContinuousDailyRequests, LedgerRequests);
	}
	else
		UE_LOG(LogAgentSession, Log, TEXT("Play safety active: %.0f real seconds, at most %d model requests."), MaxRealtimeSeconds, MaxModelRequests);
}

bool UAgentPlaySessionSubsystem::HasReachedRequestCap() const
{
	return (!bContinuousPlay || bHasExplicitRequestCap) &&
		ModelRequests >= ClampRequestLimit(MaxModelRequests);
}

bool UAgentPlaySessionSubsystem::IsExpired() const
{
	const double ElapsedSeconds = FPlatformTime::Seconds() - StartedAt;
	if (bStopRequested || (!bContinuousPlay && ElapsedSeconds >= ClampDuration(MaxRealtimeSeconds)) ||
		(bContinuousPlay && bHasExplicitTimeCap && ElapsedSeconds >= ClampDuration(MaxRealtimeSeconds)))
	{
		return true;
	}
	if (!HasReachedRequestCap()) return false;
	if (ModelRequestsInFlight <= 0) return true;
	return RequestCapReachedAt >= 0.0 && Now() - RequestCapReachedAt >= RequestDrainGraceSeconds;
}

bool UAgentPlaySessionSubsystem::TryReserveModelRequest(const FString& AgentId, bool bLight)
{
	// A reached cap may be draining requests already in flight, but it must never admit another one.
	if (IsExpired() || HasReachedRequestCap()) return false;
	if (bContinuousPlay)
	{
		if (!bLedgerPersistenceHealthy) return false;
		RefillAllowance();
		if (TodayUtc() != LedgerDate) { LedgerDate = TodayUtc(); LedgerRequests = 0; LedgerLightRequests = 0; }
		if (bLight ? LedgerLightRequests >= FMath::Clamp(ContinuousDailyLightRequests, 1, 40000) : LedgerRequests >= FMath::Clamp(ContinuousDailyRequests, 1, 20000)) return false;
		const double Time = Now();
		if (!AgentId.IsEmpty())
			if (const double* Last = LastRequestByAgent.Find(AgentId); Last && Time - *Last < ContinuousAgentSpacingSeconds) return false;
		const double Cost = bLight ? LightRequestCost : 1.0;
		if (Allowance < Cost) return false;
		const double PreviousAllowance = Allowance;
		Allowance -= Cost;
		if (bLight) ++LedgerLightRequests; else ++LedgerRequests;
		double PreviousAgentRequest = 0.0;
		const double* PreviousAgentRequestPtr = AgentId.IsEmpty() ? nullptr : LastRequestByAgent.Find(AgentId);
		const bool bHadPreviousAgentRequest = PreviousAgentRequestPtr != nullptr;
		if (PreviousAgentRequestPtr) PreviousAgentRequest = *PreviousAgentRequestPtr;
		if (!AgentId.IsEmpty()) LastRequestByAgent.Add(AgentId, Time);
		if (!SaveLedger())
		{
			if (bLight) --LedgerLightRequests; else --LedgerRequests;
			Allowance = PreviousAllowance;
			if (!AgentId.IsEmpty())
			{
				LastRequestByAgent.Remove(AgentId);
				if (bHadPreviousAgentRequest) LastRequestByAgent.Add(AgentId, PreviousAgentRequest);
			}
			bLedgerPersistenceHealthy = false;
			UE_LOG(LogAgentSession, Error, TEXT("Continuous play is refusing model requests because the daily budget ledger could not be saved to %s."), *LedgerPath());
			return false;
		}
	}
	++ModelRequests;
	++ModelRequestsInFlight;
	if (HasReachedRequestCap())
	{
		RequestCapReachedAt = Now();
		UE_LOG(LogAgentSession, Display, TEXT("Model request cap reached; accepting no more requests and draining %d in-flight request(s) for up to %.0f seconds."),
			ModelRequestsInFlight, RequestDrainGraceSeconds);
	}
	return true;
}

void UAgentPlaySessionSubsystem::CompleteModelRequest()
{
	if (ModelRequestsInFlight <= 0)
	{
		UE_LOG(LogAgentSession, Warning, TEXT("Received model-request completion with no request in flight."));
		return;
	}
	--ModelRequestsInFlight;
	UE_LOG(LogAgentSession, Verbose, TEXT("Model request completed; %d request(s) remain in flight."), ModelRequestsInFlight);
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
	LedgerLightRequests = 0;
	bLedgerPersistenceHealthy = true;
	const FString Path = LedgerPath();
	if (!FPaths::FileExists(Path)) return;

	FString Json;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Json, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
	{
		bLedgerPersistenceHealthy = false;
		UE_LOG(LogAgentSession, Error, TEXT("Continuous play is refusing model requests because the daily budget ledger is unreadable: %s."), *Path);
		return;
	}
	FString Date;
	double Requests = 0.0;
	FDateTime ParsedDate;
	if (!Root->TryGetStringField(TEXT("date"), Date) || Date.IsEmpty() ||
		!FDateTime::ParseIso8601(*(Date + TEXT("T00:00:00Z")), ParsedDate) ||
		!Root->TryGetNumberField(TEXT("requests"), Requests) || !FMath::IsFinite(Requests) || Requests < 0.0)
	{
		bLedgerPersistenceHealthy = false;
		UE_LOG(LogAgentSession, Error, TEXT("Continuous play is refusing model requests because the daily budget ledger is invalid: %s."), *Path);
		return;
	}
	if (Date.Compare(LedgerDate, ESearchCase::CaseSensitive) > 0)
	{
		bLedgerPersistenceHealthy = false;
		UE_LOG(LogAgentSession, Error, TEXT("Continuous play is refusing model requests because the daily budget ledger date is in the future: %s."), *Path);
		return;
	}
	// Ledgers written before the light tier have no light_requests field; that means none were spent.
	double LightRequests = 0.0;
	Root->TryGetNumberField(TEXT("light_requests"), LightRequests);
	if (!FMath::IsFinite(LightRequests) || LightRequests < 0.0)
	{
		bLedgerPersistenceHealthy = false;
		UE_LOG(LogAgentSession, Error, TEXT("Continuous play is refusing model requests because the daily budget ledger is invalid: %s."), *Path);
		return;
	}
	if (Date == LedgerDate)
	{
		LedgerRequests = Requests >= 20000.0 ? 20000 : static_cast<int32>(FMath::CeilToInt(Requests));
		LedgerLightRequests = LightRequests >= 40000.0 ? 40000 : static_cast<int32>(FMath::CeilToInt(LightRequests));
	}
}

bool UAgentPlaySessionSubsystem::SaveLedger() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("date"), LedgerDate);
	Root->SetNumberField(TEXT("requests"), LedgerRequests);
	Root->SetNumberField(TEXT("light_requests"), LedgerLightRequests);
	FString Json;
	if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json))) return false;
	const FString Path = LedgerPath();
	const FString Directory = FPaths::GetPath(Path);
	if (!IFileManager::Get().DirectoryExists(*Directory) && !IFileManager::Get().MakeDirectory(*Directory, true)) return false;
	const FString Temporary = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Json, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		IFileManager::Get().Delete(*Temporary, false, true);
		return false;
	}
	if (!IFileManager::Get().Move(*Path, *Temporary, true, true))
	{
		IFileManager::Get().Delete(*Temporary, false, true);
		return false;
	}
	return true;
}

bool UAgentPlaySessionSubsystem::CheckDeadline(float DeltaSeconds)
{
	if (!IsExpired() || bStopRequested) return true;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return true;
	bStopRequested = true;
	UE_LOG(LogAgentSession, Warning, TEXT("Ending play: %.1f real seconds elapsed, %d model requests (%d still in flight). No automatic restart."),
		FPlatformTime::Seconds() - StartedAt, ModelRequests, ModelRequestsInFlight);
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
	UE_LOG(LogAgentSession, Log, TEXT("Play ended after %.1f real seconds with %d model requests (%d still in flight)."),
		FPlatformTime::Seconds() - StartedAt, ModelRequests, ModelRequestsInFlight);
	Super::Deinitialize();
}
