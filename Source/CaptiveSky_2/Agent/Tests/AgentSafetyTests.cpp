#include "Misc/AutomationTest.h"
#include "AgentPlaySessionSubsystem.h"
#include "AutonomousAgentAIController.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentSafetyTest, "CaptiveSky2.Agent.SessionSafety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentSafetyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Thirty minute maximum"), UAgentPlaySessionSubsystem::ClampDuration(99999), 1800.0);
	TestEqual(TEXT("Zero cannot disable watchdog"), UAgentPlaySessionSubsystem::ClampDuration(0), 1.0);
	TestEqual(TEXT("Short smoke-test duration"), UAgentPlaySessionSubsystem::ClampDuration(12), 12.0);
	TestEqual(TEXT("Zero model-request budget is a valid closed cap"), UAgentPlaySessionSubsystem::ClampRequestLimit(0), 0);
	TestEqual(TEXT("Negative model-request budgets clamp to zero"), UAgentPlaySessionSubsystem::ClampRequestLimit(-1), 0);
	TestEqual(TEXT("Request budget cannot exceed hard cap"), UAgentPlaySessionSubsystem::ClampRequestLimit(1000), 120);
	float SessionSeconds = 1800.f;
	int32 SessionRequests = 120;
	bool bHasExplicitTimeCap = false;
	bool bHasExplicitRequestCap = false;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=240 -CaptiveSkyMaxModelRequests=8"),
		SessionSeconds, SessionRequests, &bHasExplicitTimeCap, &bHasExplicitRequestCap);
	TestEqual(TEXT("Command line can shorten real-time cap"), SessionSeconds, 240.f);
	TestEqual(TEXT("Command line can lower request cap"), SessionRequests, 8);
	TestTrue(TEXT("An explicitly supplied real-time cap is tracked for continuous play"), bHasExplicitTimeCap);
	TestTrue(TEXT("An explicitly supplied request cap is tracked for continuous play"), bHasExplicitRequestCap);
	SessionSeconds = 300.f;
	SessionRequests = 12;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=1800 -CaptiveSkyMaxModelRequests=120"), SessionSeconds, SessionRequests);
	TestEqual(TEXT("Command line cannot lengthen configured session"), SessionSeconds, 300.f);
	TestEqual(TEXT("Command line cannot raise configured request cap"), SessionRequests, 12);
	SessionSeconds = 1800.f;
	SessionRequests = 120;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=0 -CaptiveSkyMaxModelRequests=0"), SessionSeconds, SessionRequests);
	TestEqual(TEXT("Command line cannot disable real-time cap"), SessionSeconds, 1.f);
	TestEqual(TEXT("Command line zero request cap disables all model calls"), SessionRequests, 0);
	SessionSeconds = 1800.f;
	SessionRequests = 120;
	UAgentPlaySessionSubsystem::ApplyCommandLineOverrides(TEXT("-CaptiveSkyMaxRealtimeSeconds=12"), SessionSeconds, SessionRequests);
	TestEqual(TEXT("Omitting the request override preserves the configured request cap"), SessionRequests, 120);
	TestEqual(TEXT("Legacy 15-second Blueprint interval is bounded"), AAutonomousAgentAIController::BackgroundDelay(0, 15), 60.0);
	TestEqual(TEXT("Repeats back off"), AAutonomousAgentAIController::BackgroundDelay(3, 60), 240.0);
	TestEqual(TEXT("Backoff capped at five minutes"), AAutonomousAgentAIController::BackgroundDelay(1000, 60), 300.0);
	TestTrue(TEXT("Bounded play keeps the per-resident fallback limit"),
		AAutonomousAgentAIController::IsAutonomousRequestLimitReached(AAutonomousAgentAIController::BoundedAutonomousRequestLimit, false));
	TestFalse(TEXT("Continuous play does not exhaust a resident's thinking turns at the bounded fallback limit"),
		AAutonomousAgentAIController::IsAutonomousRequestLimitReached(AAutonomousAgentAIController::BoundedAutonomousRequestLimit, true));
	TestFalse(TEXT("Continuous thinking remains available beyond thirty attempts; shared session budgets govern dispatch"),
		AAutonomousAgentAIController::IsAutonomousRequestLimitReached(39, true));
	// Exercise reservations without registering a ticker or issuing any model calls.
	UGameInstance* Instance = NewObject<UGameInstance>();
	UAgentPlaySessionSubsystem* Session = NewObject<UAgentPlaySessionSubsystem>(Instance);
	Session->StartedAt = FPlatformTime::Seconds();
	Session->MaxModelRequests = 2;
	TestTrue(TEXT("First reservation accepted"), Session->TryReserveModelRequest());
	TestTrue(TEXT("Last reservation accepted"), Session->TryReserveModelRequest());
	TestEqual(TEXT("Accepted calls are tracked in flight"), Session->ModelRequestsInFlight, 2);
	TestFalse(TEXT("Request cap drains accepted work before ending play"), Session->IsExpired());
	TestFalse(TEXT("Exhausted budget rejects further calls"), Session->TryReserveModelRequest());
	TestEqual(TEXT("Rejected request is not counted"), Session->ModelRequests, 2);
	Session->CompleteModelRequest();
	TestFalse(TEXT("One remaining request keeps the session alive"), Session->IsExpired());
	Session->CompleteModelRequest();
	TestTrue(TEXT("Drained request budget expires the session"), Session->IsExpired());
	TestEqual(TEXT("Completed calls leave no requests in flight"), Session->ModelRequestsInFlight, 0);

	UAgentPlaySessionSubsystem* NoModelRequests = NewObject<UAgentPlaySessionSubsystem>(Instance);
	NoModelRequests->StartedAt = FPlatformTime::Seconds();
	NoModelRequests->MaxModelRequests = 0;
	NoModelRequests->MaxRealtimeSeconds = 60.f;
	TestFalse(TEXT("Zero model-request cap does not end a no-model diagnostic session early"), NoModelRequests->IsExpired());
	TestFalse(TEXT("Zero model-request cap rejects the first reservation"), NoModelRequests->TryReserveModelRequest());
	TestEqual(TEXT("Rejected zero-cap reservation does not increment the count"), NoModelRequests->ModelRequests, 0);
	NoModelRequests->StartedAt = FPlatformTime::Seconds() - 2.0;
	NoModelRequests->MaxRealtimeSeconds = 1.f;
	TestTrue(TEXT("Zero model-request cap still obeys the real-time watchdog"), NoModelRequests->IsExpired());

	UAgentPlaySessionSubsystem* TimedOutDrain = NewObject<UAgentPlaySessionSubsystem>(Instance);
	TimedOutDrain->StartedAt = FPlatformTime::Seconds();
	TimedOutDrain->NowOverride = 1000.0;
	TimedOutDrain->MaxModelRequests = 1;
	TestTrue(TEXT("Single capped request is accepted before drain begins"), TimedOutDrain->TryReserveModelRequest());
	TestFalse(TEXT("In-flight request is protected during bounded drain grace"), TimedOutDrain->IsExpired());
	TimedOutDrain->NowOverride = 1000.0 + UAgentPlaySessionSubsystem::RequestDrainGraceSeconds - 0.01;
	TestFalse(TEXT("Drain grace has not expired early"), TimedOutDrain->IsExpired());
	TimedOutDrain->NowOverride = 1000.0 + UAgentPlaySessionSubsystem::RequestDrainGraceSeconds;
	TestTrue(TEXT("A stuck request cannot outlive the bounded drain grace"), TimedOutDrain->IsExpired());

	UAgentPlaySessionSubsystem* RealtimeCappedDrain = NewObject<UAgentPlaySessionSubsystem>(Instance);
	RealtimeCappedDrain->StartedAt = FPlatformTime::Seconds();
	RealtimeCappedDrain->MaxRealtimeSeconds = 1.f;
	RealtimeCappedDrain->MaxModelRequests = 1;
	TestTrue(TEXT("Request accepted before wall-clock deadline"), RealtimeCappedDrain->TryReserveModelRequest());
	RealtimeCappedDrain->StartedAt = FPlatformTime::Seconds() - 2.0;
	TestTrue(TEXT("Hard real-time cap still ends play with a request in flight"), RealtimeCappedDrain->IsExpired());

	Session->ModelRequests = 120;
	Session->MaxModelRequests = 10000;
	TestFalse(TEXT("Configuration cannot exceed hard request cap"), Session->TryReserveModelRequest());
	Session->ModelRequests = 0;
	Session->StartedAt = FPlatformTime::Seconds() - 1801;
	TestFalse(TEXT("Wall-clock deadline rejects calls"), Session->TryReserveModelRequest());

	// Continuous play: no end time, a refilling allowance, per-resident spacing, and a daily ceiling that survives restarts.
	const FString Ledger = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("SessionSafety") / TEXT("ModelBudget.json"));
	const FString LedgerDirectory = FPaths::GetPath(Ledger);
	if (!TestTrue(TEXT("Continuous-play ledger fixture directory is available"),
		IFileManager::Get().DirectoryExists(*LedgerDirectory) || IFileManager::Get().MakeDirectory(*LedgerDirectory, true))) return false;
	IFileManager::Get().Delete(*Ledger, false, true, true);
	auto MakeContinuous = [&Ledger, Instance](double Now)
	{
		UAgentPlaySessionSubsystem* Continuous = NewObject<UAgentPlaySessionSubsystem>(Instance);
		Continuous->bContinuousPlay = true;
		Continuous->ContinuousRequestsPerHour = 60.f; // one a minute
		Continuous->ContinuousBurst = 4;
		Continuous->ContinuousDailyRequests = 6;
		Continuous->LedgerPathOverride = Ledger;
		Continuous->NowOverride = Now;
		Continuous->StartedAt = FPlatformTime::Seconds() - 100000; // long past any bounded deadline
		Continuous->BeginContinuous();
		return Continuous;
	};
	UAgentPlaySessionSubsystem* Continuous = MakeContinuous(1000.0);
	TestFalse(TEXT("Continuous play never expires by time"), Continuous->IsExpired());
	UAgentPlaySessionSubsystem* ExplicitlyTimeCapped = MakeContinuous(2000.0);
	ExplicitlyTimeCapped->bHasExplicitTimeCap = true;
	TestTrue(TEXT("Continuous play with an explicit real-time cap expires when elapsed"), ExplicitlyTimeCapped->IsExpired());
	TestFalse(TEXT("Continuous play with an elapsed explicit time cap rejects new requests"),
		ExplicitlyTimeCapped->TryReserveModelRequest(TEXT("Aster")));
	UAgentPlaySessionSubsystem* ExplicitlyRequestCapped = MakeContinuous(2000.0);
	ExplicitlyRequestCapped->bHasExplicitRequestCap = true;
	ExplicitlyRequestCapped->MaxModelRequests = 1;
	ExplicitlyRequestCapped->ModelRequests = 1;
	ExplicitlyRequestCapped->ModelRequestsInFlight = 1;
	ExplicitlyRequestCapped->RequestCapReachedAt = 2000.0;
	TestFalse(TEXT("Continuous play drains a request at its explicit request cap"), ExplicitlyRequestCapped->IsExpired());
	TestFalse(TEXT("Continuous play with an explicit request cap rejects new requests"),
		ExplicitlyRequestCapped->TryReserveModelRequest(TEXT("Aster")));
	ExplicitlyRequestCapped->CompleteModelRequest();
	TestTrue(TEXT("Continuous play ends once its explicitly capped request drains"), ExplicitlyRequestCapped->IsExpired());
	UAgentPlaySessionSubsystem* ZeroRequestContinuous = MakeContinuous(2000.0);
	ZeroRequestContinuous->bHasExplicitRequestCap = true;
	ZeroRequestContinuous->MaxModelRequests = 0;
	TestFalse(TEXT("Continuous play with an explicit zero request cap can continue without model calls"), ZeroRequestContinuous->IsExpired());
	TestFalse(TEXT("Continuous play with an explicit zero request cap rejects reservations"),
		ZeroRequestContinuous->TryReserveModelRequest(TEXT("Aster")));
	TestTrue(TEXT("A fresh launch has half a burst to spend"), Continuous->TryReserveModelRequest(TEXT("Aster")));
	TestFalse(TEXT("One resident cannot fire requests back to back"), Continuous->TryReserveModelRequest(TEXT("Aster")));
	TestTrue(TEXT("The first accepted request creates a durable daily ledger"), FPaths::FileExists(Ledger));
	TestTrue(TEXT("Another resident may still think"), Continuous->TryReserveModelRequest(TEXT("Raven")));
	TestFalse(TEXT("A spent allowance makes residents wait"), Continuous->TryReserveModelRequest(TEXT("Innkeeper")));
	Continuous->NowOverride = 1061.0;
	TestTrue(TEXT("The allowance refills with time"), Continuous->TryReserveModelRequest(TEXT("Innkeeper")));
	Continuous->NowOverride = 1600.0; // plenty of refill; the burst cap holds it at 4
	for (const TCHAR* Agent : { TEXT("Aster"), TEXT("Raven"), TEXT("Innkeeper") }) Continuous->TryReserveModelRequest(Agent);
	TestFalse(TEXT("The daily ceiling holds even with allowance left"), Continuous->TryReserveModelRequest(TEXT("Visitor")));
	TestFalse(TEXT("A spent day does not end play"), Continuous->IsExpired());
	UAgentPlaySessionSubsystem* Restarted = MakeContinuous(5000.0);
	TestFalse(TEXT("The daily ceiling survives a restart"), Restarted->TryReserveModelRequest(TEXT("Aster")));
	if (!TestTrue(TEXT("Invalid-ledger fixture can be written"), FFileHelper::SaveStringToFile(TEXT("not-json"), *Ledger))) return false;
	AddExpectedError(TEXT("daily budget ledger is unreadable"), EAutomationExpectedErrorFlags::Contains, 1);
	UAgentPlaySessionSubsystem* CorruptLedger = MakeContinuous(5500.0);
	TestFalse(TEXT("A corrupt ledger fails closed instead of resetting the daily allowance"), CorruptLedger->TryReserveModelRequest(TEXT("Aster")));

	// A persistence failure must refuse the request rather than silently making the daily cap volatile.
	const FString Blocker = FPaths::GetPath(Ledger) / (TEXT("NotADirectory_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	if (!TestTrue(TEXT("Persistence-failure fixture blocker can be created"), FFileHelper::SaveStringToFile(TEXT("fixture"), *Blocker))) return false;
	UAgentPlaySessionSubsystem* PersistenceFailure = NewObject<UAgentPlaySessionSubsystem>(Instance);
	PersistenceFailure->bContinuousPlay = true;
	PersistenceFailure->ContinuousRequestsPerHour = 60.f;
	PersistenceFailure->ContinuousBurst = 4;
	PersistenceFailure->LedgerPathOverride = Blocker / TEXT("ModelBudget.json");
	PersistenceFailure->NowOverride = 6000.0;
	PersistenceFailure->StartedAt = FPlatformTime::Seconds() - 100000;
	PersistenceFailure->BeginContinuous();
	AddExpectedError(TEXT("daily budget ledger could not be saved"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("An unpersistable request is rejected before dispatch"), PersistenceFailure->TryReserveModelRequest(TEXT("Aster")));
	TestEqual(TEXT("A failed ledger write does not consume a model request"), PersistenceFailure->ModelRequests, 0);
	TestFalse(TEXT("A persistence failure fails closed for later requests"), PersistenceFailure->TryReserveModelRequest(TEXT("Raven")));
	IFileManager::Get().Delete(*Blocker, false, true, true);

	// Light-model requests draw a fraction of the allowance and have their own daily ceiling, kept in the same ledger.
	IFileManager::Get().Delete(*Ledger, false, true, true);
	UAgentPlaySessionSubsystem* LightSession = MakeContinuous(9000.0);
	LightSession->ContinuousDailyLightRequests = 5;
	for (int32 Index = 0; Index < 5; ++Index)
		TestTrue(*FString::Printf(TEXT("Light request %d is accepted"), Index + 1), LightSession->TryReserveModelRequest(FString(), true));
	TestFalse(TEXT("The light daily ceiling holds with allowance left"), LightSession->TryReserveModelRequest(FString(), true));
	TestEqual(TEXT("Light requests are not counted as full requests"), LightSession->LedgerRequests, 0);
	TestEqual(TEXT("Light requests are counted on their own"), LightSession->LedgerLightRequests, 5);
	TestEqual(TEXT("A light request draws a fifth of the allowance"), LightSession->Allowance, 1.0, 1.0e-6);
	TestEqual(TEXT("Bounded-mode style counting still sees every request"), LightSession->ModelRequests, 5);
	UAgentPlaySessionSubsystem* LightRestarted = MakeContinuous(9100.0);
	TestEqual(TEXT("Light requests survive a restart"), LightRestarted->LedgerLightRequests, 5);
	IFileManager::Get().Delete(*Ledger, false, true, true);
	if (!TestTrue(TEXT("Pre-light-tier ledger fixture can be written"),
		FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"date\":\"%s\",\"requests\":3}"), *FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d"))), *Ledger))) return false;
	UAgentPlaySessionSubsystem* OldLedger = MakeContinuous(9200.0);
	TestEqual(TEXT("An older ledger keeps its full-request count"), OldLedger->LedgerRequests, 3);
	TestEqual(TEXT("An older ledger has spent no light requests"), OldLedger->LedgerLightRequests, 0);
	TestTrue(TEXT("An older ledger still allows light requests"), OldLedger->TryReserveModelRequest(FString(), true));
	IFileManager::Get().Delete(*Ledger, false, true, true);
	return true;
}
