#include "Misc/AutomationTest.h"
#include "AgentPlaySessionSubsystem.h"
#include "AutonomousAgentAIController.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentSafetyTest, "CaptiveSky2.Agent.SessionSafety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentSafetyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Thirty minute maximum"), UAgentPlaySessionSubsystem::ClampDuration(99999), 1800.0);
	TestEqual(TEXT("Zero cannot disable watchdog"), UAgentPlaySessionSubsystem::ClampDuration(0), 1.0);
	TestEqual(TEXT("Short smoke-test duration"), UAgentPlaySessionSubsystem::ClampDuration(12), 12.0);
	TestEqual(TEXT("Request budget has a positive floor"), UAgentPlaySessionSubsystem::ClampRequestLimit(0), 1);
	TestEqual(TEXT("Request budget cannot exceed hard cap"), UAgentPlaySessionSubsystem::ClampRequestLimit(1000), 120);
	float SessionSeconds = 1800.f;
	int32 SessionRequests = 120;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=240 -CaptiveSkyMaxModelRequests=8"), SessionSeconds, SessionRequests);
	TestEqual(TEXT("Command line can shorten real-time cap"), SessionSeconds, 240.f);
	TestEqual(TEXT("Command line can lower request cap"), SessionRequests, 8);
	SessionSeconds = 300.f;
	SessionRequests = 12;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=1800 -CaptiveSkyMaxModelRequests=120"), SessionSeconds, SessionRequests);
	TestEqual(TEXT("Command line cannot lengthen configured session"), SessionSeconds, 300.f);
	TestEqual(TEXT("Command line cannot raise configured request cap"), SessionRequests, 12);
	SessionSeconds = 1800.f;
	SessionRequests = 120;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=0 -CaptiveSkyMaxModelRequests=0"), SessionSeconds, SessionRequests);
	TestEqual(TEXT("Command line cannot disable real-time cap"), SessionSeconds, 1.f);
	TestEqual(TEXT("Command line cannot disable request cap"), SessionRequests, 1);
	TestEqual(TEXT("Legacy 15-second Blueprint interval is bounded"), AAutonomousAgentAIController::BackgroundDelay(0, 15), 60.0);
	TestEqual(TEXT("Repeats back off"), AAutonomousAgentAIController::BackgroundDelay(3, 60), 240.0);
	TestEqual(TEXT("Backoff capped at five minutes"), AAutonomousAgentAIController::BackgroundDelay(1000, 60), 300.0);
	// Exercise reservations without registering a ticker or issuing any model calls.
	UGameInstance* Instance = NewObject<UGameInstance>();
	UAgentPlaySessionSubsystem* Session = NewObject<UAgentPlaySessionSubsystem>(Instance);
	Session->StartedAt = FPlatformTime::Seconds();
	Session->MaxModelRequests = 2;
	TestTrue(TEXT("First reservation accepted"), Session->TryReserveModelRequest());
	TestTrue(TEXT("Last reservation accepted"), Session->TryReserveModelRequest());
	TestTrue(TEXT("Budget exhaustion expires session"), Session->IsExpired());
	TestFalse(TEXT("Exhausted budget rejects further calls"), Session->TryReserveModelRequest());
	TestEqual(TEXT("Rejected request is not counted"), Session->ModelRequests, 2);
	Session->ModelRequests = 120;
	Session->MaxModelRequests = 10000;
	TestFalse(TEXT("Configuration cannot exceed hard request cap"), Session->TryReserveModelRequest());
	Session->ModelRequests = 0;
	Session->StartedAt = FPlatformTime::Seconds() - 1801;
	TestFalse(TEXT("Wall-clock deadline rejects calls"), Session->TryReserveModelRequest());
	return true;
}
