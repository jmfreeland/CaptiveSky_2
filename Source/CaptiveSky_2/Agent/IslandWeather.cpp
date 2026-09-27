#include "IslandWeather.h"
#include "IslandDayNight.h"
#include "IslandFirefly.h"
#include "IslandTidepoolCrab.h"
#include "IslandPoolRippleEffect.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWaveProcedural.h"
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
	RainGroundImpactStreaks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RainGroundImpactStreaks"));
	RainGroundImpactStreaks->SetupAttachment(RootComponent);
	RainGroundImpactStreaks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainGroundImpactStreaks->SetCastShadow(false);
	RainGroundImpactStreaks->bReceivesDecals = false;
	RainGroundImpactStreaks->SetVisibility(false);
	WindAmbienceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WindAmbience"));
	WindAmbienceAudio->SetupAttachment(RootComponent);
	WindAmbienceAudio->bAutoActivate = false;
	WindAmbienceAudio->bAllowSpatialization = false;
	RainAmbienceAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RainAmbience"));
	RainAmbienceAudio->SetupAttachment(RootComponent);
	RainAmbienceAudio->bAutoActivate = false;
	RainAmbienceAudio->bAllowSpatialization = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RainMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (RainMesh.Succeeded())
	{
		RainStreaks->SetStaticMesh(RainMesh.Object);
		RainGroundImpactStreaks->SetStaticMesh(RainMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RainMaterial(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (RainMaterial.Succeeded()) RainStreaks->SetMaterial(0, RainMaterial.Object);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

void AIslandWeather::BeginPlay()
{
	Super::BeginPlay();
	InitializeWeatherAmbience();
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
	if (RainGroundImpactStreaks) RainGroundImpactStreaks->SetVisibility(false, true);
	if (WindAmbienceAudio) WindAmbienceAudio->Stop();
	if (RainAmbienceAudio) RainAmbienceAudio->Stop();
	WindAmbienceWave = nullptr;
	RainAmbienceWave = nullptr;
	RainStreakMaterial = nullptr;
	ActiveRainStreakCount = 0;
	ActiveRainGroundImpactCount = 0;
	for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
		if (Firefly.IsValid()) Firefly->Destroy();
	NightFireflies.Reset();
	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabs)
		if (Crab.IsValid()) Crab->Destroy();
	DayCrabs.Reset();
	Super::EndPlay(EndPlayReason);
}

void AIslandWeather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCloudRendering();
	UpdateRainRendering();
	UpdateWindPoolResponse();
	UpdateWeatherAmbience(DeltaSeconds);
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
		ClearRainGroundResponse();
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
	RainGroundImpactStreaks->SetWorldLocation(VisualizationCenter);
	if (CurrentRainIntensity >= 0.55f) UpdateRainPoolResponse();
	UpdateRainGroundResponse(VisualizationCenter, WindObserver, Now);
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

void AIslandWeather::UpdateRainGroundResponse(const FVector& Center, const AActor* Observer, double Now)
{
	if (!RainGroundImpactStreaks || !RainStreakMaterial || CurrentRainIntensity < 0.35f)
	{
		ClearRainGroundResponse();
		return;
	}

	if (!bRainGroundImpactPoolInitialized)
	{
		RainGroundImpactStreaks->SetMaterial(0, RainStreakMaterial);
		RainGroundImpactStreaks->ClearInstances();
		for (int32 Index = 0; Index < 3; ++Index)
			RainGroundImpactStreaks->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), false);
		bRainGroundImpactPoolInitialized = true;
	}

	if (Now >= NextRainGroundImpactTime)
	{
		const double Seed = WeatherSeed * 0.071 + (++RainGroundImpactSequence) * 0.6180339887498949;
		const double Angle = FMath::Frac(Seed * 1.37) * 2.0 * PI;
		const float Radius = FMath::Sqrt(static_cast<float>(FMath::Frac(Seed * 2.11))) * FMath::Min(900.f, FMath::Max(250.f, RainVisualizationRadius * 0.35f));
		const FVector Candidate = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const float TraceHeight = FMath::Clamp(RainVisualizationHeight, 1000.f, 5000.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandRainGroundImpact), false, this);
		if (Observer) Params.AddIgnoredActor(Observer);
		FHitResult Hit;
		const FVector TraceStart = Candidate + FVector(0.f, 0.f, TraceHeight * 0.5f);
		const FVector TraceEnd = Candidate - FVector(0.f, 0.f, TraceHeight * 1.5f);
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) &&
			Hit.GetActor() && !Hit.GetActor()->ActorHasTag(TEXT("TideglassPool")))
		{
			LastRainGroundImpactLocation = Hit.ImpactPoint;
			RainGroundImpactStartedAt = Now;
			ActiveRainGroundImpactCount = 3;
		}
		const float RainAlpha = FMath::Clamp((CurrentRainIntensity - 0.35f) / 0.65f, 0.f, 1.f);
		NextRainGroundImpactTime = Now + FMath::Lerp(3.0f, 0.75f, RainAlpha);
	}

	const float Age = static_cast<float>(Now - RainGroundImpactStartedAt);
	const float Alpha = FMath::Clamp(Age / 0.8f, 0.f, 1.f);
	const float Envelope = FMath::Sin(PI * Alpha);
	if (ActiveRainGroundImpactCount == 0 || Alpha >= 1.f)
	{
		ActiveRainGroundImpactCount = 0;
		RainGroundImpactStreaks->SetVisibility(false, true);
		return;
	}

	RainGroundImpactStreaks->SetVisibility(true, true);
	const FVector LocalImpact = RainGroundImpactStreaks->GetComponentTransform().InverseTransformPosition(LastRainGroundImpactLocation);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Angle = Index * (2.f * PI / 3.f) + RainGroundImpactSequence * 0.41f;
		const FVector Direction = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.85f).GetSafeNormal();
		const FVector Position = LocalImpact + FVector(Direction.X, Direction.Y, 0.f) * (4.f + 22.f * Alpha) + FVector(0.f, 0.f, 18.f * Alpha);
		const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);
		const FVector Scale(0.004f * Envelope, 0.004f * Envelope, 0.16f * Envelope);
		RainGroundImpactStreaks->UpdateInstanceTransform(Index, FTransform(Rotation, Position, Scale), false, Index == 2, true);
	}
}

void AIslandWeather::ClearRainGroundResponse()
{
	ActiveRainGroundImpactCount = 0;
	NextRainGroundImpactTime = 0.0;
	if (RainGroundImpactStreaks) RainGroundImpactStreaks->SetVisibility(false, true);
}

FVector2D AIslandWeather::CalculateAmbienceGains(float HorizontalWindSpeed, float RainIntensity)
{
	const float WindStrength = FMath::SmoothStep(18.f, 150.f, FMath::Clamp(HorizontalWindSpeed, 0.f, 300.f));
	const float RainStrength = FMath::SmoothStep(0.06f, 0.72f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	// Deliberately low ceilings: these are a quiet environmental bed, not foreground effects.
	return FVector2D(0.055f * WindStrength, 0.035f * RainStrength);
}

void AIslandWeather::InitializeWeatherAmbience()
{
	if (!WindAmbienceAudio || !RainAmbienceAudio || (WindAmbienceWave && RainAmbienceWave)) return;
	constexpr int32 SampleRate = 24000;
	auto MakeWave = [this](const TCHAR* Name)
	{
		USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this, Name);
		if (!Wave) return static_cast<USoundWaveProcedural*>(nullptr);
		Wave->SetSampleRate(24000);
		Wave->NumChannels = 2;
		Wave->Duration = 10000.f;
		Wave->bLooping = false;
		Wave->SoundGroup = SOUNDGROUP_Effects;
		return Wave;
	};
	WindNoiseStream.Initialize(0x49A31);
	RainNoiseStream.Initialize(0x67C21);
	WindAmbienceWave = MakeWave(TEXT("GeneratedWindAmbience"));
	RainAmbienceWave = MakeWave(TEXT("GeneratedRainAmbience"));
	WindAmbienceAudio->SetSound(WindAmbienceWave);
	RainAmbienceAudio->SetSound(RainAmbienceWave);
	WindAmbienceAudio->VolumeMultiplier = 0.f;
	RainAmbienceAudio->VolumeMultiplier = 0.f;
	QueueAmbienceSamples(WindAmbienceWave, WindNoiseStream, WindNoiseFilterLeft, WindNoiseFilterRight, false);
	QueueAmbienceSamples(RainAmbienceWave, RainNoiseStream, RainNoiseFilterLeft, RainNoiseFilterRight, true);
	QueueAmbienceSamples(WindAmbienceWave, WindNoiseStream, WindNoiseFilterLeft, WindNoiseFilterRight, false);
	QueueAmbienceSamples(RainAmbienceWave, RainNoiseStream, RainNoiseFilterLeft, RainNoiseFilterRight, true);
	static_assert(SampleRate == 24000, "Keep weather PCM and procedural-wave sample rates aligned.");
}

void AIslandWeather::QueueAmbienceSamples(USoundWaveProcedural* Wave, FRandomStream& Random,
	float& FilterLeft, float& FilterRight, bool bHighPass)
{
	if (!Wave) return;
	constexpr int32 SampleRate = 24000;
	constexpr int32 Frames = SampleRate / 2;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(Frames * 2);
	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		const float WhiteLeft = Random.FRandRange(-1.f, 1.f);
		const float WhiteRight = Random.FRandRange(-1.f, 1.f);
		// Low-pass noise gives wind a soft body; the complementary high-pass is a distant rain hiss.
		FilterLeft += (WhiteLeft - FilterLeft) * (bHighPass ? 0.10f : 0.018f);
		FilterRight += (WhiteRight - FilterRight) * (bHighPass ? 0.10f : 0.018f);
		const float SignalLeft = bHighPass ? WhiteLeft - FilterLeft : FilterLeft;
		const float SignalRight = bHighPass ? WhiteRight - FilterRight : FilterRight;
		const float Amplitude = bHighPass ? 0.10f : 0.20f;
		Samples[Frame * 2] = static_cast<int16>(FMath::Clamp(SignalLeft * Amplitude, -1.f, 1.f) * 32767.f);
		Samples[Frame * 2 + 1] = static_cast<int16>(FMath::Clamp(SignalRight * Amplitude, -1.f, 1.f) * 32767.f);
	}
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}

void AIslandWeather::UpdateWeatherAmbience(float DeltaSeconds)
{
	if (!GetWorld() || !WindAmbienceWave || !RainAmbienceWave || !WindAmbienceAudio || !RainAmbienceAudio) return;
	AmbienceUpdateAccumulator += FMath::Max(0.f, DeltaSeconds);
	if (AmbienceUpdateAccumulator < 0.25f) return;
	AmbienceUpdateAccumulator = 0.f;
	APawn* Listener = nullptr;
	if (APlayerController* Player = GetWorld()->GetFirstPlayerController()) Listener = Player->GetPawn();
	if (!Listener)
	{
		WindAmbienceAudio->SetVolumeMultiplier(0.f);
		RainAmbienceAudio->SetVolumeMultiplier(0.f);
		WindAmbienceAudio->Stop();
		RainAmbienceAudio->Stop();
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector2D Gains = CalculateAmbienceGains(GetLocalWind(Listener->GetActorLocation(), Listener).Size(), SampleRainIntensity(Now));
	WindAmbienceAudio->SetVolumeMultiplier(Gains.X);
	RainAmbienceAudio->SetVolumeMultiplier(Gains.Y);
	if (Gains.X > 0.0001f)
	{
		if (!WindAmbienceAudio->IsPlaying()) WindAmbienceAudio->Play();
	}
	else WindAmbienceAudio->Stop();
	if (Gains.Y > 0.0001f)
	{
		if (!RainAmbienceAudio->IsPlaying()) RainAmbienceAudio->Play();
	}
	else RainAmbienceAudio->Stop();
	constexpr int32 BytesPerSecond = 24000 * 2 * sizeof(int16);
	if (WindAmbienceWave->GetAvailableAudioByteCount() < BytesPerSecond)
		QueueAmbienceSamples(WindAmbienceWave, WindNoiseStream, WindNoiseFilterLeft, WindNoiseFilterRight, false);
	if (RainAmbienceWave->GetAvailableAudioByteCount() < BytesPerSecond)
		QueueAmbienceSamples(RainAmbienceWave, RainNoiseStream, RainNoiseFilterLeft, RainNoiseFilterRight, true);
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

void AIslandWeather::UpdateWindPoolResponse()
{
	if (!GetWorld() || CurrentRainIntensity >= 0.55f || RainPoolRipple.IsValid() || WindPoolRipple.IsValid()) return;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextWindPoolRippleTime) return;

	AActor* Pool = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("TideglassPool"))) continue;
		Pool = *It;
		break;
	}
	if (!Pool) return;

	const FVector LocalWind = GetLocalWind(Pool->GetActorLocation(), Pool);
	const float Speed = LocalWind.Size2D();
	const float Activity = AIslandPoolRippleEffect::WindRippleActivity(Speed);
	if (Activity <= 0.f) return;

	const FVector Flow = LocalWind.GetSafeNormal2D();
	const FVector Side(-Flow.Y, Flow.X, 0.f);
	const double Phase = Now * 0.41 + WeatherSeed * 0.19;
	const FVector Offset = Flow * 28.f + Side * static_cast<float>(FMath::Sin(Phase) * 28.0);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(Pool->GetActorLocation() + Offset, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsWindImpact(Speed);
		WindPoolRipple = Ripple;
		NextWindPoolRippleTime = Now + FMath::Lerp(12.0, 5.0, static_cast<double>(Activity));
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
	DayCrabs.RemoveAll([](const TWeakObjectPtr<AIslandTidepoolCrab>& Crab) { return !Crab.IsValid(); });
	if (!GetWorld()) return;

	float CurrentHour = -1.f;
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		CurrentHour = It->CurrentHour;
		break;
	}
	const bool bNight = CurrentHour >= 19.f || (CurrentHour >= 0.f && CurrentHour < 5.f);
	const bool bDay = CurrentHour >= 6.f && CurrentHour < 19.f;

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
	if (CurrentHour < 0.f || !Habitat)
	{
		for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
			if (Firefly.IsValid()) Firefly->Destroy();
		NightFireflies.Reset();
		for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabs)
			if (Crab.IsValid()) Crab->Destroy();
		DayCrabs.Reset();
		return;
	}

	if (!bNight)
	{
		for (const TWeakObjectPtr<AIslandFirefly>& Firefly : NightFireflies)
			if (Firefly.IsValid()) Firefly->Destroy();
		NightFireflies.Reset();
	}
	else
	{
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

	for (const TWeakObjectPtr<AIslandTidepoolCrab>& Crab : DayCrabs)
		if (Crab.IsValid()) Crab->SetSheltered(!bDay);

	if (!bDay)
		return;
	constexpr int32 DayPopulation = 2;
	while (DayCrabs.Num() < DayPopulation)
	{
		const int32 Index = DayCrabs.Num();
		const float Angle = Index * PI;
		const FVector ShoreOffset(FMath::Cos(Angle) * 720.f, FMath::Sin(Angle) * 720.f, 900.f);
		const FVector TraceStart = Habitat->GetActorLocation() + ShoreOffset;
		FHitResult GroundHit;
		FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(IslandTidepoolCrabShore), false, this);
		GroundParams.AddIgnoredActor(Habitat);
		if (!GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceStart - FVector(0.f, 0.f, 2200.f), ECC_WorldStatic, GroundParams)) break;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Location = GroundHit.Location + FVector(0.f, 0.f, 12.f);
		if (AIslandTidepoolCrab* Crab = GetWorld()->SpawnActor<AIslandTidepoolCrab>(Location, FRotator::ZeroRotator, SpawnParameters))
			DayCrabs.Add(Crab);
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
	if (HasUpwindObstruction(Position, Wind, Observer)) Wind *= 0.15f;
	return Wind;
}

bool AIslandWeather::HasUpwindObstruction(const FVector& Position, const FVector& Wind, const AActor* Observer) const
{
	if (!GetWorld() || Wind.IsNearlyZero()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandWindShelter), false);
	if (Observer) Params.AddIgnoredActor(Observer);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Position, Position - Wind.GetSafeNormal() * 600.f, ECC_Visibility, Params);
}

FString AIslandWeather::DescribeWindShelterAt(const FVector& Position, const AActor* Observer) const
{
	if (!GetWorld()) return TEXT("Wind shelter cannot be assessed because no world weather is active.");
	const FVector AmbientWind = SampleWind(Position, GetWorld()->GetTimeSeconds());
	if (AmbientWind.Size() < 35.f)
		return TEXT("The ambient wind is currently too light to judge this site's wind shelter. This check does not assess overhead rain cover or perch support.");
	if (HasUpwindObstruction(Position, AmbientWind, Observer))
		return TEXT("Solid geometry currently blocks the upwind visibility trace within six metres, so this point is sheltered from the present horizontal wind. This does not establish overhead rain cover or safe perch support.");
	return TEXT("No solid geometry blocks the current six-metre upwind trace, so this point is exposed to the present horizontal wind. This does not assess overhead rain cover or perch support.");
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
	return FString::Printf(TEXT(" Local weather simulation: %s; wind towards world XY (%.2f, %.2f), %.1f metres/second, vertical current %.1f metres/second.%s Cloud coverage, density and storm character follow slow deterministic weather cycles and gently soften sunlight/skylight. Current rain intensity is %.0f%%; a finite rain-streak field is visible during showers. Strong rain also creates sparse Tideglass ripples and small collision-sampled ground splashes, while nearby fireflies fly lower and dim their natural pulse. Weather sounds are not yet implemented."),
		Conditions,
		Wind.GetSafeNormal().X, Wind.GetSafeNormal().Y, Wind.Size() / 100.f, Wind.Z / 100.f,
		bFeelingLocalGust ? TEXT(" A fading local gust is still changing the wind nearby.") : TEXT(""), Rain * 100.f);
}
