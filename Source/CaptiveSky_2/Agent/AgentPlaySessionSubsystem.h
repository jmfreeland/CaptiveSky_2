#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgentPlaySessionSubsystem.generated.h"

/**
 * Independent wall-clock watchdog. Exists for every play session, not as a placed actor.
 *
 * Bounded play (the default) ends after MaxRealtimeSeconds or MaxModelRequests. When the request cap is
 * reached, no more requests are accepted, but the session drains already-reserved work for a short,
 * bounded grace period. Continuous play is an explicit opt-in (-CaptiveSkyContinuous or bContinuousPlay)
 * for an unattended screen: it has no default
 * end time, but explicit command-line smoke-test caps still apply. Model requests draw on an allowance that
 * refills steadily, each resident must leave a few seconds between its own requests, and a daily ceiling
 * recorded in Saved/CaptiveSky/ModelBudget.json holds across restarts. When the allowance is spent, residents simply wait.
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Agent|Safety")
	int32 ModelRequestsInFlight = 0;
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
	/** Light-model requests per real (UTC) day across restarts; hard-capped at 40000. */
	UPROPERTY(Config, BlueprintReadOnly, Category="Agent|Continuous")
	int32 ContinuousDailyLightRequests = 6000;
	/** A light request draws this share of a full request from the shared allowance. */
	static constexpr double LightRequestCost = 0.2;
	static constexpr double ContinuousAgentSpacingSeconds = 12.0;
	static constexpr double RequestDrainGraceSeconds = 45.0;
	bool IsContinuous() const { return bContinuousPlay; }
	/** Tests point this at a scratch file; empty uses Saved/CaptiveSky/ModelBudget.json. */
	FString LedgerPathOverride;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** AgentId lets continuous play space out each resident's requests; empty skips that check.
	 *  A light request (the cheap model) costs a fraction of the allowance and has its own daily ceiling. */
	bool TryReserveModelRequest(const FString& AgentId = FString(), bool bLight = false);
	/** Releases a reservation after its provider callback; request and real-time caps remain authoritative. */
	void CompleteModelRequest();
	bool IsExpired() const;
	static double ClampDuration(double Requested);
	static int32 ClampRequestLimit(int32 Requested);
	/** Applies command-line smoke-test limits without permitting either active cap to increase. */
	static void ApplyCommandLineOverrides(const FString& CommandLine, float& InOutSeconds, int32& InOutRequests,
		bool* bOutHasExplicitTimeCap = nullptr, bool* bOutHasExplicitRequestCap = nullptr);
private:
	friend class FAgentSafetyTest;
	double StartedAt = 0;
	bool bHasExplicitTimeCap = false;
	bool bHasExplicitRequestCap = false;
	FTSTicker::FDelegateHandle Watchdog;
	bool bStopRequested = false;
	bool CheckDeadline(float DeltaSeconds);
	double NowOverride = -1.0;
	double RequestCapReachedAt = -1.0;
	double Allowance = 0.0;
	double AllowanceUpdatedAt = 0.0;
	TMap<FString, double> LastRequestByAgent;
	FString LedgerDate;
	int32 LedgerRequests = 0;
	int32 LedgerLightRequests = 0;
	bool bLedgerPersistenceHealthy = true;
	double Now() const;
	bool HasReachedRequestCap() const;
	void BeginContinuous();
	void RefillAllowance();
	FString LedgerPath() const;
	void LoadLedger();
	bool SaveLedger() const;
	static FString TodayUtc();
};
