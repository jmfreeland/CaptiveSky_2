#include "IslandArrangement.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIslandArrangement::AIslandArrangement()
{
	Stones = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stones"));
	RootComponent = Stones;
	Stones->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stones->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) Stones->SetStaticMesh(Sphere.Object);
	Tags.AddUnique(TEXT("IslandArrangement"));
	Tags.AddUnique(TEXT("AgentMade"));
}

int32 AIslandArrangement::StoneCountFor(EIslandArrangementForm Form)
{
	switch (Form)
	{
	case EIslandArrangementForm::Line: return 7;
	case EIslandArrangementForm::Spiral: return 11;
	case EIslandArrangementForm::Pair: return 8;
	case EIslandArrangementForm::Ring:
	default: return 9;
	}
}

FLinearColor AIslandArrangement::WeatheredTint(int32 AgeDays)
{
	const float Weathering = FMath::Clamp(AgeDays / static_cast<float>(DaysToWeather), 0.f, 1.f);
	return FMath::Lerp(FLinearColor(0.74f, 0.72f, 0.67f), FLinearColor(0.27f, 0.33f, 0.2f), Weathering);
}

int32 AIslandArrangement::GetVisibleStoneCount() const
{
	return Stones->GetInstanceCount();
}

void AIslandArrangement::ShowSite(const FIslandArrangementSite& Site, int32 Today)
{
	SiteId = Site.Id;
	Tags.AddUnique(Site.Id);
	Stones->ClearInstances();
	if (!Site.bHasWork) return;
	if (!Surface) Surface = Stones->CreateAndSetMaterialInstanceDynamic(0);
	CurrentTint = WeatheredTint(Today - Site.Day);
	if (Surface) Surface->SetVectorParameterValue(TEXT("Color"), CurrentTint);

	FRandomStream Shape(Site.Seed);
	auto Place = [this, &Shape](const FVector2D& Where, float Size)
	{
		const float Thickness = Shape.FRandRange(0.06f, 0.09f);
		Stones->AddInstance(FTransform(FRotator(Shape.FRandRange(-5.f, 5.f), Shape.FRandRange(0.f, 360.f), 0.f),
			FVector(Where.X, Where.Y, Thickness * 35.f), FVector(Size, Size * Shape.FRandRange(0.7f, 0.95f), Thickness)));
	};
	const int32 Count = StoneCountFor(Site.Form);
	const float Facing = Shape.FRandRange(0.f, 2.f * PI);
	switch (Site.Form)
	{
	case EIslandArrangementForm::Ring:
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = Facing + 2.f * PI * Index / Count;
			Place(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 65.f, Shape.FRandRange(0.16f, 0.22f));
		}
		break;
	case EIslandArrangementForm::Line:
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// Graduated: the stones shrink along the line.
			const float Along = (Index - (Count - 1) * 0.5f) * 26.f;
			Place(FVector2D(FMath::Cos(Facing), FMath::Sin(Facing)) * Along, FMath::Lerp(0.24f, 0.12f, Index / static_cast<float>(Count - 1)));
		}
		break;
	case EIslandArrangementForm::Spiral:
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = Facing + Index * 0.75f;
			Place(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (10.f + Index * 7.5f), FMath::Lerp(0.11f, 0.2f, Index / static_cast<float>(Count - 1)));
		}
		break;
	case EIslandArrangementForm::Pair:
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// Two low stacks facing each other.
			const float Side = Index < Count / 2 ? 1.f : -1.f;
			const int32 Level = Index % (Count / 2);
			const float Size = FMath::Lerp(0.26f, 0.14f, Level / static_cast<float>(Count / 2));
			const float Thickness = Shape.FRandRange(0.07f, 0.09f);
			Stones->AddInstance(FTransform(FRotator(0.f, Shape.FRandRange(0.f, 360.f), 0.f),
				FVector(FMath::Cos(Facing) * 45.f * Side, FMath::Sin(Facing) * 45.f * Side, Thickness * 50.f + Level * 7.f), FVector(Size, Size * 0.85f, Thickness)));
		}
		break;
	}
	// Each response is a small arc of stones set just outside the original work.
	for (int32 Response = 0; Response < FMath::Min(Site.Responses.Num(), MaxResponses); ++Response)
	{
		FRandomStream Answer(Site.Seed + 101 * (Response + 1));
		const float Centre = Facing + PI * 0.6f + Response * 2.f * PI / MaxResponses;
		for (int32 Index = 0; Index < StonesPerResponse; ++Index)
		{
			const float Angle = Centre + (Index - 1) * 0.22f;
			Place(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Answer.FRandRange(105.f, 120.f), Answer.FRandRange(0.1f, 0.14f));
		}
	}
}
