#include "IslandTidepoolMinnows.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "IslandWeather.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIslandTidepoolMinnows::AIslandTidepoolMinnows()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.06f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("MinnowSchool"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseShapeMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;

	Fish.Reserve(FishCount);
	for (int32 Index = 0; Index < FishCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("Minnow_%d"), Index));
		UStaticMeshComponent* Minnow = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		Minnow->SetupAttachment(RootComponent);
		Minnow->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		Minnow->SetRelativeScale3D(FVector(0.12f, 0.035f, 0.045f));
		Minnow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Minnow->SetCastShadow(false);
		Fish.Add(Minnow);
	}
}

void AIslandTidepoolMinnows::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	if (BaseShapeMaterial)
	{
		UMaterialInstanceDynamic* Silver = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		if (Silver)
		{
			Silver->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.42f, 0.68f, 0.76f));
			for (UStaticMeshComponent* Minnow : Fish)
				if (Minnow) Minnow->SetMaterial(0, Silver);
		}
	}
	UpdateSchool(0.f);
}

void AIslandTidepoolMinnows::RespondToQuietObservation(const FVector& ObserverLocation)
{
	ScatterDirection = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (ScatterDirection.IsNearlyZero()) ScatterDirection = GetActorForwardVector();
	ScatterRemaining = 2.4f;
}

bool AIslandTidepoolMinnows::RespondToSurfaceRipple()
{
	if (SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f) return false;
	SurfacePulseRemaining = 1.2f;
	SurfacePulseCooldownRemaining = 3.f;
	return true;
}

float AIslandTidepoolMinnows::RainMovementScale(float RainIntensity)
{
	const float RainActivity = FMath::SmoothStep(0.35f, 0.85f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	return FMath::Lerp(1.f, 0.68f, RainActivity);
}

float AIslandTidepoolMinnows::GetScatterAlpha() const
{
	if (ScatterRemaining <= 0.f) return 0.f;
	if (ScatterRemaining > 1.7f)
		return FMath::SmoothStep(0.f, 0.7f, 2.4f - ScatterRemaining);
	return FMath::SmoothStep(0.f, 1.7f, ScatterRemaining);
}

float AIslandTidepoolMinnows::GetSurfacePulseAlpha() const
{
	if (SurfacePulseRemaining <= 0.f) return 0.f;
	const float Elapsed = 1.2f - SurfacePulseRemaining;
	return FMath::SmoothStep(0.f, 0.18f, Elapsed) * (1.f - FMath::SmoothStep(0.45f, 1.2f, Elapsed));
}

void AIslandTidepoolMinnows::CheckForLowRavenFlyby()
{
	if (!GetWorld() || ScatterRemaining > 0.f || RavenFlybyCooldownRemaining > 0.f) return;

	constexpr float FlybyRadius = 550.f;
	constexpr float MinimumHeight = 150.f;
	constexpr float MaximumHeight = 700.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		if (It->LocomotionState != ERavenLocomotionState::Flying) continue;
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		if (Offset.Z < MinimumHeight || Offset.Z > MaximumHeight || Offset.SizeSquared2D() > FMath::Square(FlybyRadius))
			continue;

		// A bird gliding low over the shallows briefly breaks the school's pattern; it is
		// a visible world response, not a hunt, capture, model call, or lasting change.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTidepoolMinnows::UpdateSchool(float RainIntensity)
{
	const float TuckScale = RainMovementScale(RainIntensity);
	const float ScatterAlpha = GetScatterAlpha();
	const float CircleScale = 1.f + 0.65f * GetSurfacePulseAlpha();
	const FVector Side(-ScatterDirection.Y, ScatterDirection.X, 0.f);
	for (int32 Index = 0; Index < Fish.Num(); ++Index)
	{
		UStaticMeshComponent* Minnow = Fish[Index];
		if (!Minnow) continue;
		const float Angle = Phase + ElapsedSeconds * 0.62f + Index * 2.f * PI / FishCount;
		const FVector IdleOffset(
			FMath::Cos(Angle) * 135.f * TuckScale * CircleScale,
			FMath::Sin(Angle) * 82.f * TuckScale * CircleScale,
			17.f + FMath::Sin(Angle * 1.7f) * 7.f);
		const float FanOffset = (Index - (FishCount - 1) * 0.5f) * 22.f;
		const FVector ScatterOffset = ScatterDirection * 210.f + Side * FanOffset;
		Minnow->SetRelativeLocation(IdleOffset + ScatterOffset * ScatterAlpha);

		FVector Facing(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		Facing += ScatterDirection * (ScatterAlpha * 1.2f);
		if (!Facing.IsNearlyZero())
			Minnow->SetRelativeRotation(FRotator(0.f, Facing.Rotation().Yaw, 0.f));
	}
}

void AIslandTidepoolMinnows::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ElapsedSeconds += SafeDelta;
	ScatterRemaining = FMath::Max(0.f, ScatterRemaining - SafeDelta);
	SurfacePulseRemaining = FMath::Max(0.f, SurfacePulseRemaining - SafeDelta);
	SurfacePulseCooldownRemaining = FMath::Max(0.f, SurfacePulseCooldownRemaining - SafeDelta);
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - SafeDelta);
	RavenCheckRemaining -= SafeDelta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.35f;
		CheckForLowRavenFlyby();
	}
	const float Rain = Weather.IsValid() && GetWorld()
		? Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds())
		: 0.f;
	UpdateSchool(Rain);
}
