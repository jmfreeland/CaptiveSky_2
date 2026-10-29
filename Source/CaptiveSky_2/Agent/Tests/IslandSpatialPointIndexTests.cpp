#include "Misc/AutomationTest.h"
#include "Agent/IslandSpatialPointIndex.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandSpatialPointIndexTest,
	"CaptiveSky2.Agent.IslandSpatialPointIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandSpatialPointIndexTest::RunTest(const FString& Parameters)
{
	FIslandSpatialPointIndex Index(1000.f);
	TestFalse(TEXT("An empty index has no neighbors"), Index.IsWithinDistance(FVector::ZeroVector, 100.f));

	Index.Add(FVector(-1000.f, -1000.f, 50.f));
	TestEqual(TEXT("A finite point is indexed"), Index.Num(), 1);
	TestTrue(TEXT("A point in an adjacent negative-coordinate cell is found"),
		Index.IsWithinDistance(FVector(-1.f, -1.f, 9999.f), 1500.f));
	TestFalse(TEXT("An exact-radius neighbor is not considered too close"),
		Index.IsWithinDistance(FVector(0.f, -1000.f, 50.f), 1000.f));
	TestFalse(TEXT("A zero radius never reports a neighbor"), Index.IsWithinDistance(FVector(-1000.f, -1000.f, 50.f), 0.f));
	Index.Add(FVector(std::numeric_limits<double>::quiet_NaN(), 0.f, 0.f));
	TestEqual(TEXT("Non-finite coordinates are ignored"), Index.Num(), 1);
	Index.Reset();
	TestEqual(TEXT("Reset clears the indexed-point count"), Index.Num(), 0);

	FRandomStream Random(0x1A51);
	TArray<FVector> Points;
	Points.Reserve(512);
	for (int32 PointIndex = 0; PointIndex < 512; ++PointIndex)
	{
		const FVector Point(Random.FRandRange(-25000.f, 25000.f), Random.FRandRange(-25000.f, 25000.f),
			Random.FRandRange(-1000.f, 5000.f));
		Points.Add(Point);
		Index.Add(Point);
	}

	for (int32 ProbeIndex = 0; ProbeIndex < 256; ++ProbeIndex)
	{
		const FVector Probe(Random.FRandRange(-25000.f, 25000.f), Random.FRandRange(-25000.f, 25000.f),
			Random.FRandRange(-1000.f, 5000.f));
		const float Radius = Random.FRandRange(1.f, 3500.f);
		bool bBruteForceResult = false;
		for (const FVector& Point : Points)
		{
			if (FVector::Dist2D(Probe, Point) < Radius)
			{
				bBruteForceResult = true;
				break;
			}
		}
		TestEqual(FString::Printf(TEXT("Spatial result matches exact brute-force clearance at probe %d"), ProbeIndex),
			Index.IsWithinDistance(Probe, Radius), bBruteForceResult);
	}
	return true;
}
