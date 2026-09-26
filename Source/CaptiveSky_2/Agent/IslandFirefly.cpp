#include "IslandFirefly.h"
#include "IslandWeather.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AIslandFirefly::AIslandFirefly()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	GlowingBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlowingBody"));
	GlowingBody->SetupAttachment(RootComponent);
	GlowingBody->SetRelativeScale3D(FVector(0.05f));
	GlowingBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlowingBody->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FireflySphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (FireflySphere.Succeeded()) GlowingBody->SetStaticMesh(FireflySphere.Object);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetLightColor(FLinearColor(0.55f, 1.f, 0.42f));
	Glow->SetAttenuationRadius(260.f);
	Glow->SetCastShadows(false);
	Glow->SetIntensity(0.f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
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
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	UpdateGlow(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
}

FVector AIslandFirefly::WindDisplacement(const FVector& LocalWind)
{
	return LocalWind.GetClampedToMaxSize(250.f) * 0.12f;
}

void AIslandFirefly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()) return;

	const double Time = GetWorld()->GetTimeSeconds();
	const float T = static_cast<float>(Time);
	const float MotionTime = T * MotionRate;
	const FVector Offset(
		WanderRadius * (0.72f * FMath::Sin(MotionTime * 0.31f + Phase) + 0.28f * FMath::Sin(MotionTime * 0.17f + Phase * 1.7f)),
		WanderRadius * 0.65f * (0.7f * FMath::Sin(MotionTime * 0.23f + Phase * 2.1f) + 0.3f * FMath::Sin(MotionTime * 0.41f + Phase)),
		HoverHeight + 34.f * FMath::Sin(MotionTime * 0.73f + Phase * 1.3f));
	const FVector Wind = Weather.IsValid() ? Weather->GetLocalWind(GetActorLocation(), this) : FVector::ZeroVector;
	SetActorLocation(HomeLocation + Offset + WindDisplacement(Wind), false);
	UpdateGlow(Time);
}

void AIslandFirefly::UpdateGlow(double IslandTimeSeconds)
{
	if (!Glow) return;
	const float T = static_cast<float>(IslandTimeSeconds);
	const float Pulse = FMath::Max(0.f, FMath::Sin(T * 4.2f * PulseRate + Phase));
	Glow->SetIntensity(GlowIntensity * (0.12f + 0.88f * FMath::Pow(Pulse, 5.f)));
}
