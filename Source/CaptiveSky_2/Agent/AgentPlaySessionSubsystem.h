#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgentPlaySessionSubsystem.generated.h"

/** Independent wall-clock watchdog. Exists for every play session, not as a placed actor. */
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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool TryReserveModelRequest();
	bool IsExpired() const;
	static double ClampDuration(double Requested);
private:
	friend class FAgentSafetyTest;
	double StartedAt = 0;
	FTSTicker::FDelegateHandle Watchdog;
	bool bStopRequested = false;
	bool CheckDeadline(float DeltaSeconds);
};
