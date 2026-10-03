#include "IslandTideglassDragonfly.h"

#include "IslandWeather.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor DragonflyBodyColors[] = {
		FLinearColor(0.13f, 0.26f, 0.12f), // moss green
		FLinearColor(0.10f, 0.24f, 0.28f), // blue-green
		FLinearColor(0.34f, 0.19f, 0.08f)  // copper
	};
	const FLinearColor DragonflyWingColors[] = {
		FLinearColor(0.55f, 0.75f, 0.61f),
		FLinearColor(0.52f, 0.73f, 0.78f),
		FLinearColor(0.78f, 0.64f, 0.43f)
	};
}

AIslandTideglassDragonfly::AIslandTideglassDragonfly()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.06f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("TideglassDragonfly"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;
	UStaticMesh* SphereMesh = Sphere.Succeeded() ? Sphere.Object : nullptr;
	auto MakeBodyPart = [this, SphereMesh](const TCHAR* Name, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(SphereMesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		return Part;
	};
	Head = MakeBodyPart(TEXT("Head"), FVector(8.f, 0.f, 0.f), FVector(0.045f, 0.045f, 0.045f));
	Thorax = MakeBodyPart(TEXT("Thorax"), FVector(0.f, 0.f, 0.f), FVector(0.07f, 0.055f, 0.055f));
	Abdomen = MakeBodyPart(TEXT("Abdomen"), FVector(-11.f, 0.f, 0.f), FVector(0.19f, 0.025f, 0.027f));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const bool bLeft = Index % 2 == 0;
		UStaticMeshComponent* Wing = MakeBodyPart(
			*FString::Printf(TEXT("Wing_%d"), Index),
			FVector(Index < 2 ? 2.f : -4.f, bLeft ? -5.f : 5.f, 1.7f),
			FVector(Index < 2 ? 0.105f : 0.085f, 0.025f, 0.0045f));
		Wing->SetRelativeRotation(FRotator(0.f, bLeft ? -12.f : 12.f, 0.f));
		Wings.Add(Wing);
	}
}

void AIslandTideglassDragonfly::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureAppearance();
}

void AIslandTideglassDragonfly::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	Phase = FMath::FRandRange(0.f, 2.f * PI);
	MotionRate = FMath::FRandRange(0.8f, 1.2f);
	WingPhase = FMath::FRandRange(0.f, 2.f * PI);
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	ConfigureAppearance();
}

void AIslandTideglassDragonfly::SetColorVariant(int32 Variant)
{
	ColorVariant = ((Variant % UE_ARRAY_COUNT(DragonflyBodyColors)) + UE_ARRAY_COUNT(DragonflyBodyColors)) % UE_ARRAY_COUNT(DragonflyBodyColors);
	ConfigureAppearance();
}

void AIslandTideglassDragonfly::ConfigureAppearance()
{
	if (!BaseMaterial) return;
	if (!BodyMaterial) BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), DragonflyBodyColors[ColorVariant]);
		if (Head) Head->SetMaterial(0, BodyMaterial);
		if (Thorax) Thorax->SetMaterial(0, BodyMaterial);
		if (Abdomen) Abdomen->SetMaterial(0, BodyMaterial);
	}
	WingMaterials.SetNum(Wings.Num());
	for (int32 Index = 0; Index < Wings.Num(); ++Index)
	{
		if (!WingMaterials[Index]) WingMaterials[Index] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (!WingMaterials[Index]) continue;
		WingMaterials[Index]->SetVectorParameterValue(TEXT("Color"), DragonflyWingColors[ColorVariant]);
		Wings[Index]->SetMaterial(0, WingMaterials[Index]);
	}
}

FVector AIslandTideglassDragonfly::WindDisplacement(const FVector& LocalWind)
{
	return LocalWind.GetClampedToMaxSize(300.f) * 0.18f;
}

float AIslandTideglassDragonfly::RainMovementScale(float RainIntensity)
{
	const float RainActivity = FMath::SmoothStep(0.35f, 0.85f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	return FMath::Lerp(1.f, 0.3f, RainActivity);
}

void AIslandTideglassDragonfly::RespondToQuietObservation(const FVector& ObserverLocation)
{
	ScatterDirection = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (ScatterDirection.IsNearlyZero()) ScatterDirection = GetActorRightVector();
	ScatterRemaining = 1.6f;
}

FVector AIslandTideglassDragonfly::ResolveFlightPath(const FVector& Start, const FVector& Desired) const
{
	UWorld* World = GetWorld();
	if (!World || Start.Equals(Desired)) return Desired;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(5.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TideglassDragonflyFlight), false, this);
	FHitResult Hit;
	if (!World->SweepSingleByChannel(Hit, Start, Desired, FQuat::Identity, ECC_WorldStatic, Shape, Query)) return Desired;
	if (Hit.bStartPenetrating) return Start;
	const FVector Contact = Hit.Location + Hit.Normal * 2.f;
	const FVector Remaining = Desired - Contact;
	const FVector Slide = Remaining - Hit.Normal * FVector::DotProduct(Remaining, Hit.Normal);
	if (Slide.IsNearlyZero()) return Contact;
	FHitResult SlideHit;
	if (World->SweepSingleByChannel(SlideHit, Contact, Contact + Slide, FQuat::Identity, ECC_WorldStatic, Shape, Query))
		return SlideHit.bStartPenetrating ? Contact : SlideHit.Location;
	return Contact + Slide;
}

void AIslandTideglassDragonfly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()) return;
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	const float Time = GetWorld()->GetTimeSeconds();
	ScatterRemaining = FMath::Max(0.f, ScatterRemaining - SafeDelta);
	const float Rain = Weather.IsValid() ? Weather->SampleRainIntensity(Time) : 0.f;
	const float MoveScale = RainMovementScale(Rain);
	const float MotionTime = Time * MotionRate;
	const float ScatterAlpha = FMath::SmoothStep(0.f, 1.2f, ScatterRemaining);
	const FVector Offset(
		(190.f * FMath::Sin(MotionTime * 0.38f + Phase) + ScatterDirection.X * 270.f * ScatterAlpha) * MoveScale,
		(150.f * FMath::Sin(MotionTime * 0.29f + Phase * 1.7f) + ScatterDirection.Y * 270.f * ScatterAlpha) * MoveScale,
		110.f + 55.f * FMath::Sin(MotionTime * 0.9f + Phase * 1.2f) + (ScatterAlpha * 35.f));
	const FVector LocalWind = Weather.IsValid() ? Weather->GetLocalWind(GetActorLocation(), this) : FVector::ZeroVector;
	const FVector Desired = HomeLocation + Offset + WindDisplacement(LocalWind);
	const FVector Previous = GetActorLocation();
	const FVector Safe = ResolveFlightPath(Previous, Desired);
	SetActorLocation(Safe, false);
	const FVector Facing = (Safe - Previous).GetSafeNormal2D();
	if (!Facing.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Facing.Rotation(), SafeDelta, 2.5f));
	const float Beat = FMath::Sin(Time * 34.f * MoveScale + WingPhase) * 26.f;
	for (int32 Index = 0; Index < Wings.Num(); ++Index)
	{
		const float Side = (Index % 2 == 0) ? -1.f : 1.f;
		if (Wings[Index]) Wings[Index]->SetRelativeRotation(FRotator(Beat * (Index < 2 ? 0.5f : -0.5f), Side * (12.f + Index * 2.f), Side * Beat));
	}
}
