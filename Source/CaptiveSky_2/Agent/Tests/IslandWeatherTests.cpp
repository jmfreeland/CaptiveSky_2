#include "Misc/AutomationTest.h"
#include "IslandWeather.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherTest, "CaptiveSky2.Agent.IslandWeather",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherTest::RunTest(const FString& Parameters)
{
	const AIslandWeather* Weather = GetDefault<AIslandWeather>();
	for (int32 I = 0; I < 1000; ++I)
	{
		const FVector Position(I * 50.0, -I * 30.0, 700.0);
		const FVector Wind = Weather->SampleWind(Position, I);
		TestTrue(TEXT("Wind remains finite and bounded"), !Wind.ContainsNaN() && Wind.Size() <= Weather->MaximumWindSpeed + 0.01f);
		const float Cloud = Weather->SampleCloudCover(I);
		TestTrue(TEXT("Cloud cover is normalized"), Cloud >= 0.f && Cloud <= 1.f);
		TestTrue(TEXT("Sampling is repeatable"), Wind.Equals(Weather->SampleWind(Position, I)));
	}
	TestFalse(TEXT("Weather changes over time"), Weather->SampleWind(FVector::ZeroVector, 0).Equals(Weather->SampleWind(FVector::ZeroVector, 100)));
	TestFalse(TEXT("Currents vary across the Island"), Weather->SampleWind(FVector::ZeroVector, 0).Equals(Weather->SampleWind(FVector(1000, 1000, 0), 0)));

	FIslandTransientGust Gust;
	Gust.Center = FVector::ZeroVector;
	Gust.Direction = FVector::ForwardVector;
	Gust.PeakSpeed = 200.f;
	Gust.Radius = 1000.f;
	Gust.StartedAt = 10.0;
	Gust.ExpiresAt = 20.0;
	TestTrue(TEXT("Gust begins at full strength at its center"), AIslandWeather::EvaluateTransientGust(Gust, FVector::ZeroVector, 10.0).Equals(FVector(200.f, 0.f, 0.f)));
	TestTrue(TEXT("Gust fades with distance and time"), AIslandWeather::EvaluateTransientGust(Gust, FVector(500.f, 0.f, 0.f), 15.0).Equals(FVector(50.f, 0.f, 0.f)));
	TestTrue(TEXT("Gust has no effect beyond its radius"), AIslandWeather::EvaluateTransientGust(Gust, FVector(1001.f, 0.f, 0.f), 15.0).IsNearlyZero());
	TestTrue(TEXT("Gust expires cleanly"), AIslandWeather::EvaluateTransientGust(Gust, FVector::ZeroVector, 20.0).IsNearlyZero());
	return true;
}
