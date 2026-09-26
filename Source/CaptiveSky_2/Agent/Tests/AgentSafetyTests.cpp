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
