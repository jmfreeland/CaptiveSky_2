#include "Misc/AutomationTest.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "IslandTrail.h"
#include "IslandInteractionTestPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

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
	TestFalse(TEXT("An unused hidden print slot needs no transform update"), Trail::ShouldRefreshPrintInstance(false, false, 0.f));
	TestTrue(TEXT("A visible active print refreshes as it shrinks"), Trail::ShouldRefreshPrintInstance(true, true, 0.5f));
	TestTrue(TEXT("A newly visible print receives its first transform"), Trail::ShouldRefreshPrintInstance(true, false, 0.5f));
	TestTrue(TEXT("A print that just dried receives one hide transform"), Trail::ShouldRefreshPrintInstance(true, true, 0.f));
	TestFalse(TEXT("A hidden dried print is not rewritten every tick"), Trail::ShouldRefreshPrintInstance(true, false, 0.f));
	TestTrue(TEXT("An expired visible print receives one hide transform"), Trail::ShouldRefreshPrintInstance(false, true, 0.f));

	TestEqual(TEXT("A few footfalls wear nothing"), Trail::WearAmount(Trail::WearStartSteps), 0.f);
	const int32 FirstMidpoint = Trail::WearStartSteps + (Trail::WearFullSteps - Trail::WearStartSteps) / 3;
	const int32 SecondMidpoint = Trail::WearStartSteps + (Trail::WearFullSteps - Trail::WearStartSteps) * 2 / 3;
	TestTrue(TEXT("Repeated passes wear the same ground progressively"), Trail::WearAmount(FirstMidpoint) < Trail::WearAmount(SecondMidpoint));
	TestEqual(TEXT("A well-used path is fully worn"), Trail::WearAmount(Trail::WearFullSteps), 1.f);
	TestEqual(TEXT("Wear is capped"), Trail::WearAmount(100000), 1.f);
	const FLinearColor Dry = Trail::WearColor(0.f);
	const FLinearColor Wet = Trail::WearColor(1.f);
	TestTrue(TEXT("Wet worn earth is darker"), Wet.R < Dry.R && Wet.G < Dry.G);

	const FIntPoint WearCenter(4, -2);
	auto AddWearCell = [](TMap<FIntPoint, FIslandTrailCell>& Cells, const FIntPoint& Key)
	{
		FIslandTrailCell& Cell = Cells.Add(Key);
		Cell.Steps = Trail::WearFullSteps;
	};
	TMap<FIntPoint, FIslandTrailCell> StraightWear;
	AddWearCell(StraightWear, WearCenter + FIntPoint(-1, 0));
	AddWearCell(StraightWear, WearCenter + FIntPoint(1, 0));
	float StraightAspect = 0.f;
	const FQuat StraightRotation = Trail::CalculateWearRotation(WearCenter, StraightWear, FVector::UpVector, StraightAspect);
	TestTrue(TEXT("Linked worn cells align their long axis with the route"),
		FMath::Abs(FVector::DotProduct(StraightRotation.RotateVector(FVector::ForwardVector), FVector::ForwardVector)) > 0.99f);
	TestTrue(TEXT("A straight path becomes gently elongated"), StraightAspect > 1.3f && StraightAspect <= 1.45f);

	TMap<FIntPoint, FIslandTrailCell> DiagonalWear;
	AddWearCell(DiagonalWear, WearCenter + FIntPoint(-1, -1));
	AddWearCell(DiagonalWear, WearCenter + FIntPoint(1, 1));
	float DiagonalAspect = 0.f;
	const FQuat DiagonalRotation = Trail::CalculateWearRotation(WearCenter, DiagonalWear, FVector::UpVector, DiagonalAspect);
	const FVector DiagonalAxis = FVector(1.f, 1.f, 0.f).GetSafeNormal();
	TestTrue(TEXT("Diagonal worn cells follow their diagonal route"),
		FMath::Abs(FVector::DotProduct(DiagonalRotation.RotateVector(FVector::ForwardVector), DiagonalAxis)) > 0.99f);

	TMap<FIntPoint, FIslandTrailCell> CrossWear;
	AddWearCell(CrossWear, WearCenter + FIntPoint(-1, 0));
	AddWearCell(CrossWear, WearCenter + FIntPoint(1, 0));
	AddWearCell(CrossWear, WearCenter + FIntPoint(0, -1));
	AddWearCell(CrossWear, WearCenter + FIntPoint(0, 1));
	float CrossAspect = 0.f;
	const FQuat CrossRotation = Trail::CalculateWearRotation(WearCenter, CrossWear, FVector::UpVector, CrossAspect);
	float RepeatedCrossAspect = 0.f;
	const FQuat RepeatedCrossRotation = Trail::CalculateWearRotation(WearCenter, CrossWear, FVector::UpVector, RepeatedCrossAspect);
	TestEqual(TEXT("A trail junction remains round instead of claiming one direction"), CrossAspect, 1.f);
	TestTrue(TEXT("Junction fallback rotation stays deterministic"), CrossRotation.Equals(RepeatedCrossRotation));

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

	FIslandTrailLedger Weathered;
	for (int32 I = 0; I < 200; ++I) Weathered.AddStep(Here, FVector::UpVector);
	FIslandTrailLedger Faint;
	Faint.AddStep(Here + FVector(900.f, 0.f, 0.f), FVector::UpVector);
	TestTrue(TEXT("The first day only sets the clock"), Weathered.WeatherTo(10));
	TestEqual(TEXT("It changes no steps"), Weathered.StepsAt(Here), 200);
	TestFalse(TEXT("The same day changes nothing"), Weathered.WeatherTo(10));
	TestFalse(TEXT("An earlier day changes nothing"), Weathered.WeatherTo(4));
	Weathered.WeatherTo(11);
	TestTrue(TEXT("A day of disuse costs a path some steps"), Weathered.StepsAt(Here) < 200 && Weathered.StepsAt(Here) > 180);
	Weathered.WeatherTo(40);
	TestTrue(TEXT("A month of disuse mostly undoes it"), Weathered.StepsAt(Here) < 120);
	Faint.Decay(1);
	TestEqual(TEXT("A single footfall is gone in a day"), Faint.Cells.Num(), 0);
	FIslandTrailLedger Reloaded;
	TestTrue(TEXT("The weathering day is saved"), Reloaded.FromJson(Weathered.ToJson()));
	TestEqual(TEXT("And reloaded"), Reloaded.LastDecayDay, 40);

	TestTrue(TEXT("Virgin ground says nothing"), Trail::DescribeTrail(0, 0, 0.f).IsEmpty());
	TestTrue(TEXT("A faint track is noticed"), Trail::DescribeTrail(Trail::WearStartSteps / 2, 0, 0.f).Contains(TEXT("faintly")));
	TestTrue(TEXT("A worn track is noticed"), Trail::DescribeTrail(Trail::WearStartSteps, 0, 0.f).Contains(TEXT("worn thin")));
	TestTrue(TEXT("A bare track is noticed"), Trail::DescribeTrail(Trail::WearFullSteps, 0, 0.f).Contains(TEXT("trodden bare")));
	TestTrue(TEXT("Wet prints are noticed"), Trail::DescribeTrail(0, 5, 0.9f).Contains(TEXT("Fresh footprints")));
	TestTrue(TEXT("Drying prints are noticed"), Trail::DescribeTrail(0, 5, 0.2f).Contains(TEXT("shrinking")));
	TestTrue(TEXT("One print is not a trail"), Trail::DescribeTrail(0, 1, 0.9f).IsEmpty());

	FIslandTrailLedger Full;
	for (int32 I = 0; I < FIslandTrailLedger::MaxCells; ++I) Full.AddStep(FVector(I * FIslandTrailLedger::CellSize, 0.f, 0.f), FVector::UpVector);
	TestFalse(TEXT("A full ledger refuses new ground"), Full.AddStep(FVector(0.f, 99999.f, 0.f), FVector::UpVector));
	TestTrue(TEXT("But still counts old paths"), Full.AddStep(FVector(0.f, 0.f, 0.f), FVector::UpVector));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTrailVisitorTest, "CaptiveSky2.Agent.IslandTrailVisitor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTrailVisitorTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Synthetic visitor trail world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandTrailSubsystem* Trail = World->GetSubsystem<UIslandTrailSubsystem>();
	TestNotNull(TEXT("Synthetic world owns the persistent trail subsystem"), Trail);
	if (!Trail)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Trail->bAllowStorage = false;
	World->BeginPlay();

	ACharacter* Visitor = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 90.f), FRotator::ZeroRotator);
	UCaptiveSkyAmbientSpeechWidget* Caption = NewObject<UCaptiveSkyAmbientSpeechWidget>(World, NAME_None, RF_Transient);
	AIslandInteractionTestPlayerController* Controller = World->SpawnActor<AIslandInteractionTestPlayerController>();
	TestNotNull(TEXT("Visitor pawn created over the trail"), Visitor);
	TestNotNull(TEXT("Caption widget created for trail reading"), Caption);
	TestNotNull(TEXT("Local visitor controller created for trail reading"), Controller);
	if (Visitor && Caption && Controller)
	{
		FIslandTrailCell& Cell = Trail->GetLedgerMutable().Cells.Add(FIslandTrailLedger::CellFor(Visitor->GetActorLocation()));
		Cell.Steps = UIslandTrailSubsystem::WearFullSteps;
		Cell.Position = Visitor->GetActorLocation();
		Cell.NormalX = 0.f;
		Cell.NormalY = 0.f;
		Controller->SetFixturePawn(Visitor);
		Controller->SetCaptionWidget(Caption);
		Controller->BindFixtureInput();
		Controller->RefreshInteractionHintForTest();
		TestEqual(TEXT("A visitor over persistent worn tracks gets a read-only prompt when no landmark is nearer"),
			Caption->GetDisplayedInteractionHint().ToString(), FString(TEXT("E: read the nearby tracks")));
		TestTrue(TEXT("E is bound to the visitor's interaction handler"), Controller->PressBoundE());
		TestTrue(TEXT("The visitor learns that repeated feet wore this track"),
			Caption->GetDisplayedCaption().ToString().Contains(TEXT("trodden bare along a track")));
		TestEqual(TEXT("Reading the trail does not add footfalls"), Trail->GetLedger().StepsAt(Visitor->GetActorLocation()), UIslandTrailSubsystem::WearFullSteps);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
