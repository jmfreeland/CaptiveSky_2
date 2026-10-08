#include "IslandDrift.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWeather.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TAutoConsoleVariable<int32> CVarIslandDrift(TEXT("Island.Drift"), 1,
		TEXT("1 shows windborne leaves, petals and seeds around the viewer; 0 hides them."));

	constexpr int32 SlotsPerKind[static_cast<int32>(EIslandDriftKind::Count)] = {18, 28, 20, 30};

	// Flat leaf-like boxes (cm = 100 * scale) for leaves and petals, a small round seed head.
	FVector BaseScale(EIslandDriftKind Kind)
	{
		switch (Kind)
		{
		case EIslandDriftKind::Seed: return FVector(0.014f, 0.014f, 0.014f);
		case EIslandDriftKind::Petal: return FVector(0.034f, 0.046f, 0.003f);
		case EIslandDriftKind::DryLeaf: return FVector(0.065f, 0.105f, 0.004f);
		default: return FVector(0.07f, 0.11f, 0.004f);
		}
	}

	FLinearColor KindColor(EIslandDriftKind Kind)
	{
		switch (Kind)
		{
		case EIslandDriftKind::Seed: return FLinearColor(0.78f, 0.75f, 0.66f);
		case EIslandDriftKind::Petal: return FLinearColor(0.72f, 0.36f, 0.46f);
		case EIslandDriftKind::DryLeaf: return FLinearColor(0.34f, 0.18f, 0.05f);
		default: return FLinearColor(0.13f, 0.24f, 0.04f);
		}
	}
}

AIslandDrift::AIslandDrift()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	static const TCHAR* LayerNames[] = {TEXT("Seeds"), TEXT("Petals"), TEXT("DryLeaves"), TEXT("GreenLeaves")};
	int32 Slot = 0;
	for (int32 KindIndex = 0; KindIndex < static_cast<int32>(EIslandDriftKind::Count); ++KindIndex)
	{
		FirstSlotOfKind.Add(Slot);
		Slot += SlotsPerKind[KindIndex];
		UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(LayerNames[KindIndex]);
		Layer->SetupAttachment(Root);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetCanEverAffectNavigation(false);
		Layer->SetCastShadow(false);
		Layer->SetMobility(EComponentMobility::Movable);
		const bool bSeed = KindIndex == static_cast<int32>(EIslandDriftKind::Seed);
		if (bSeed ? Sphere.Succeeded() : Cube.Succeeded()) Layer->SetStaticMesh(bSeed ? Sphere.Object : Cube.Object);
		if (Basic.Succeeded()) Layer->SetMaterial(0, Basic.Object);
		MoteLayers.Add(Layer);
	}
	check(Slot == MaxMotes);
	Motes.SetNum(MaxMotes);
	Random.Initialize(0x7d11f7);
	Tags.AddUnique(TEXT("IslandDrift"));
}

int32 AIslandDrift::DesiredCount(float WindSpeed, float Rain, float Daylight)
{
	const float Lift = FMath::SmoothStep(25.f, 260.f, FMath::Max(0.f, WindSpeed));
	const float DayGate = FMath::SmoothStep(0.1f, 0.5f, FMath::Clamp(Daylight, 0.f, 1.f));
	const float DryGate = 1.f - FMath::SmoothStep(0.05f, 0.35f, FMath::Clamp(Rain, 0.f, 1.f));
	const float Wanted = (6.f + (MaxMotes - 6.f) * Lift) * DayGate * DryGate;
	return FMath::Clamp(FMath::RoundToInt(Wanted), 0, MaxMotes);
}

float AIslandDrift::LiftWindSpeed(EIslandDriftKind Kind)
{
	switch (Kind)
	{
	case EIslandDriftKind::Seed: return 0.f;
	case EIslandDriftKind::Petal: return 35.f;
	case EIslandDriftKind::DryLeaf: return 70.f;
	default: return 95.f;
	}
}

float AIslandDrift::SinkSpeed(EIslandDriftKind Kind)
{
	switch (Kind)
	{
	case EIslandDriftKind::Seed: return 10.f;
	case EIslandDriftKind::Petal: return 28.f;
	case EIslandDriftKind::DryLeaf: return 45.f;
	default: return 58.f;
	}
}

FVector AIslandDrift::StepVelocity(const FVector& Velocity, const FVector& Wind, EIslandDriftKind Kind, float Seconds)
{
	// Light things follow the air quickly; a green leaf lags behind a gust.
	static constexpr float Response[] = {0.5f, 0.8f, 1.1f, 1.4f};
	const float Tau = Response[static_cast<int32>(Kind)];
	const float Alpha = 1.f - FMath::Exp(-FMath::Max(0.f, Seconds) / Tau);
	const FVector Target(Wind.X, Wind.Y, Wind.Z - SinkSpeed(Kind));
	return Velocity + (Target - Velocity) * Alpha;
}

EIslandDriftKind AIslandDrift::KindForSlot(int32 Index)
{
	int32 Remaining = Index;
	for (int32 KindIndex = 0; KindIndex < static_cast<int32>(EIslandDriftKind::Count); ++KindIndex)
	{
		if (Remaining < SlotsPerKind[KindIndex]) return static_cast<EIslandDriftKind>(KindIndex);
		Remaining -= SlotsPerKind[KindIndex];
	}
	return EIslandDriftKind::GreenLeaf;
}

FString AIslandDrift::DescribeDrift(float WindSpeed, float Rain, float Daylight)
{
	if (Rain > 0.3f || Daylight < 0.3f) return FString();
	if (WindSpeed >= 260.f) return TEXT(" The wind is tearing leaves loose and flinging them past in tumbling handfuls.");
	if (WindSpeed >= 120.f) return TEXT(" Dry leaves and petals tumble across the ground on the wind.");
	if (WindSpeed >= 40.f) return TEXT(" Petals and seeds drift past on the breeze.");
	return TEXT(" A few seeds hang almost motionless in the still air.");
}

int32 AIslandDrift::GetActiveCount() const
{
	int32 Count = 0;
	for (const FIslandDriftMote& Mote : Motes) Count += Mote.bActive ? 1 : 0;
	return Count;
}

float AIslandDrift::GroundHeightAt(const FVector& XY, float ReferenceZ) const
{
	if (const UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandDriftGround), false, this);
		if (World->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, ReferenceZ + 800.f), FVector(XY.X, XY.Y, ReferenceZ - 2500.f), ECC_Visibility, Query))
			return Hit.ImpactPoint.Z;
	}
	return ReferenceZ - 100.f;
}

void AIslandDrift::Place(FIslandDriftMote& Mote, int32 Slot, const FVector& Viewer, const FVector& Wind, bool bAnywhere)
{
	const EIslandDriftKind Kind = KindForSlot(Slot);
	const float WindSpeed = Wind.Size2D();
	float Angle;
	float Distance;
	if (bAnywhere || WindSpeed < 40.f)
	{
		Angle = Random.FRand() * 2.f * PI;
		Distance = Radius * FMath::Sqrt(Random.FRand());
	}
	else
	{
		Angle = FMath::Atan2(-Wind.Y, -Wind.X) + Random.FRandRange(-1.1f, 1.1f);
		Distance = Radius * Random.FRandRange(0.8f, 1.f);
	}
	const FVector2D XY(Viewer.X + FMath::Cos(Angle) * Distance, Viewer.Y + FMath::Sin(Angle) * Distance);
	Mote.GroundZ = GroundHeightAt(FVector(XY.X, XY.Y, 0.f), Viewer.Z);
	Mote.Position = FVector(XY.X, XY.Y, Mote.GroundZ + Random.FRandRange(MinHeightAboveGround, MaxHeightAboveGround));
	Mote.Velocity = FVector(Wind.X, Wind.Y, -SinkSpeed(Kind));
	Mote.Life = Random.FRandRange(10.f, 22.f);
	Mote.Age = bAnywhere ? Random.FRand() * Mote.Life * 0.7f : 0.f;
	Mote.SpinAxis = Random.GetUnitVector();
	Mote.Spin = Random.FRand() * 2.f * PI;
	Mote.SpinRate = Random.FRandRange(1.2f, 4.5f) * (Kind == EIslandDriftKind::Seed ? 0.4f : 1.f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
	Mote.Phase = Random.FRand() * 2.f * PI;
	Mote.Size = Random.FRandRange(0.75f, 1.3f);
	Mote.bActive = true;
}

void AIslandDrift::EnsureMaterials()
{
	if (LayerMaterials.Num() == MoteLayers.Num()) return;
	LayerMaterials.Reset();
	for (int32 KindIndex = 0; KindIndex < MoteLayers.Num(); ++KindIndex)
	{
		UMaterialInstanceDynamic* Material = nullptr;
		if (MoteLayers[KindIndex] && MoteLayers[KindIndex]->GetMaterial(0))
		{
			Material = UMaterialInstanceDynamic::Create(MoteLayers[KindIndex]->GetMaterial(0), this);
			if (Material)
			{
				Material->SetVectorParameterValue(TEXT("Color"), KindColor(static_cast<EIslandDriftKind>(KindIndex)));
				MoteLayers[KindIndex]->SetMaterial(0, Material);
			}
		}
		LayerMaterials.Add(Material);
	}
}

void AIslandDrift::Advance(float Dt, const FVector& Viewer, const TFunctionRef<FVector(const FVector&)>& WindAt, float Rain, float Daylight)
{
	Dt = FMath::Clamp(Dt, 0.f, 0.1f);
	EnsureMaterials();
	SetActorLocation(Viewer);

	const FVector ViewerWind = WindAt(Viewer);
	const float ViewerWindSpeed = ViewerWind.Size2D();
	const int32 Desired = DesiredCount(ViewerWindSpeed, Rain, Daylight);

	int32 Active = 0;
	for (int32 Slot = 0; Slot < MaxMotes; ++Slot)
	{
		FIslandDriftMote& Mote = Motes[Slot];
		if (!Mote.bActive) continue;
		const EIslandDriftKind Kind = KindForSlot(Slot);
		const FVector Wind = WindAt(Mote.Position);
		Mote.Velocity = StepVelocity(Mote.Velocity, Wind, Kind, Dt);
		// Eddies: a sideways wobble and some lift that grow with the wind, strongest on the broad leaves.
		const float Amp = (4.f + 0.16f * Wind.Size2D()) * (Kind == EIslandDriftKind::Seed ? 0.6f : 1.f);
		const FVector Flutter(FMath::Cos(Mote.Age * 2.3f + Mote.Phase), FMath::Sin(Mote.Age * 1.9f + Mote.Phase * 1.3f),
			0.6f * FMath::Sin(Mote.Age * 3.1f + Mote.Phase));
		Mote.Position += (Mote.Velocity + Flutter * Amp) * Dt;
		Mote.Age += Dt;
		Mote.Spin += Mote.SpinRate * Dt;
		const bool bLanded = Mote.Position.Z <= Mote.GroundZ + 4.f;
		const bool bGone = FVector::DistSquared2D(Mote.Position, Viewer) > FMath::Square(Radius * 1.15f);
		if (bLanded || bGone || Mote.Age >= Mote.Life) Mote.bActive = false;
		else ++Active;
	}

	if (!bPopulated)
	{
		bPopulated = true;
		for (int32 Slot = 0; Slot < MaxMotes && Active < Desired; ++Slot)
		{
			if (Motes[Slot].bActive || ViewerWindSpeed < LiftWindSpeed(KindForSlot(Slot))) continue;
			Place(Motes[Slot], Slot, Viewer, ViewerWind, true);
			++Active;
		}
	}
	else
	{
		SpawnBudget = FMath::Min(SpawnBudget + Dt * 8.f, 4.f);
		for (int32 Attempt = 0; Attempt < 12 && Active < Desired && SpawnBudget >= 1.f; ++Attempt)
		{
			const int32 Slot = Random.RandRange(0, MaxMotes - 1);
			if (Motes[Slot].bActive || ViewerWindSpeed < LiftWindSpeed(KindForSlot(Slot))) continue;
			Place(Motes[Slot], Slot, Viewer, ViewerWind, false);
			SpawnBudget -= 1.f;
			++Active;
		}
	}

	for (int32 KindIndex = 0; KindIndex < MoteLayers.Num(); ++KindIndex)
	{
		UInstancedStaticMeshComponent* Layer = MoteLayers[KindIndex];
		if (!Layer) continue;
		const EIslandDriftKind Kind = static_cast<EIslandDriftKind>(KindIndex);
		const int32 Count = SlotsPerKind[KindIndex];
		while (Layer->GetInstanceCount() < Count) Layer->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.0001f)));
		for (int32 Local = 0; Local < Count; ++Local)
		{
			const FIslandDriftMote& Mote = Motes[FirstSlotOfKind[KindIndex] + Local];
			FTransform Transform(FQuat::Identity, Viewer, FVector(0.0001f));
			if (Mote.bActive)
			{
				const float FadeIn = FMath::SmoothStep(0.f, 0.8f, Mote.Age);
				const float FadeOut = FMath::SmoothStep(0.f, 1.2f, Mote.Life - Mote.Age);
				Transform = FTransform(FQuat(Mote.SpinAxis, Mote.Spin), Mote.Position, BaseScale(Kind) * (Mote.Size * FadeIn * FadeOut));
			}
			Layer->UpdateInstanceTransform(Local, Transform, true, Local == Count - 1, true);
		}
	}
}

TStatId UIslandDriftSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandDriftSubsystem, STATGROUP_Tickables);
}

bool UIslandDriftSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandDriftSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Drift = InWorld.SpawnActor<AIslandDrift>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
}

void UIslandDriftSubsystem::Deinitialize()
{
	if (AIslandDrift* Actor = Drift.Get()) Actor->Destroy();
	Drift.Reset();
	Super::Deinitialize();
}

void UIslandDriftSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	AIslandDrift* Actor = Drift.Get();
	if (!World || !Actor) return;
	const APlayerController* Player = World->GetFirstPlayerController();
	const bool bShow = CVarIslandDrift.GetValueOnGameThread() != 0 && Player && Player->GetPawn();
	Actor->SetActorHiddenInGame(!bShow);
	if (!bShow) return;

	const double Now = World->GetTimeSeconds();
	if (!Weather.IsValid() && Now >= NextWeatherSearch)
	{
		NextWeatherSearch = Now + 3.0;
		for (TActorIterator<AIslandWeather> It(World); It; ++It) { Weather = *It; break; }
	}

	float Rain = 0.f;
	float Daylight = 1.f;
	bool bIndoors = false;
	if (const UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>())
	{
		Rain = Environment->GetRainIntensity();
		Daylight = Environment->GetDaylight();
		bIndoors = Environment->GetIndoors() > 0.5f;
	}
	const AIslandWeather* WeatherActor = Weather.Get();
	const auto WindAt = [WeatherActor, Now](const FVector& Position)
	{
		return WeatherActor ? WeatherActor->SampleWind(Position, Now) : FVector::ZeroVector;
	};
	Actor->Advance(DeltaTime, Player->GetPawn()->GetActorLocation(), WindAt, bIndoors ? 1.f : Rain, Daylight);
}
