#include "IslandCurio.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIslandCurio::AIslandCurio()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CairnRock(TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Stones = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stones"));
	Husks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Husks"));
	Seed = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Seed"));
	for (UStaticMeshComponent* Part : {static_cast<UStaticMeshComponent*>(Stones), static_cast<UStaticMeshComponent*>(Husks), Seed.Get()})
	{
		Part->SetupAttachment(RootComponent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		if (Sphere.Succeeded()) Part->SetStaticMesh(Sphere.Object);
	}
	if (CairnRock.Succeeded()) Stones->SetStaticMesh(CairnRock.Object);
	// Keep both the authored rock treatment and the tinted engine-shape fallback useful.
	// This is only a default component material; ShowRecord selects the textured material
	// when Starter Content is available, without modifying any project asset.
	if (BasicShapeMaterial.Succeeded()) Stones->SetMaterial(0, BasicShapeMaterial.Object);
	Seed->SetVisibility(false);
	Seed->SetCastShadow(false);
	SeedGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("SeedGlow"));
	SeedGlow->SetupAttachment(RootComponent);
	SeedGlow->SetMobility(EComponentMobility::Movable);
	SeedGlow->SetLightColor(FLinearColor(0.78f, 0.95f, 0.62f));
	SeedGlow->SetAttenuationRadius(160.f);
	SeedGlow->SetCastShadows(false);
	SeedGlow->SetIntensity(0.f);
	Tags.AddUnique(TEXT("IslandCurio"));
}

int32 AIslandCurio::GetVisibleStoneCount() const
{
	return Stones->GetInstanceCount();
}

bool AIslandCurio::IsGlowing() const
{
	return SeedGlow->Intensity > 0.f;
}

void AIslandCurio::ChooseSurfaces()
{
	if (bSurfaceChosen) return;
	bSurfaceChosen = true;
	// The engine shape material exposes a Color parameter; tint instead of importing new assets.
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* Surface = Part->CreateAndSetMaterialInstanceDynamic(0))
			Surface->SetVectorParameterValue(TEXT("Color"), Color);
	};
	const bool bHasAuthoredRock = Stones && Stones->GetStaticMesh() &&
		Stones->GetStaticMesh()->GetPathName() == TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock");
	UMaterialInterface* RockMaterial = bHasAuthoredRock
		? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Props/Materials/M_Rock.M_Rock"), nullptr, LOAD_NoWarn | LOAD_Quiet)
		: nullptr;
	if (RockMaterial)
		Stones->SetMaterial(0, RockMaterial);
	else
		Tint(Stones, Shown.Kind == EIslandCurioKind::PaleStone ? FLinearColor(0.78f, 0.77f, 0.72f) : FLinearColor(0.34f, 0.33f, 0.31f));
	Tint(Husks, FLinearColor(0.18f, 0.22f, 0.10f));
	Tint(Seed, FLinearColor(0.85f, 0.96f, 0.70f));
}

void AIslandCurio::ShowRecord(const FIslandCurioRecord& Record)
{
	Shown = Record;
	ChooseSurfaces();
	Stones->ClearInstances();
	Husks->ClearInstances();
	Seed->SetVisibility(false);
	SeedGlow->SetIntensity(0.f);
	const FVector Ground(0.f, 0.f, -GroundClearance);
	FRandomStream Shape(static_cast<int32>(GetTypeHash(Record.Id.ToString())));

	switch (Record.Kind)
	{
	case EIslandCurioKind::PaleStone:
		// Mostly sunk into the ground: easy to walk past, noticeable up close.
		Stones->AddInstance(FTransform(FRotator(0.f, Shape.FRandRange(0.f, 360.f), 0.f), Ground + FVector(0.f, 0.f, 2.f), FVector(0.24f, 0.17f, 0.07f)));
		break;
	case EIslandCurioKind::Cairn:
	{
		float Height = 0.f;
		for (int32 Index = 0; Index < FMath::Clamp(Record.State, 0, CairnMaxStones); ++Index)
		{
			const float Width = FMath::Lerp(0.42f, 0.16f, Index / static_cast<float>(CairnMaxStones)) * Shape.FRandRange(0.9f, 1.1f);
			const float Thickness = Shape.FRandRange(0.07f, 0.1f);
			const FVector Offset(Shape.FRandRange(-2.5f, 2.5f), Shape.FRandRange(-2.5f, 2.5f), Height + Thickness * 50.f);
			Stones->AddInstance(FTransform(FRotator(Shape.FRandRange(-4.f, 4.f), Shape.FRandRange(0.f, 360.f), 0.f), Ground + Offset, FVector(Width, Width * 0.85f, Thickness)));
			Height += Thickness * 100.f * 0.85f;
		}
		break;
	}
	case EIslandCurioKind::SeedPod:
	{
		const int32 Opened = FMath::Clamp(Record.State, 0, PodOpenState);
		constexpr int32 HuskCount = 6;
		for (int32 Index = 0; Index < HuskCount; ++Index)
		{
			// Leaves peel outward in pairs, one pair per day-visit.
			const float Open = Index < Opened * 2 ? 1.f : 0.f;
			const float Yaw = 360.f * Index / HuskCount;
			const FRotator Leaf(0.f, Yaw, 0.f);
			const FVector Out = Leaf.RotateVector(FVector(1.f, 0.f, 0.f));
			const FVector Position = Ground + FVector(0.f, 0.f, 22.f) + Out * FMath::Lerp(5.f, 13.f, Open);
			const FRotator Tilt = FRotator(FMath::Lerp(-8.f, -62.f, Open), Yaw, 0.f);
			Husks->AddInstance(FTransform(Tilt, Position, FVector(0.09f, 0.14f, 0.26f)));
		}
		if (Opened >= PodOpenState)
		{
			Seed->SetRelativeLocation(Ground + FVector(0.f, 0.f, 16.f));
			Seed->SetRelativeScale3D(FVector(0.07f));
			Seed->SetVisibility(true);
			SeedGlow->SetRelativeLocation(Ground + FVector(0.f, 0.f, 26.f));
			SeedGlow->SetIntensity(6.f);
		}
		break;
	}
	}
}
