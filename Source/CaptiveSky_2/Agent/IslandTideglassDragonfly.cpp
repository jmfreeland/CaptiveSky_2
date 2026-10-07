#include "IslandTideglassDragonfly.h"

#include "IslandPoolRippleEffect.h"
#include "IslandWeather.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void BuildWingSurface(UProceduralMeshComponent* Wing, float Side, float Sweep, float Length, float MaximumHalfWidth)
	{
		if (!Wing) return;

		constexpr int32 LengthSegments = 10;
		constexpr int32 WidthSegments = 4;
		constexpr int32 SurfaceVertexCount = (LengthSegments + 1) * (WidthSegments + 1);
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;
		Vertices.Reserve(SurfaceVertexCount * 2);
		Normals.Reserve(SurfaceVertexCount * 2);
		UVs.Reserve(SurfaceVertexCount * 2);
		Colors.Reserve(SurfaceVertexCount * 2);
		Tangents.Reserve(SurfaceVertexCount * 2);
		Triangles.Reserve(LengthSegments * WidthSegments * 12);

		for (int32 Surface = 0; Surface < 2; ++Surface)
		{
			for (int32 LengthIndex = 0; LengthIndex <= LengthSegments; ++LengthIndex)
			{
				const float Along = static_cast<float>(LengthIndex) / LengthSegments;
				const float Arch = FMath::Max(0.f, FMath::Sin(Along * PI));
				const float CenterX = Sweep * Arch + 1.5f * Along;
				const float HalfWidth = MaximumHalfWidth * FMath::Pow(Arch, 0.72f);
				for (int32 WidthIndex = 0; WidthIndex <= WidthSegments; ++WidthIndex)
				{
					const float Across = -1.f + 2.f * WidthIndex / WidthSegments;
					Vertices.Emplace(CenterX + Across * HalfWidth, Side * Along * Length,
						Surface == 0 ? 0.35f * Arch : -0.35f * Arch);
					Normals.Emplace(0.f, 0.f, Surface == 0 ? 1.f : -1.f);
					UVs.Emplace(Along, (Across + 1.f) * 0.5f);
					Colors.Emplace(FLinearColor::White);
					Tangents.Emplace(FVector(1.f, 0.f, 0.f), false);
				}
			}

			const int32 SurfaceOffset = Surface * SurfaceVertexCount;
			for (int32 LengthIndex = 0; LengthIndex < LengthSegments; ++LengthIndex)
			{
				for (int32 WidthIndex = 0; WidthIndex < WidthSegments; ++WidthIndex)
				{
					const int32 A = SurfaceOffset + LengthIndex * (WidthSegments + 1) + WidthIndex;
					const int32 B = A + WidthSegments + 1;
					const int32 C = B + 1;
					const int32 D = A + 1;
					const bool bFaceUp = (Side > 0.f) == (Surface == 0);
					if (bFaceUp)
					{
						Triangles.Add(A);
						Triangles.Add(C);
						Triangles.Add(B);
						Triangles.Add(A);
						Triangles.Add(D);
						Triangles.Add(C);
					}
					else
					{
						Triangles.Add(A);
						Triangles.Add(B);
						Triangles.Add(C);
						Triangles.Add(A);
						Triangles.Add(C);
						Triangles.Add(D);
					}
				}
			}
		}

		Wing->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
	}

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
		UProceduralMeshComponent* Wing = CreateDefaultSubobject<UProceduralMeshComponent>(
			FName(*FString::Printf(TEXT("Wing_%d"), Index)));
		Wing->SetupAttachment(RootComponent);
		Wing->SetRelativeLocation(FVector(Index < 2 ? 2.f : -4.f, bLeft ? -5.f : 5.f, 1.7f));
		Wing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wing->SetCanEverAffectNavigation(false);
		Wing->SetCastShadow(false);
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
		if (Wings[Index] && Wings[Index]->GetNumSections() == 0)
		{
			const bool bForewing = Index < 2;
			BuildWingSurface(Wings[Index], Index % 2 == 0 ? -1.f : 1.f,
				bForewing ? 1.7f : -1.7f,
				bForewing ? 33.f : 39.f,
				bForewing ? 4.4f : 6.8f);
		}
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

void AIslandTideglassDragonfly::CheckForLowRavenFlyby()
{
	if (!GetWorld() || ScatterRemaining > 0.f || RavenFlybyCooldownRemaining > 0.f) return;

	constexpr float FlybyRadius = 425.f;
	constexpr float MinimumHeight = 120.f;
	constexpr float MaximumHeight = 480.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		if (It->LocomotionState != ERavenLocomotionState::Flying) continue;
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		if (Offset.Z < MinimumHeight || Offset.Z > MaximumHeight || Offset.SizeSquared2D() > FMath::Square(FlybyRadius))
			continue;

		// A close, low wing shadow sends the dragonfly briefly sideways; it resumes its
		// independent pool-side patrol without pursuing, attacking, or persisting a change.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 8.f;
		return;
	}
}

bool AIslandTideglassDragonfly::RespondToSurfaceRipple(const FVector& RippleLocation)
{
	if (RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f ||
		FVector::DistSquared2D(GetActorLocation(), RippleLocation) > FMath::Square(250.f) ||
		FMath::Abs(GetActorLocation().Z - RippleLocation.Z) > 300.f)
		return false;

	// Give each color morph its own small station around the disturbance so the
	// whole daytime group does not converge on one point. This remains a brief
	// hover above the water, not a landing or a persistent change.
	constexpr float HoverRadius = 42.f;
	const float HoverAngle = 2.f * PI * static_cast<float>(ColorVariant) / 3.f;
	RippleInterestLocation = RippleLocation + FVector(
		FMath::Cos(HoverAngle) * HoverRadius,
		FMath::Sin(HoverAngle) * HoverRadius,
		110.f);
	RippleInterestRemaining = 1.8f;
	RippleInterestCooldownRemaining = 7.f;
	return true;
}

void AIslandTideglassDragonfly::CheckForNearbyNaturalSurfaceRipple()
{
	if (!GetWorld() || RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f) return;

	for (TActorIterator<AIslandPoolRippleEffect> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("WindImpact")) && !It->ActorHasTag(TEXT("RainImpact"))) continue;
		if (RespondToSurfaceRipple(It->GetActorLocation())) return;
	}
}

float AIslandTideglassDragonfly::GetRippleInterestAlpha() const
{
	if (RippleInterestRemaining <= 0.f) return 0.f;
	const float Elapsed = 1.8f - RippleInterestRemaining;
	return FMath::SmoothStep(0.f, 0.45f, Elapsed) * (1.f - FMath::SmoothStep(1.2f, 1.8f, Elapsed));
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
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - SafeDelta);
	RippleInterestRemaining = FMath::Max(0.f, RippleInterestRemaining - SafeDelta);
	RippleInterestCooldownRemaining = FMath::Max(0.f, RippleInterestCooldownRemaining - SafeDelta);
	RavenCheckRemaining -= SafeDelta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.35f;
		CheckForLowRavenFlyby();
	}
	RippleCheckRemaining -= SafeDelta;
	if (RippleCheckRemaining <= 0.f)
	{
		RippleCheckRemaining = 0.35f;
		CheckForNearbyNaturalSurfaceRipple();
	}
	const float Rain = Weather.IsValid() ? Weather->SampleRainIntensity(Time) : 0.f;
	const float MoveScale = RainMovementScale(Rain);
	const float MotionTime = Time * MotionRate;
	const float ScatterAlpha = FMath::SmoothStep(0.f, 1.2f, ScatterRemaining);
	const FVector Offset(
		(190.f * FMath::Sin(MotionTime * 0.38f + Phase) + ScatterDirection.X * 270.f * ScatterAlpha) * MoveScale,
		(150.f * FMath::Sin(MotionTime * 0.29f + Phase * 1.7f) + ScatterDirection.Y * 270.f * ScatterAlpha) * MoveScale,
		110.f + 55.f * FMath::Sin(MotionTime * 0.9f + Phase * 1.2f) + (ScatterAlpha * 35.f));
	const FVector LocalWind = Weather.IsValid() ? Weather->GetLocalWind(GetActorLocation(), this) : FVector::ZeroVector;
	const FVector PatrolLocation = HomeLocation + Offset + WindDisplacement(LocalWind);
	const FVector RippleInterestLocationWithOrbit = RippleInterestLocation + FVector(0.f, 0.f, 12.f) + Offset * 0.12f;
	const FVector Desired = FMath::Lerp(PatrolLocation, RippleInterestLocationWithOrbit, GetRippleInterestAlpha());
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
