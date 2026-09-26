#include "IslandWeather.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "IslandPoolRippleEffect.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWeather, Log, All);

AIslandWeather::AIslandWeather()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RainStreaks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainStreaks"));
	RainStreaks->SetupAttachment(RootComponent);
	RainStreaks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainStreaks->SetCastShadow(false);
	RainStreaks->bReceivesDecals = false;
	RainStreaks->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RainMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (RainMesh.Succeeded()) RainStreaks->SetStaticMesh(RainMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RainMaterial(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (RainMaterial.Succeeded()) RainStreaks->SetMaterial(0, RainMaterial.Object);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

void AIslandWeather::BeginPlay()
{
	Super::BeginPlay();
	UpdateCloudRendering();
	UpdateRainRendering();
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
	if (RainStreaks) RainStreaks->SetVisibility(false, true);
	RainStreakMaterial = nullptr;
	ActiveRainStreakCount = 0;
	for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
		if (Firefly.IsValid()) Firefly->Destroy();
	NightFireflies.Reset();
	Super::EndPlay(EndPlayReason);
}

void AIslandWeather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCloudRendering();
	UpdateRainRendering();
}

bool AIslandWeather::InitializeRainRendering()
{
	if (bRainPoolInitialized) return true;
	if (!RainStreaks || !RainStreaks->GetStaticMesh()) return false;
	UMaterialInterface* BaseMaterial = RainStreaks->GetMaterial(0);
	if (!BaseMaterial || (BaseMaterial->GetBlendMode() != BLEND_Translucent && BaseMaterial->GetBlendMode() != BLEND_Additive))
	{
		UE_LOG(LogIslandWeather, Warning, TEXT("Rain streak rendering requires a translucent particle material; keeping the visual disabled."));
		return false;
	}
	RainStreakMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (!RainStreakMaterial) return false;
	RainStreaks->SetMaterial(0, RainStreakMaterial);
	const int32 PoolSize = FMath::Clamp(RainStreakCount, 16, 192);
	RainStreaks->ClearInstances();
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		RainStreaks->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), false);
	}
	bRainPoolInitialized = true;
	return true;
}

void AIslandWeather::UpdateRainRendering()
{
	if (!GetWorld() || !RainStreaks) return;
	CurrentRainIntensity = SampleRainIntensity(GetWorld()->GetTimeSeconds());
	if (CurrentRainIntensity <= 0.04f || !InitializeRainRendering())
	{
		ActiveRainStreakCount = 0;
		RainStreaks->SetVisibility(false, true);
		return;
	}

	const int32 PoolSize = FMath::Clamp(RainStreakCount, 16, 192);
	ActiveRainStreakCount = FMath::Clamp(FMath::RoundToInt(PoolSize * CurrentRainIntensity), 1, PoolSize);
	RainStreaks->SetVisibility(true, true);
	const double Now = GetWorld()->GetTimeSeconds();
	FVector VisualizationCenter = GetActorLocation();
	const AActor* WindObserver = this;
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		if (APawn* PlayerPawn = PlayerController->GetPawn())
		{
			VisualizationCenter = PlayerPawn->GetActorLocation();
			WindObserver = PlayerPawn;
		}
	RainStreaks->SetWorldLocation(VisualizationCenter);
	if (CurrentRainIntensity >= 0.55f) UpdateRainPoolResponse();
	const FVector Wind = GetLocalWind(VisualizationCenter, WindObserver);
	const FVector Flow = FVector(Wind.X, Wind.Y, -1800.f).GetSafeNormal();
	const FQuat StreakRotation = FQuat::FindBetweenNormals(FVector::UpVector, Flow);
	const float Radius = FMath::Clamp(RainVisualizationRadius, 1000.f, 12000.f);
	const float Height = FMath::Clamp(RainVisualizationHeight, 1000.f, 5000.f);
	const float FallSpeed = 1800.f;
	const float FallPeriod = Height / FallSpeed;

	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		FVector Position = FVector::ZeroVector;
		FVector Scale = FVector::ZeroVector;
		if (Index < ActiveRainStreakCount)
		{
			const double Seed = WeatherSeed * 0.071 + Index * 0.6180339887498949;
			const float X = static_cast<float>(FMath::Frac(Seed * 1.37) * 2.0 - 1.0);
			const float Y = static_cast<float>(FMath::Frac(Seed * 2.11) * 2.0 - 1.0);
			const float StartHeight = static_cast<float>(FMath::Frac(Seed * 3.17 + Now / FallPeriod) * Height);
			const float FallAge = (Height - StartHeight) / FallSpeed;
			const float OffsetX = static_cast<float>(FMath::Fmod(X * Radius + Wind.X * FallAge, Radius * 2.f));
			const float OffsetY = static_cast<float>(FMath::Fmod(Y * Radius + Wind.Y * FallAge, Radius * 2.f));
			Position = FVector(OffsetX, OffsetY, StartHeight - Height * 0.5f);
			Scale = FVector(0.008f, 0.008f, 0.45f);
		}
		const FTransform Transform(StreakRotation, Position, Scale);
		RainStreaks->UpdateInstanceTransform(Index, Transform, false, Index == PoolSize - 1, true);
	}
}

void AIslandWeather::UpdateRainPoolResponse()
{
	if (!GetWorld() || RainPoolRipple.IsValid() || GetWorld()->GetTimeSeconds() < NextRainPoolRippleTime) return;
	AActor* Pool = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("TideglassPool"))) continue;
		Pool = *It;
		break;
	}
	if (!Pool) return;
	const double Now = GetWorld()->GetTimeSeconds();
	const double Phase = Now * 1.7 + WeatherSeed * 0.37;
	const FVector Offset(FMath::Cos(Phase) * 38.f, FMath::Sin(Phase * 1.13) * 38.f, 0.f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(Pool->GetActorLocation() + Offset, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsRainImpact();
		RainPoolRipple = Ripple;
		NextRainPoolRippleTime = Now + 4.5;
	}
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
		bHasStormCloudsParameter = ScalarParameters.ContainsByPredicate([this](const FMaterialParameterInfo& Parameter) { return Parameter.Name == StormCloudsParameter; });
		if (!bHasCloudCoverageParameter && !bHasCloudDensityParameter && !bHasStormCloudsParameter)
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
		if (bHasStormCloudsParameter) OriginalStormClouds = DynamicMaterial->K2_GetScalarParameterValue(StormCloudsParameter);
		Cloud->SetMaterial(DynamicMaterial);
		UE_LOG(LogIslandWeather, Log, TEXT("Weather linked cloud material %s (coverage %s, density %s, storm %s)."), *BaseMaterial->GetName(), bHasCloudCoverageParameter ? TEXT("enabled") : TEXT("unavailable"), bHasCloudDensityParameter ? TEXT("enabled") : TEXT("unavailable"), bHasStormCloudsParameter ? TEXT("enabled") : TEXT("unavailable"));
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
	if (bHasStormCloudsParameter)
		WeatherCloudMaterial->SetScalarParameterValue(StormCloudsParameter, OriginalStormClouds + 0.45f * SampleRainIntensity(GetWorld()->GetTimeSeconds()));
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

float AIslandWeather::SampleRainIntensity(double Seconds) const
{
	const double RainPeriod = FMath::Max(30.f, CycleSeconds) * FMath::Clamp(RainCycleMultiplier, 1.f, 8.f);
	const double FrontPhase = Seconds / RainPeriod * 2.0 * PI + WeatherSeed * 0.13 + 2.1;
	const float FrontStrength = static_cast<float>(0.5 + 0.5 * FMath::Sin(FrontPhase));
	const float RainFront = FMath::SmoothStep(0.62f, 0.90f, FrontStrength);
	const float CloudGate = FMath::SmoothStep(0.48f, 0.78f, SampleCloudCover(Seconds));
	return FMath::Clamp(RainFront * CloudGate, 0.f, 1.f);
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
	const float Rain = SampleRainIntensity(Now);
	const FVector Wind = GetLocalWind(Position, Observer);
	const bool bFeelingLocalGust = TransientGusts.ContainsByPredicate([&Position, Now](const FIslandTransientGust& Gust)
	{
		return !EvaluateTransientGust(Gust, Position, Now).IsNearlyZero(5.f);
	});
	const TCHAR* Conditions = Rain > 0.55f ? TEXT("a passing rain shower") : Rain > 0.08f ? TEXT("light rain beginning or fading") : Cloud < 0.3f ? TEXT("mostly clear") : Cloud < 0.7f ? TEXT("cloud cover gathering or clearing") : TEXT("overcast, but currently dry");
	return FString::Printf(TEXT(" Local weather simulation: %s; wind towards world XY (%.2f, %.2f), %.1f metres/second, vertical current %.1f metres/second.%s Cloud coverage, density and storm character follow slow deterministic weather cycles and gently soften sunlight/skylight. Rain has a bounded simulated intensity (%.0f%%), but raindrop effects and weather sounds are not yet rendered."),
		Conditions,
		Wind.GetSafeNormal().X, Wind.GetSafeNormal().Y, Wind.Size() / 100.f, Wind.Z / 100.f,
		bFeelingLocalGust ? TEXT(" A fading local gust is still changing the wind nearby.") : TEXT(""), Rain * 100.f);
}
