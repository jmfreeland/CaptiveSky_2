#include "IslandDayNight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"

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
	if (bAdvanceTime) CurrentHour = WrapHour(CurrentHour + FMath::Max(0.f, DeltaSeconds) * 24.0 / (FMath::Max(1.f, DayLengthMinutes) * 60.0));
	UpdateLighting();
}

void AIslandDayNight::UpdateLighting()
{
	const float Height = SunHeight(CurrentHour);
	const float Daylight = FMath::SmoothStep(-0.04f, 0.18f, Height);
	const float Angle = (CurrentHour - 6.f) * 15.f;
	if (Sun)
	{
		Sun->SetActorRotation(FRotator(-Angle, 35.f, 0.f));
		ULightComponent* Light = Sun->GetLightComponent();
		Light->SetIntensity(FMath::Max(0.f, DaySunIntensity) * Daylight);
		Light->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.32f, 0.12f), FLinearColor(1.f, 0.96f, 0.88f), FMath::SmoothStep(0.f, 0.4f, Height)));
	}
	Moon->SetWorldRotation(FRotator(-Angle + 180.f, 35.f, 0.f));
	Moon->SetIntensity(FMath::Max(0.f, MoonIntensity) * FMath::SmoothStep(0.02f, 0.25f, -Height));
	if (Sky) Sky->GetLightComponent()->SetIntensity(FMath::Lerp(0.12f, 1.f, Daylight));
}

FString AIslandDayNight::DescribeTime() const
{
	const TCHAR* Phase = CurrentHour < 5.f || CurrentHour >= 20.f ? TEXT("night") :
		CurrentHour < 7.f ? TEXT("dawn") : CurrentHour < 12.f ? TEXT("morning") :
		CurrentHour < 17.f ? TEXT("afternoon") : TEXT("dusk");
	return FString::Printf(TEXT(" It is %s on the Island (approximately %02d:%02d). The sun and moon move as time passes."),
		Phase, FMath::FloorToInt(CurrentHour), FMath::FloorToInt(FMath::Frac(CurrentHour) * 60.f));
}
