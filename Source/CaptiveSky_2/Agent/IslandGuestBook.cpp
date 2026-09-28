#include "IslandGuestBook.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIslandGuestBook::AIslandGuestBook()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	ShapeMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;

	auto MakePart = [this](const TCHAR* Name, const FVector& Position, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
		if (ShapeMaterial) Part->SetMaterial(0, ShapeMaterial);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetRelativeLocation(Position);
		Part->SetRelativeScale3D(Scale);
		return Part;
	};

	// Engine cubes are 100 cm wide. Two open pages sit on a leather cover and raised spine.
	Cover = MakePart(TEXT("LeatherCover"), FVector(0.f, 0.f, 0.f), FVector(0.44f, 0.33f, 0.025f));
	LeftPage = MakePart(TEXT("LeftPage"), FVector(-10.f, 0.f, 1.7f), FVector(0.20f, 0.29f, 0.009f));
	RightPage = MakePart(TEXT("RightPage"), FVector(10.f, 0.f, 1.7f), FVector(0.20f, 0.29f, 0.009f));
	Spine = MakePart(TEXT("RaisedSpine"), FVector(0.f, 0.f, 1.4f), FVector(0.025f, 0.31f, 0.018f));

	TitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TitleText"));
	TitleText->SetupAttachment(Root);
	TitleText->SetRelativeLocation(FVector(-19.f, -13.f, 2.3f));
	TitleText->SetRelativeRotation(FRotator(90.f, 0.f, -90.f));
	TitleText->SetHorizontalAlignment(EHTA_Left);
	TitleText->SetVerticalAlignment(EVRTA_TextTop);
	TitleText->SetWorldSize(1.8f);
	TitleText->SetTextRenderColor(FColor(104, 65, 20));
	TitleText->SetText(FText::FromString(TEXT("GUEST BOOK")));
	TitleText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TitleText->SetCastShadow(false);

	PageText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PageText"));
	PageText->SetupAttachment(Root);
	PageText->SetRelativeLocation(FVector(0.f, -13.f, 2.35f));
	PageText->SetRelativeRotation(FRotator(90.f, 0.f, -90.f));
	PageText->SetHorizontalAlignment(EHTA_Left);
	PageText->SetVerticalAlignment(EVRTA_TextTop);
	PageText->SetWorldSize(1.25f);
	PageText->SetTextRenderColor(FColor(48, 35, 25));
	PageText->SetText(FText::FromString(TEXT("The first page is blank.")));
	PageText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PageText->SetCastShadow(false);

	Tags.Add(TEXT("IslandInn"));
	Tags.Add(TEXT("GuestBookVisual"));
}

void AIslandGuestBook::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const TPair<UStaticMeshComponent*, FLinearColor> Tints[] = {
		{ Cover, FLinearColor(0.12f, 0.035f, 0.018f) },
		{ LeftPage, FLinearColor(0.78f, 0.68f, 0.51f) },
		{ RightPage, FLinearColor(0.82f, 0.72f, 0.56f) },
		{ Spine, FLinearColor(0.28f, 0.13f, 0.045f) }
	};
	for (const TPair<UStaticMeshComponent*, FLinearColor>& Part : Tints)
		if (Part.Key)
			if (UMaterialInstanceDynamic* Tint = Part.Key->CreateAndSetMaterialInstanceDynamic(0))
				Tint->SetVectorParameterValue(TEXT("Color"), Part.Value);
}

void AIslandGuestBook::SetDisplayText(const FString& Text)
{
	CurrentDisplayText = Text.IsEmpty() ? TEXT("The first page is blank.") : Text;
	if (PageText) PageText->SetText(FText::FromString(CurrentDisplayText));
}

FString AIslandGuestBook::GetDisplayText() const
{
	return CurrentDisplayText;
}

bool AIslandGuestBook::HasCollision() const
{
	return (Cover && Cover->GetCollisionEnabled() != ECollisionEnabled::NoCollision) ||
		(LeftPage && LeftPage->GetCollisionEnabled() != ECollisionEnabled::NoCollision) ||
		(RightPage && RightPage->GetCollisionEnabled() != ECollisionEnabled::NoCollision) ||
		(Spine && Spine->GetCollisionEnabled() != ECollisionEnabled::NoCollision) ||
		(TitleText && TitleText->GetCollisionEnabled() != ECollisionEnabled::NoCollision) ||
		(PageText && PageText->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
}
