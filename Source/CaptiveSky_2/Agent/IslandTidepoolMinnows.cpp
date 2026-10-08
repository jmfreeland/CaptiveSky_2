#include "IslandTidepoolMinnows.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "IslandDayNight.h"
#include "IslandPoolRippleEffect.h"
#include "IslandWeather.h"
#include "IslandTideglassSubsystem.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIslandTidepoolMinnows::AIslandTidepoolMinnows()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.06f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("MinnowSchool"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseShapeMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;

	Fish.Reserve(FishCount);
	Tails.Reserve(FishCount);
	for (int32 Index = 0; Index < FishCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("Minnow_%d"), Index));
		UStaticMeshComponent* Minnow = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		Minnow->SetupAttachment(RootComponent);
		Minnow->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		Minnow->SetRelativeScale3D(FVector(0.12f, 0.035f, 0.045f));
		Minnow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Minnow->SetCastShadow(false);
		Fish.Add(Minnow);

		const FName TailName(*FString::Printf(TEXT("MinnowTail_%d"), Index));
		UStaticMeshComponent* Tail = CreateDefaultSubobject<UStaticMeshComponent>(TailName);
		Tail->SetupAttachment(Minnow);
		Tail->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		// Relative locations are in the parent mesh's local units. Keep the fin at
		// the rear pole of the 50-unit sphere; a sub-unit offset leaves it buried
		// almost entirely inside the fish after the body is scaled down.
		Tail->SetRelativeLocation(FVector(-50.f, 0.f, 0.f));
		Tail->SetRelativeScale3D(FVector(0.28f, 0.18f, 0.45f));
		Tail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Tail->SetCastShadow(false);
		Tails.Add(Tail);
	}
}

void AIslandTidepoolMinnows::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		IslandClock = *It;
		break;
	}
	ConfigureAppearance();
	UpdateSchool(0.f);
}

void AIslandTidepoolMinnows::ConfigureAppearance()
{
	if (BaseShapeMaterial)
	{
		static const FLinearColor SchoolPalette[FishCount] =
		{
			FLinearColor(0.18f, 0.34f, 0.40f),
			FLinearColor(0.22f, 0.36f, 0.34f),
			FLinearColor(0.28f, 0.32f, 0.19f),
			FLinearColor(0.38f, 0.36f, 0.24f),
			FLinearColor(0.17f, 0.28f, 0.39f)
		};
		for (int32 Index = 0; Index < Fish.Num(); ++Index)
		{
			UMaterialInstanceDynamic* FishMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
			if (!FishMaterial) continue;
			FishMaterial->SetVectorParameterValue(TEXT("Color"), SchoolPalette[Index % FishCount]);
			if (Fish[Index]) Fish[Index]->SetMaterial(0, FishMaterial);
			if (Tails.IsValidIndex(Index) && Tails[Index]) Tails[Index]->SetMaterial(0, FishMaterial);
		}
	}
}

void AIslandTidepoolMinnows::RespondToQuietObservation(const FVector& ObserverLocation)
{
	ScatterDirection = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (ScatterDirection.IsNearlyZero()) ScatterDirection = GetActorForwardVector();
	ScatterRemaining = 2.4f;
}

bool AIslandTidepoolMinnows::RespondToSurfaceRipple()
{
	if (SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f) return false;
	SurfacePulseRemaining = 1.2f;
	SurfacePulseCooldownRemaining = 3.f;
	return true;
}

FString AIslandTidepoolMinnows::DescribeRecentSurfaceBreak(const FVector& ObserverLocation) const
{
	if (SurfaceBreakContextRemaining <= 0.f ||
		FVector::DistSquared2D(ObserverLocation, GetActorLocation()) > FMath::Square(1100.f) ||
		FMath::Abs(ObserverLocation.Z - GetActorLocation().Z) > 500.f)
	{
		return FString();
	}

	return TEXT("A minnow made a brief surface break in the Tideglass shallows within the last few minutes. The ripple has faded; the school remains wild and nothing lasting changed.");
}

void AIslandTidepoolMinnows::CheckForNaturalSurfaceRipple()
{
	if (!GetWorld() || SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f) return;

	constexpr float RippleResponseRadius = 250.f;
	for (TActorIterator<AIslandPoolRippleEffect> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("WindImpact")) && !It->ActorHasTag(TEXT("RainImpact"))) continue;
		if (FVector::DistSquared2D(GetActorLocation(), It->GetActorLocation()) > FMath::Square(RippleResponseRadius)) continue;
		RespondToSurfaceRipple();
		return;
	}
}

void AIslandTidepoolMinnows::TryCreateSurfaceBreak(float RainIntensity)
{
	if (!GetWorld() || Fish.IsEmpty() || ScatterRemaining > 0.f ||
		SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f || RainIntensity >= 0.35f)
	{
		SurfaceBreakRemaining = 4.f;
		return;
	}

	UStaticMeshComponent* FishThatBrokeSurface = Fish[SurfaceBreakFishIndex % Fish.Num()];
	if (!FishThatBrokeSurface)
	{
		SurfaceBreakRemaining = 4.f;
		return;
	}

	// Fish skim about 1.5 cm above the moving waterline; the ripple lights sit
	// 24 cm above their root, so place the actor below the same tidal surface.
	const FVector FishLocation = FishThatBrokeSurface->GetComponentLocation();
	const FVector RippleLocation(FishLocation.X, FishLocation.Y, GetActorLocation().Z + GetTideOffsetCm() - 24.f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(
		RippleLocation, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsMinnowImpact();
		SurfaceBreakFishIndex = (SurfaceBreakFishIndex + 1) % Fish.Num();
		SurfaceBreakRemaining = 19.f;
		SurfaceBreakContextRemaining = SurfaceBreakContextLifetime;
		SurfacePulseCooldownRemaining = FMath::Max(SurfacePulseCooldownRemaining, 3.f);
		return;
	}

	SurfaceBreakRemaining = 4.f;
}

float AIslandTidepoolMinnows::RainMovementScale(float RainIntensity)
{
	const float RainActivity = FMath::SmoothStep(0.35f, 0.85f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	return FMath::Lerp(1.f, 0.68f, RainActivity);
}

float AIslandTidepoolMinnows::GetTideOffsetCm() const
{
	return IslandClock.IsValid()
		? UIslandTideglassSubsystem::TideOffsetCm(IslandClock->CurrentHour, IslandClock->DayNumber)
		: 0.f;
}

float AIslandTidepoolMinnows::GetScatterAlpha() const
{
	if (ScatterRemaining <= 0.f) return 0.f;
	if (ScatterRemaining > 1.7f)
		return FMath::SmoothStep(0.f, 0.7f, 2.4f - ScatterRemaining);
	return FMath::SmoothStep(0.f, 1.7f, ScatterRemaining);
}

float AIslandTidepoolMinnows::GetSurfacePulseAlpha() const
{
	if (SurfacePulseRemaining <= 0.f) return 0.f;
	const float Elapsed = 1.2f - SurfacePulseRemaining;
	return FMath::SmoothStep(0.f, 0.18f, Elapsed) * (1.f - FMath::SmoothStep(0.45f, 1.2f, Elapsed));
}

void AIslandTidepoolMinnows::CheckForNearbyRavenDisturbance()
{
	if (!GetWorld()) return;

	constexpr float FlybyRadius = 550.f;
	constexpr float MinimumHeight = 150.f;
	constexpr float MaximumHeight = 700.f;
	constexpr float SettledRavenRadius = 450.f;
	constexpr float SettledRavenHeight = 350.f;
	constexpr float PresenceRearmRadius = 800.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		const float DistanceSquared = Offset.SizeSquared2D();
		const bool bSettledRaven = It->LocomotionState == ERavenLocomotionState::Grounded ||
			It->LocomotionState == ERavenLocomotionState::Hopping || It->LocomotionState == ERavenLocomotionState::Perched;
		if (RavenPresenceLatch.Get() == *It && DistanceSquared > FMath::Square(PresenceRearmRadius))
			RavenPresenceLatch.Reset();

		const bool bCloseSettledPresence = bSettledRaven && FMath::Abs(Offset.Z) <= SettledRavenHeight &&
			DistanceSquared <= FMath::Square(SettledRavenRadius);
		const bool bLowFlyby = It->LocomotionState == ERavenLocomotionState::Flying &&
			Offset.Z >= MinimumHeight && Offset.Z <= MaximumHeight && DistanceSquared <= FMath::Square(FlybyRadius);
		if (bCloseSettledPresence)
		{
			// The school scatters once for an approach. Remaining nearby, including
			// changing between perched and flying, does not repeatedly reset its path.
			const bool bAlreadyNoticedThisApproach = RavenPresenceLatch.Get() == *It;
			if (!bAlreadyNoticedThisApproach) RavenPresenceLatch = *It;
			if (bAlreadyNoticedThisApproach || RavenFlybyCooldownRemaining > 0.f || ScatterRemaining > 0.f) continue;
		}
		else if (!bLowFlyby || RavenFlybyCooldownRemaining > 0.f || ScatterRemaining > 0.f) continue;

		// A close, low pass or settled approach briefly breaks the school's pattern;
		// this is not a hunt, capture, model call, or lasting change.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTidepoolMinnows::UpdateSchool(float RainIntensity)
{
	const float TuckScale = RainMovementScale(RainIntensity);
	const float TideOffset = GetTideOffsetCm();
	const float ScatterAlpha = GetScatterAlpha();
	const float CircleScale = 1.f + 0.65f * GetSurfacePulseAlpha();
	const FVector Side(-ScatterDirection.Y, ScatterDirection.X, 0.f);
	for (int32 Index = 0; Index < Fish.Num(); ++Index)
	{
		UStaticMeshComponent* Minnow = Fish[Index];
		if (!Minnow) continue;
		const float Angle = Phase + ElapsedSeconds * 0.62f + Index * 2.f * PI / FishCount;
		const FVector IdleOffset(
			FMath::Cos(Angle) * 135.f * TuckScale * CircleScale,
			FMath::Sin(Angle) * 82.f * TuckScale * CircleScale,
			TideOffset + 0.8f + FMath::Sin(Angle * 1.7f) * 0.4f);
		const float FanOffset = (Index - (FishCount - 1) * 0.5f) * 22.f;
		const FVector ScatterOffset = ScatterDirection * 210.f + Side * FanOffset;
		Minnow->SetRelativeLocation(IdleOffset + ScatterOffset * ScatterAlpha);

		FVector Facing(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		Facing += ScatterDirection * (ScatterAlpha * 1.2f);
		if (!Facing.IsNearlyZero())
			Minnow->SetRelativeRotation(FRotator(0.f, Facing.Rotation().Yaw, 0.f));

		if (Tails.IsValidIndex(Index) && Tails[Index])
		{
			const float TailBeat = FMath::Sin(ElapsedSeconds * 8.f + Index * 1.35f);
			Tails[Index]->SetRelativeRotation(FRotator(0.f, TailBeat * 16.f * TuckScale, TailBeat * 3.f));
		}
	}
}

void AIslandTidepoolMinnows::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ElapsedSeconds += SafeDelta;
	ScatterRemaining = FMath::Max(0.f, ScatterRemaining - SafeDelta);
	SurfacePulseRemaining = FMath::Max(0.f, SurfacePulseRemaining - SafeDelta);
	SurfacePulseCooldownRemaining = FMath::Max(0.f, SurfacePulseCooldownRemaining - SafeDelta);
	RippleCheckRemaining -= SafeDelta;
	if (RippleCheckRemaining <= 0.f)
	{
		RippleCheckRemaining = 0.35f;
		CheckForNaturalSurfaceRipple();
	}
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - SafeDelta);
	RavenCheckRemaining -= SafeDelta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.35f;
		CheckForNearbyRavenDisturbance();
	}
	const float Rain = Weather.IsValid() && GetWorld()
		? Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds())
		: 0.f;
	SurfaceBreakRemaining -= SafeDelta;
	SurfaceBreakContextRemaining = FMath::Max(0.f, SurfaceBreakContextRemaining - SafeDelta);
	if (SurfaceBreakRemaining <= 0.f)
	{
		TryCreateSurfaceBreak(Rain);
	}
	UpdateSchool(Rain);
}
