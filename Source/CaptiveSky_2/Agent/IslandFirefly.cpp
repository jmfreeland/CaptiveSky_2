#include "IslandFirefly.h"
#include "IslandDayNight.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FLinearColor FireflyBodyEmissiveColor(0.07f, 0.16f, 0.045f);
}

AIslandFirefly::AIslandFirefly()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	GlowingBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlowingBody"));
	GlowingBody->SetupAttachment(RootComponent);
	GlowingBody->SetRelativeLocation(FVector(-1.8f, 0.f, 0.f));
	// Keep the luminous abdomen distinct from the darker, articulated insect body.
	GlowingBody->SetRelativeScale3D(FVector(0.045f, 0.027f, 0.027f));
	GlowingBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlowingBody->SetCastShadow(false);
	AuthoredBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AuthoredBodyMesh"));
	AuthoredBodyMesh->SetupAttachment(RootComponent);
	AuthoredBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AuthoredBodyMesh->SetCastShadow(false);
	AuthoredBodyMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FireflySphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AuthoredFireflyBody(
		TEXT("/Game/Generated/ComfyBlender/Firefly_AnimatedWings/SM_Firefly_01.SM_Firefly_01"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FireflyWingSurface(
		TEXT("/Game/Generated/ComfyBlender/Firefly_AnimatedWings/Materials/M_Lib_Dirt.M_Lib_Dirt"));
	if (AuthoredFireflyBody.Succeeded())
	{
		AuthoredBodyMesh->SetStaticMesh(AuthoredFireflyBody.Object);
		AuthoredBodyMesh->SetRelativeScale3D(FVector::OneVector);
		AuthoredBodyMesh->SetVisibility(true);
		bUsingAuthoredBodyMesh = true;
	}
	if (FireflySphere.Succeeded())
	{
		GlowingBody->SetStaticMesh(FireflySphere.Object);
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FireflyEmissiveMaterial(
			TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
		if (FireflyEmissiveMaterial.Succeeded()) GlowingBody->SetMaterial(0, FireflyEmissiveMaterial.Object);
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FireflyBodyMaterialInstance(
			TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FireflyBodyMaterial(
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UMaterialInterface* FireflyBodySurface = FireflyBodyMaterialInstance.Succeeded()
			? FireflyBodyMaterialInstance.Object : (FireflyBodyMaterial.Succeeded() ? FireflyBodyMaterial.Object : nullptr);
		const auto MakeBodyPart = [&](const TCHAR* Name, const FVector& Location, const FVector& Scale)
		{
			UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
			Part->SetupAttachment(RootComponent);
			Part->SetStaticMesh(FireflySphere.Object);
			Part->SetRelativeLocation(Location);
			Part->SetRelativeScale3D(Scale);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part->SetCastShadow(false);
			if (FireflyBodySurface) Part->SetMaterial(0, FireflyBodySurface);
			return Part;
		};
		AbdomenShell = MakeBodyPart(TEXT("AbdomenShell"), FVector(-3.8f, 0.f, 0.f), FVector(0.068f, 0.039f, 0.039f));
		Thorax = MakeBodyPart(TEXT("Thorax"), FVector(1.6f, 0.f, 0.3f), FVector(0.044f, 0.032f, 0.031f));
		Head = MakeBodyPart(TEXT("Head"), FVector(4.8f, 0.f, 0.7f), FVector(0.03f, 0.027f, 0.025f));
		// Keep wings as separate components so their flutter remains animated even
		// when the detailed, body-only mesh is installed.
		LeftWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWing"));
		LeftWing->SetupAttachment(RootComponent);
		LeftWing->SetStaticMesh(FireflySphere.Object);
		LeftWing->SetRelativeLocation(FVector(0.f, -3.2f, 1.8f));
		LeftWing->SetRelativeScale3D(FVector(0.018f, 0.05f, 0.003f));
		LeftWing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeftWing->SetCastShadow(false);

		RightWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWing"));
		RightWing->SetupAttachment(RootComponent);
		RightWing->SetStaticMesh(FireflySphere.Object);
		RightWing->SetRelativeLocation(FVector(0.f, 3.2f, 1.8f));
		RightWing->SetRelativeScale3D(FVector(0.018f, 0.05f, 0.003f));
		RightWing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RightWing->SetCastShadow(false);
		if (bUsingAuthoredBodyMesh)
		{
			// Keep a single animated wing pair: the imported mesh supplies the
			// detailed body while these rough, separate wings retain their flutter.
			AbdomenShell->SetVisibility(false);
			Thorax->SetVisibility(false);
			Head->SetVisibility(false);
			LeftWing->SetRelativeLocation(FVector(0.f, -3.1f, 3.2f));
			RightWing->SetRelativeLocation(FVector(0.f, 3.1f, 3.2f));
			LeftWing->SetRelativeScale3D(FVector(0.017f, 0.05f, 0.0025f));
			RightWing->SetRelativeScale3D(FVector(0.017f, 0.05f, 0.0025f));
			GlowingBody->SetRelativeLocation(FVector(0.8f, 0.f, 0.7f));
		}
		if (FireflyBodySurface)
		{
			LeftWing->SetMaterial(0, FireflyBodySurface);
			RightWing->SetMaterial(0, FireflyBodySurface);
		}
		if (bUsingAuthoredBodyMesh && FireflyWingSurface.Succeeded())
		{
			LeftWing->SetMaterial(0, FireflyWingSurface.Object);
			RightWing->SetMaterial(0, FireflyWingSurface.Object);
		}
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetLightColor(FLinearColor(0.78f, 1.f, 0.52f));
	Glow->SetAttenuationRadius(420.f);
	Glow->SetCastShadows(false);
	Glow->SetIntensity(0.f);
	Glow->SetRelativeLocation(bUsingAuthoredBodyMesh ? FVector(0.8f, 0.f, 0.7f) : FVector(-1.8f, 0.f, 0.f));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("Firefly"));
}

void AIslandFirefly::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnsureGlowMaterial();
	if (GlowMaterial) GlowMaterial->SetVectorParameterValue(TEXT("Color"), FireflyBodyEmissiveColor);
}

void AIslandFirefly::ConfigureBodyAppearance()
{
	if (bBodyAppearanceConfigured) return;
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (Part)
			if (UMaterialInstanceDynamic* Material = Part->CreateAndSetMaterialInstanceDynamic(0))
			{
				Material->SetVectorParameterValue(TEXT("Color"), Color);
				Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
			}
	};
	Tint(AbdomenShell, FLinearColor(0.035f, 0.025f, 0.014f));
	Tint(Thorax, FLinearColor(0.055f, 0.038f, 0.021f));
	Tint(Head, FLinearColor(0.09f, 0.063f, 0.032f));
	Tint(LeftWing, FLinearColor(0.045f, 0.035f, 0.02f));
	Tint(RightWing, FLinearColor(0.045f, 0.035f, 0.02f));
	bBodyAppearanceConfigured = true;
}

void AIslandFirefly::EnsureGlowMaterial()
{
	if (!GlowMaterial && GlowingBody && GlowingBody->GetMaterial(0))
		GlowMaterial = GlowingBody->CreateAndSetMaterialInstanceDynamic(0);
}

void AIslandFirefly::BeginPlay()
{
	Super::BeginPlay();
	ConfigureBodyAppearance();
	HomeLocation = GetActorLocation();
	EnsureGlowMaterial();
	Phase = FMath::FRandRange(0.f, 2.f * PI);
	MotionRate = FMath::FRandRange(0.78f, 1.24f);
	PulseRate = FMath::FRandRange(0.82f, 1.18f);
	WingBeatPhase = FMath::FRandRange(0.f, 2.f * PI);
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		DayNight = *It;
		break;
	}
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	const double IslandTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float Rain = Weather.IsValid() ? Weather->SampleRainIntensity(IslandTime) : 0.f;
	UpdateGlow(IslandTime, Rain);
	UpdateWings(IslandTime, Rain);
}

FVector AIslandFirefly::WindDisplacement(const FVector& LocalWind)
{
	return LocalWind.GetClampedToMaxSize(250.f) * 0.12f;
}

float AIslandFirefly::RainActivity(float RainIntensity)
{
	return FMath::SmoothStep(0.35f, 0.8f, FMath::Clamp(RainIntensity, 0.f, 1.f));
}

float AIslandFirefly::RainMovementScale(float RainIntensity)
{
	return FMath::Lerp(1.f, 0.38f, RainActivity(RainIntensity));
}

float AIslandFirefly::RainGlowScale(float RainIntensity)
{
	return FMath::Lerp(1.f, 0.45f, RainActivity(RainIntensity));
}

float AIslandFirefly::RainWingBeatScale(float RainIntensity)
{
	return FMath::Lerp(1.f, 0.65f, RainActivity(RainIntensity));
}

float AIslandFirefly::MoonlightGlowScale(float LunarIllumination)
{
	return FMath::Lerp(1.f, 0.72f, FMath::Clamp(LunarIllumination, 0.f, 1.f));
}

float AIslandFirefly::DaylightWanderScale(float CurrentHour)
{
	if (!FMath::IsFinite(CurrentHour)) return 1.f;

	// During bright hours the firefly stays close to its home point; twilight
	// gradually opens the larger, slow night-flight loop.
	constexpr float DaytimeWander = 0.22f;
	return DaytimeWander + (1.f - DaytimeWander) * AIslandDayNight::NightAmount(CurrentHour);
}

FVector AIslandFirefly::ResolveFlightPath(const FVector& Start, const FVector& Desired) const
{
	UWorld* World = GetWorld();
	if (!World || Start.Equals(Desired)) return Desired;

	constexpr float FlightClearance = 8.f;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(FlightClearance);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandFireflyFlight), false, this);
	FHitResult Hit;
	if (!World->SweepSingleByChannel(Hit, Start, Desired, FQuat::Identity, ECC_WorldStatic, Shape, Query))
		return Desired;
	if (Hit.bStartPenetrating) return Start;

	// Step just clear of contact, then spend the remaining movement along the blocking surface.
	const FVector Contact = Hit.Location + Hit.Normal * 2.f;
	const FVector Remaining = Desired - Contact;
	const FVector Slide = Remaining - Hit.Normal * FVector::DotProduct(Remaining, Hit.Normal);
	if (Slide.IsNearlyZero()) return Contact;

	FHitResult SlideHit;
	if (World->SweepSingleByChannel(SlideHit, Contact, Contact + Slide, FQuat::Identity, ECC_WorldStatic, Shape, Query))
		return SlideHit.bStartPenetrating ? Contact : SlideHit.Location;
	return Contact + Slide;
}

void AIslandFirefly::RespondToQuietObservation()
{
	ObservationPulseRemaining = 3.f;
}

FVector AIslandFirefly::EvaluateChimeResponseDisplacement(const FVector& DirectionToChime, float ElapsedSeconds)
{
	if (DirectionToChime.ContainsNaN() || !FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0.f ||
		ElapsedSeconds >= ChimeResponseDurationSeconds)
	{
		return FVector::ZeroVector;
	}
	const FVector Direction = DirectionToChime.GetSafeNormal2D();
	if (Direction.IsNearlyZero()) return FVector::ZeroVector;
	const float Progress = ElapsedSeconds / ChimeResponseDurationSeconds;
	const float Arc = FMath::Sin(PI * Progress);
	return Direction * ChimeApproachDistanceCm * Arc;
}

bool AIslandFirefly::RespondToSoftChime(AIslandListeningStonesChime* Chime)
{
	if (!IsValid(Chime) || !GetWorld() || Chime->GetWorld() != GetWorld()) return false;
	RespondedChimes.RemoveAll([](const TWeakObjectPtr<AIslandListeningStonesChime>& HeardChime)
	{
		const AIslandListeningStonesChime* Actor = HeardChime.Get();
		return !Actor || Actor->IsActorBeingDestroyed() || !Actor->IsAudibleAt(Actor->GetActorLocation());
	});
	if (Chime->IsActorBeingDestroyed()) return false;
	const TWeakObjectPtr<AIslandListeningStonesChime> ChimeRef(Chime);
	if (RespondedChimes.Contains(ChimeRef) ||
		!Chime->IsAudibleAt(GetActorLocation()))
	{
		return false;
	}
	RespondedChimes.Add(ChimeRef);
	ChimeResponseDirection = (Chime->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	ChimeResponseElapsed = 0.f;
	ChimeResponseRemaining = ChimeResponseDurationSeconds;
	return true;
}

void AIslandFirefly::CheckForNearbyStoneChime()
{
	if (!GetWorld() || ChimeResponseRemaining > 0.f) return;
	RespondedChimes.RemoveAll([](const TWeakObjectPtr<AIslandListeningStonesChime>& HeardChime)
	{
		const AIslandListeningStonesChime* Actor = HeardChime.Get();
		return !Actor || Actor->IsActorBeingDestroyed() || !Actor->IsAudibleAt(Actor->GetActorLocation());
	});
	for (TActorIterator<AIslandListeningStonesChime> It(GetWorld()); It; ++It)
	{
		if (RespondToSoftChime(*It)) return;
	}
}

void AIslandFirefly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()) return;

	const double Time = GetWorld()->GetTimeSeconds();
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ObservationPulseRemaining = FMath::Max(0.f, ObservationPulseRemaining - SafeDelta);
	ChimeResponseElapsed = FMath::Min(ChimeResponseDurationSeconds, ChimeResponseElapsed + SafeDelta);
	ChimeResponseRemaining = FMath::Max(0.f, ChimeResponseRemaining - SafeDelta);
	ChimeCheckRemaining -= SafeDelta;
	if (ChimeCheckRemaining <= 0.f)
	{
		ChimeCheckRemaining = 0.2f;
		CheckForNearbyStoneChime();
	}
	const float T = static_cast<float>(Time);
	const float Rain = Weather.IsValid() ? Weather->SampleRainIntensity(Time) : 0.f;
	const float RainActivityFactor = RainActivity(Rain);
	const float MotionTime = T * MotionRate;
	const float CircadianWander = DayNight.IsValid()
		? DaylightWanderScale(DayNight->CurrentHour)
		: 1.f;
	const float WeatherWander = RainMovementScale(Rain);
	const FVector Offset(
		WanderRadius * WeatherWander * CircadianWander * (0.72f * FMath::Sin(MotionTime * 0.31f + Phase) + 0.28f * FMath::Sin(MotionTime * 0.17f + Phase * 1.7f)),
		WanderRadius * 0.65f * WeatherWander * CircadianWander * (0.7f * FMath::Sin(MotionTime * 0.23f + Phase * 2.1f) + 0.3f * FMath::Sin(MotionTime * 0.41f + Phase)),
		FMath::Lerp(HoverHeight, 22.f, RainActivityFactor) + CircadianWander * FMath::Lerp(34.f, 10.f, RainActivityFactor) * FMath::Sin(MotionTime * 0.73f + Phase * 1.3f));
	const FVector Wind = Weather.IsValid() ? Weather->GetLocalWind(GetActorLocation(), this) : FVector::ZeroVector;
	const FVector ChimeDisplacement = ChimeResponseRemaining > 0.f
		? EvaluateChimeResponseDisplacement(ChimeResponseDirection, ChimeResponseElapsed)
		: FVector::ZeroVector;
	const FVector DesiredLocation = HomeLocation + Offset + WindDisplacement(Wind) + ChimeDisplacement;
	SetActorLocation(ResolveFlightPath(GetActorLocation(), DesiredLocation), false);
	UpdateGlow(Time, Rain);
	UpdateWings(Time, Rain);
}

void AIslandFirefly::UpdateWings(double IslandTimeSeconds, float RainIntensity)
{
	const float Beat = FMath::Sin(static_cast<float>(IslandTimeSeconds) * 38.f * RainWingBeatScale(RainIntensity) + WingBeatPhase) * 42.f * RainWingBeatScale(RainIntensity);
	if (LeftWing) LeftWing->SetRelativeRotation(FRotator(0.f, 0.f, 18.f + Beat));
	if (RightWing) RightWing->SetRelativeRotation(FRotator(0.f, 0.f, -18.f - Beat));
}

void AIslandFirefly::UpdateGlow(double IslandTimeSeconds, float RainIntensity)
{
	ConfigureBodyAppearance();
	if (!Glow) return;
	const float T = static_cast<float>(IslandTimeSeconds);
	const float Pulse = FMath::Max(0.f, FMath::Sin(T * 4.2f * PulseRate + Phase));
	// Keep a visible ember between flashes so the insect does not disappear at the pulse trough.
	const float NaturalPulse = 0.24f + 0.76f * FMath::Pow(Pulse, 4.f);
	const float ObservationAccent = 1.f + 0.7f * FMath::Clamp(ObservationPulseRemaining / 3.f, 0.f, 1.f);
	const float ChimeAccent = FMath::Clamp(ChimeResponseRemaining / ChimeResponseDurationSeconds, 0.f, 1.f);
	const float ChimePulse = NaturalPulse * (1.f + 0.25f * ChimeAccent);
	const float LunarIllumination = DayNight.IsValid()
		? AIslandDayNight::LunarIllumination(DayNight->DayNumber, DayNight->CurrentHour)
		: 0.f;
	const float GlowScale = RainGlowScale(RainIntensity) * MoonlightGlowScale(LunarIllumination) * ChimePulse * ObservationAccent;
	Glow->SetIntensity(GlowIntensity * GlowScale);
	if (GlowMaterial)
	{
		GlowMaterial->SetVectorParameterValue(TEXT("Color"), FireflyBodyEmissiveColor * GlowScale);
	}
}
