#include "IslandGrade.h"
#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "IslandEnvironmentSubsystem.h"

static TAutoConsoleVariable<float> CVarIslandGrade(
	TEXT("Island.Grade"), 1.f,
	TEXT("Strength of the dynamic colour grade: 0 off, 1 default (subtle), up to 2."),
	ECVF_Default);

FIslandGrade ComputeIslandGrade(const FIslandGradeInputs& In, float Strength)
{
	const auto Unit = [](float Value) { return FMath::Clamp(Value, 0.f, 1.f); };
	const float Night = 1.f - FMath::SmoothStep(0.f, 0.35f, Unit(In.Daylight));
	const float Gold = Unit(In.GoldenHour) * (1.f - 0.7f * Unit(In.Storm));
	const float Gray = Unit(FMath::Max3(Unit(In.Storm), 0.7f * Unit(In.Rain), 0.4f * Unit(In.CloudCover)));
	const float Wet = Unit(In.Wetness) * (1.f - Gray);
	const float Mist = Unit(In.Mist);

	FIslandGrade Out;
	Out.Saturation = 1.f + 0.07f * Gold - 0.10f * Gray - 0.06f * Night + 0.03f * Wet;
	Out.Gain = FVector3f(1.f, 1.f, 1.f)
		+ Gold * FVector3f(0.045f, 0.010f, -0.050f)
		+ Night * FVector3f(-0.040f, -0.015f, 0.050f)
		+ Gray * FVector3f(-0.020f, 0.f, 0.015f)
		+ Mist * FVector3f(0.010f, 0.012f, 0.015f);
	Out.Contrast = 1.f + 0.03f * Gold + 0.025f * Wet - 0.03f * Mist - 0.02f * Gray;

	const float S = FMath::Clamp(Strength, 0.f, 2.f);
	Out.Saturation = 1.f + (Out.Saturation - 1.f) * S;
	Out.Gain = FVector3f(1.f, 1.f, 1.f) + (Out.Gain - FVector3f(1.f, 1.f, 1.f)) * S;
	Out.Contrast = 1.f + (Out.Contrast - 1.f) * S;
	return Out;
}

AIslandGradeActor::AIslandGradeActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Post = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Post"));
	Post->SetupAttachment(RootComponent);
	Post->bUnbound = true;
	Post->Priority = 0.f;
	PrimaryActorTick.bCanEverTick = false;
}

void AIslandGradeActor::Apply(const FIslandGrade& Grade, float BlendWeight)
{
	if (!Post) return;
	FPostProcessSettings& Settings = Post->Settings;
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(Grade.Saturation, Grade.Saturation, Grade.Saturation, 1.0);
	Settings.bOverride_ColorGain = true;
	Settings.ColorGain = FVector4(Grade.Gain.X, Grade.Gain.Y, Grade.Gain.Z, 1.0);
	Settings.bOverride_ColorContrast = true;
	Settings.ColorContrast = FVector4(Grade.Contrast, Grade.Contrast, Grade.Contrast, 1.0);
	Post->BlendWeight = FMath::Clamp(BlendWeight, 0.f, 1.f);
}

FIslandGrade UIslandGradeSubsystem::Settle(const FIslandGrade& Current, const FIslandGrade& Target, float Seconds)
{
	const float Alpha = 1.f - FMath::Exp(-FMath::Max(0.f, Seconds) / SettleSeconds);
	FIslandGrade Out;
	Out.Saturation = FMath::Lerp(Current.Saturation, Target.Saturation, Alpha);
	Out.Gain = FMath::Lerp(Current.Gain, Target.Gain, Alpha);
	Out.Contrast = FMath::Lerp(Current.Contrast, Target.Contrast, Alpha);
	return Out;
}

TStatId UIslandGradeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandGradeSubsystem, STATGROUP_Tickables);
}

bool UIslandGradeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandGradeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Actor = InWorld.SpawnActor<AIslandGradeActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	bSnap = true;
}

void UIslandGradeSubsystem::Deinitialize()
{
	if (AIslandGradeActor* Grade = Actor.Get()) Grade->Destroy();
	Actor.Reset();
	Super::Deinitialize();
}

void UIslandGradeSubsystem::Tick(float DeltaTime)
{
	SinceUpdate += DeltaTime;
	if (SinceUpdate < 0.25f && !bSnap) return;
	const float Elapsed = SinceUpdate;
	SinceUpdate = 0.f;

	AIslandGradeActor* Grade = Actor.Get();
	const UIslandEnvironmentSubsystem* Environment = GetWorld() ? GetWorld()->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
	if (!Grade || !Environment) return;

	FIslandGradeInputs Inputs;
	Inputs.Daylight = Environment->GetDaylight();
	Inputs.GoldenHour = Environment->GetGoldenHour();
	Inputs.Storm = Environment->GetStorm();
	Inputs.Rain = Environment->GetRainIntensity();
	Inputs.Wetness = Environment->GetWetness();
	Inputs.Mist = Environment->GetMist();
	Inputs.CloudCover = Environment->GetCloudCover();

	const float Strength = CVarIslandGrade.GetValueOnGameThread();
	const FIslandGrade Target = ComputeIslandGrade(Inputs, Strength);
	Current = bSnap ? Target : Settle(Current, Target, Elapsed);
	bSnap = false;
	Grade->Apply(Current, Strength > 0.f ? 1.f : 0.f);
}
