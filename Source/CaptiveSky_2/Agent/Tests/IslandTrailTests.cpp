#include "Misc/AutomationTest.h"
#include "IslandTrail.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTrailTest, "CaptiveSky2.Agent.IslandTrail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTrailTest::RunTest(const FString& Parameters)
{
	using Trail = UIslandTrailSubsystem;
	TestEqual(TEXT("Dry ground leaves no print"), Trail::PrintLifetime(0.f), 0.f);
	TestEqual(TEXT("Barely damp ground leaves none"), Trail::PrintLifetime(Trail::PrintWetness - 0.01f), 0.f);
	TestTrue(TEXT("Wetter ground holds prints longer"), Trail::PrintLifetime(0.3f) < Trail::PrintLifetime(0.9f));
	TestTrue(TEXT("A print lasts at least some seconds"), Trail::PrintLifetime(Trail::PrintWetness) >= 20.f);

	const float Life = 60.f;
	TestEqual(TEXT("A fresh print is full size"), Trail::PrintScale(0.f, Life, 0.8f), 1.f);
	TestEqual(TEXT("It holds until it starts to dry"), Trail::PrintScale(Life * 0.5f, Life, 0.8f), 1.f);
	TestTrue(TEXT("Then it shrinks"), Trail::PrintScale(Life * 0.85f, Life, 0.8f) < 1.f);
	TestEqual(TEXT("And is gone at the end"), Trail::PrintScale(Life, Life, 0.8f), 0.f);
	TestEqual(TEXT("A print dries away with the ground"), Trail::PrintScale(1.f, Life, 0.f), 0.f);
	TestTrue(TEXT("Half-dried ground shrinks it"), Trail::PrintScale(1.f, Life, 0.06f) < 1.f);

	TestEqual(TEXT("A few footfalls wear nothing"), Trail::WearAmount(Trail::WearStartSteps), 0.f);
	TestTrue(TEXT("More footfalls wear more"), Trail::WearAmount(60) < Trail::WearAmount(120));
	TestEqual(TEXT("A well-used path is fully worn"), Trail::WearAmount(Trail::WearFullSteps), 1.f);
	TestEqual(TEXT("Wear is capped"), Trail::WearAmount(100000), 1.f);
	const FLinearColor Dry = Trail::WearColor(0.f);
	const FLinearColor Wet = Trail::WearColor(1.f);
	TestTrue(TEXT("Wet worn earth is darker"), Wet.R < Dry.R && Wet.G < Dry.G);

	FIslandTrailLedger Ledger;
	const FVector Here(1234.f, -560.f, 90.f);
	TestEqual(TEXT("Unwalked ground has no steps"), Ledger.StepsAt(Here), 0);
	for (int32 I = 0; I < 5; ++I) Ledger.AddStep(Here + FVector(I * 4.f, 0.f, 0.f), FVector(0.1f, 0.f, 0.995f));
	TestEqual(TEXT("Footfalls in one cell add up"), Ledger.StepsAt(Here), 5);
	TestEqual(TEXT("A neighbouring cell is separate"), Ledger.StepsAt(Here + FVector(FIslandTrailLedger::CellSize * 2.f, 0.f, 0.f)), 0);
	TestEqual(TEXT("Negative coordinates have their own cells"), FIslandTrailLedger::CellFor(FVector(-1.f, -1.f, 0.f)), FIntPoint(-1, -1));

	const FString Json = Ledger.ToJson();
	FIslandTrailLedger Loaded;
	TestTrue(TEXT("The ledger reloads"), Loaded.FromJson(Json));
	TestEqual(TEXT("Steps survive a save"), Loaded.StepsAt(Here), 5);
	const FIslandTrailCell* Cell = Loaded.Cells.Find(FIslandTrailLedger::CellFor(Here));
	if (TestNotNull(TEXT("Cell present"), Cell))
	{
		TestTrue(TEXT("Position survives"), Cell->Position.Equals(Here + FVector(16.f, 0.f, 0.f), 0.01f));
		TestTrue(TEXT("Slope survives"), FMath::IsNearlyEqual(Cell->NormalX, 0.1f, 0.001f));
	}
	FIslandTrailLedger Garbage;
	Garbage.AddStep(Here, FVector::UpVector);
	TestFalse(TEXT("Unreadable text is refused"), Garbage.FromJson(TEXT("not json")));
	TestFalse(TEXT("A future version is refused"), Garbage.FromJson(TEXT("{\"version\":99,\"cells\":[]}")));
	TestFalse(TEXT("A malformed row is refused"), Garbage.FromJson(TEXT("{\"version\":1,\"cells\":[[1,2,3]]}")));
	TestEqual(TEXT("A refused load leaves the ledger alone"), Garbage.StepsAt(Here), 1);

	FIslandTrailLedger Full;
	for (int32 I = 0; I < FIslandTrailLedger::MaxCells; ++I) Full.AddStep(FVector(I * FIslandTrailLedger::CellSize, 0.f, 0.f), FVector::UpVector);
	TestFalse(TEXT("A full ledger refuses new ground"), Full.AddStep(FVector(0.f, 99999.f, 0.f), FVector::UpVector));
	TestTrue(TEXT("But still counts old paths"), Full.AddStep(FVector(0.f, 0.f, 0.f), FVector::UpVector));
	return true;
}
