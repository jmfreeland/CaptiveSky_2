#include "IslandDew.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "IslandEnvironmentSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* AIslandDewActor::MaterialPath = TEXT("/Game/Materials/M_IslandDew.M_IslandDew");

static TAutoConsoleVariable<float> CVarIslandDew(
	TEXT("Island.Dew"), -1.f,
	TEXT("Force the dew's strength 0..1 for testing; negative follows the weather."),
	ECVF_Default);

namespace
{
	constexpr int32 SeatsPerTick = 48;
	constexpr float MinHeightAboveGround = 6.f;
	constexpr float MaxHeightAboveGround = 45.f;
}

float ComputeDewStrength(const FIslandDewInputs& In)
{
	const auto Unit = [](float Value) { return FMath::Clamp(Value, 0.f, 1.f); };
	// Needs the sun up to catch the beads, but fades as it climbs and burns the dew off.
	const float Sun = FMath::SmoothStep(0.02f, 0.12f, In.SunHeight) * (1.f - FMath::SmoothStep(0.45f, 0.75f, In.SunHeight));
	const float Morning = In.Hour < 12.f ? 1.f : 0.f;
	const float Moisture = 0.45f + 0.55f * Unit(In.Wetness);
	const float Clearing = 1.f - FMath::SmoothStep(0.05f, 0.3f, Unit(In.Rain));
	const float OpenSky = 1.f - FMath::SmoothStep(0.5f, 0.9f, Unit(In.CloudCover));
	const float Calm = 1.f - FMath::SmoothStep(0.1f, 0.4f, Unit(In.Storm));
	return Sun * Morning * Moisture * Clearing * OpenSky * Calm;
}

AIslandDewActor::AIslandDewActor()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Glints = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Glints"));
	Glints->SetupAttachment(Root);
	Glints->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glints->SetCanEverAffectNavigation(false);
	Glints->SetCastShadow(false);
	Glints->SetMobility(EComponentMobility::Movable);
	Glints->bAffectDistanceFieldLighting = false;
	Glints->bAffectDynamicIndirectLighting = false;
	Glints->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) Glints->SetStaticMesh(Sphere.Object);
	PrimaryActorTick.bCanEverTick = false;
	Slots.SetNum(MaxGlints);
	Random.Initialize(0x0de3a1);
	Tags.AddUnique(TEXT("IslandDew"));
}

int32 AIslandDewActor::DesiredCount(float Strength)
{
	return FMath::Clamp(FMath::RoundToInt(MaxGlints * FMath::Clamp(Strength, 0.f, 1.f)), 0, MaxGlints);
}

bool AIslandDewActor::FindNearestGlint(const FVector& Origin, float MaxDistance, FVector& OutLocation) const
{
	if (!HasMaterial() || !Glints || !Glints->IsVisible() || CurrentStrength < 0.3f || MaxDistance <= 0.f)
		return false;

	const int32 ActiveCount = FMath::Min(DesiredCount(CurrentStrength), Slots.Num());
	const float MaxDistanceSquared = FMath::Square(MaxDistance);
	float NearestDistanceSquared = MaxDistanceSquared;
	bool bFound = false;
	for (int32 Index = 0; Index < ActiveCount; ++Index)
	{
		const FIslandDewGlint& Glint = Slots[Index];
		if (!Glint.bPlaced) continue;
		const float DistanceSquared = FVector::DistSquared(Origin, Glint.Position);
		if (DistanceSquared > NearestDistanceSquared) continue;
		NearestDistanceSquared = DistanceSquared;
		OutLocation = Glint.Position;
		bFound = true;
	}
	return bFound;
}

float AIslandDewActor::GlintSize(float Distance)
{
	// A sphere mesh is 100 cm across; this is the diameter wanted, in cm.
	return 2.5f + FMath::Clamp(Distance, 0.f, Radius) * 0.007f;
}

float AIslandDewActor::GroundHeightAt(const FVector2D& XY, float ReferenceZ) const
{
	if (const UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandDewGround), false, this);
		if (World->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, ReferenceZ + 800.f), FVector(XY.X, XY.Y, ReferenceZ - 2500.f), ECC_Visibility, Query))
			return Hit.ImpactPoint.Z;
	}
	return ReferenceZ - 100.f;
}

void AIslandDewActor::Seat(int32 Slot, const FVector& Viewer)
{
	const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
	const float Distance = Radius * FMath::Sqrt(Random.FRand());
	const FVector2D XY(Viewer.X + FMath::Cos(Angle) * Distance, Viewer.Y + FMath::Sin(Angle) * Distance);
	FIslandDewGlint& Glint = Slots[Slot];
	Glint.Position = FVector(XY.X, XY.Y, GroundHeightAt(XY, Viewer.Z) + Random.FRandRange(MinHeightAboveGround, MaxHeightAboveGround));
	Glint.Size = GlintSize(Distance);
	Glint.bPlaced = true;
}

void AIslandDewActor::Advance(const FVector& Viewer, float Strength)
{
	if (!Glints) return;
	if (!Material)
	{
		UMaterialInterface* Parent = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet));
		if (!Parent) return;
		Material = UMaterialInstanceDynamic::Create(Parent, this);
		Glints->SetMaterial(0, Material);
	}

	const int32 Wanted = DesiredCount(Strength);
	CurrentStrength = FMath::Clamp(Strength, 0.f, 1.f);
	const bool bVisible = Strength > 0.002f && Wanted > 0;
	Glints->SetVisibility(bVisible);
	if (!bVisible) return;
	Material->SetScalarParameterValue(TEXT("Intensity"), Strength);

	while (Glints->GetInstanceCount() < MaxGlints) Glints->AddInstance(FTransform(FQuat::Identity, Viewer, FVector(0.0001f)), true);

	int32 SeatsLeft = SeatsPerTick;
	const float RadiusSquared = Radius * Radius;
	for (int32 Slot = 0; Slot < MaxGlints; ++Slot)
	{
		FIslandDewGlint& Glint = Slots[Slot];
		const bool bActive = Slot < Wanted;
		if (bActive && (!Glint.bPlaced || FVector::DistSquared2D(Glint.Position, Viewer) > RadiusSquared))
		{
			if (SeatsLeft <= 0) continue;
			--SeatsLeft;
			Seat(Slot, Viewer);
		}
		const FTransform Transform(FQuat::Identity, bActive && Glint.bPlaced ? Glint.Position : Viewer,
			FVector(bActive && Glint.bPlaced ? Glint.Size / 100.f : 0.0001f));
		Glints->UpdateInstanceTransform(Slot, Transform, true, Slot == MaxGlints - 1, true);
	}
}

FString UIslandDewSubsystem::DescribeDew(float InStrength)
{
	if (InStrength < 0.3f) return FString();
	return InStrength > 0.7f
		? TEXT(" Dew beads every blade of grass and glitters in the low sun.")
		: TEXT(" A little dew glints on the grass.");
}

TStatId UIslandDewSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandDewSubsystem, STATGROUP_Tickables);
}

bool UIslandDewSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandDewSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Actor = InWorld.SpawnActor<AIslandDewActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
}

void UIslandDewSubsystem::Deinitialize()
{
	if (AIslandDewActor* Dew = Actor.Get()) Dew->Destroy();
	Actor.Reset();
	Super::Deinitialize();
}

void UIslandDewSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	AIslandDewActor* Dew = Actor.Get();
	const UIslandEnvironmentSubsystem* Environment = World ? World->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Dew || !Environment || !Player) return;

	FIslandDewInputs Inputs;
	Inputs.SunHeight = Environment->GetSunHeight();
	Inputs.Hour = Environment->GetIslandHour();
	Inputs.Rain = Environment->GetRainIntensity();
	Inputs.Wetness = Environment->GetWetness();
	Inputs.CloudCover = Environment->GetCloudCover();
	Inputs.Storm = Environment->GetStorm();
	const float Forced = CVarIslandDew.GetValueOnGameThread();
	const float Target = Forced >= 0.f ? FMath::Min(Forced, 1.f) : ComputeDewStrength(Inputs);
	Strength = FMath::FInterpConstantTo(Strength, Target, DeltaTime, 1.f / 20.f);

	FVector ViewLocation;
	FRotator ViewRotation;
	Player->GetPlayerViewPoint(ViewLocation, ViewRotation);
	Dew->Advance(ViewLocation, Strength);
}
