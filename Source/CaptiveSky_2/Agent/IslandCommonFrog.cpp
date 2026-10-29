#include "IslandCommonFrog.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "IslandDayNight.h"
#include "IslandTidepoolCrab.h"
#include "IslandTidepoolMinnows.h"
#include "IslandTideglassDragonfly.h"
#include "IslandWeather.h"
#include "RatAgentAIController.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr TCHAR FrogMeshPath[] = TEXT("/Game/Generated/ComfyBlender/Tideglass_CommonFrog_Candidate/SM_Tideglass_CommonFrog_Candidate_01.SM_Tideglass_CommonFrog_Candidate_01");
	constexpr float FrogObserverRadiusCm = 240.f;
	constexpr float FrogObserverHeightCm = 180.f;
	constexpr float FrogObserverRearmRadiusCm = 520.f;
	constexpr float FrogObserverRearmHeightCm = 260.f;
	constexpr float FrogHopHeightCm = 13.f;
	constexpr float FrogHopDurationSeconds = 0.48f;
}

AIslandCommonFrog::AIslandCommonFrog()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("TideglassFrog"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicInstance(TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> DetailedMesh(FrogMeshPath, LOAD_NoWarn | LOAD_Quiet);
	BaseShapeMaterial = BasicInstance.Succeeded() ? BasicInstance.Object : (Basic.Succeeded() ? Basic.Object : nullptr);

	DetailedBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DetailedBody"));
	DetailedBody->SetupAttachment(RootComponent);
	DetailedBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetailedBody->SetCanEverAffectNavigation(false);
	DetailedBody->SetGenerateOverlapEvents(false);
	DetailedBody->SetCastShadow(false);
	UStaticMesh* FrogMesh = DetailedMesh.Get();
	const bool bHasDetailedMesh = FrogMesh != nullptr;
	if (bHasDetailedMesh)
	{
		DetailedBody->SetStaticMesh(FrogMesh);
		DetailedBody->SetRelativeLocation(FVector(0.f, 0.f, 0.25f));
	}
	DetailedBody->SetVisibility(bHasDetailedMesh, true);
	DetailedBody->SetHiddenInGame(!bHasDetailedMesh, true);

	if (bHasDetailedMesh || !Sphere.Succeeded()) return;

	UStaticMesh* SphereMesh = Sphere.Object;
	auto AddPart = [this, SphereMesh](const TCHAR* Name, const FVector& Location, const FVector& Scale,
		TArray<TObjectPtr<UStaticMeshComponent>>& Group, const FRotator& Rotation)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
		Part->SetupAttachment(RootComponent);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetStaticMesh(SphereMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
		Group.Add(Part);
	};

	// Engine-sphere fallback stays frog-sized (all transforms are centimetres).
	AddPart(TEXT("FallbackBody"), FVector(0.f, 0.f, 2.25f), FVector(0.060f, 0.044f, 0.024f), GreenParts, FRotator::ZeroRotator);
	AddPart(TEXT("FallbackHead"), FVector(2.45f, 0.f, 2.45f), FVector(0.034f, 0.035f, 0.024f), GreenParts, FRotator::ZeroRotator);
	for (int32 Side : {-1, 1})
	{
		AddPart(*FString::Printf(TEXT("EyeRidge_%d"), Side), FVector(2.35f, Side * 1.72f, 2.75f),
			FVector(0.019f, 0.004f, 0.004f), MaskParts, FRotator::ZeroRotator);
		AddPart(*FString::Printf(TEXT("Eye_%d"), Side), FVector(2.55f, Side * 1.32f, 3.53f),
			FVector(0.008f, 0.008f, 0.008f), EyeParts, FRotator::ZeroRotator);
		AddPart(*FString::Printf(TEXT("Pupil_%d"), Side), FVector(2.92f, Side * 1.73f, 3.42f),
			FVector(0.0032f, 0.0030f, 0.0042f), PupilParts, FRotator::ZeroRotator);
		AddPart(*FString::Printf(TEXT("FrontLeg_%d"), Side), FVector(0.72f, Side * 1.95f, 1.00f),
			FVector(0.028f, 0.009f, 0.008f), GreenParts, FRotator(0.f, Side * 25.f, 0.f));
		AddPart(*FString::Printf(TEXT("HindThigh_%d"), Side), FVector(-2.25f, Side * 1.55f, 1.40f),
			FVector(0.031f, 0.016f, 0.012f), GreenParts, FRotator(0.f, Side * 16.f, 0.f));
		AddPart(*FString::Printf(TEXT("HindCalf_%d"), Side), FVector(-3.25f, Side * 2.55f, 0.82f),
			FVector(0.024f, 0.009f, 0.007f), GreenParts, FRotator(0.f, -Side * 42.f, 0.f));
	}
}

void AIslandCommonFrog::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	const int32 Seed = FMath::FloorToInt(HomeLocation.X * 0.31f + HomeLocation.Y * 0.73f) ^ 0x51A7;
	Random.Initialize(Seed);
	AmbientHopRemaining = Random.FRandRange(22.f, 42.f);
	DragonflyCheckRemaining = Random.FRandRange(0.35f, 0.9f);
	RestingRotation = GetActorRotation();
	FindEnvironment();

	if (!BaseShapeMaterial) return;
	GreenMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
	MaskMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
	EyeMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
	PupilMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
	SetColor(GreenMaterial, FLinearColor(0.30f, 0.33f, 0.18f));
	SetColor(MaskMaterial, FLinearColor(0.12f, 0.14f, 0.08f));
	SetColor(EyeMaterial, FLinearColor(0.74f, 0.60f, 0.21f));
	SetColor(PupilMaterial, FLinearColor(0.025f, 0.028f, 0.02f));
	for (UStaticMeshComponent* Part : GreenParts) if (Part && GreenMaterial) Part->SetMaterial(0, GreenMaterial);
	for (UStaticMeshComponent* Part : MaskParts) if (Part && MaskMaterial) Part->SetMaterial(0, MaskMaterial);
	for (UStaticMeshComponent* Part : EyeParts) if (Part && EyeMaterial) Part->SetMaterial(0, EyeMaterial);
	for (UStaticMeshComponent* Part : PupilParts) if (Part && PupilMaterial) Part->SetMaterial(0, PupilMaterial);
}

void AIslandCommonFrog::FindEnvironment()
{
	if (!GetWorld()) return;
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It) { IslandClock = *It; break; }
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It) { Weather = *It; break; }
}

void AIslandCommonFrog::CheckForPlayerDisturbance()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(Player))
	{
		PlayerPresenceLatch.Reset();
		return;
	}

	const FVector Offset = Player->GetActorLocation() - GetActorLocation();
	const float Distance = Offset.Size2D();
	if (PlayerPresenceLatch.Get() == Player &&
		(ShouldRearmFromObserver(Distance, FrogObserverRearmRadiusCm) ||
		FMath::Abs(Offset.Z) > FrogObserverRearmHeightCm))
	{
		PlayerPresenceLatch.Reset();
	}
	if (PlayerPresenceLatch.Get() == Player || !ShouldHopFromObserver(Distance, Offset.Z)) return;
	if (RespondToQuietObservation(Player->GetActorLocation())) PlayerPresenceLatch = Player;
}

void AIslandCommonFrog::CheckForNearbyWildlife()
{
	if (!GetWorld() || StartleCooldownRemaining > 0.f || HopRemaining > 0.f) return;

	if (!CrabPresenceLatch.IsValid()) CrabPresenceLatch.Reset();
	for (TActorIterator<AIslandTidepoolCrab> It(GetWorld()); It; ++It)
	{
		AIslandTidepoolCrab* Crab = *It;
		if (!IsValid(Crab)) continue;
		const bool bScurrying = Crab->IsScurrying();
		const FVector Offset = Crab->GetActorLocation() - GetActorLocation();
		const float Distance = Offset.Size2D();
		if (CrabPresenceLatch.Get() == Crab && (!bScurrying ||
			ShouldRearmFromObserver(Distance, FrogObserverRearmRadiusCm) ||
			FMath::Abs(Offset.Z) > FrogObserverRearmHeightCm))
			CrabPresenceLatch.Reset();
		if (CrabPresenceLatch.Get() == Crab ||
			!ShouldHopFromScurryingCrab(bScurrying, Distance, Offset.Z)) continue;
		if (RespondToQuietObservation(Crab->GetActorLocation()))
		{
			CrabPresenceLatch = Crab;
			return;
		}
	}

	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		ARavenAgentAIController* Raven = *It;
		const APawn* RavenPawn = IsValid(Raven) ? Raven->GetPawn() : nullptr;
		if (!IsValid(RavenPawn)) continue;
		const bool bSettled = Raven->LocomotionState == ERavenLocomotionState::Grounded ||
			Raven->LocomotionState == ERavenLocomotionState::Hopping ||
			Raven->LocomotionState == ERavenLocomotionState::Perched;
		const FVector Offset = RavenPawn->GetActorLocation() - GetActorLocation();
		const float Distance = Offset.Size2D();
		if (RavenPresenceLatch.Get() == Raven && (!bSettled ||
			ShouldRearmFromObserver(Distance, FrogObserverRearmRadiusCm) ||
			FMath::Abs(Offset.Z) > FrogObserverRearmHeightCm))
			RavenPresenceLatch.Reset();
		if (!bSettled || RavenPresenceLatch.Get() == Raven || !ShouldHopFromObserver(Distance, Offset.Z)) continue;
		if (RespondToQuietObservation(RavenPawn->GetActorLocation()))
		{
			RavenPresenceLatch = Raven;
			return;
		}
	}

	for (TActorIterator<ARatAgentAIController> It(GetWorld()); It; ++It)
	{
		ARatAgentAIController* Rat = *It;
		const APawn* RatPawn = IsValid(Rat) ? Rat->GetPawn() : nullptr;
		if (!IsValid(RatPawn)) continue;
		const bool bAwake = Rat->IsAwake();
		const FVector Offset = RatPawn->GetActorLocation() - GetActorLocation();
		const float Distance = Offset.Size2D();
		if (RatPresenceLatch.Get() == Rat && (!bAwake ||
			ShouldRearmFromObserver(Distance, FrogObserverRearmRadiusCm) ||
			FMath::Abs(Offset.Z) > FrogObserverRearmHeightCm))
			RatPresenceLatch.Reset();
		if (!bAwake || RatPresenceLatch.Get() == Rat || !ShouldHopFromObserver(Distance, Offset.Z)) continue;

		FCollisionQueryParams Query(SCENE_QUERY_STAT(CommonFrogRatSight), false, this);
		Query.AddIgnoredActor(RatPawn);
		FHitResult SightHit;
		const FVector EyeLocation = GetActorLocation() + FVector(0.f, 0.f, 12.f);
		const FVector RatEyeLocation = RatPawn->GetActorLocation() + FVector(0.f, 0.f, 16.f);
		const bool bHasLineOfSight = !GetWorld()->LineTraceSingleByChannel(
			SightHit, EyeLocation, RatEyeLocation, ECC_Visibility, Query);
		if (!ShouldHopFromRat(bAwake, bHasLineOfSight, Distance, Offset.Z)) continue;
		if (RespondToQuietObservation(RatPawn->GetActorLocation()))
		{
			RatPresenceLatch = Rat;
			return;
		}
	}

	CheckForNearbyMinnowSurfaceBreak();
}

void AIslandCommonFrog::CheckForNearbyDragonfly()
{
	if (!GetWorld()) return;
	if (DragonflyPresenceLatch.IsValid())
	{
		const FVector Offset = DragonflyPresenceLatch->GetActorLocation() - GetActorLocation();
		if (Offset.Size2D() > 430.f || Offset.Z < 0.f || Offset.Z > 300.f)
			DragonflyPresenceLatch.Reset();
	}
	if (DragonflyPresenceLatch.IsValid() || WildlifeAttentionRemaining > 0.f ||
		WildlifeAttentionCooldownRemaining > 0.f || StartleCooldownRemaining > 0.f || HopRemaining > 0.f) return;

	const FVector EyeLocation = GetActorLocation() + FVector(0.f, 0.f, 8.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CommonFrogDragonflySight), false, this);
	for (TActorIterator<AIslandTideglassDragonfly> It(GetWorld()); It; ++It)
	{
		AIslandTideglassDragonfly* Dragonfly = *It;
		if (!IsValid(Dragonfly)) continue;
		const FVector DragonflyLocation = Dragonfly->GetActorLocation();
		if (!ShouldWatchLowDragonfly(FVector::Dist2D(GetActorLocation(), DragonflyLocation),
			DragonflyLocation.Z - GetActorLocation().Z)) continue;
		FHitResult SightHit;
		if (GetWorld()->LineTraceSingleByChannel(SightHit, EyeLocation, DragonflyLocation, ECC_Visibility, Query) &&
			SightHit.GetActor() != Dragonfly) continue;
		if (RespondToNearbyDragonfly(Dragonfly->GetActorLocation()))
		{
			DragonflyPresenceLatch = Dragonfly;
			return;
		}
	}
}

void AIslandCommonFrog::CheckForNearbyMinnowSurfaceBreak()
{
	if (!GetWorld()) return;
	if (MinnowPresenceLatch.IsValid())
	{
		const FVector Offset = MinnowPresenceLatch->GetActorLocation() - GetActorLocation();
		if ((!MinnowPresenceLatch->IsMakingSurfaceBreak() && WildlifeAttentionRemaining <= 0.f) ||
			!ShouldWatchMinnowSurfaceBreak(Offset.Size2D(), Offset.Z))
			MinnowPresenceLatch.Reset();
	}
	if (MinnowPresenceLatch.IsValid() || DragonflyPresenceLatch.IsValid() || WildlifeAttentionRemaining > 0.f ||
		WildlifeAttentionCooldownRemaining > 0.f || StartleCooldownRemaining > 0.f || HopRemaining > 0.f) return;

	for (TActorIterator<AIslandTidepoolMinnows> It(GetWorld()); It; ++It)
	{
		AIslandTidepoolMinnows* School = *It;
		if (!IsValid(School) || !School->IsMakingSurfaceBreak()) continue;
		const FVector SurfaceLocation = School->GetSurfaceBreakWorldLocation();
		const FVector Offset = SurfaceLocation - GetActorLocation();
		if (!ShouldWatchMinnowSurfaceBreak(Offset.Size2D(), Offset.Z)) continue;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(CommonFrogMinnowSight), false, this);
		Query.AddIgnoredActor(School);
		FHitResult SightHit;
		const FVector EyeLocation = GetActorLocation() + FVector(0.f, 0.f, 8.f);
		if (GetWorld()->LineTraceSingleByChannel(SightHit, EyeLocation, SurfaceLocation, ECC_Visibility, Query) &&
			SightHit.GetActor() != School) continue;
		if (RespondToMinnowSurfaceBreak(SurfaceLocation))
		{
			MinnowPresenceLatch = School;
			return;
		}
	}
}

bool AIslandCommonFrog::ShouldHopFromObserver(float DistanceCm, float VerticalOffsetCm)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && DistanceCm <= FrogObserverRadiusCm &&
		FMath::IsFinite(VerticalOffsetCm) && FMath::Abs(VerticalOffsetCm) <= FrogObserverHeightCm;
}

bool AIslandCommonFrog::ShouldHopFromScurryingCrab(bool bCrabScurrying, float DistanceCm, float VerticalOffsetCm)
{
	return bCrabScurrying && ShouldHopFromObserver(DistanceCm, VerticalOffsetCm);
}

bool AIslandCommonFrog::ShouldHopFromRat(bool bRatAwake, bool bHasLineOfSight, float DistanceCm, float VerticalOffsetCm)
{
	return bRatAwake && bHasLineOfSight && ShouldHopFromObserver(DistanceCm, VerticalOffsetCm);
}

bool AIslandCommonFrog::ShouldRearmFromObserver(float DistanceCm, float RearmRadiusCm)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && FMath::IsFinite(RearmRadiusCm) &&
		RearmRadiusCm > 0.f && DistanceCm > RearmRadiusCm;
}

float AIslandCommonFrog::AmbientActivityScale(float RainIntensity, float NightAmount)
{
	if (!FMath::IsFinite(RainIntensity) || !FMath::IsFinite(NightAmount)) return 0.f;
	return FMath::Clamp(0.18f + 0.50f * FMath::Clamp(RainIntensity, 0.f, 1.f) +
		0.50f * FMath::Clamp(NightAmount, 0.f, 1.f), 0.f, 1.f);
}

bool AIslandCommonFrog::ShouldWatchLowDragonfly(float DistanceCm, float VerticalOffsetCm)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 35.f && DistanceCm <= 280.f &&
		FMath::IsFinite(VerticalOffsetCm) && VerticalOffsetCm >= 45.f && VerticalOffsetCm <= 220.f;
}

bool AIslandCommonFrog::RespondToNearbyDragonfly(const FVector& DragonflyLocation)
{
	if (StartleCooldownRemaining > 0.f || HopRemaining > 0.f || WildlifeAttentionRemaining > 0.f ||
		WildlifeAttentionCooldownRemaining > 0.f || !FMath::IsFinite(DragonflyLocation.X) ||
		!FMath::IsFinite(DragonflyLocation.Y) || !FMath::IsFinite(DragonflyLocation.Z)) return false;
	const FVector Offset = DragonflyLocation - GetActorLocation();
	if (!ShouldWatchLowDragonfly(Offset.Size2D(), Offset.Z)) return false;
	RestingRotation = GetActorRotation();
	WildlifeAttentionLocation = DragonflyLocation;
	// Give nearby players enough time to notice the frog tracking the passing insect.
	WildlifeAttentionRemaining = 2.4f;
	WildlifeAttentionCooldownRemaining = 16.f;
	return true;
}

bool AIslandCommonFrog::ShouldWatchMinnowSurfaceBreak(float DistanceCm, float VerticalOffsetCm)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && DistanceCm <= 1100.f &&
		FMath::IsFinite(VerticalOffsetCm) && FMath::Abs(VerticalOffsetCm) <= 500.f;
}

bool AIslandCommonFrog::RespondToMinnowSurfaceBreak(const FVector& SurfaceBreakLocation)
{
	if (StartleCooldownRemaining > 0.f || HopRemaining > 0.f || WildlifeAttentionRemaining > 0.f ||
		WildlifeAttentionCooldownRemaining > 0.f || !FMath::IsFinite(SurfaceBreakLocation.X) ||
		!FMath::IsFinite(SurfaceBreakLocation.Y) || !FMath::IsFinite(SurfaceBreakLocation.Z)) return false;
	const FVector Offset = SurfaceBreakLocation - GetActorLocation();
	if (!ShouldWatchMinnowSurfaceBreak(Offset.Size2D(), Offset.Z)) return false;
	RestingRotation = GetActorRotation();
	WildlifeAttentionLocation = SurfaceBreakLocation;
	WildlifeAttentionRemaining = 1.8f;
	WildlifeAttentionCooldownRemaining = 12.f;
	return true;
}

float AIslandCommonFrog::HopArcHeight(float Alpha, float MaximumHeightCm)
{
	if (!FMath::IsFinite(Alpha) || !FMath::IsFinite(MaximumHeightCm)) return 0.f;
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	if (ClampedAlpha <= 0.f || ClampedAlpha >= 1.f) return 0.f;
	return FMath::Sin(PI * ClampedAlpha) * FMath::Clamp(MaximumHeightCm, 0.f, 80.f);
}

void AIslandCommonFrog::SetColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color)
{
	if (Material) Material->SetVectorParameterValue(TEXT("Color"), Color);
}

FVector AIslandCommonFrog::ResolveGroundLocation(const FVector& Desired) const
{
	if (!GetWorld()) return HomeLocation;
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CommonFrogGround), false, this);
	const FVector Start(Desired.X, Desired.Y, HomeLocation.Z + 600.f);
	const FVector End(Desired.X, Desired.Y, HomeLocation.Z - 800.f);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Query) && Hit.ImpactNormal.Z >= 0.48f &&
		FMath::Abs(Hit.ImpactPoint.Z - HomeLocation.Z) <= 140.f)
	{
		return Hit.ImpactPoint;
	}
	return HomeLocation;
}

bool AIslandCommonFrog::BeginHop(const FVector& RequestedDirection, float PreferredDistanceCm)
{
	WildlifeAttentionRemaining = 0.f;
	DragonflyPresenceLatch.Reset();
	MinnowPresenceLatch.Reset();
	FVector Direction(RequestedDirection.X, RequestedDirection.Y, 0.f);
	if (!Direction.Normalize()) return false;
	FVector Desired = GetActorLocation() + Direction * FMath::Clamp(PreferredDistanceCm, 45.f, 210.f);
	FVector Offset = Desired - HomeLocation;
	Offset.Z = 0.f;
	if (Offset.Size() > MaximumHomeRadiusCm)
	{
		Desired = HomeLocation + Offset.GetSafeNormal() * MaximumHomeRadiusCm;
	}
	const FVector GroundTarget = ResolveGroundLocation(Desired);
	if (GroundTarget.Equals(GetActorLocation(), 10.f)) return false;

	HopStartLocation = GetActorLocation();
	HopTargetLocation = GroundTarget;
	HopElapsed = 0.f;
	HopDuration = FrogHopDurationSeconds;
	HopRemaining = HopDuration;
	const FVector Travel = HopTargetLocation - HopStartLocation;
	SetActorRotation(FRotator(0.f, Travel.Rotation().Yaw, 0.f));
	return true;
}

bool AIslandCommonFrog::RespondToQuietObservation(const FVector& ObserverLocation)
{
	if (StartleCooldownRemaining > 0.f || HopRemaining > 0.f || !FMath::IsFinite(ObserverLocation.X) ||
		!FMath::IsFinite(ObserverLocation.Y) || !FMath::IsFinite(ObserverLocation.Z)) return false;
	const FVector Offset = ObserverLocation - GetActorLocation();
	if (!ShouldHopFromObserver(Offset.Size2D(), Offset.Z)) return false;
	FVector Away = -Offset;
	if (!Away.Normalize()) Away = FVector(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
	if (!BeginHop(Away, Random.FRandRange(105.f, 170.f))) return false;
	StartleCooldownRemaining = 7.f;
	return true;
}

void AIslandCommonFrog::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Delta = FMath::Max(0.f, FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.f);
	StartleCooldownRemaining = FMath::Max(0.f, StartleCooldownRemaining - Delta);
	WildlifeAttentionCooldownRemaining = FMath::Max(0.f, WildlifeAttentionCooldownRemaining - Delta);
	WildlifeAttentionRemaining = FMath::Max(0.f, WildlifeAttentionRemaining - Delta);
	ObserverCheckRemaining -= Delta;
	if (ObserverCheckRemaining <= 0.f)
	{
		ObserverCheckRemaining = 0.30f;
		CheckForPlayerDisturbance();
		CheckForNearbyWildlife();
	}
	DragonflyCheckRemaining -= Delta;
	if (DragonflyCheckRemaining <= 0.f)
	{
		DragonflyCheckRemaining = Random.FRandRange(0.55f, 0.9f);
		CheckForNearbyDragonfly();
	}

	if (HopRemaining > 0.f)
	{
		HopElapsed = FMath::Min(HopDuration, HopElapsed + Delta);
		const float Alpha = HopDuration > KINDA_SMALL_NUMBER ? HopElapsed / HopDuration : 1.f;
		const FVector Position = FMath::Lerp(HopStartLocation, HopTargetLocation, Alpha) +
			FVector(0.f, 0.f, HopArcHeight(Alpha, FrogHopHeightCm));
		SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
		HopRemaining = FMath::Max(0.f, HopDuration - HopElapsed);
		if (HopRemaining <= 0.f)
		{
			SetActorLocation(HopTargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
			RestingRotation = GetActorRotation();
		}
		return;
	}

	if (WildlifeAttentionRemaining > 0.f)
	{
		const FVector LookTarget = DragonflyPresenceLatch.IsValid()
			? DragonflyPresenceLatch->GetActorLocation() : WildlifeAttentionLocation;
		const FVector ToDragonfly = LookTarget - GetActorLocation();
		FRotator LookRotation = ToDragonfly.Rotation();
		LookRotation.Pitch = 0.f;
		LookRotation.Roll = 0.f;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), LookRotation, Delta, 5.f));
		return;
	}
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), RestingRotation, Delta, 2.5f));

	AmbientHopRemaining -= Delta;
	if (AmbientHopRemaining > 0.f) return;
	const float Rain = Weather.IsValid() && GetWorld() ? Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds()) : 0.f;
	const float Night = IslandClock.IsValid() ? AIslandDayNight::NightAmount(IslandClock->CurrentHour) : 0.f;
	const float Activity = AmbientActivityScale(Rain, Night);
	AmbientHopRemaining = Activity >= 0.42f ? Random.FRandRange(24.f, 48.f) : Random.FRandRange(36.f, 70.f);
	if (Activity < 0.42f) return;
	FVector Direction(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), 0.f);
	if (!Direction.Normalize()) Direction = FVector::ForwardVector;
	BeginHop(Direction, Random.FRandRange(70.f, 135.f));
}
