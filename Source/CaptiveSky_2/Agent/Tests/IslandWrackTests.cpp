#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "IslandChronicle.h"
#include "IslandDayNight.h"
#include "IslandInteractionUtility.h"
#include "IslandWrack.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

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

	const TArray<FString> Chronicle = {
		TEXT("{\"type\":\"decision\",\"agent\":\"Raven\",\"day\":3,\"text\":\"The wind is dropping, so I will check the eastern roost again.\"}"),
		TEXT("{\"type\":\"session\",\"day\":3,\"text\":\"The session ends. It is a long line of text here.\"}"),
		TEXT("{\"type\":\"decision\",\"agent\":\"Aster\",\"day\":9,\"text\":\"Too recent to have been bottled and carried away.\"}"),
		TEXT("{\"type\":\"decision\",\"agent\":\"Aster\",\"day\":2,\"text\":\"short\"}"),
		TEXT("not json at all")};
	const FString Echo = Wrack::EchoFromChronicle(Chronicle, 4, 5);
	TestTrue(TEXT("An earlier thought goes in the bottle"), Echo.Contains(TEXT("eastern roost")) && Echo.Contains(TEXT("Raven")));
	TestFalse(TEXT("A thought from today stays out"), Echo.Contains(TEXT("Too recent")));
	TestTrue(TEXT("No suitable line means no note"), Wrack::EchoFromChronicle(Chronicle, 4, 1).IsEmpty() && Wrack::EchoFromChronicle(TArray<FString>(), 4, 5).IsEmpty());

	TestEqual(TEXT("Tags round-trip to ids"), Wrack::ItemIdFromTag(Wrack::TargetTagFor(42)), 42);
	TestEqual(TEXT("Other tags are not wrack"), Wrack::ItemIdFromTag(FName(TEXT("WindArch"))), 0);
	TestEqual(TEXT("A malformed tag is not wrack"), Wrack::ItemIdFromTag(FName(TEXT("Wrack_x"))), 0);

	FIslandWrackLedger RavenAwareness;
	const int32 NearestShorefall = RavenAwareness.Add(Kind::Kelp, FVector(55674.f, 0.f, 0.f), 0.f, 11, 10).Id;
	RavenAwareness.Add(Kind::Shells, FVector(58000.f, 0.f, 0.f), 0.f, 12, 10);
	const int32 AlreadyTurned = RavenAwareness.Add(Kind::Driftwood, FVector(54000.f, 0.f, 0.f), 0.f, 13, 10).Id;
	FIslandWrackItem TurnedItem;
	RavenAwareness.Turn(AlreadyTurned, TEXT("Aster"), TurnedItem);
	RavenAwareness.Add(Kind::Float, FVector(UIslandWrackSubsystem::RavenShoreAwarenessRadius + 1.f, 0.f, 0.f), 0.f, 14, 10);
	RavenAwareness.Add(Kind::Shells, FVector(2000.f, 0.f, 0.f), 0.f, 15, 10);
	const FString RavenShoreCue = Wrack::DescribeShoreForRaven(RavenAwareness.Items, FVector::ZeroVector, 10);
	TestTrue(TEXT("The Raven receives the nearest distant shore target"), RavenShoreCue.Contains(Wrack::TargetTagFor(NearestShorefall).ToString()));
	TestTrue(TEXT("The Raven's shore cue explains that flight and interaction are optional"), RavenShoreCue.Contains(TEXT("invitation, not an obligation")));
	TestFalse(TEXT("Already-turned wrack is not offered again"), RavenShoreCue.Contains(Wrack::TargetTagFor(AlreadyTurned).ToString()));
	TestFalse(TEXT("Wrack beyond the Raven's awareness radius is omitted"), RavenShoreCue.Contains(TEXT("Wrack_4")));
	TestFalse(TEXT("Nearby wrack remains in the local observation channel"), RavenShoreCue.Contains(TEXT("Wrack_5")));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWrackBottleInteractionTest, "CaptiveSky2.Agent.IslandWrackBottleInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWrackBottleInteractionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Transient wrack interaction world is created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	const FString TestFolder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/WrackBottle"));
	const FString TestStem = FPaths::Combine(TestFolder, FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const FString ChroniclePath = TestStem + TEXT("_chronicle.jsonl");
	const FString WrackPath = TestStem + TEXT("_wrack.json");
	IFileManager::Get().MakeDirectory(*TestFolder, true);
	ON_SCOPE_EXIT
	{
		IFileManager::Get().Delete(*ChroniclePath, false, true);
		IFileManager::Get().Delete(*WrackPath, false, true);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	UIslandChronicleSubsystem* Chronicle = World->GetSubsystem<UIslandChronicleSubsystem>();
	UIslandWrackSubsystem* Wrack = World->GetSubsystem<UIslandWrackSubsystem>();
	if (!TestNotNull(TEXT("Transient world chronicle subsystem exists"), Chronicle) ||
		!TestNotNull(TEXT("Transient world wrack subsystem exists"), Wrack)) return false;
	Chronicle->ChronicleFileOverride = ChroniclePath;
	Wrack->StorageFileOverride = WrackPath;
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Island clock is available for the interaction day"), Clock)) return false;
	Clock->DayNumber = 5;

	const FString EarlierThought = TEXT("I left the western stones in a little circle so the raven might notice.");
	const FString ChronicleLine = UIslandChronicleSubsystem::FormatEntry(FDateTime::UtcNow(), 3, TEXT("18:30"),
		TEXT("decision"), TEXT("Aster"), EarlierThought, {});
	TestTrue(TEXT("Only the transient test chronicle receives the earlier thought"),
		FFileHelper::SaveStringToFile(ChronicleLine + LINE_TERMINATOR, *ChroniclePath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	const FIslandWrackItem& Float = Wrack->GetLedgerMutable().Add(EIslandWrackKind::Float,
		FVector(100.f, 200.f, 0.f), 0.f, 22, 4);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATargetPoint* Raven = World->SpawnActor<ATargetPoint>(FVector(100.f, 200.f, 0.f), FRotator::ZeroRotator, Spawn);
	ATargetPoint* FloatActor = World->SpawnActor<ATargetPoint>(Float.Position, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Raven observer fixture exists"), Raven) || !TestNotNull(TEXT("Glass float target fixture exists"), FloatActor)) return false;
	FloatActor->Tags = { UIslandWrackSubsystem::TargetTagFor(Float.Id), TEXT("IslandLandmark"), TEXT("StormWrack") };
	FString Fact;
	TestTrue(TEXT("Inspecting the float uses the ordinary landmark interaction route"),
		IslandInteractionUtility::Perform(Raven, FloatActor, Fact));
	TestTrue(TEXT("The earlier thought is attributed and quoted as the bottle's discovery"),
		Fact.Contains(TEXT("Aster")) && Fact.Contains(TEXT("Island day 3")) && Fact.Contains(EarlierThought));
	const FIslandWrackItem* TurnedFloat = Wrack->GetLedger().Find(Float.Id);
	if (TestNotNull(TEXT("The inspected float remains in the local shore ledger"), TurnedFloat))
	{
		TestTrue(TEXT("The float stays turned over"), TurnedFloat->bTurned);
		TestEqual(TEXT("The raven receives credit for finding the note"), TurnedFloat->TurnedBy, Raven->GetName());
		TestTrue(TEXT("The discovered note is saved with the wrack record"), TurnedFloat->Find.Contains(EarlierThought));
	}
	FString SavedWrack;
	FIslandWrackLedger Reloaded;
	TestTrue(TEXT("The test-only shore file was written"), FFileHelper::LoadFileToString(SavedWrack, *WrackPath));
	TestTrue(TEXT("The bottle's opened state and note survive reloading"), Reloaded.FromJson(SavedWrack) &&
		Reloaded.Find(Float.Id) && Reloaded.Find(Float.Id)->bTurned && Reloaded.Find(Float.Id)->Find.Contains(EarlierThought));
	return true;
}
