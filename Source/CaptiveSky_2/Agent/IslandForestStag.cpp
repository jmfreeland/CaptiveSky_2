#include "IslandForestStag.h"

#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "IslandLightning.h"
#include "RavenAgentAIController.h"
#include "UObject/ConstructorHelpers.h"

AIslandForestStag::AIslandForestStag()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("WoodlandDeer"));

	DeerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DeerMesh"));
	DeerMesh->SetupAttachment(RootComponent);
	DeerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeerMesh->SetCanEverAffectNavigation(false);
	DeerMesh->SetGenerateOverlapEvents(false);
	DeerMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
		TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Meshes/SK_DeerStag.SK_DeerStag"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GrazeAsset(
		TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Animations/ANIM_DeerStag_IdleGraze.ANIM_DeerStag_IdleGraze"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAsset(
		TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Animations/ANIM_DeerStag_Walk.ANIM_DeerStag_Walk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAsset(
		TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Animations/ANIM_DeerStag_Run.ANIM_DeerStag_Run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SleepAsset(
		TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Animations/ANIM_DeerStag_Sleep.ANIM_DeerStag_Sleep"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WakeAsset(
		TEXT("/Game/AnimalVarietyPack/DeerStagAndDoe/Animations/ANIM_DeerStag_SleepToGoBackUp.ANIM_DeerStag_SleepToGoBackUp"));

	if (MeshAsset.Succeeded()) DeerMesh->SetSkeletalMesh(MeshAsset.Object);
	if (GrazeAsset.Succeeded()) GrazeAnimation = GrazeAsset.Object;
	if (WalkAsset.Succeeded()) WalkAnimation = WalkAsset.Object;
	if (RunAsset.Succeeded()) RunAnimation = RunAsset.Object;
	if (SleepAsset.Succeeded()) SleepAnimation = SleepAsset.Object;
	if (WakeAsset.Succeeded()) WakeAnimation = WakeAsset.Object;
	DeerMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void AIslandForestStag::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	BeginGrazing();
}

void AIslandForestStag::PlayLoop(UAnimSequence* Animation)
{
	if (!DeerMesh || !Animation) return;
	DeerMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	DeerMesh->PlayAnimation(Animation, true);
}

void AIslandForestStag::BeginGrazing()
{
	bMoving = false;
	bWakingUp = false;
	bStartled = false;
	MoveSpeed = 0.f;
	WakeRemaining = 0.f;
	ActivityRemaining = FMath::FRandRange(7.f, 15.f);
	PlayLoop(GrazeAnimation);
}

void AIslandForestStag::CheckForNearbyThunder()
{
	if (!GetWorld()) return;

	constexpr float AudibleThunderRadius = 120000.f;
	for (TActorIterator<AIslandLightning> It(GetWorld()); It; ++It)
	{
		if (LastHeardThunder.Get() == *It || !It->HasThunderReached(GetActorLocation())) continue;
		const FVector StrikeLocation = It->GetStrikeGroundLocation();
		if (FVector::DistSquared2D(GetActorLocation(), StrikeLocation) > FMath::Square(AudibleThunderRadius)) continue;

		LastHeardThunder = *It;
		if (bResting) SetResting(false);
		RespondToQuietObservation(StrikeLocation);
		return;
	}
}

void AIslandForestStag::CheckForNearbyRavenFlyby()
{
	if (!GetWorld() || bResting || bWakingUp || bMoving || bStartled || RavenFlybyCooldownRemaining > 0.f) return;

	constexpr float FlybyRadius = 700.f;
	constexpr float MinimumHeight = 180.f;
	constexpr float MaximumHeight = 800.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		if (It->LocomotionState != ERavenLocomotionState::Flying) continue;
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		if (Offset.Z < MinimumHeight || Offset.Z > MaximumHeight ||
			Offset.SizeSquared2D() > FMath::Square(FlybyRadius))
			continue;

		// A low wing shadow briefly startles the stag; the existing bounded run returns it
		// to its ordinary grazing routine without treating the raven as a predator target.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 12.f;
		return;
	}
}

bool AIslandForestStag::FindGround(const FVector& NearPoint, FVector& OutGround) const
{
	UWorld* World = GetWorld();
	if (!World) return false;
	const FVector Start(NearPoint.X, NearPoint.Y, NearPoint.Z + 900.f);
	const FVector End(NearPoint.X, NearPoint.Y, NearPoint.Z - 1400.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandForestStagGround), false, this);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Query) || Hit.ImpactNormal.Z < 0.72f)
		return false;
	OutGround = Hit.ImpactPoint;
	return true;
}

bool AIslandForestStag::ChooseWanderTarget(FVector& OutTarget) const
{
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(280.f, 700.f);
		const FVector Candidate = HomeLocation + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		FVector Ground;
		if (!FindGround(Candidate, Ground) || FMath::Abs(Ground.Z - HomeLocation.Z) > 260.f) continue;
		OutTarget = Ground + FVector(0.f, 0.f, 2.f);
		return true;
	}
	return false;
}

void AIslandForestStag::StartMove(const FVector& Target, bool bRun)
{
	if (bResting) return;
	bWakingUp = false;
	WakeRemaining = 0.f;
	TargetLocation = Target;
	bMoving = true;
	bStartled = bRun;
	MoveSpeed = bRun ? 300.f : 78.f;
	ActivityRemaining = bRun ? 1.15f : 0.f;
	PlayLoop(bRun ? RunAnimation : WalkAnimation);
}

void AIslandForestStag::RespondToQuietObservation(const FVector& ObserverLocation)
{
	if (bResting) return;
	FVector Away = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = GetActorRightVector();
	FVector Ground;
	FVector Desired = GetActorLocation() + Away * 360.f;
	FVector HomeOffset = Desired - HomeLocation;
	HomeOffset.Z = 0.f;
	if (HomeOffset.Size2D() > 700.f)
		Desired = HomeLocation + HomeOffset.GetSafeNormal2D() * 700.f;
	if (FindGround(Desired, Ground)) StartMove(Ground + FVector(0.f, 0.f, 2.f), true);
	else
	{
		FVector Wander;
		if (ChooseWanderTarget(Wander)) StartMove(Wander, true);
	}
}

void AIslandForestStag::SetResting(bool bShouldRest)
{
	if (bResting == bShouldRest) return;
	bResting = bShouldRest;
	bMoving = false;
	bWakingUp = false;
	bStartled = false;
	MoveSpeed = 0.f;
	ActivityRemaining = 0.f;
	WakeRemaining = 0.f;
	if (bResting)
	{
		PlayLoop(SleepAnimation);
	}
	else if (WakeAnimation)
	{
		bWakingUp = true;
		WakeRemaining = FMath::Max(0.05f, WakeAnimation->GetPlayLength());
		if (DeerMesh)
		{
			DeerMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			DeerMesh->PlayAnimation(WakeAnimation, false);
		}
	}
	else
	{
		BeginGrazing();
	}
}

void AIslandForestStag::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()) return;
	const float Delta = FMath::Max(0.f, DeltaSeconds);
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - Delta);
	ThunderCheckRemaining -= Delta;
	if (ThunderCheckRemaining <= 0.f)
	{
		ThunderCheckRemaining = 0.35f;
		CheckForNearbyThunder();
	}
	RavenCheckRemaining -= Delta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.35f;
		CheckForNearbyRavenFlyby();
	}
	if (bResting) return;
	if (bWakingUp)
	{
		WakeRemaining -= Delta;
		if (WakeRemaining <= 0.f) BeginGrazing();
		return;
	}
	if (!bMoving)
	{
		ActivityRemaining -= Delta;
		if (ActivityRemaining <= 0.f)
		{
			FVector Destination;
			if (ChooseWanderTarget(Destination)) StartMove(Destination, false);
			else ActivityRemaining = 2.f;
		}
		return;
	}

	const FVector Current = GetActorLocation();
	const FVector ToTarget = TargetLocation - Current;
	if (ToTarget.SizeSquared2D() <= FMath::Square(24.f) ||
		(ActivityRemaining > 0.f && (ActivityRemaining -= Delta) <= 0.f))
	{
		BeginGrazing();
		return;
	}
	const FVector Direction = ToTarget.GetSafeNormal2D();
	FVector Ground;
	const FVector Desired = Current + Direction * MoveSpeed * Delta;
	if (!FindGround(Desired, Ground) || FMath::Abs(Ground.Z - HomeLocation.Z) > 260.f)
	{
		BeginGrazing();
		return;
	}
	SetActorLocation(Ground + FVector(0.f, 0.f, 2.f), false);
	const float MeshForwardYawOffset = -90.f;
	SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw + MeshForwardYawOffset, 0.f));
}
