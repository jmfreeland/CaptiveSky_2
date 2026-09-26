#include "IslandWindMoteEffect.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AIslandWindMoteEffect::AIslandWindMoteEffect()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	Tags.AddUnique(TEXT("IslandTransientEffect"));
	Tags.AddUnique(TEXT("WindArchGustMotes"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MoteSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FName MeshName(*FString::Printf(TEXT("WindMote_%02d"), Index));
		const FName LightName(*FString::Printf(TEXT("WindMoteLight_%02d"), Index));
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(MeshName);
		Mesh->SetupAttachment(RootComponent);
		if (MoteSphere.Succeeded()) Mesh->SetStaticMesh(MoteSphere.Object);
		const float Scale = 0.035f + 0.008f * Index;
		Mesh->SetRelativeScale3D(FVector(Scale, Scale, Scale));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		MoteMeshes.Add(Mesh);

		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(LightName);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetLightColor(FLinearColor(0.58f, 0.82f, 1.f));
		Light->SetAttenuationRadius(140.f);
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
		if (MoteMeshes[Index]) MoteMeshes[Index]->SetRelativeLocation(Offset);
		if (MoteLights.IsValidIndex(Index) && MoteLights[Index])
		{
			MoteLights[Index]->SetRelativeLocation(Offset);
			MoteLights[Index]->SetIntensity(70.f * RemainingLight * (0.55f + 0.45f * FMath::Sin(Flutter * 1.3f) * FMath::Sin(Flutter * 1.3f)));
		}
	}
}
