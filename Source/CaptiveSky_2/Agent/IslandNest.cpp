#include "IslandNest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void AddTwigWithTaperedTips(UInstancedStaticMeshComponent* TwigBodies, UInstancedStaticMeshComponent* TwigTips,
		const FTransform& TwigTransform)
	{
		if (!TwigBodies) return;
		TwigBodies->AddInstance(TwigTransform, false);
		if (!TwigTips || !TwigTips->GetStaticMesh()) return;

		const FVector Scale = TwigTransform.GetScale3D();
		const FVector Axis = TwigTransform.TransformVector(FVector::UpVector).GetSafeNormal();
		const float HalfTwigLength = 50.f * FMath::Abs(Scale.Z);
		const float TipOffset = HalfTwigLength * 0.84f;
		const FVector TipScale(Scale.X * 0.70f, Scale.Y * 0.70f, Scale.Z * 0.46f);
		const FQuat TwigRotation = TwigTransform.GetRotation();
		const FQuat TurnTipOutward(FVector::ForwardVector, PI);

		// Engine cone primitives taper to one point. Keep most of each cone inside the
		// body, but expose a short narrow end that remains legible beyond the nest close-up.
		TwigTips->AddInstance(FTransform(TwigRotation, TwigTransform.GetLocation() + Axis * TipOffset, TipScale), false);
		TwigTips->AddInstance(FTransform(TwigRotation * TurnTipOutward,
			TwigTransform.GetLocation() - Axis * TipOffset, TipScale), false);
	}
}

AIslandNest::AIslandNest()
{
	Twigs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Twigs"));
	RootComponent = Twigs;
	Twigs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Twigs->SetCanEverAffectNavigation(false);
	Twigs->SetCastShadow(false);
	TwigTips = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TwigTips"));
	TwigTips->SetupAttachment(Twigs);
	TwigTips->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TwigTips->SetCanEverAffectNavigation(false);
	TwigTips->SetCastShadow(false);
	FallenTwigs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FallenTwigs"));
	FallenTwigs->SetupAttachment(Twigs);
	FallenTwigs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FallenTwigs->SetCanEverAffectNavigation(false);
	FallenTwigs->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TwigMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TwigTipMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (TwigMesh.Succeeded())
	{
		Twigs->SetStaticMesh(TwigMesh.Object);
		FallenTwigs->SetStaticMesh(TwigMesh.Object);
	}
	if (TwigTipMesh.Succeeded()) TwigTips->SetStaticMesh(TwigTipMesh.Object);
	Tags.AddUnique(TEXT("IslandNest"));
	Tags.AddUnique(TEXT("AgentMade"));
}

int32 AIslandNest::GetVisibleTwigCount() const
{
	return Twigs->GetInstanceCount();
}

int32 AIslandNest::GetVisibleTwigTipCount() const
{
	return TwigTips ? TwigTips->GetInstanceCount() : 0;
}

int32 AIslandNest::GetVisibleFallenTwigCount() const
{
	return FallenTwigs->GetInstanceCount();
}

void AIslandNest::SetWoven(FName InSiteTag, int32 InLayers, bool bShowStormDebris)
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
	if (TwigTips) TwigTips->SetMaterial(0, Twigs->GetMaterial(0));
	FallenTwigs->SetMaterial(0, Twigs->GetMaterial(0));

	// Seeded by the site so a persisted nest keeps the same weave every session.
	FRandomStream Weave(static_cast<int32>(GetTypeHash(SiteTag.ToString())));
	Twigs->ClearInstances();
	if (TwigTips) TwigTips->ClearInstances();
	FallenTwigs->ClearInstances();
	if (WovenLayers > 0)
	{
		// Cross a small floor beneath the first hoop so a nest reads as a cup, not an open ring stack.
		// These are still visual-only twigs; the nest does not gain collision or shelter semantics.
		for (int32 Index = 0; Index < FoundationTwigCount; ++Index)
		{
			const bool bAlongX = Index < 3;
			const float CourseOffset = bAlongX
				? (static_cast<float>(Index) - 1.f) * 5.f
				: (Index == 3 ? -2.5f : 2.5f);
			const FVector Position(
				bAlongX ? Weave.FRandRange(-1.f, 1.f) : CourseOffset,
				bAlongX ? CourseOffset : Weave.FRandRange(-1.f, 1.f),
				1.5f + Weave.FRandRange(-0.25f, 0.25f));
			const float AxisYaw = bAlongX ? 0.f : 90.f;
			const FRotator Lie(90.f + Weave.FRandRange(-4.f, 4.f), AxisYaw + Weave.FRandRange(-3.f, 3.f), 0.f);
			const float TwigLength = Weave.FRandRange(0.31f, 0.35f);
			AddTwigWithTaperedTips(Twigs, TwigTips, FTransform(Lie, Position, FVector(0.022f, 0.022f, TwigLength)));
		}
	}
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
			AddTwigWithTaperedTips(Twigs, TwigTips, FTransform(Lie, Position, Scale));
		}
	}
	if (bShowStormDebris)
	{
		FRandomStream Debris(static_cast<int32>(GetTypeHash(SiteTag.ToString()) ^ 0x4F1BBD5Du));
		for (int32 Index = 0; Index < StormDebrisTwigCount; ++Index)
		{
			const float Angle = Debris.FRandRange(0.f, 2.f * PI);
			const float Radius = Debris.FRandRange(20.f, 42.f);
			const FVector Position(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Debris.FRandRange(1.f, 3.f));
			const FRotator Lie(90.f + Debris.FRandRange(-18.f, 18.f),
				FMath::RadiansToDegrees(Angle) + Debris.FRandRange(-65.f, 65.f), Debris.FRandRange(-12.f, 12.f));
			AddTwigWithTaperedTips(FallenTwigs, TwigTips,
				FTransform(Lie, Position, FVector(0.022f, 0.022f, Debris.FRandRange(0.26f, 0.38f))));
		}
	}
}
