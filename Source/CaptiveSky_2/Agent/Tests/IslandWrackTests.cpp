#include "Misc/AutomationTest.h"
#include "IslandWrack.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWrackTest, "CaptiveSky2.Agent.IslandWrack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWrackTest::RunTest(const FString& Parameters)
{
	using Wrack = UIslandWrackSubsystem;
	using Kind = EIslandWrackKind;

	int32 Counts[4] = {0, 0, 0, 0};
	for (int32 Seed = 0; Seed < 1000; ++Seed) ++Counts[static_cast<int32>(Wrack::PickKind(Seed))];
	TestTrue(TEXT("Driftwood is common"), Counts[0] > Counts[3]);
	TestTrue(TEXT("Floats are rare but exist"), Counts[3] > 0 && Counts[3] < Counts[1]);
	TestTrue(TEXT("Every kind turns up"), Counts[0] > 0 && Counts[1] > 0 && Counts[2] > 0);

	for (int32 K = 0; K < 4; ++K)
	{
		const Kind ThisKind = static_cast<Kind>(K);
		TestFalse(TEXT("Every kind has something underneath"), Wrack::FindFor(ThisKind, 11).IsEmpty());
		TestEqual(TEXT("What is underneath is fixed by the seed"), Wrack::FindFor(ThisKind, 77), Wrack::FindFor(ThisKind, 77));
		TestFalse(TEXT("Every kind can be described"), Wrack::DescribeItem(ThisKind, 0, false).IsEmpty());
		TestTrue(TEXT("A turned item says so"), Wrack::DescribeItem(ThisKind, 0, true).Contains(TEXT("turned over")));
		const TArray<FIslandWrackPiece> Pieces = AIslandWrack::Layout(ThisKind, 5);
		TestTrue(TEXT("A heap has pieces"), Pieces.Num() >= 1 && Pieces.Num() <= 10);
		TestEqual(TEXT("A heap is the same each time"), AIslandWrack::Layout(ThisKind, 5).Num(), Pieces.Num());
		for (const FIslandWrackPiece& Piece : Pieces)
		{
			const FVector Spot = Piece.Transform.GetLocation();
			TestTrue(TEXT("Pieces lie close together"), FVector2D(Spot.X, Spot.Y).Size() < 160.f);
			TestTrue(TEXT("Pieces rest near the sand"), Spot.Z < -AIslandWrack::OriginLift + 15.f && Spot.Z > -AIslandWrack::OriginLift - 1.f);
		}
		TestTrue(TEXT("The sea takes it back in time"), FIslandWrackLedger::LifespanDays(ThisKind) > 0);
		TestTrue(TEXT("Wet wrack differs from old wrack"), !AIslandWrack::ItemColor(ThisKind, 0, false).Equals(AIslandWrack::ItemColor(ThisKind, 100, false), 0.01f)
			|| ThisKind == Kind::Shells);
		TestFalse(TEXT("A turned item looks disturbed"), AIslandWrack::ItemColor(ThisKind, 2, false).Equals(AIslandWrack::ItemColor(ThisKind, 2, true), 0.01f));
	}
	TestTrue(TEXT("Only driftwood is long"), AIslandWrack::UsesLongPieces(Kind::Driftwood) && !AIslandWrack::UsesLongPieces(Kind::Kelp));
	TestTrue(TEXT("Driftwood bleaches"), AIslandWrack::ItemColor(Kind::Driftwood, 14, false).R > AIslandWrack::ItemColor(Kind::Driftwood, 0, false).R);
	TestTrue(TEXT("Seeds make different heaps"), AIslandWrack::Layout(Kind::Kelp, 1)[0].Transform.GetLocation() != AIslandWrack::Layout(Kind::Kelp, 2)[0].Transform.GetLocation());

	TestEqual(TEXT("Tags round-trip to ids"), Wrack::ItemIdFromTag(Wrack::TargetTagFor(42)), 42);
	TestEqual(TEXT("Other tags are not wrack"), Wrack::ItemIdFromTag(FName(TEXT("WindArch"))), 0);
	TestEqual(TEXT("A malformed tag is not wrack"), Wrack::ItemIdFromTag(FName(TEXT("Wrack_x"))), 0);

	FIslandWrackLedger Ledger;
	const int32 A = Ledger.Add(Kind::Driftwood, FVector(100.f, 200.f, 30.f), 45.f, 123, 10).Id;
	const int32 B = Ledger.Add(Kind::Kelp, FVector(500.f, 200.f, 30.f), 10.f, 456, 10).Id;
	TestTrue(TEXT("Ids are unique"), A != B && A > 0);
	TestEqual(TEXT("Two items lie on the shore"), Ledger.Items.Num(), 2);

	FIslandWrackItem Turned;
	TestTrue(TEXT("The first look turns it"), Ledger.Turn(A, TEXT("Raven"), Turned) == EIslandWrackTurn::Turned);
	TestEqual(TEXT("It remembers who"), Turned.TurnedBy, FString(TEXT("Raven")));
	TestEqual(TEXT("And what was there"), Turned.Find, Wrack::FindFor(Kind::Driftwood, 123));
	TestTrue(TEXT("The second look finds it already turned"), Ledger.Turn(A, TEXT("Aster"), Turned) == EIslandWrackTurn::AlreadyTurned);
	TestEqual(TEXT("The first visitor keeps the credit"), Turned.TurnedBy, FString(TEXT("Raven")));
	TestTrue(TEXT("An unknown item is missing"), Ledger.Turn(999, TEXT("Aster"), Turned) == EIslandWrackTurn::Missing);

	FIslandWrackLedger Loaded;
	TestTrue(TEXT("The shore reloads"), Loaded.FromJson(Ledger.ToJson()));
	TestEqual(TEXT("Items survive a save"), Loaded.Items.Num(), 2);
	const FIslandWrackItem* Reloaded = Loaded.Find(A);
	if (TestNotNull(TEXT("Item present"), Reloaded))
	{
		TestTrue(TEXT("Turned state survives"), Reloaded->bTurned);
		TestEqual(TEXT("The finder survives"), Reloaded->TurnedBy, FString(TEXT("Raven")));
		TestTrue(TEXT("Position survives"), Reloaded->Position.Equals(FVector(100.f, 200.f, 30.f), 0.01f));
		TestEqual(TEXT("Seed survives"), Reloaded->Seed, 123);
	}
	TestEqual(TEXT("New ids keep counting after a reload"), Loaded.Add(Kind::Shells, FVector::ZeroVector, 0.f, 1, 11).Id, B + 1);

	FIslandWrackLedger Garbage;
	Garbage.Add(Kind::Shells, FVector::ZeroVector, 0.f, 1, 0);
	TestFalse(TEXT("Unreadable text is refused"), Garbage.FromJson(TEXT("not json")));
	TestFalse(TEXT("A future version is refused"), Garbage.FromJson(TEXT("{\"version\":99,\"items\":[]}")));
	TestFalse(TEXT("An unknown kind is refused"), Garbage.FromJson(TEXT("{\"version\":1,\"items\":[{\"id\":1,\"kind\":\"anchor\",\"x\":0,\"y\":0,\"z\":0,\"day\":0}]}")));
	TestEqual(TEXT("A refused load leaves the ledger alone"), Garbage.Items.Num(), 1);

	FIslandWrackLedger Tide;
	Tide.Add(Kind::Kelp, FVector::ZeroVector, 0.f, 1, 10);
	Tide.Add(Kind::Driftwood, FVector(1000.f, 0.f, 0.f), 0.f, 2, 10);
	TestFalse(TEXT("Nothing goes on the day it lands"), Tide.Weather(10));
	TestFalse(TEXT("Nothing goes within its span"), Tide.Weather(10 + FIslandWrackLedger::LifespanDays(Kind::Kelp)));
	TestTrue(TEXT("The sea takes the kelp first"), Tide.Weather(11 + FIslandWrackLedger::LifespanDays(Kind::Kelp)));
	TestEqual(TEXT("The driftwood stays"), Tide.Items.Num(), 1);
	TestTrue(TEXT("Driftwood lasts longer than kelp"), FIslandWrackLedger::LifespanDays(Kind::Driftwood) > FIslandWrackLedger::LifespanDays(Kind::Kelp));
	TestTrue(TEXT("Then it goes too"), Tide.Weather(11 + FIslandWrackLedger::LifespanDays(Kind::Driftwood)) && Tide.Items.IsEmpty());

	FIslandWrackLedger Full;
	int32 First = 0;
	for (int32 I = 0; I < FIslandWrackLedger::MaxItems + 5; ++I)
	{
		const int32 Id = Full.Add(Kind::Shells, FVector(I * 300.f, 0.f, 0.f), 0.f, I, I).Id;
		if (I == 0) First = Id;
	}
	TestEqual(TEXT("A full shore is capped"), Full.Items.Num(), FIslandWrackLedger::MaxItems);
	TestNull(TEXT("The oldest wrack was taken back"), Full.Find(First));
	return true;
}
