#include "IslandArrangement.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "IslandDayNight.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
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
	MotifStones = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MotifStones"));
	MotifStones->SetupAttachment(Stones);
	MotifStones->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MotifStones->SetCanEverAffectNavigation(false);
	MotifStones->SetCastShadow(false);
	if (Sphere.Succeeded()) MotifStones->SetStaticMesh(Sphere.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterial.Succeeded()) MotifStones->SetMaterial(0, BasicMaterial.Object);
	ForageTwigs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ForageTwigs"));
	ForageTwigs->SetupAttachment(Stones);
	ForageTwigs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForageTwigs->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TwigMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (TwigMesh.Succeeded()) ForageTwigs->SetStaticMesh(TwigMesh.Object);
	Lichen = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Lichen"));
	Lichen->SetupAttachment(Stones);
	Lichen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lichen->SetCanEverAffectNavigation(false);
	Lichen->SetCastShadow(false);
	Lichen->SetVisibility(false);
	if (Sphere.Succeeded())
	{
		Lichen->SetStaticMesh(Sphere.Object);
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> Emissive(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
		if (Emissive.Succeeded()) Lichen->SetMaterial(0, Emissive.Object);
	}
	LichenLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LichenLight"));
	LichenLight->SetupAttachment(Stones);
	LichenLight->SetMobility(EComponentMobility::Movable);
	LichenLight->SetLightColor(FLinearColor(0.35f, 1.f, 0.75f));
	LichenLight->SetAttenuationRadius(260.f);
	LichenLight->SetCastShadows(false);
	LichenLight->SetIntensity(0.f);
	LichenLight->SetRelativeLocation(FVector(0.f, 0.f, 28.f));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.5f;
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

float AIslandArrangement::LichenGlow(int32 AgeDays)
{
	return FMath::SmoothStep(static_cast<float>(DaysToWeather), static_cast<float>(DaysToWeather + DaysToGlow), static_cast<float>(AgeDays));
}

int32 AIslandArrangement::GetVisibleLichenCount() const
{
	return Lichen ? Lichen->GetInstanceCount() : 0;
}

float AIslandArrangement::GetLichenLightIntensity() const
{
	return LichenLight ? LichenLight->Intensity : 0.f;
}

void AIslandArrangement::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLichen();
}

void AIslandArrangement::UpdateLichen()
{
	if (!DayNight.IsValid() && GetWorld())
		for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It) { DayNight = *It; break; }
	// Age keeps advancing with the clock during a long session; without a clock the shown day stands.
	const int32 Today = DayNight.IsValid() ? DayNight->DayNumber : ShownDay;
	const float Night = DayNight.IsValid() ? AIslandDayNight::NightAmount(DayNight->CurrentHour) : 0.f;
	const float Age = bHasWork ? LichenGlow(Today - WorkDay) : 0.f;
	LichenLevel = Age * Night;
	Lichen->SetVisibility(Age > 0.f);
	LichenLight->SetIntensity(LichenLightIntensity * LichenLevel);
	if (LichenSurface) LichenSurface->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.11f, 0.07f) * LichenLevel);
}

int32 AIslandArrangement::GetVisibleStoneCount() const
{
	return Stones->GetInstanceCount();
}

int32 AIslandArrangement::GetVisibleMotifCount() const
{
	return MotifStones ? MotifStones->GetInstanceCount() : 0;
}

int32 AIslandArrangement::GetVisibleForageTwigCount() const
{
	return ForageTwigs ? ForageTwigs->GetInstanceCount() : 0;
}

bool AIslandArrangement::HasForageableTwigs() const
{
	return bForageAvailable && ForageTwigs && ForageTwigs->GetInstanceCount() > 0;
}

bool AIslandArrangement::GatherForageableTwigs()
{
	if (!HasForageableTwigs()) return false;
	bForageAvailable = false;
	ForageTwigs->ClearInstances();
	return true;
}

void AIslandArrangement::ShowForageTwigs()
{
	if (!ForageTwigs || !bForageAvailable || ForageTwigs->GetInstanceCount() > 0) return;
	if (!ForageSurface)
	{
		if (UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Wood_Oak.M_Wood_Oak"), nullptr, LOAD_NoWarn | LOAD_Quiet))
			ForageTwigs->SetMaterial(0, Wood);
		else if (UMaterialInstanceDynamic* Tint = ForageTwigs->CreateAndSetMaterialInstanceDynamic(0))
		{
			Tint->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.15f, 0.08f));
			ForageSurface = Tint;
		}
	}
	FRandomStream Layout(static_cast<int32>(GetTypeHash(SiteId.ToString()) ^ 0x7a6f4d31u));
	for (int32 Index = 0; Index < 7; ++Index)
	{
		const FVector Location(Layout.FRandRange(-22.f, 22.f), Layout.FRandRange(-22.f, 22.f), Layout.FRandRange(1.f, 3.f));
		const FRotator Rotation(90.f + Layout.FRandRange(-12.f, 12.f), Layout.FRandRange(0.f, 360.f), 0.f);
		const float LengthScale = Layout.FRandRange(0.19f, 0.31f);
		ForageTwigs->AddInstance(FTransform(Rotation, Location, FVector(0.015f, 0.015f, LengthScale)));
	}
}

void AIslandArrangement::ShowSite(const FIslandArrangementSite& Site, int32 Today)
{
	SiteId = Site.Id;
	Tags.AddUnique(Site.Id);
	Stones->ClearInstances();
	MotifStones->ClearInstances();
	Lichen->ClearInstances();
	bHasWork = Site.bHasWork;
	WorkDay = Site.Day;
	ShownDay = Today;
	SetActorTickEnabled(false);
	LichenLevel = 0.f;
	LichenLight->SetIntensity(0.f);
	bForageAvailable = !Site.bHasWork && Site.ForageGatheredDay < Today;
	if (bForageAvailable) ShowForageTwigs();
	else ForageTwigs->ClearInstances();
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
	// A small pale three-stone mark is the visible motif carried through a creative lineage.
	// It is stable across sessions and deliberately separate from the main work's shape seed.
	if (!MotifSurface) MotifSurface = MotifStones->CreateAndSetMaterialInstanceDynamic(0);
	if (MotifSurface) MotifSurface->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.82f, 0.77f, 0.65f));
	FRandomStream Motif(Site.MotifSeed != 0 ? Site.MotifSeed : Site.Seed);
	const float MotifAngle = Motif.FRandRange(0.f, 2.f * PI);
	const FVector2D MotifAxis(FMath::Cos(MotifAngle), FMath::Sin(MotifAngle));
	const FVector2D MotifSide(-MotifAxis.Y, MotifAxis.X);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Along = (Index - 1) * 17.f;
		const float Across = (Index == 1 ? 0.f : (Index == 0 ? -1.f : 1.f)) * 5.f;
		const float Thickness = Motif.FRandRange(0.045f, 0.06f);
		const FVector2D Mark = MotifAxis * (58.f + Along) + MotifSide * Across;
		const float Size = Motif.FRandRange(0.08f, 0.105f);
		MotifStones->AddInstance(FTransform(FRotator(0.f, Motif.FRandRange(0.f, 360.f), 0.f),
			FVector(Mark.X, Mark.Y, Thickness * 35.f), FVector(Size, Size * Motif.FRandRange(0.72f, 0.9f), Thickness)));
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
	// One lichen patch per stone, set on its crown. It stays hidden until the work is old enough to glow.
	if (!LichenSurface) LichenSurface = Lichen->CreateAndSetMaterialInstanceDynamic(0);
	FRandomStream Growth(Site.Seed ^ 0x1c4e3a);
	for (int32 Index = 0; Index < Stones->GetInstanceCount(); ++Index)
	{
		FTransform Stone;
		Stones->GetInstanceTransform(Index, Stone, false);
		const FVector Scale = Stone.GetScale3D();
		const float Reach = 50.f * Scale.X * 0.35f;
		const FVector Crown = Stone.GetLocation() + FVector(Growth.FRandRange(-Reach, Reach), Growth.FRandRange(-Reach, Reach), 50.f * Scale.Z * 0.82f);
		const float Patch = Scale.X * Growth.FRandRange(0.28f, 0.42f);
		Lichen->AddInstance(FTransform(FRotator(0.f, Growth.FRandRange(0.f, 360.f), 0.f), Crown, FVector(Patch, Patch * 0.8f, 0.018f)));
	}
	SetActorTickEnabled(true);
	UpdateLichen();
}
