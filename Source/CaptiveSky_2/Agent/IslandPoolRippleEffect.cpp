#include "IslandPoolRippleEffect.h"
#include "IslandTideglassSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AIslandPoolRippleEffect::AIslandPoolRippleEffect()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.025f;
	// An unlit engine material keeps this small interaction cue legible on water
	// even when the scene's specular facets and direct sun are brighter than its light.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	RippleMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;
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

	StartleRing = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MinnowStartleRing"));
	StartleRing->SetupAttachment(RootComponent);
	// Lift the surface cue just clear of the water's moving facets so the narrow
	// procedural band cannot disappear into the prototype pool mesh.
	StartleRing->SetRelativeLocation(FVector(0.f, 0.f, 32.f));
	StartleRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StartleRing->SetCastShadow(false);
	StartleRing->SetCanEverAffectNavigation(false);
	StartleRing->SetGenerateOverlapEvents(false);
	StartleRing->SetVisibility(false, true);
	BuildStartleRing();
}

void AIslandPoolRippleEffect::BeginPlay()
{
	Super::BeginPlay();
	EnsureStartleRingMaterial();
	UpdateRipple(0.f);
}

void AIslandPoolRippleEffect::ConfigureAsRainImpact()
{
	DurationSeconds = 1.15f;
	SurfaceRadius = 68.f;
	// Weather is atmosphere, not an interaction cue: keep its reflected glints soft enough
	// that they do not read as bright floating beads around the pool.
	PeakLightIntensity = 0.75f;
	Tags.AddUnique(TEXT("RainImpact"));
}

void AIslandPoolRippleEffect::ConfigureAsWindImpact(float HorizontalWindSpeed)
{
	const float Activity = WindRippleActivity(HorizontalWindSpeed);
	DurationSeconds = FMath::Lerp(1.25f, 2.2f, Activity);
	SurfaceRadius = FMath::Lerp(58.f, 112.f, Activity);
	PeakLightIntensity = FMath::Lerp(0.4f, 1.35f, Activity);
	Tags.AddUnique(TEXT("WindImpact"));
	UpdateRipple(FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.f, 1.f));
}

void AIslandPoolRippleEffect::ConfigureAsMinnowImpact()
{
	DurationSeconds = 0.95f;
	SurfaceRadius = 48.f;
	PeakLightIntensity = 0.65f;
	Tags.AddUnique(TEXT("MinnowImpact"));
	TriggerWaterRipple(0.42f);
	UpdateRipple(FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.f, 1.f));
}

void AIslandPoolRippleEffect::ConfigureAsMinnowStartleImpact()
{
	DurationSeconds = 1.15f;
	SurfaceRadius = 72.f;
	PeakLightIntensity = 2.4f;
	Tags.AddUnique(TEXT("MinnowImpact"));
	Tags.AddUnique(TEXT("MinnowStartleImpact"));
	EnsureStartleRingMaterial();
	TriggerWaterRipple(0.62f);
	if (StartleRing) StartleRing->SetVisibility(true, true);
	UpdateRipple(FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.f, 1.f));
}

void AIslandPoolRippleEffect::TriggerWaterRipple(float Strength)
{
	if (!GetWorld()) return;
	if (UIslandTideglassSubsystem* Tideglass = GetWorld()->GetSubsystem<UIslandTideglassSubsystem>())
		Tideglass->TriggerSurfaceRipple(GetActorLocation(), DurationSeconds, SurfaceRadius, Strength);
}

float AIslandPoolRippleEffect::WindRippleActivity(float HorizontalWindSpeed)
{
	const float Speed = FMath::IsFinite(HorizontalWindSpeed) ? FMath::Max(0.f, HorizontalWindSpeed) : 0.f;
	return FMath::SmoothStep(45.f, 150.f, Speed);
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
	if (StartleRing && StartleRing->IsVisible())
	{
		StartleRing->SetRelativeScale3D(FVector(RingRadius, RingRadius, 1.f));
		if (StartleRingMaterial)
		{
			// The moving point lights provide the readable cue; keep the continuous band
			// subdued so it reads as a passing water ripple rather than a neon marker.
			const FLinearColor RingTint = FLinearColor(0.008f, 0.12f, 0.15f) * Pulse;
			StartleRingMaterial->SetVectorParameterValue(TEXT("Color"), RingTint);
			StartleRingMaterial->SetVectorParameterValue(TEXT("BaseColor"), RingTint);
			StartleRingMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), RingTint);
		}
	}
	for (int32 Index = 0; Index < RippleLights.Num(); ++Index)
	{
		const float Angle = (2.f * PI * Index) / RippleLights.Num();
		UPointLightComponent* Light = RippleLights[Index];
		Light->SetRelativeLocation(FVector(FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius, 24.f));
		Light->SetIntensity(PeakLightIntensity * Pulse);
	}
}

void AIslandPoolRippleEffect::BuildStartleRing()
{
	if (!StartleRing) return;

	constexpr int32 SegmentCount = 64;
	// A narrow ring disappears among the prototype water's broad specular facets.
	// Keep this a soft band, but give it enough surface area to read at game scale.
	constexpr float InnerRadius = 0.90f;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(SegmentCount * 2);
	Triangles.Reserve(SegmentCount * 6);
	Normals.Reserve(SegmentCount * 2);
	UVs.Reserve(SegmentCount * 2);
	Colors.Reserve(SegmentCount * 2);
	Tangents.Reserve(SegmentCount * 2);

	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const float Angle = 2.f * PI * Index / SegmentCount;
		const float CosAngle = FMath::Cos(Angle);
		const float SinAngle = FMath::Sin(Angle);
		Vertices.Add(FVector(CosAngle * InnerRadius, SinAngle * InnerRadius, 0.f));
		Vertices.Add(FVector(CosAngle, SinAngle, 0.f));
		for (int32 Edge = 0; Edge < 2; ++Edge)
		{
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D((CosAngle + 1.f) * 0.5f, (SinAngle + 1.f) * 0.5f));
			Colors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(FVector(-SinAngle, CosAngle, 0.f), false));
		}
	}

	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const int32 CurrentInner = Index * 2;
		const int32 CurrentOuter = CurrentInner + 1;
		const int32 NextInner = ((Index + 1) % SegmentCount) * 2;
		const int32 NextOuter = NextInner + 1;
		Triangles.Append({ CurrentInner, CurrentOuter, NextOuter, CurrentInner, NextOuter, NextInner });
	}

	StartleRing->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
	if (RippleMaterial) StartleRing->SetMaterial(0, RippleMaterial);
}

void AIslandPoolRippleEffect::EnsureStartleRingMaterial()
{
	if (!StartleRing || StartleRingMaterial || !RippleMaterial) return;
	StartleRingMaterial = UMaterialInstanceDynamic::Create(RippleMaterial, this);
	if (StartleRingMaterial) StartleRing->SetMaterial(0, StartleRingMaterial);
}
