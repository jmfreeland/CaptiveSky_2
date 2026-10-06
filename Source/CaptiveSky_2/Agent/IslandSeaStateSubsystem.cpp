#include "IslandSeaStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GerstnerWaterWaveSubsystem.h"
#include "GerstnerWaterWaves.h"
#include "HAL/IConsoleManager.h"
#include "IslandWeather.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterWaves.h"

static TAutoConsoleVariable<float> CVarIslandSea(
	TEXT("Island.Sea"),
	-1.f,
	TEXT("Developer override for the Water-plugin ocean's sea state, 0 glassy to 1 full storm. Negative follows the weather."),
	ECVF_Default);

float UIslandSeaStateSubsystem::TargetSea(float Storm, float Wind01)
{
	return FMath::Clamp(0.6f * FMath::Clamp(Wind01, 0.f, 1.f) + 0.5f * FMath::Clamp(Storm, 0.f, 1.f), 0.f, 1.f);
}

float UIslandSeaStateSubsystem::StepSea(float Current, float Target, float DeltaTime)
{
	const float Seconds = Target > Current ? RiseSeconds : FallSeconds;
	return FMath::Lerp(Current, Target, 1.f - FMath::Exp(-FMath::Max(DeltaTime, 0.f) / Seconds));
}

float UIslandSeaStateSubsystem::AmplitudeScale(float Sea)
{
	return FMath::Lerp(CalmAmplitudeScale, 1.f, FMath::Clamp(Sea, 0.f, 1.f));
}

float UIslandSeaStateSubsystem::SteepnessScale(float Sea)
{
	return FMath::Lerp(CalmSteepnessScale, StormSteepnessScale, FMath::Clamp(Sea, 0.f, 1.f));
}

bool UIslandSeaStateSubsystem::BindWaves(UGerstnerWaterWaves* InWaves)
{
	RestoreWaves();
	UGerstnerWaterWaveGeneratorSimple* Simple = InWaves ? Cast<UGerstnerWaterWaveGeneratorSimple>(InWaves->GerstnerWaveGenerator) : nullptr;
	if (!Simple) return false;
	Waves = InWaves;
	Generator = Simple;
	AuthoredMinAmplitude = Simple->MinAmplitude;
	AuthoredMaxAmplitude = Simple->MaxAmplitude;
	AuthoredSmallSteepness = Simple->SmallWaveSteepness;
	AuthoredLargeSteepness = Simple->LargeWaveSteepness;
	AppliedSea = -1.f;
	return true;
}

void UIslandSeaStateSubsystem::ApplySea(float InSea)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(IslandSeaState_ApplySea);
	UGerstnerWaterWaves* Target = Waves.Get();
	UGerstnerWaterWaveGeneratorSimple* Simple = Generator.Get();
	if (!Target || !Simple) return;
	const float Amplitude = AmplitudeScale(InSea);
	const float Steepness = SteepnessScale(InSea);
	Simple->MinAmplitude = FMath::Max(AuthoredMinAmplitude * Amplitude, 0.0001f);
	Simple->MaxAmplitude = FMath::Max(AuthoredMaxAmplitude * Amplitude, 0.0001f);
	Simple->SmallWaveSteepness = FMath::Clamp(AuthoredSmallSteepness * Steepness, 0.f, 1.f);
	Simple->LargeWaveSteepness = FMath::Clamp(AuthoredLargeSteepness * Steepness, 0.f, 1.f);
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(IslandSeaState_RecomputeWaves);
		Target->RecomputeWaves(false);
	}
	if (GEngine)
		if (UGerstnerWaterWaveSubsystem* GPU = GEngine->GetEngineSubsystem<UGerstnerWaterWaveSubsystem>())
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(IslandSeaState_RebuildGPUData);
			GPU->RebuildGPUData();
		}
	AppliedSea = InSea;
}

void UIslandSeaStateSubsystem::RestoreWaves()
{
	UGerstnerWaterWaves* Target = Waves.Get();
	UGerstnerWaterWaveGeneratorSimple* Simple = Generator.Get();
	if (Target && Simple)
	{
		Simple->MinAmplitude = AuthoredMinAmplitude;
		Simple->MaxAmplitude = AuthoredMaxAmplitude;
		Simple->SmallWaveSteepness = AuthoredSmallSteepness;
		Simple->LargeWaveSteepness = AuthoredLargeSteepness;
		Target->RecomputeWaves(false);
		if (GEngine)
			if (UGerstnerWaterWaveSubsystem* GPU = GEngine->GetEngineSubsystem<UGerstnerWaterWaveSubsystem>()) GPU->RebuildGPUData();
	}
	Waves.Reset();
	Generator.Reset();
	AppliedSea = -1.f;
	bSeaInitialized = false;
}

float UIslandSeaStateSubsystem::SampleWeatherSea() const
{
	UWorld* World = GetWorld();
	if (!World) return 0.f;
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		const double Now = World->GetTimeSeconds();
		const float Storm = It->SampleStormIntensity(Now);
		const float WindMax = FMath::Max(1.f, It->MaximumWindSpeed * (1.f + AIslandWeather::StormWindBoost));
		const float Wind01 = It->SampleWind(It->GetActorLocation(), Now).Size2D() / WindMax;
		return TargetSea(Storm, Wind01);
	}
	return 0.f;
}

bool UIslandSeaStateSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandSeaStateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (TActorIterator<AWaterBodyOcean> It(&InWorld); It; ++It)
	{
		UWaterBodyComponent* Body = It->GetWaterBodyComponent();
		UWaterWavesBase* Base = Body ? Body->GetWaterWaves() : nullptr;
		UGerstnerWaterWaves* Gerstner = Base ? Cast<UGerstnerWaterWaves>(Base->GetWaterWaves()) : nullptr;
		if (BindWaves(Gerstner))
		{
			UE_LOG(LogTemp, Log, TEXT("IslandSeaState: following the weather on %s (authored amplitude %.1f-%.1f cm, steepness %.2f/%.2f)"),
				*It->GetActorNameOrLabel(), AuthoredMinAmplitude, AuthoredMaxAmplitude, AuthoredSmallSteepness, AuthoredLargeSteepness);
			break;
		}
	}
}

void UIslandSeaStateSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SinceUpdate += DeltaTime;
	if (SinceUpdate < UpdateIntervalSeconds) return;
	const float Elapsed = SinceUpdate;
	SinceUpdate = 0.f;

	const float Console = CVarIslandSea.GetValueOnGameThread();
	const float Forced = Console >= 0.f ? Console : ForcedSea;
	if (Forced >= 0.f)
	{
		Sea = FMath::Clamp(Forced, 0.f, 1.f);
		bSeaInitialized = true;
	}
	else if (!bSeaInitialized)
	{
		Sea = SampleWeatherSea();
		bSeaInitialized = true;
	}
	else
	{
		Sea = StepSea(Sea, SampleWeatherSea(), Elapsed);
	}
	if (FMath::Abs(Sea - AppliedSea) >= 0.01f) ApplySea(Sea);
}

TStatId UIslandSeaStateSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandSeaStateSubsystem, STATGROUP_Tickables);
}

bool UIslandSeaStateSubsystem::IsTickable() const
{
	return !IsTemplate() && Waves.IsValid();
}

void UIslandSeaStateSubsystem::Deinitialize()
{
	RestoreWaves();
	Super::Deinitialize();
}
