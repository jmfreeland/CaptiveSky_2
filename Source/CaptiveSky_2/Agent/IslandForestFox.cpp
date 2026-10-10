#include "IslandForestFox.h"

#include "AgentBrainComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AIslandForestFox::AIslandForestFox()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.15f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("WoodlandFox"));

	FoxMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FoxMesh"));
	FoxMesh->SetupAttachment(RootComponent);
	FoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FoxMesh->SetCanEverAffectNavigation(false);
	FoxMesh->SetGenerateOverlapEvents(false);
	FoxMesh->SetCastShadow(true);
	FoxMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Meshes/SK_Fox.SK_Fox"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_IdleBreathe.ANIM_Fox_IdleBreathe"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> LookAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_IdleLookAround.ANIM_Fox_IdleLookAround"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_Walk.ANIM_Fox_Walk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_Run.ANIM_Fox_Run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SleepAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_Sleeping.ANIM_Fox_Sleeping"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WakeAsset(
		TEXT("/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_RestToGoBackUp.ANIM_Fox_RestToGoBackUp"));

	if (MeshAsset.Succeeded()) FoxMesh->SetSkeletalMesh(MeshAsset.Object);
	if (IdleAsset.Succeeded()) IdleAnimation = IdleAsset.Object;
	if (LookAsset.Succeeded()) LookAroundAnimation = LookAsset.Object;
	if (WalkAsset.Succeeded()) WalkAnimation = WalkAsset.Object;
	if (RunAsset.Succeeded()) RunAnimation = RunAsset.Object;
	if (SleepAsset.Succeeded()) SleepAnimation = SleepAsset.Object;
	if (WakeAsset.Succeeded()) WakeAnimation = WakeAsset.Object;
}

void AIslandForestFox::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	BeginForaging();
}

void AIslandForestFox::CheckForNearbyResident()
{
	UWorld* World = GetWorld();
	if (!World) return;

	constexpr float NoticeRadius = 600.f;
	constexpr float RearmRadius = 850.f;
	constexpr float QuietMovementSpeed = 180.f;
	bool bResidentWithinRearmRange = false;
	AActor* QuietResident = nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == this ||
			!Candidate->FindComponentByClass<UAgentBrainComponent>())
			continue;

		const FVector Offset = Candidate->GetActorLocation() - GetActorLocation();
		const float DistanceSquared = Offset.SizeSquared2D();
		if (DistanceSquared > FMath::Square(RearmRadius)) continue;
		bResidentWithinRearmRange = true;
		if (DistanceSquared > FMath::Square(NoticeRadius) ||
			Candidate->GetVelocity().SizeSquared2D() > FMath::Square(QuietMovementSpeed))
			continue;

		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandForestFoxResidentNotice), false, this);
		FHitResult Hit;
		const FVector FoxEye = GetActorLocation() + FVector(0.f, 0.f, 70.f);
		const FVector ResidentHead = Candidate->GetActorLocation() + FVector(0.f, 0.f, 65.f);
		if (World->LineTraceSingleByChannel(Hit, FoxEye, ResidentHead, ECC_Visibility, Query) &&
			Hit.GetActor() != Candidate)
			continue;

		QuietResident = Candidate;
		break;
	}

	if (!bResidentWithinRearmRange) bResidentPresenceNearby = false;
	if (!QuietResident || bResidentPresenceNearby || bResting || bWaking ||
		ResidentPresenceCooldownRemaining > 0.f)
		return;
	if (bNoticing)
	{
		// A nearby resident group that arrives during an already-started wildlife
		// glance belongs to the same encounter; don't redirect the retreat to a
		// different observer on the fox's next scheduled presence check.
		bResidentPresenceNearby = true;
		ResidentPresenceCooldownRemaining = 8.f;
		return;
	}

	// One nearby group earns a single wary look and retreat, with no repeated
	// response until the residents leave the wider woodland-edge area.
	if (RespondToQuietObservation(QuietResident->GetActorLocation()))
	{
		bResidentPresenceNearby = true;
		ResidentPresenceCooldownRemaining = 8.f;
	}
}

void AIslandForestFox::PlayLoop(UAnimSequence* Animation)
{
	if (!FoxMesh || !Animation) return;
	FoxMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	FoxMesh->PlayAnimation(Animation, true);
}

void AIslandForestFox::BeginForaging()
{
	bMoving = false;
	bNoticing = false;
	bStartled = false;
	bWaking = false;
	MoveSpeed = 0.f;
	NoticeRemaining = 0.f;
	MoveRemaining = 0.f;
	ActivityRemaining = FMath::FRandRange(4.f, 9.f);
	PlayLoop(IdleAnimation);
}

bool AIslandForestFox::FindGround(const FVector& NearPoint, FVector& OutGround) const
{
	UWorld* World = GetWorld();
	if (!World) return false;
	const FVector Start(NearPoint.X, NearPoint.Y, NearPoint.Z + 900.f);
	const FVector End(NearPoint.X, NearPoint.Y, NearPoint.Z - 1400.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandForestFoxGround), false, this);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Query) || Hit.ImpactNormal.Z < 0.72f)
		return false;
	OutGround = Hit.ImpactPoint;
	return true;
}

bool AIslandForestFox::ChooseForageTarget(FVector& OutTarget) const
{
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(220.f, 560.f);
		const FVector Candidate = HomeLocation + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		FVector Ground;
		if (!FindGround(Candidate, Ground) || FMath::Abs(Ground.Z - HomeLocation.Z) > 220.f) continue;
		OutTarget = Ground + FVector(0.f, 0.f, 2.f);
		return true;
	}
	return false;
}

void AIslandForestFox::StartMove(const FVector& Target, bool bRun)
{
	if (bResting) return;
	bWaking = false;
	bNoticing = false;
	TargetLocation = Target;
	bMoving = true;
	bStartled = bRun;
	MoveSpeed = bRun ? 250.f : 62.f;
	MoveRemaining = bRun ? 2.0f : 0.f;
	ActivityRemaining = 0.f;
	PlayLoop(bRun ? RunAnimation : WalkAnimation);
}

bool AIslandForestFox::RespondToQuietObservation(const FVector& ObserverLocation)
{
	if (bResting || bWaking) return false;
	FVector Away = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = GetActorRightVector();
	FVector Desired = GetActorLocation() + Away * 360.f;
	FVector FromHome = Desired - HomeLocation;
	FromHome.Z = 0.f;
	if (FromHome.Size2D() > 650.f)
		Desired = HomeLocation + FromHome.GetSafeNormal2D() * 650.f;
	FVector Ground;
	if (FindGround(Desired, Ground)) Desired = Ground + FVector(0.f, 0.f, 2.f);
	else
	{
		FVector LocalForageTarget;
		if (!ChooseForageTarget(LocalForageTarget)) return false;
		Desired = LocalForageTarget;
	}
	bMoving = false;
	bStartled = false;
	MoveSpeed = 0.f;
	TargetLocation = Desired;
	if (LookAroundAnimation)
	{
		bNoticing = true;
		NoticeRemaining = FMath::Clamp(LookAroundAnimation->GetPlayLength(), 0.5f, 2.0f);
		FoxMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		FoxMesh->PlayAnimation(LookAroundAnimation, false);
	}
	else StartMove(TargetLocation, true);
	return true;
}

void AIslandForestFox::SetResting(bool bShouldRest)
{
	if (bResting == bShouldRest) return;
	bResting = bShouldRest;
	bMoving = false;
	bNoticing = false;
	bStartled = false;
	bWaking = false;
	MoveSpeed = 0.f;
	MoveRemaining = 0.f;
	if (bResting)
	{
		PlayLoop(SleepAnimation);
	}
	else if (WakeAnimation)
	{
		bWaking = true;
		ActivityRemaining = FMath::Max(0.05f, WakeAnimation->GetPlayLength());
		FoxMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		FoxMesh->PlayAnimation(WakeAnimation, false);
	}
	else
	{
		BeginForaging();
	}
}

void AIslandForestFox::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Delta = FMath::Max(0.f, DeltaSeconds);
	ResidentPresenceCooldownRemaining = FMath::Max(0.f, ResidentPresenceCooldownRemaining - Delta);
	ResidentPresenceCheckRemaining -= Delta;
	if (ResidentPresenceCheckRemaining <= 0.f)
	{
		ResidentPresenceCheckRemaining = 0.35f;
		CheckForNearbyResident();
	}
	if (bResting) return;
	if (bWaking)
	{
		ActivityRemaining -= Delta;
		if (ActivityRemaining <= 0.f) BeginForaging();
		return;
	}
	if (bNoticing)
	{
		NoticeRemaining -= Delta;
		if (NoticeRemaining <= 0.f) StartMove(TargetLocation, true);
		return;
	}
	if (!bMoving)
	{
		ActivityRemaining -= Delta;
		if (ActivityRemaining <= 0.f)
		{
			FVector Destination;
			if (ChooseForageTarget(Destination))
				StartMove(Destination, false);
			else
				ActivityRemaining = 2.f;
		}
		return;
	}

	const FVector Current = GetActorLocation();
	const FVector ToTarget = TargetLocation - Current;
	if (ToTarget.SizeSquared2D() <= FMath::Square(24.f) ||
		(MoveRemaining > 0.f && (MoveRemaining -= Delta) <= 0.f))
	{
		BeginForaging();
		return;
	}
	const FVector Direction = ToTarget.GetSafeNormal2D();
	FVector Ground;
	const FVector Desired = Current + Direction * MoveSpeed * Delta;
	if (!FindGround(Desired, Ground) || FMath::Abs(Ground.Z - HomeLocation.Z) > 220.f)
	{
		BeginForaging();
		return;
	}
	SetActorLocation(Ground + FVector(0.f, 0.f, 2.f), false);
	// The imported fox mesh faces along +Y in its source preview.
	SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw - 90.f, 0.f));
}
