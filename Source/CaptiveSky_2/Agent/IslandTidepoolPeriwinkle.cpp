#include "IslandTidepoolPeriwinkle.h"

#include "IslandDayNight.h"
#include "IslandTideglassSubsystem.h"
#include "IslandWeather.h"
#include "ProceduralMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIslandTidepoolPeriwinkle::AIslandTidepoolPeriwinkle()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("TidepoolPeriwinkle"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeInstance(TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseShapeMaterial = BasicShapeInstance.Succeeded() ? BasicShapeInstance.Object : (BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr);

	Foot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foot"));
	Foot->SetupAttachment(RootComponent);
	Foot->SetRelativeLocation(FVector(1.f, 0.f, 2.3f));
	Foot->SetRelativeScale3D(FVector(0.25f, 0.17f, 0.04f));
	Foot->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);

	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(RootComponent);
	Shell->SetRelativeLocation(FVector(-4.f, 0.f, 13.f));
	Shell->SetRelativeScale3D(FVector(0.21f, 0.19f, 0.18f));
	Shell->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);

	Aperture = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellAperture"));
	Aperture->SetupAttachment(RootComponent);
	Aperture->SetRelativeLocation(FVector(8.f, 0.f, 8.f));
	Aperture->SetRelativeScale3D(FVector(0.035f, 0.105f, 0.10f));
	Aperture->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);

	for (int32 Side : {-1, 1})
	{
		const FName StalkName(*FString::Printf(TEXT("EyeStalk_%d"), Side));
		UStaticMeshComponent* Stalk = CreateDefaultSubobject<UStaticMeshComponent>(StalkName);
		Stalk->SetupAttachment(RootComponent);
		Stalk->SetRelativeLocation(FVector(9.f, Side * 6.f, 7.f));
		Stalk->SetRelativeRotation(FRotator(0.f, Side * 10.f, -Side * 12.f));
		Stalk->SetRelativeScale3D(FVector(0.018f, 0.018f, 0.10f));
		Stalk->SetStaticMesh(Cylinder.Succeeded() ? Cylinder.Object : nullptr);
		EyeStalks.Add(Stalk);

		const FName EyeName(*FString::Printf(TEXT("Eye_%d"), Side));
		UStaticMeshComponent* Eye = CreateDefaultSubobject<UStaticMeshComponent>(EyeName);
		Eye->SetupAttachment(Stalk);
		Eye->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
		Eye->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.035f));
		Eye->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		Eyes.Add(Eye);
	}

	ShellSpiral = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShellGrowthSpiral"));
	ShellSpiral->SetupAttachment(RootComponent);
	ShellSpiral->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShellSpiral->SetCanEverAffectNavigation(false);
	ShellSpiral->SetGenerateOverlapEvents(false);
	ShellSpiral->SetCastShadow(false);

	const TArray<UPrimitiveComponent*> NonBlockingParts = { Foot.Get(), Shell.Get(), Aperture.Get(), ShellSpiral.Get() };
	for (UPrimitiveComponent* Part : NonBlockingParts)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
	}
	for (UStaticMeshComponent* Part : EyeStalks)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
	}
	for (UStaticMeshComponent* Part : Eyes)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
	}

	BuildShellSpiral();
}

void AIslandTidepoolPeriwinkle::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	Phase = FMath::Fmod(FMath::Abs(HomeLocation.X * 0.001f + HomeLocation.Y * 0.0017f), 2.f * PI);
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		IslandClock = *It;
		break;
	}
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		IslandWeather = *It;
		break;
	}
	if (!BaseShapeMaterial)
	{
		BaseShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
		if (!BaseShapeMaterial)
			BaseShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (BaseShapeMaterial)
	{
		FootMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		ShellMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		SpiralMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		if (FootMaterial) Foot->SetMaterial(0, FootMaterial);
		if (ShellMaterial) Shell->SetMaterial(0, ShellMaterial);
		if (SpiralMaterial) ShellSpiral->SetMaterial(0, SpiralMaterial);
		if (Aperture && SpiralMaterial) Aperture->SetMaterial(0, SpiralMaterial);
	}
	ApplyPalette();
}

float AIslandTidepoolPeriwinkle::ExposureRestAmount(float TideOffsetCm)
{
	if (!FMath::IsFinite(TideOffsetCm)) return 0.f;
	const float NormalizedTide = FMath::Clamp(
		TideOffsetCm / UIslandTideglassSubsystem::MaximumTideOffsetCm, -1.f, 1.f);
	// Stay open while water is returning; tuck in only through the exposed ebb.
	return 1.f - FMath::SmoothStep(-0.72f, -0.12f, NormalizedTide);
}

float AIslandTidepoolPeriwinkle::RainRestAmount(float RainIntensity)
{
	if (!FMath::IsFinite(RainIntensity)) return 0.f;
	return 0.42f * FMath::SmoothStep(0.45f, 0.9f, FMath::Clamp(RainIntensity, 0.f, 1.f));
}

float AIslandTidepoolPeriwinkle::WetShellAmount(float TideOffsetCm, float RainIntensity)
{
	const float TideWetness = FMath::IsFinite(TideOffsetCm)
		? 1.f - ExposureRestAmount(TideOffsetCm)
		: 0.f;
	const float RainWetness = FMath::IsFinite(RainIntensity)
		? 0.55f * FMath::SmoothStep(0.45f, 0.9f, FMath::Clamp(RainIntensity, 0.f, 1.f))
		: 0.f;
	return FMath::Clamp(FMath::Max(TideWetness, RainWetness), 0.f, 1.f);
}

bool AIslandTidepoolPeriwinkle::RespondToQuietObservation()
{
	if (ObservationCooldownRemaining > 0.f) return false;
	ObservationRestRemaining = 1.6f;
	ObservationCooldownRemaining = 4.2f;
	return true;
}

void AIslandTidepoolPeriwinkle::SetColorVariant(int32 Variant)
{
	ColorVariant = FMath::Clamp(Variant, 0, 2);
	ApplyPalette();
}

void AIslandTidepoolPeriwinkle::ApplyPalette()
{
	static const FLinearColor ShellPalettes[] = {
		FLinearColor(0.46f, 0.24f, 0.095f),
		FLinearColor(0.31f, 0.34f, 0.19f),
		FLinearColor(0.48f, 0.34f, 0.18f)
	};
	const FLinearColor ShellColor = ShellPalettes[FMath::Clamp(ColorVariant, 0, 2)];
	const float Wetness = FMath::Clamp(ShellWetness, 0.f, 1.f);
	const FLinearColor DampShellColor = FMath::Lerp(ShellColor,
		FLinearColor(ShellColor.R * 0.78f, ShellColor.G * 0.84f, ShellColor.B * 0.9f, 1.f), Wetness);
	if (FootMaterial) FootMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.25f, 0.15f));
	if (ShellMaterial) ShellMaterial->SetVectorParameterValue(TEXT("Color"), DampShellColor);
	if (SpiralMaterial) SpiralMaterial->SetVectorParameterValue(TEXT("Color"), DampShellColor * 0.42f);
	for (UStaticMeshComponent* Eye : Eyes)
		if (Eye && BaseShapeMaterial) Eye->SetMaterial(0, BaseShapeMaterial);
}

void AIslandTidepoolPeriwinkle::BuildShellSpiral()
{
	if (!ShellSpiral) return;

	constexpr int32 SampleCount = 72;
	constexpr int32 TubeSides = 5;
	constexpr float ShellCenterX = -4.f;
	constexpr float ShellCenterZ = 13.f;
	constexpr float ShellRadiusX = 10.5f;
	constexpr float ShellRadiusY = 9.5f;
	constexpr float ShellRadiusZ = 9.f;
	constexpr float StripeRadius = 0.48f;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(SampleCount * TubeSides);
	Triangles.Reserve((SampleCount - 1) * TubeSides * 6);
	Normals.Reserve(SampleCount * TubeSides);
	UVs.Reserve(SampleCount * TubeSides);
	Colors.Reserve(SampleCount * TubeSides);
	Tangents.Reserve(SampleCount * TubeSides);

	for (int32 Sample = 0; Sample < SampleCount; ++Sample)
	{
		const float T = static_cast<float>(Sample) / (SampleCount - 1);
		const float Angle = T * 4.4f * PI;
		const float Radius = FMath::Lerp(0.45f, 8.25f, T);
		const float XOffset = Radius * FMath::Cos(Angle);
		const float Y = Radius * FMath::Sin(Angle);
		const float SurfaceTerm = FMath::Max(0.06f, 1.f - FMath::Square(XOffset / ShellRadiusX) - FMath::Square(Y / ShellRadiusY));
		const float Z = ShellCenterZ + ShellRadiusZ * FMath::Sqrt(SurfaceTerm) + 0.35f;
		const FVector Center(ShellCenterX + XOffset, Y, Z);
		const FVector SurfaceNormal(XOffset / FMath::Square(ShellRadiusX), Y / FMath::Square(ShellRadiusY), (Z - ShellCenterZ) / FMath::Square(ShellRadiusZ));
		const FVector Normal = SurfaceNormal.GetSafeNormal();
		const FVector Tangent(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		const FVector Side = FVector::CrossProduct(Tangent, Normal).GetSafeNormal();
		for (int32 Ring = 0; Ring < TubeSides; ++Ring)
		{
			const float RingAngle = 2.f * PI * Ring / TubeSides;
			const FVector Offset = (Normal * FMath::Cos(RingAngle) + Side * FMath::Sin(RingAngle)) * StripeRadius;
			Vertices.Add(Center + Offset);
			Normals.Add((Normal * FMath::Cos(RingAngle) + Side * FMath::Sin(RingAngle)).GetSafeNormal());
			UVs.Add(FVector2D(T, static_cast<float>(Ring) / TubeSides));
			Colors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(Tangent, false));
		}
	}

	for (int32 Sample = 0; Sample < SampleCount - 1; ++Sample)
	{
		for (int32 Ring = 0; Ring < TubeSides; ++Ring)
		{
			const int32 A = Sample * TubeSides + Ring;
			const int32 B = Sample * TubeSides + (Ring + 1) % TubeSides;
			const int32 C = (Sample + 1) * TubeSides + (Ring + 1) % TubeSides;
			const int32 D = (Sample + 1) * TubeSides + Ring;
			Triangles.Append({ A, B, C, A, C, D, A, C, B, A, D, C });
		}
	}
	ShellSpiral->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void AIslandTidepoolPeriwinkle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedSeconds += FMath::Max(0.f, DeltaSeconds);
	const float TideOffset = IslandClock.IsValid()
		? UIslandTideglassSubsystem::TideOffsetCm(IslandClock->CurrentHour, IslandClock->DayNumber)
		: UIslandTideglassSubsystem::MaximumTideOffsetCm;
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ObservationRestRemaining = FMath::Max(0.f, ObservationRestRemaining - SafeDelta);
	ObservationCooldownRemaining = FMath::Max(0.f, ObservationCooldownRemaining - SafeDelta);
	const float TideRest = ExposureRestAmount(TideOffset);
	const float ObservationRest = FMath::SmoothStep(0.f, 0.35f, ObservationRestRemaining);
	const float RainIntensity = IslandWeather.IsValid() && GetWorld()
		? IslandWeather->SampleRainIntensity(GetWorld()->GetTimeSeconds())
		: 0.f;
	const float TargetShellWetness = WetShellAmount(TideOffset, RainIntensity);
	const float PreviousShellWetness = ShellWetness;
	ShellWetness = FMath::FInterpTo(ShellWetness, TargetShellWetness, SafeDelta, 0.8f);
	if (FMath::Abs(ShellWetness - PreviousShellWetness) > 0.001f)
		ApplyPalette();
	const float WeatherRest = RainRestAmount(RainIntensity);
	const float ExposureRest = FMath::Max(TideRest, FMath::Max(ObservationRest, WeatherRest));
	const float StalkExtension = FMath::Lerp(1.f, 0.46f, ExposureRest);
	const float ApertureExtension = FMath::Lerp(1.f, 0.5f, ExposureRest);
	const float AttentionScale = FMath::Lerp(1.f, 0.35f, ExposureRest);
	// Close visitors draw a quiet glance; the animal stays planted on its rock.
	float TargetAttentionYaw = 0.f;
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		TargetAttentionYaw = CalculateAttentionYaw(PlayerPawn->GetActorLocation());
	UpdateAttentionYaw(TargetAttentionYaw, DeltaSeconds);
	for (UStaticMeshComponent* EyeStalk : EyeStalks)
		if (EyeStalk) EyeStalk->SetRelativeScale3D(FVector(0.018f, 0.018f, 0.10f * StalkExtension));
	if (Aperture) Aperture->SetRelativeScale3D(FVector(0.035f, 0.105f, 0.10f * ApertureExtension));

	// Barely perceptible antenna and body motion rewards a close look without
	// turning naturally slow intertidal life into a conspicuous moving prop.
	const float ActivityScale = FMath::Lerp(1.f, 0.3f, ExposureRest);
	const float Sway = FMath::Sin(ElapsedSeconds * 0.8f + Phase) * 2.4f * ActivityScale;
	for (int32 Index = 0; Index < EyeStalks.Num(); ++Index)
	{
		if (EyeStalks[Index])
			EyeStalks[Index]->SetRelativeRotation(FRotator(0.f, (Index == 0 ? 1.f : -1.f) * (12.f + Sway) + AttentionYaw * AttentionScale, (Index == 0 ? -1.f : 1.f) * 12.f));
	}
	SetActorRotation(FRotator(0.f, Sway * 0.45f, 0.f));
	const float BodyBob = FMath::Sin(ElapsedSeconds * 0.55f + Phase) * FMath::Lerp(0.8f, 0.2f, ExposureRest);
	SetActorLocation(HomeLocation + FVector(0.f, 0.f, BodyBob));
}

void AIslandTidepoolPeriwinkle::UpdateAttentionYaw(float TargetYaw, float DeltaSeconds)
{
	// Turn toward a nearby visitor attentively, but relax the gaze more slowly.
	const float InterpSpeed = FMath::IsNearlyZero(TargetYaw) ? 1.2f : 2.4f;
	AttentionYaw = FMath::FInterpTo(AttentionYaw, TargetYaw, DeltaSeconds, InterpSpeed);
}

float AIslandTidepoolPeriwinkle::CalculateAttentionYaw(const FVector& TargetLocation) const
{
	constexpr float AttentionRadius = 220.f;
	constexpr float MaximumYaw = 35.f;
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero() || ToTarget.SizeSquared() > FMath::Square(AttentionRadius))
		return 0.f;

	const FVector LocalDirection = GetActorTransform().InverseTransformVectorNoScale(ToTarget);
	if (LocalDirection.X <= 0.f)
		return 0.f;
	return FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(LocalDirection.Y, LocalDirection.X)), -MaximumYaw, MaximumYaw);
}
