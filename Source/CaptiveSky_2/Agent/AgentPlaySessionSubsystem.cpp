#include "AgentPlaySessionSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAgentSession, Log, All);

double UAgentPlaySessionSubsystem::ClampDuration(double Requested)
{
	return FMath::IsFinite(Requested) ? FMath::Clamp(Requested, 1.0, 1800.0) : 1800.0;
}

void UAgentPlaySessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartedAt = FPlatformTime::Seconds();
	ModelRequests = 0;
	bStopRequested = false;
	Watchdog = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAgentPlaySessionSubsystem::CheckDeadline), 0.25f);
	UE_LOG(LogAgentSession, Log, TEXT("Play safety active: %.0f real seconds, at most %d model requests."), ClampDuration(MaxRealtimeSeconds), FMath::Clamp(MaxModelRequests, 1, 120));
}

bool UAgentPlaySessionSubsystem::IsExpired() const
{
	return bStopRequested || FPlatformTime::Seconds() - StartedAt >= ClampDuration(MaxRealtimeSeconds) || ModelRequests >= FMath::Clamp(MaxModelRequests, 1, 120);
}

bool UAgentPlaySessionSubsystem::TryReserveModelRequest()
{
	if (IsExpired()) return false;
	++ModelRequests;
	return true;
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
