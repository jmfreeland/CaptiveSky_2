#include "IslandInnHearthSubsystem.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

bool UIslandInnHearthSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandInnHearthSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnHearth")))
		{
			HearthAnchor = *It;
			InitializeFlames(*It);
			break;
		}
	}
	for (TActorIterator<APointLight> It(&InWorld); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("InnHearthLight"))) continue;
		HearthLight = It->PointLightComponent;
		if (HearthLight.IsValid())
		{
			BaseIntensity = FMath::Max(0.f, HearthLight->Intensity);
			SetBanked();
			return;
		}
	}
}

bool UIslandInnHearthSubsystem::TendHearth(FString& OutFact)
{
	OutFact.Reset();
	if (!HearthLight.IsValid())
	{
		OutFact = TEXT("You reached the inn hearth, but its tagged light is not present. Nothing changed.");
		return false;
	}

	if (bLit)
	{
		SetBanked();
		OutFact = TEXT("You banked the hearth. Its light is out; it will stay that way until someone tends it again. No permanent change was made.");
		return true;
	}

	bLit = true;
	SecondsRemaining = BurnDurationSeconds;
	FlickerTime = 0.f;
	ApplyFlicker();
	OutFact = FlameMeshes.Num() == 3
		? TEXT("You kindled three small, stylized flames and the hearth's warm light. They flicker for about five minutes of play (roughly three Island hours at the current clock rate), then bank themselves unless someone tends the hearth again. Nearby, the fire gives a small simulated warmth cue; it does not model body temperature, room temperature, shelter, or a lasting change.")
		: TEXT("You kindled the hearth's warm light. It will flicker for about five minutes of play (roughly three Island hours at the current clock rate), then bank itself unless someone tends it again. The visible flame could not be created; no lasting hearth change was saved.");
	return true;
}

FString UIslandInnHearthSubsystem::DescribeHearth() const
{
	if (bLit && HearthLight.IsValid())
	{
		return FString::Printf(TEXT(" The inn hearth is lit%s, with about %.0f seconds of play left before it banks itself. You may move_to/interact with target InnHearth to bank it early, or leave it be."), FlameMeshes.Num() == 3 ? TEXT(" with three small stylized flames") : TEXT(" but its flame is not visible"), SecondsRemaining);
	}
	return TEXT(" The inn hearth is dark and banked. You may move_to/interact with target InnHearth if you choose to kindle its light; no one has to tend it.");
}

float UIslandInnHearthSubsystem::GetWarmthFactorAt(const FVector& Location) const
{
	if (!bLit || !HearthAnchor.IsValid()) return 0.f;
	const float Distance = FVector::Dist(Location, HearthAnchor->GetActorLocation());
	if (Distance >= WarmthCueRadius) return 0.f;
	return 1.f - FMath::SmoothStep(0.f, WarmthCueRadius, Distance);
}

FString UIslandInnHearthSubsystem::DescribeWarmthAt(const FVector& Location) const
{
	return GetWarmthFactorAt(Location) > 0.f
		? TEXT(" A small simulated warmth cue reaches this spot from the hearth; it is proximity-only and does not measure body or room temperature, dryness, or shelter.")
		: FString();
}

void UIslandInnHearthSubsystem::Tick(float DeltaTime)
{
	if (!bLit || !HearthLight.IsValid())
	{
		if (bLit) SetBanked();
		return;
	}
	SecondsRemaining = FMath::Max(0.f, SecondsRemaining - FMath::Max(0.f, DeltaTime));
	if (SecondsRemaining <= 0.f)
	{
		SetBanked();
		return;
	}
	FlickerTime += FMath::Max(0.f, DeltaTime);
	ApplyFlicker();
}

TStatId UIslandInnHearthSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandInnHearthSubsystem, STATGROUP_Tickables);
}

bool UIslandInnHearthSubsystem::IsTickable() const
{
	return bLit;
}

void UIslandInnHearthSubsystem::Deinitialize()
{
	SetBanked();
	HearthLight.Reset();
	HearthAnchor.Reset();
	FlameMeshes.Reset();
	FlameBaseTransforms.Reset();
	Super::Deinitialize();
}

void UIslandInnHearthSubsystem::SetBanked()
{
	bLit = false;
	SecondsRemaining = 0.f;
	FlickerTime = 0.f;
	if (HearthLight.IsValid()) HearthLight->SetIntensity(0.f);
	for (const TWeakObjectPtr<UStaticMeshComponent>& Flame : FlameMeshes)
		if (Flame.IsValid()) Flame->SetVisibility(false);
}

void UIslandInnHearthSubsystem::ApplyFlicker()
{
	if (!HearthLight.IsValid()) return;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(FlickerTime * 17.f + FMath::Sin(FlickerTime * 5.3f));
	HearthLight->SetIntensity(BaseIntensity * FMath::Lerp(0.78f, 1.08f, Pulse));
	for (int32 Index = 0; Index < FlameMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* Flame = FlameMeshes[Index].Get();
		if (!Flame || !FlameBaseTransforms.IsValidIndex(Index)) continue;
		FTransform Transform = FlameBaseTransforms[Index];
		const float Phase = FlickerTime * (11.f + Index * 3.7f) + Index * 1.9f;
		FVector Scale = Transform.GetScale3D();
		Scale.Z *= 0.91f + 0.12f * (0.5f + 0.5f * FMath::Sin(Phase));
		Scale.Y *= 0.95f + 0.05f * FMath::Sin(Phase * 0.67f);
		Transform.SetScale3D(Scale);
		Transform.AddToTranslation(FVector(0.f, 0.f, 1.5f * FMath::Sin(Phase * 0.61f)));
		Flame->SetRelativeTransform(Transform);
		Flame->SetVisibility(true);
	}
}

void UIslandInnHearthSubsystem::InitializeFlames(AActor* Anchor)
{
	if (!IsValid(Anchor) || !Anchor->GetRootComponent()) return;
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInterface* FlameMaterial = GEngine ? GEngine->EmissiveMeshMaterial.Get() : nullptr;
	if (!FlameMaterial)
		FlameMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!FlameMaterial)
		FlameMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Cone || !FlameMaterial) return;

	UMaterialInstanceDynamic* FlameMID = UMaterialInstanceDynamic::Create(FlameMaterial, Anchor);
	if (FlameMID)
	{
		TArray<FMaterialParameterInfo> VectorParameters;
		TArray<FGuid> VectorParameterIds;
		FlameMaterial->GetAllVectorParameterInfo(VectorParameters, VectorParameterIds);
		for (const FMaterialParameterInfo& Parameter : VectorParameters)
			FlameMID->SetVectorParameterValue(Parameter.Name, FLinearColor(1.25f, 0.22f, 0.025f, 1.f));
		TArray<FMaterialParameterInfo> ScalarParameters;
		TArray<FGuid> ScalarParameterIds;
		FlameMaterial->GetAllScalarParameterInfo(ScalarParameters, ScalarParameterIds);
		for (const FMaterialParameterInfo& Parameter : ScalarParameters)
			if (Parameter.Name.ToString().Contains(TEXT("Emissive"), ESearchCase::IgnoreCase) || Parameter.Name.ToString().Contains(TEXT("Intensity"), ESearchCase::IgnoreCase))
				FlameMID->SetScalarParameterValue(Parameter.Name, 4.f);
	}

	const FVector FlameLocations[] = { FVector(0.f, 0.f, -12.f), FVector(0.f, -17.f, -21.f), FVector(0.f, 18.f, -22.f) };
	const FVector FlameScales[] = { FVector(0.2f, 0.2f, 0.55f), FVector(0.12f, 0.12f, 0.38f), FVector(0.11f, 0.12f, 0.35f) };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(FlameLocations); ++Index)
	{
		UStaticMeshComponent* Flame = NewObject<UStaticMeshComponent>(Anchor, NAME_None, RF_Transient);
		if (!Flame) continue;
		Flame->SetStaticMesh(Cone);
		Flame->SetMaterial(0, FlameMID ? static_cast<UMaterialInterface*>(FlameMID) : FlameMaterial);
		Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Flame->SetCastShadow(false);
		Flame->SetMobility(EComponentMobility::Movable);
		Flame->SetupAttachment(Anchor->GetRootComponent());
		Anchor->AddInstanceComponent(Flame);
		Flame->SetRelativeLocation(FlameLocations[Index]);
		Flame->SetRelativeScale3D(FlameScales[Index]);
		Flame->RegisterComponent();
		Flame->SetVisibility(false);
		FlameMeshes.Add(Flame);
		FlameBaseTransforms.Add(Flame->GetRelativeTransform());
	}
}
