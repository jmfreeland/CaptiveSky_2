#include "IslandPoolRippleEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"

AIslandPoolRippleEffect::AIslandPoolRippleEffect()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.025f;
	Tags.AddUnique(TEXT("IslandTransientEffect"));
	Tags.AddUnique(TEXT("TideglassRipple"));

	constexpr int32 LightCount = 8;
	for (int32 Index = 0; Index < LightCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("RippleLight_%02d"), Index));
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(ComponentName);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetLightColor(FLinearColor(0.28f, 0.78f, 1.f));
		Light->SetAttenuationRadius(90.f);
		Light->SetCastShadows(false);
		Light->SetIntensity(0.f);
		RippleLights.Add(Light);
	}
}

void AIslandPoolRippleEffect::BeginPlay()
{
	Super::BeginPlay();
	UpdateRipple(0.f);
}

void AIslandPoolRippleEffect::ConfigureAsRainImpact()
{
	DurationSeconds = 1.15f;
	SurfaceRadius = 68.f;
	PeakLightIntensity = 18.f;
	Tags.AddUnique(TEXT("RainImpact"));
}

void AIslandPoolRippleEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedSeconds += FMath::Max(0.f, DeltaSeconds);
	const float Alpha = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.f, 1.f);
	UpdateRipple(Alpha);
	if (Alpha >= 1.f) Destroy();
}

void AIslandPoolRippleEffect::UpdateRipple(float Alpha)
{
	const float RingRadius = FMath::Lerp(12.f, SurfaceRadius, Alpha);
	const float Pulse = FMath::Max(0.f, FMath::Sin(Alpha * PI));
	for (int32 Index = 0; Index < RippleLights.Num(); ++Index)
	{
		const float Angle = (2.f * PI * Index) / RippleLights.Num();
		UPointLightComponent* Light = RippleLights[Index];
		Light->SetRelativeLocation(FVector(FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius, 24.f));
		Light->SetIntensity(PeakLightIntensity * Pulse);
	}
}
