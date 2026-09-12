#include "IslandWeather.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AIslandWeather::AIslandWeather()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = false;
}

float AIslandWeather::SampleCloudCover(double Seconds) const
{
	const double Phase = Seconds / FMath::Max(30.f, CycleSeconds) * 2.0 * PI + WeatherSeed * 0.37;
	return static_cast<float>(0.5 + 0.5 * FMath::Sin(Phase));
}

FVector AIslandWeather::SampleWind(const FVector& Position, double Seconds) const
{
	const double Phase = Seconds / FMath::Max(30.f, CycleSeconds) * 2.0 * PI + WeatherSeed * 0.37;
	const double Heading = Phase * 0.3 + FMath::Sin(Seconds / 43.0) * 0.25;
	const double Gust = 0.65 + 0.35 * FMath::Sin(Seconds / 7.0 + Position.X / 1700.0 + Position.Y / 2300.0);
	const double Speed = FMath::Clamp(MaximumWindSpeed, 0.f, 300.f) * (0.25 + 0.75 * SampleCloudCover(Seconds)) * Gust;
	return FVector(FMath::Cos(Heading), FMath::Sin(Heading),
		0.18 * FMath::Sin(Position.X / 2100.0 + Seconds / 19.0)) .GetSafeNormal() * Speed;
}

FVector AIslandWeather::GetLocalWind(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return FVector::ZeroVector;
	FVector Wind = SampleWind(Position, GetWorld()->GetTimeSeconds());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandWindShelter), false);
	if (Observer) Params.AddIgnoredActor(Observer);
	FHitResult Hit;
	// Nearby upwind geometry attenuates the actual movement signal, not just its description.
	if (GetWorld()->LineTraceSingleByChannel(Hit, Position, Position - Wind.GetSafeNormal() * 600.f, ECC_Visibility, Params))
		Wind *= 0.15f;
	return Wind;
}

FString AIslandWeather::DescribeAt(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return FString();
	const float Cloud = SampleCloudCover(GetWorld()->GetTimeSeconds());
	const FVector Wind = GetLocalWind(Position, Observer);
	return FString::Printf(TEXT(" Local weather simulation: %s; wind towards world XY (%.2f, %.2f), %.1f metres/second, vertical current %.1f metres/second. Cloud cover is a simulation signal; cloud/rain visuals and sounds are not yet connected."),
		Cloud < 0.3f ? TEXT("mostly clear") : Cloud < 0.7f ? TEXT("cloud cover gathering or clearing") : TEXT("overcast"),
		Wind.GetSafeNormal().X, Wind.GetSafeNormal().Y, Wind.Size() / 100.f, Wind.Z / 100.f);
}
