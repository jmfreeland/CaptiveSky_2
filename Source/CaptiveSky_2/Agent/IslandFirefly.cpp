#include "IslandFirefly.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "UObject/ConstructorHelpers.h"

AIslandFirefly::AIslandFirefly()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	GlowingBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlowingBody"));
	GlowingBody->SetupAttachment(RootComponent);
	GlowingBody->SetRelativeLocation(FVector(-1.8f, 0.f, 0.f));
	GlowingBody->SetRelativeScale3D(FVector(0.028f, 0.018f, 0.018f));
	GlowingBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlowingBody->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FireflySphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (FireflySphere.Succeeded())
	{
		GlowingBody->SetStaticMesh(FireflySphere.Object);
		// Thin ellipsoids are a placeholder for wings until a proper insect mesh is chosen.
		LeftWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWing"));
		LeftWing->SetupAttachment(RootComponent);
		LeftWing->SetStaticMesh(FireflySphere.Object);
		LeftWing->SetRelativeLocation(FVector(0.f, -1.25f, 1.1f));
		LeftWing->SetRelativeScale3D(FVector(0.018f, 0.024f, 0.003f));
		LeftWing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeftWing->SetCastShadow(false);

		RightWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWing"));
		RightWing->SetupAttachment(RootComponent);
		RightWing->SetStaticMesh(FireflySphere.Object);
		RightWing->SetRelativeLocation(FVector(0.f, 1.25f, 1.1f));
		RightWing->SetRelativeScale3D(FVector(0.018f, 0.024f, 0.003f));
		RightWing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RightWing->SetCastShadow(false);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetLightColor(FLinearColor(0.55f, 1.f, 0.42f));
	Glow->SetAttenuationRadius(260.f);
	Glow->SetCastShadows(false);
	Glow->SetIntensity(0.f);
	Glow->SetRelativeLocation(FVector(-1.8f, 0.f, 0.f));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("Firefly"));
}

void AIslandFirefly::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	Phase = FMath::FRandRange(0.f, 2.f * PI);
	MotionRate = FMath::FRandRange(0.78f, 1.24f);
	PulseRate = FMath::FRandRange(0.82f, 1.18f);
	WingBeatPhase = FMath::FRandRange(0.f, 2.f * PI);
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

bool AIslandFirefly::RespondToSoftChime(AIslandListeningStonesChime* Chime)
{
	if (!IsValid(Chime) || !GetWorld() || Chime->GetWorld() != GetWorld()) return false;
	RespondedChimes.RemoveAll([](const TWeakObjectPtr<AIslandListeningStonesChime>& HeardChime)
	{
		const AIslandListeningStonesChime* Actor = HeardChime.Get();
		return !Actor || Actor->IsActorBeingDestroyed();
	});
	if (Chime->IsActorBeingDestroyed()) return false;
	const TWeakObjectPtr<AIslandListeningStonesChime> ChimeRef(Chime);
	if (RespondedChimes.Contains(ChimeRef) ||
		FVector::DistSquared(GetActorLocation(), Chime->GetActorLocation()) > FMath::Square(AIslandListeningStonesChime::AudibleRadius))
	{
		return false;
	}
	RespondedChimes.Add(ChimeRef);
	ChimeResponseRemaining = 1.2f;
	return true;
}

void AIslandFirefly::CheckForNearbyStoneChime()
{
	if (!GetWorld() || ChimeResponseRemaining > 0.f) return;
	RespondedChimes.RemoveAll([](const TWeakObjectPtr<AIslandListeningStonesChime>& HeardChime)
	{
		const AIslandListeningStonesChime* Actor = HeardChime.Get();
		return !Actor || Actor->IsActorBeingDestroyed();
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
	const FVector Offset(
		WanderRadius * RainMovementScale(Rain) * (0.72f * FMath::Sin(MotionTime * 0.31f + Phase) + 0.28f * FMath::Sin(MotionTime * 0.17f + Phase * 1.7f)),
		WanderRadius * 0.65f * RainMovementScale(Rain) * (0.7f * FMath::Sin(MotionTime * 0.23f + Phase * 2.1f) + 0.3f * FMath::Sin(MotionTime * 0.41f + Phase)),
		FMath::Lerp(HoverHeight, 22.f, RainActivityFactor) + FMath::Lerp(34.f, 10.f, RainActivityFactor) * FMath::Sin(MotionTime * 0.73f + Phase * 1.3f));
	const FVector Wind = Weather.IsValid() ? Weather->GetLocalWind(GetActorLocation(), this) : FVector::ZeroVector;
	const FVector DesiredLocation = HomeLocation + Offset + WindDisplacement(Wind);
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
	if (!Glow) return;
	const float T = static_cast<float>(IslandTimeSeconds);
	const float Pulse = FMath::Max(0.f, FMath::Sin(T * 4.2f * PulseRate + Phase));
	const float NaturalPulse = 0.12f + 0.88f * FMath::Pow(Pulse, 5.f);
	const float ObservationAccent = 1.f + 0.7f * FMath::Clamp(ObservationPulseRemaining / 3.f, 0.f, 1.f);
	const float ChimeAccent = FMath::Clamp(ChimeResponseRemaining / 1.2f, 0.f, 1.f);
	const float ChimePulse = FMath::Lerp(NaturalPulse, FMath::Max(NaturalPulse, 0.32f), ChimeAccent);
	Glow->SetIntensity(GlowIntensity * RainGlowScale(RainIntensity) * ChimePulse * ObservationAccent);
}
