#include "IslandNest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AIslandNest::AIslandNest()
{
	Twigs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Twigs"));
	RootComponent = Twigs;
	Twigs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Twigs->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TwigMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (TwigMesh.Succeeded()) Twigs->SetStaticMesh(TwigMesh.Object);
	Tags.AddUnique(TEXT("IslandNest"));
	Tags.AddUnique(TEXT("AgentMade"));
}

int32 AIslandNest::GetVisibleTwigCount() const
{
	return Twigs->GetInstanceCount();
}

void AIslandNest::SetWoven(FName InSiteTag, int32 InLayers)
{
	SiteTag = InSiteTag;
	WovenLayers = FMath::Max(0, InLayers);
	if (!bSurfaceChosen)
	{
		bSurfaceChosen = true;
		// Reuse the Island's imported oak material when present; otherwise tint the engine shape material.
		if (UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Wood_Oak.M_Wood_Oak"), nullptr, LOAD_NoWarn | LOAD_Quiet))
			Twigs->SetMaterial(0, Wood);
		else if (UMaterialInstanceDynamic* Tint = Twigs->CreateAndSetMaterialInstanceDynamic(0))
			Tint->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.13f, 0.06f));
	}

	// Seeded by the site so a persisted nest keeps the same weave every session.
	FRandomStream Weave(static_cast<int32>(GetTypeHash(SiteTag.ToString())));
	Twigs->ClearInstances();
	for (int32 Layer = 0; Layer < WovenLayers; ++Layer)
	{
		// Lower layers form the cup; later layers widen and raise the rim.
		const float RingRadius = 11.f + Layer * 3.5f;
		const float Height = 1.5f + Layer * 3.f;
		for (int32 Index = 0; Index < TwigsPerLayer; ++Index)
		{
			const float Angle = 2.f * PI * (Index + Weave.FRandRange(-0.3f, 0.3f)) / TwigsPerLayer + Layer * 0.45f;
			const FVector Position(FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius, Height + Weave.FRandRange(-1.f, 1.f));
			// Alternate tangent and radial courses so successive rings visibly interlace instead of stacking as hoops.
			// Pitch 90 lays the cylinder's long axis flat; the seeded yaw variation keeps each course imperfect.
			const float WeaveDirection = Layer % 2 == 0 ? 0.f : 90.f;
			const FRotator Lie(90.f + Weave.FRandRange(-14.f, 14.f),
				FMath::RadiansToDegrees(Angle) + 90.f + WeaveDirection + Weave.FRandRange(-8.f, 8.f), 0.f);
			const FVector Scale(0.022f, 0.022f, Weave.FRandRange(0.26f, 0.38f));
			Twigs->AddInstance(FTransform(Lie, Position, Scale));
		}
	}
}
