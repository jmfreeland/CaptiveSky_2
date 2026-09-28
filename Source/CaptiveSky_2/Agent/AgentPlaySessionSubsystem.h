#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgentPlaySessionSubsystem.generated.h"

/**
 * Independent wall-clock watchdog. Exists for every play session, not as a placed actor.
 *
 * Bounded play (the default) ends after MaxRealtimeSeconds or MaxModelRequests. Continuous play is an
 * explicit opt-in (-CaptiveSkyContinuous or bContinuousPlay) for an unattended screen: play never ends by
 * itself; instead model requests draw on an allowance that refills steadily, each resident must leave a
 * few seconds between its own requests, and a daily ceiling recorded in Saved/CaptiveSky/ModelBudget.json
 * holds across restarts. When the allowance is spent, residents simply wait.
 */
UCLASS(Config=Game)
class CAPTIVESKY_2_API UAgentPlaySessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Safety")
	float MaxRealtimeSeconds = 1800.f;
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Safety")
	int32 MaxModelRequests = 120;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Agent|Safety")
	int32 ModelRequests = 0;
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Continuous")
	bool bContinuousPlay = false;
	/** Steady refill of the shared allowance; hard-capped at 600 per hour. */
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Continuous")
	float ContinuousRequestsPerHour = 90.f;
	/** Largest burst the allowance can hold; hard-capped at 60. */
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Continuous")
	int32 ContinuousBurst = 12;
	/** Requests per real (UTC) day across restarts; hard-capped at 20000. */
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Continuous")
	int32 ContinuousDailyRequests = 1500;
	static constexpr double ContinuousAgentSpacingSeconds = 12.0;
	bool IsContinuous() const { return bContinuousPlay; }
	/** Tests point this at a scratch file; empty uses Saved/CaptiveSky/ModelBudget.json. */
	FString LedgerPathOverride;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** AgentId lets continuous play space out each resident's requests; empty skips that check. */
	bool TryReserveModelRequest(const FString& AgentId = FString());
	bool IsExpired() const;
	static double ClampDuration(double Requested);
	static int32 ClampRequestLimit(int32 Requested);
	/** Applies command-line smoke-test limits without permitting either active cap to increase. */
	static void ApplyCommandLineOverrides(const FString& CommandLine, float& InOutSeconds, int32& InOutRequests);
private:
	friend class FAgentSafetyTest;
	double StartedAt = 0;
	FTSTicker::FDelegateHandle Watchdog;
	bool bStopRequested = false;
	bool CheckDeadline(float DeltaSeconds);
	double NowOverride = -1.0;
	double Allowance = 0.0;
	double AllowanceUpdatedAt = 0.0;
	TMap<FString, double> LastRequestByAgent;
	FString LedgerDate;
	int32 LedgerRequests = 0;
	double Now() const;
	void BeginContinuous();
	void RefillAllowance();
	FString LedgerPath() const;
	void LoadLedger();
	void SaveLedger() const;
	static FString TodayUtc();
};
