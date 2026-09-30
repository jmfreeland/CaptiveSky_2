#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWorldStateSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

AIslandDayNight::AIslandDayNight()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(RootComponent);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->bAtmosphereSunLight = true;
	Moon->AtmosphereSunLightIndex = 1;
	Moon->SetLightColor(FLinearColor(0.55f, 0.68f, 1.f));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

float AIslandDayNight::WrapHour(double Hour)
{
	return static_cast<float>(FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0));
}

float AIslandDayNight::SunHeight(float Hour)
{
	return FMath::Sin((WrapHour(Hour) - 6.f) * PI / 12.f);
}

float AIslandDayNight::LunarPhaseProgress(int32 IslandDay, float IslandHour)
{
	const double ElapsedDays = static_cast<double>(FMath::Max(1, IslandDay) - 1) +
		WrapHour(IslandHour) / 24.0;
	return static_cast<float>(FMath::Fmod(ElapsedDays, LunarCycleDays) / LunarCycleDays);
}

float AIslandDayNight::LunarIllumination(int32 IslandDay, float IslandHour)
{
	const float PhaseAngle = LunarPhaseProgress(IslandDay, IslandHour) * 2.f * PI;
	return 0.5f - 0.5f * FMath::Cos(PhaseAngle);
}

float AIslandDayNight::CloudSunlightTransmission(float CloudCover)
{
	return FMath::Lerp(1.f, 0.76f, FMath::Clamp(CloudCover, 0.f, 1.f));
}

float AIslandDayNight::CloudSkylightTransmission(float CloudCover)
{
	return FMath::Lerp(1.f, 0.70f, FMath::Clamp(CloudCover, 0.f, 1.f));
}

void AIslandDayNight::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CurrentHour = WrapHour(StartHour);
	UpdateLighting();
}

void AIslandDayNight::BeginPlay()
{
	Super::BeginPlay();
	CurrentHour = WrapHour(StartHour);
	DayNumber = 1;
	SecondsSinceSave = 0.f;
	if (const UIslandWorldStateSubsystem* WorldState = bResumeSavedTime ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr)
	{
		if (const TOptional<float> Saved = WorldState->GetSavedHour(); Saved.IsSet()) CurrentHour = WrapHour(Saved.GetValue());
		DayNumber = WorldState->GetSavedDay().Get(1);
	}
	if (Sun) Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	if (Sky) Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	UpdateLighting();
}

#if WITH_EDITOR
void AIslandDayNight::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	if (!GetWorld() || !GetWorld()->IsGameWorld()) CurrentHour = WrapHour(StartHour);
	UpdateLighting();
}
#endif

void AIslandDayNight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float PreviousHour = CurrentHour;
	if (bAdvanceTime) CurrentHour = WrapHour(CurrentHour + FMath::Max(0.f, DeltaSeconds) * 24.0 / (FMath::Max(1.f, DayLengthMinutes) * 60.0));
	if (CurrentHour < PreviousHour) ++DayNumber;
	UpdateLighting();
	// Save occasionally too, so a crash or forced stop loses at most a minute of Island time.
	SecondsSinceSave += FMath::Max(0.f, DeltaSeconds);
	if (SecondsSinceSave >= 60.f) PersistHour();
}

void AIslandDayNight::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PersistHour();
	Super::EndPlay(EndPlayReason);
}

void AIslandDayNight::PersistHour()
{
	SecondsSinceSave = 0.f;
	if (!GetWorld()) return;
	UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!WorldState) return;
	if (bResumeSavedTime) WorldState->SaveClock(CurrentHour, DayNumber);
	if (const UIslandEnvironmentSubsystem* Environment = GetWorld()->GetSubsystem<UIslandEnvironmentSubsystem>())
		WorldState->SaveWetness(Environment->GetWetness());
}

void AIslandDayNight::UpdateLighting()
{
	const float Height = SunHeight(CurrentHour);
	const float Daylight = FMath::SmoothStep(-0.04f, 0.18f, Height);
	const float Angle = (CurrentHour - 6.f) * 15.f;
	float CloudCover = 0.f;
	if (GetWorld())
	{
		for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
		{
			CloudCover = It->SampleCloudCover(GetWorld()->GetTimeSeconds());
			break;
		}
	}
	if (Sun)
	{
		Sun->SetActorRotation(FRotator(-Angle, 35.f, 0.f));
		ULightComponent* Light = Sun->GetLightComponent();
		Light->SetIntensity(FMath::Max(0.f, DaySunIntensity) * Daylight * CloudSunlightTransmission(CloudCover));
		Light->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.32f, 0.12f), FLinearColor(1.f, 0.96f, 0.88f), FMath::SmoothStep(0.f, 0.4f, Height)));
	}
	const float LunarProgress = LunarPhaseProgress(DayNumber, CurrentHour);
	const float LunarIlluminationAmount = LunarIllumination(DayNumber, CurrentHour);
	// New moon follows the sun below the horizon; full moon travels opposite it.
	// Intermediate phases therefore shift the moon's rise and set through the night.
	Moon->SetWorldRotation(FRotator(-Angle + LunarProgress * 360.f, 35.f, 0.f));
	Moon->SetIntensity(FMath::Max(0.f, MoonIntensity) * LunarIlluminationAmount *
		FMath::SmoothStep(0.02f, 0.25f, -Height));
	if (Sky) Sky->GetLightComponent()->SetIntensity(FMath::Lerp(NightSkylightFloor, 1.f, Daylight) * CloudSkylightTransmission(CloudCover));
}

FString AIslandDayNight::DescribeTime() const
{
	const TCHAR* Phase = CurrentHour < 5.f || CurrentHour >= 20.f ? TEXT("night") :
		CurrentHour < 7.f ? TEXT("dawn") : CurrentHour < 12.f ? TEXT("morning") :
		CurrentHour < 17.f ? TEXT("afternoon") : TEXT("dusk");
	const float LunarProgress = LunarPhaseProgress(DayNumber, CurrentHour);
	const TCHAR* LunarTrend = LunarProgress < 0.5f ? TEXT("waxing") : TEXT("waning");
	const float LunarLight = LunarIllumination(DayNumber, CurrentHour);
	const TCHAR* LunarBrightness = LunarLight < 0.12f ? TEXT("very faint") : LunarLight < 0.4f ? TEXT("faint") :
		LunarLight < 0.72f ? TEXT("moderate") : TEXT("bright");
	return FString::Printf(TEXT(" It is %s on the Island (approximately %02d:%02d). The lunar phase is %s with %s illumination; the moon's rise and set shift through the cycle. The sun follows its daily arc; cloud cover gently softens direct and ambient daylight."),
		Phase, FMath::FloorToInt(CurrentHour), FMath::FloorToInt(FMath::Frac(CurrentHour) * 60.f), LunarTrend, LunarBrightness);
}

static FAutoConsoleCommandWithWorldAndArgs GIslandHourCommand(
	TEXT("Island.Hour"),
	TEXT("Developer override for testing and filming: jump the Island clock to an hour (0..24). The day number is unchanged. Usage: Island.Hour <hour>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 1) return;
		for (TActorIterator<AIslandDayNight> It(World); It; ++It) It->CurrentHour = AIslandDayNight::WrapHour(FCString::Atof(*Args[0]));
	}));
