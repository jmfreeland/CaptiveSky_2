#include "IslandWeather.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWeather, Log, All);

AIslandWeather::AIslandWeather()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

void AIslandWeather::BeginPlay()
{
	Super::BeginPlay();
	UpdateCloudRendering();
	RefreshNightEcology();
	GetWorldTimerManager().SetTimer(EcologyTimerHandle, this, &AIslandWeather::RefreshNightEcology, 30.f, true, 30.f);
}

void AIslandWeather::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(EcologyTimerHandle);
	if (UVolumetricCloudComponent* Cloud = CloudComponent.Get())
		if (OriginalCloudMaterial) Cloud->SetMaterial(OriginalCloudMaterial);
	CloudComponent.Reset();
	WeatherCloudMaterial = nullptr;
	OriginalCloudMaterial = nullptr;
	for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
		if (Firefly.IsValid()) Firefly->Destroy();
	NightFireflies.Reset();
	Super::EndPlay(EndPlayReason);
}

void AIslandWeather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCloudRendering();
}

bool AIslandWeather::InitializeCloudRendering()
{
	if (CloudComponent.IsValid() && WeatherCloudMaterial) return true;
	if (!GetWorld()) return false;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextCloudDiscoveryTime) return false;
	NextCloudDiscoveryTime = Now + 1.0;
	for (TActorIterator<AVolumetricCloud> It(GetWorld()); It; ++It)
	{
		UVolumetricCloudComponent* Cloud = It->FindComponentByClass<UVolumetricCloudComponent>();
		if (!Cloud) continue;
		UMaterialInterface* BaseMaterial = Cloud->GetMaterial();
		if (!BaseMaterial) continue;

		TArray<FMaterialParameterInfo> ScalarParameters;
		TArray<FGuid> ParameterIds;
		BaseMaterial->GetAllScalarParameterInfo(ScalarParameters, ParameterIds);
		bHasCloudCoverageParameter = ScalarParameters.ContainsByPredicate([this](const FMaterialParameterInfo& Parameter) { return Parameter.Name == CloudCoverageParameter; });
		bHasCloudDensityParameter = ScalarParameters.ContainsByPredicate([this](const FMaterialParameterInfo& Parameter) { return Parameter.Name == CloudDensityParameter; });
		if (!bHasCloudCoverageParameter && !bHasCloudDensityParameter)
		{
			if (!bCloudParameterWarningLogged)
			{
				UE_LOG(LogIslandWeather, Warning, TEXT("Cloud material %s exposes neither configured weather parameter; leaving it unchanged."), *BaseMaterial->GetName());
				bCloudParameterWarningLogged = true;
			}
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (!DynamicMaterial) return false;
		CloudComponent = Cloud;
		OriginalCloudMaterial = BaseMaterial;
		WeatherCloudMaterial = DynamicMaterial;
		if (bHasCloudCoverageParameter) OriginalCloudCoverage = DynamicMaterial->K2_GetScalarParameterValue(CloudCoverageParameter);
		if (bHasCloudDensityParameter) OriginalCloudDensity = DynamicMaterial->K2_GetScalarParameterValue(CloudDensityParameter);
		Cloud->SetMaterial(DynamicMaterial);
		UE_LOG(LogIslandWeather, Log, TEXT("Weather linked cloud material %s (coverage %s, density %s)."), *BaseMaterial->GetName(), bHasCloudCoverageParameter ? TEXT("enabled") : TEXT("unavailable"), bHasCloudDensityParameter ? TEXT("enabled") : TEXT("unavailable"));
		return true;
	}
	return false;
}

void AIslandWeather::UpdateCloudRendering()
{
	if (!GetWorld()) return;
	if (!WeatherCloudMaterial && !InitializeCloudRendering()) return;
	const float Cover = SampleCloudCover(GetWorld()->GetTimeSeconds());
	if (bHasCloudCoverageParameter)
		WeatherCloudMaterial->SetScalarParameterValue(CloudCoverageParameter, OriginalCloudCoverage + (Cover - 0.5f) * 0.08f);
	if (bHasCloudDensityParameter)
		WeatherCloudMaterial->SetScalarParameterValue(CloudDensityParameter, OriginalCloudDensity * FMath::Lerp(0.82f, 1.18f, Cover));
}

void AIslandWeather::RefreshNightEcology()
{
	NightFireflies.RemoveAll([](const TWeakObjectPtr<AIslandFirefly>& Firefly) { return !Firefly.IsValid(); });
	if (!GetWorld()) return;

	bool bNight = false;
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		bNight = It->CurrentHour >= 19.f || It->CurrentHour < 5.f;
		break;
	}

	AActor* Habitat = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TideglassPool")))
		{
			Habitat = *It;
			break;
		}
	}

	constexpr int32 NightPopulation = 3;
	if (!bNight || !Habitat)
	{
		for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
			if (Firefly.IsValid()) Firefly->Destroy();
		NightFireflies.Reset();
		return;
	}

	while (NightFireflies.Num() < NightPopulation)
	{
		const FVector GroundOffset(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(15.f, 35.f));
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AIslandFirefly* Firefly = GetWorld()->SpawnActor<AIslandFirefly>(Habitat->GetActorLocation() + GroundOffset, FRotator::ZeroRotator, SpawnParameters))
			NightFireflies.Add(Firefly);
		else
			break;
	}
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
	const double Now = GetWorld()->GetTimeSeconds();
	FVector Wind = SampleWind(Position, Now);
	for (const FIslandTransientGust& Gust : TransientGusts)
	{
		Wind += EvaluateTransientGust(Gust, Position, Now);
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandWindShelter), false);
	if (Observer) Params.AddIgnoredActor(Observer);
	FHitResult Hit;
	// Nearby upwind geometry attenuates the actual movement signal, not just its description.
	if (GetWorld()->LineTraceSingleByChannel(Hit, Position, Position - Wind.GetSafeNormal() * 600.f, ECC_Visibility, Params))
		Wind *= 0.15f;
	return Wind;
}

FVector AIslandWeather::EvaluateTransientGust(const FIslandTransientGust& Gust, const FVector& Position, double CurrentTime)
{
	const double Duration = Gust.ExpiresAt - Gust.StartedAt;
	if (Duration <= 0.0 || CurrentTime < Gust.StartedAt || CurrentTime >= Gust.ExpiresAt || Gust.Radius <= 0.f) return FVector::ZeroVector;
	const float SpatialWeight = 1.f - FMath::Clamp(FVector::Dist(Position, Gust.Center) / Gust.Radius, 0.f, 1.f);
	const float TemporalWeight = FMath::Clamp(static_cast<float>((Gust.ExpiresAt - CurrentTime) / Duration), 0.f, 1.f);
	return Gust.Direction * Gust.PeakSpeed * SpatialWeight * TemporalWeight;
}

void AIslandWeather::AddTransientGust(const FVector& Center, const FVector& Direction, float PeakSpeed, float Radius, float DurationSeconds)
{
	if (!GetWorld() || Direction.IsNearlyZero()) return;
	const double Now = GetWorld()->GetTimeSeconds();
	TransientGusts.RemoveAll([Now](const FIslandTransientGust& Gust) { return Gust.ExpiresAt <= Now; });
	// Keep the signal lightweight even if several residents act in quick succession.
	if (TransientGusts.Num() >= 16) TransientGusts.RemoveAt(0);

	FIslandTransientGust& Gust = TransientGusts.AddDefaulted_GetRef();
	Gust.Center = Center;
	Gust.Direction = Direction.GetSafeNormal();
	Gust.PeakSpeed = FMath::Clamp(PeakSpeed, 0.f, 300.f);
	Gust.Radius = FMath::Clamp(Radius, 100.f, 3000.f);
	Gust.StartedAt = Now;
	Gust.ExpiresAt = Now + FMath::Clamp(DurationSeconds, 1.f, 18.f);
}

FString AIslandWeather::DescribeAt(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return FString();
	const double Now = GetWorld()->GetTimeSeconds();
	const float Cloud = SampleCloudCover(Now);
	const FVector Wind = GetLocalWind(Position, Observer);
	const bool bFeelingLocalGust = TransientGusts.ContainsByPredicate([&Position, Now](const FIslandTransientGust& Gust)
	{
		return !EvaluateTransientGust(Gust, Position, Now).IsNearlyZero(5.f);
	});
	return FString::Printf(TEXT(" Local weather simulation: %s; wind towards world XY (%.2f, %.2f), %.1f metres/second, vertical current %.1f metres/second.%s Cloud coverage and density shift with the simulation and gently soften sunlight and skylight; rain and weather sounds are not yet rendered."),
		Cloud < 0.3f ? TEXT("mostly clear") : Cloud < 0.7f ? TEXT("cloud cover gathering or clearing") : TEXT("overcast"),
		Wind.GetSafeNormal().X, Wind.GetSafeNormal().Y, Wind.Size() / 100.f, Wind.Z / 100.f,
		bFeelingLocalGust ? TEXT(" A fading local gust is still changing the wind nearby.") : TEXT(""));
}
