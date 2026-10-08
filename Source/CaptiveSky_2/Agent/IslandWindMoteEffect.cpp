#include "IslandWindMoteEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor WindMoteGlowColor(0.38f, 0.72f, 1.15f);
	constexpr float WindMoteBaseIntensity = 280.f;
	constexpr float WindMoteLightRadius = 600.f;
}

AIslandWindMoteEffect::AIslandWindMoteEffect()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	Tags.AddUnique(TEXT("IslandTransientEffect"));
	Tags.AddUnique(TEXT("WindArchGustMotes"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MoteSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MoteGlowMaterial(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FName MeshName(*FString::Printf(TEXT("WindMote_%02d"), Index));
		const FName LightName(*FString::Printf(TEXT("WindMoteLight_%02d"), Index));
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(MeshName);
		Mesh->SetupAttachment(RootComponent);
		if (MoteSphere.Succeeded()) Mesh->SetStaticMesh(MoteSphere.Object);
		const float Scale = 0.14f + 0.03f * Index;
		Mesh->SetRelativeScale3D(FVector(Scale, Scale, Scale));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		UMaterialInstanceDynamic* Glow = MoteGlowMaterial.Succeeded()
			? UMaterialInstanceDynamic::Create(MoteGlowMaterial.Object, this, FName(*FString::Printf(TEXT("WindMoteGlow_%02d"), Index)))
			: nullptr;
		if (Glow)
		{
			Glow->SetVectorParameterValue(TEXT("Color"), WindMoteGlowColor);
			Mesh->SetMaterial(0, Glow);
		}
		MoteMaterials.Add(Glow);
		MoteMeshes.Add(Mesh);

		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(LightName);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetLightColor(FLinearColor(0.58f, 0.82f, 1.f));
		Light->SetAttenuationRadius(WindMoteLightRadius);
		Light->SetCastShadows(false);
		Light->SetIntensity(0.f);
		MoteLights.Add(Light);
	}
}

void AIslandWindMoteEffect::InitializeGust(const FVector& Direction, float Radius, float InDurationSeconds)
{
	if (!Direction.IsNearlyZero()) FlowDirection = Direction.GetSafeNormal();
	FlowRadius = FMath::Clamp(Radius, 100.f, 3000.f);
	DurationSeconds = FMath::Clamp(InDurationSeconds, 1.f, 18.f);
	UpdateMotes(0.f);
}

bool AIslandWindMoteEffect::FindNearestVisibleMote(const FVector& Origin, float MaxDistance, FVector& OutLocation) const
{
	if (MaxDistance <= 0.f) return false;

	float NearestDistanceSquared = FMath::Square(MaxDistance);
	bool bFound = false;
	for (const UStaticMeshComponent* Mote : MoteMeshes)
	{
		if (!IsValid(Mote) || !Mote->IsVisible() || Mote->GetRelativeScale3D().GetMax() <= 0.04f)
			continue;
		const FVector Location = Mote->GetComponentLocation();
		const float DistanceSquared = FVector::DistSquared(Origin, Location);
		if (DistanceSquared > NearestDistanceSquared) continue;
		NearestDistanceSquared = DistanceSquared;
		OutLocation = Location;
		bFound = true;
	}
	return bFound;
}

void AIslandWindMoteEffect::BeginPlay()
{
	Super::BeginPlay();
	UpdateMotes(0.f);
}

void AIslandWindMoteEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedSeconds += FMath::Max(0.f, DeltaSeconds);
	const float Alpha = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.f, 1.f);
	UpdateMotes(Alpha);
	if (Alpha >= 1.f) Destroy();
}

void AIslandWindMoteEffect::UpdateMotes(float Alpha)
{
	const FVector Side = FVector::CrossProduct(FlowDirection, FVector::UpVector).GetSafeNormal(SMALL_NUMBER, FVector::RightVector);
	const FVector Up = FVector::CrossProduct(Side, FlowDirection).GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	const float RemainingLight = 1.f - Alpha;
	for (int32 Index = 0; Index < MoteMeshes.Num(); ++Index)
	{
		const float Phase = (static_cast<float>(Index) / MoteMeshes.Num()) + Alpha;
		const float Along = (2.f * FMath::Fmod(Phase, 1.f) - 1.f) * FlowRadius;
		const float Flutter = static_cast<float>(ElapsedSeconds) * 2.4f + Index * 2.1f;
		const FVector Offset = FlowDirection * Along + Side * (FMath::Sin(Flutter) * 35.f) + Up * (FMath::Cos(Flutter * 0.77f) * 28.f);
		const float Pulse = 0.82f + 0.18f * FMath::Square(FMath::Sin(Flutter * 1.3f));
		const float FadeScale = FMath::Sqrt(RemainingLight);
		if (MoteMeshes[Index])
		{
			const float BaseScale = 0.14f + 0.03f * Index;
			MoteMeshes[Index]->SetRelativeLocation(Offset);
			MoteMeshes[Index]->SetRelativeScale3D(FVector(BaseScale * Pulse * FadeScale));
		}
		if (MoteMaterials.IsValidIndex(Index) && MoteMaterials[Index])
			MoteMaterials[Index]->SetVectorParameterValue(TEXT("Color"), WindMoteGlowColor * (RemainingLight * Pulse));
		if (MoteLights.IsValidIndex(Index) && MoteLights[Index])
		{
			MoteLights[Index]->SetRelativeLocation(Offset);
			MoteLights[Index]->SetIntensity(WindMoteBaseIntensity * RemainingLight * Pulse);
		}
	}
}
