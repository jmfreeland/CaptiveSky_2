#include "IslandRainbow.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "IslandEnvironmentSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* AIslandRainbowActor::MaterialPath = TEXT("/Game/Materials/M_IslandRainbow.M_IslandRainbow");

static TAutoConsoleVariable<float> CVarIslandRainbow(
	TEXT("Island.Rainbow"), -1.f,
	TEXT("Force the rainbow's strength 0..1 for testing; negative follows the weather."),
	ECVF_Default);

float ComputeRainbowStrength(const FIslandRainbowInputs& In)
{
	const auto Unit = [](float Value) { return FMath::Clamp(Value, 0.f, 1.f); };
	// sin(3 deg) .. sin(42 deg) is the window in which a bow stands above the horizon.
	const float Sun = FMath::SmoothStep(0.05f, 0.14f, In.SunHeight) * (1.f - FMath::SmoothStep(0.5f, 0.66f, In.SunHeight));
	const float Damp = FMath::SmoothStep(0.35f, 0.75f, Unit(In.Wetness));
	const float Clearing = 1.f - FMath::SmoothStep(0.25f, 0.65f, Unit(In.Rain));
	const float OpenSky = 1.f - FMath::SmoothStep(0.55f, 0.9f, Unit(In.CloudCover));
	const float Calm = 1.f - FMath::SmoothStep(0.1f, 0.4f, Unit(In.Storm));
	return Sun * Damp * Clearing * OpenSky * Calm;
}

AIslandRainbowActor::AIslandRainbowActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	Plane->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded()) Plane->SetStaticMesh(PlaneMesh.Object);
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetGenerateOverlapEvents(false);
	Plane->SetCastShadow(false);
	Plane->bAffectDistanceFieldLighting = false;
	Plane->bAffectDynamicIndirectLighting = false;
	Plane->bUseAsOccluder = false;
	Plane->SetCanEverAffectNavigation(false);
	Plane->SetVisibility(false);
	PrimaryActorTick.bCanEverTick = false;
}

void AIslandRainbowActor::Apply(const FVector& ViewerLocation, const FVector& Axis, float Intensity)
{
	if (!Plane) return;
	if (!Material)
	{
		UMaterialInterface* Parent = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet));
		if (!Parent) return;
		Material = UMaterialInstanceDynamic::Create(Parent, this);
		Plane->SetMaterial(0, Material);
	}
	const bool bVisible = Intensity > 0.002f;
	Plane->SetVisibility(bVisible);
	if (!bVisible) return;
	SetActorLocationAndRotation(ViewerLocation + Axis * Distance, FRotationMatrix::MakeFromZ(-Axis).Rotator());
	// The engine plane is 100 cm square.
	SetActorScale3D(FVector(PlaneSize / 100.f));
	Material->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	Material->SetScalarParameterValue(TEXT("HorizonZ"), ViewerLocation.Z);
}

FVector UIslandRainbowSubsystem::AntiSolarAxis(float IslandHour)
{
	// Mirrors AIslandDayNight, which aims the sun light with pitch -(hour - 6) * 15 and yaw 35: the
	// light's forward vector points at the anti-solar point.
	return FRotator(-(IslandHour - 6.f) * 15.f, 35.f, 0.f).Vector().GetSafeNormal();
}

FString UIslandRainbowSubsystem::DescribeRainbow(float InStrength)
{
	if (InStrength < 0.3f) return FString();
	return InStrength > 0.7f
		? TEXT(" A rainbow stands bright in the sky opposite the sun, the shower's last gift.")
		: TEXT(" A faint rainbow shows in the sky opposite the sun.");
}

TStatId UIslandRainbowSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandRainbowSubsystem, STATGROUP_Tickables);
}

bool UIslandRainbowSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandRainbowSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Actor = InWorld.SpawnActor<AIslandRainbowActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
}

void UIslandRainbowSubsystem::Deinitialize()
{
	if (AIslandRainbowActor* Bow = Actor.Get()) Bow->Destroy();
	Actor.Reset();
	Super::Deinitialize();
}

void UIslandRainbowSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	AIslandRainbowActor* Bow = Actor.Get();
	const UIslandEnvironmentSubsystem* Environment = World ? World->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Bow || !Environment || !Player) return;

	FIslandRainbowInputs Inputs;
	Inputs.SunHeight = Environment->GetSunHeight();
	Inputs.Rain = Environment->GetRainIntensity();
	Inputs.Wetness = Environment->GetWetness();
	Inputs.CloudCover = Environment->GetCloudCover();
	Inputs.Storm = Environment->GetStorm();
	const float Forced = CVarIslandRainbow.GetValueOnGameThread();
	const float Target = Forced >= 0.f ? FMath::Min(Forced, 1.f) : ComputeRainbowStrength(Inputs);
	Strength = FMath::FInterpConstantTo(Strength, Target, DeltaTime, 1.f / 20.f);

	FVector ViewLocation;
	FRotator ViewRotation;
	Player->GetPlayerViewPoint(ViewLocation, ViewRotation);
	Bow->Apply(ViewLocation, AntiSolarAxis(Environment->GetIslandHour()), Strength);
}
