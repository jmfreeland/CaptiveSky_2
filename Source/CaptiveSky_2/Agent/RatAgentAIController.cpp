#include "RatAgentAIController.h"

#include "Animation/AnimSequence.h"
#include "AgentMemoryComponent.h"
#include "AgentConsolidationComponent.h"
#include "AutonomousAgentCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "IslandInnkeeperSubsystem.h"
#include "IslandListeningStonesChime.h"
#include "IslandTidepoolPeriwinkle.h"
#include "RavenAgentAIController.h"

namespace
{
	constexpr float WalkSpeedThreshold = 12.f;
	constexpr float RunSpeedThreshold = 105.f;
	constexpr float IdleLookInterval = 7.5f;
	constexpr float IdleLookJitter = 2.5f;
	constexpr float RavenScanInterval = 0.8f;
	constexpr float RavenAttentionDuration = 1.7f;
	constexpr float RavenAttentionCooldown = 16.f;
	constexpr float StoneChimeAttentionDuration = 1.7f;
	constexpr float StoneChimeAttentionCooldown = 16.f;
	constexpr float PeriwinkleAttentionDuration = 2.2f;
	constexpr float PeriwinkleAttentionCooldown = 24.f;
	constexpr float InnkeeperScanInterval = 1.4f;
	constexpr float InnkeeperAttentionDuration = 1.8f;
	constexpr float InnkeeperAttentionCooldown = 28.f;

	UAnimSequence* LoadFoxAnimation(const TCHAR* AssetName)
	{
		const FString Path = FString::Printf(TEXT("/Game/AnimalVarietyPack/Fox/Animations/%s.%s"), AssetName, AssetName);
		return LoadObject<UAnimSequence>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

ARatAgentAIController::ARatAgentAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

ERatLocomotionMode ARatAgentAIController::SelectLocomotionMode(float HorizontalSpeed, bool bSleeping)
{
	if (bSleeping) return ERatLocomotionMode::Sleeping;
	if (!FMath::IsFinite(HorizontalSpeed) || HorizontalSpeed < WalkSpeedThreshold) return ERatLocomotionMode::Idle;
	return HorizontalSpeed >= RunSpeedThreshold ? ERatLocomotionMode::Running : ERatLocomotionMode::Walking;
}

bool ARatAgentAIController::ShouldBeginRavenNotice(float DistanceCm, bool bHasLineOfSight,
	bool bRatBusy, bool bRatResting, bool bRavenResting, bool bAlreadyNoticing)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && DistanceCm <= RavenNoticeRadius &&
		bHasLineOfSight && !bRatBusy && !bRatResting && !bRavenResting && !bAlreadyNoticing;
}

bool ARatAgentAIController::ShouldBeginStoneChimeNotice(float DistanceCm, bool bChimeAudible,
	bool bRatBusy, bool bRatResting, bool bAlreadyNoticing)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f &&
		DistanceCm <= AIslandListeningStonesChime::AudibleRadius && bChimeAudible &&
		!bRatBusy && !bRatResting && !bAlreadyNoticing;
}

bool ARatAgentAIController::ShouldBeginPeriwinkleNotice(float DistanceCm, bool bHasLineOfSight,
	bool bRatBusy, bool bRatResting, bool bAlreadyNoticing)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && DistanceCm <= PeriwinkleNoticeRadius &&
		bHasLineOfSight && !bRatBusy && !bRatResting && !bAlreadyNoticing;
}

bool ARatAgentAIController::ShouldBeginInnkeeperNotice(float DistanceCm, bool bHasLineOfSight,
	bool bRatBusy, bool bRatResting, bool bInnkeeperResting, bool bAlreadyNoticing)
{
	return FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && DistanceCm <= InnkeeperNoticeRadius &&
		bHasLineOfSight && !bRatBusy && !bRatResting && !bInnkeeperResting && !bAlreadyNoticing;
}

void ARatAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RavenScanRemaining = 0.f;
	RavenAttentionRemaining = 0.f;
	RavenAttentionCooldownRemaining = 0.f;
	RavenAttentionTarget = FVector::ZeroVector;
	StoneChimeScanRemaining = 0.f;
	StoneChimeAttentionRemaining = 0.f;
	StoneChimeAttentionCooldownRemaining = 0.f;
	StoneChimeAttentionTarget = FVector::ZeroVector;
	PeriwinkleScanRemaining = 0.f;
	PeriwinkleAttentionRemaining = 0.f;
	PeriwinkleAttentionCooldownRemaining = 0.f;
	PeriwinkleAttentionTarget = FVector::ZeroVector;
	InnkeeperScanRemaining = 0.f;
	InnkeeperAttentionRemaining = 0.f;
	InnkeeperAttentionCooldownRemaining = 0.f;
	InnkeeperAttentionTarget = FVector::ZeroVector;
	CacheAnimations(InPawn);
	UpdateAnimation(0.f);
}

void ARatAgentAIController::OnUnPossess()
{
	RatBody.Reset();
	IdleAnimation = nullptr;
	LookAroundAnimation = nullptr;
	WalkAnimation = nullptr;
	RunAnimation = nullptr;
	SleepAnimation = nullptr;
	CurrentAnimation = nullptr;
	RavenScanRemaining = 0.f;
	RavenAttentionRemaining = 0.f;
	RavenAttentionCooldownRemaining = 0.f;
	RavenAttentionTarget = FVector::ZeroVector;
	StoneChimeScanRemaining = 0.f;
	StoneChimeAttentionRemaining = 0.f;
	StoneChimeAttentionCooldownRemaining = 0.f;
	StoneChimeAttentionTarget = FVector::ZeroVector;
	PeriwinkleScanRemaining = 0.f;
	PeriwinkleAttentionRemaining = 0.f;
	PeriwinkleAttentionCooldownRemaining = 0.f;
	PeriwinkleAttentionTarget = FVector::ZeroVector;
	InnkeeperScanRemaining = 0.f;
	InnkeeperAttentionRemaining = 0.f;
	InnkeeperAttentionCooldownRemaining = 0.f;
	InnkeeperAttentionTarget = FVector::ZeroVector;
	Super::OnUnPossess();
}

void ARatAgentAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateRavenAttention(DeltaSeconds);
	UpdateStoneChimeAttention(DeltaSeconds);
	UpdatePeriwinkleAttention(DeltaSeconds);
	UpdateInnkeeperAttention(DeltaSeconds);
	UpdateAnimation(DeltaSeconds);
}

void ARatAgentAIController::UpdateInnkeeperAttention(float DeltaSeconds)
{
	APawn* Rat = GetPawn();
	UWorld* World = GetWorld();
	if (!Rat || !World) return;

	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	InnkeeperAttentionCooldownRemaining = FMath::Max(0.f, InnkeeperAttentionCooldownRemaining - SafeDelta);
	InnkeeperAttentionRemaining = FMath::Max(0.f, InnkeeperAttentionRemaining - SafeDelta);
	if (InnkeeperAttentionRemaining > 0.f)
	{
		if (IsActionInProgress() || IsResting())
		{
			InnkeeperAttentionRemaining = 0.f;
			return;
		}

		const FVector Direction = InnkeeperAttentionTarget - Rat->GetActorLocation();
		if (!Direction.IsNearlyZero())
		{
			FRotator Facing = Direction.Rotation();
			Facing.Pitch = 0.f;
			Facing.Roll = 0.f;
			Rat->SetActorRotation(FMath::RInterpTo(Rat->GetActorRotation(), Facing, SafeDelta, 2.f));
		}
		return;
	}

	if (RavenAttentionRemaining > 0.f || StoneChimeAttentionRemaining > 0.f ||
		PeriwinkleAttentionRemaining > 0.f || InnkeeperAttentionCooldownRemaining > 0.f)
	{
		InnkeeperScanRemaining = FMath::Max(InnkeeperScanRemaining,
			FMath::Max(FMath::Max(RavenAttentionRemaining, StoneChimeAttentionRemaining), PeriwinkleAttentionRemaining));
		return;
	}
	InnkeeperScanRemaining -= SafeDelta;
	if (InnkeeperScanRemaining > 0.f) return;
	InnkeeperScanRemaining = InnkeeperScanInterval;
	if (IsActionInProgress() || IsResting()) return;

	const UIslandInnkeeperSubsystem* InnkeeperSubsystem = World->GetSubsystem<UIslandInnkeeperSubsystem>();
	if (!InnkeeperSubsystem || InnkeeperSubsystem->AgentId.IsEmpty()) return;

	AAutonomousAgentCharacter* BestInnkeeper = nullptr;
	float BestDistanceSquared = FMath::Square(InnkeeperNoticeRadius);
	for (TActorIterator<AAutonomousAgentCharacter> It(World); It; ++It)
	{
		AAutonomousAgentCharacter* Resident = *It;
		if (!IsValid(Resident) || Resident == Rat || !Resident->Memory ||
			Resident->Memory->GetResolvedAgentId() != InnkeeperSubsystem->AgentId)
			continue;

		const UAgentConsolidationComponent* ResidentRest = Resident->FindComponentByClass<UAgentConsolidationComponent>();
		const bool bInnkeeperResting = ResidentRest && !ResidentRest->IsAwake();
		const float DistanceSquared = FVector::DistSquared(Rat->GetActorLocation(), Resident->GetActorLocation());
		if (DistanceSquared > BestDistanceSquared || !ShouldBeginInnkeeperNotice(
			FMath::Sqrt(DistanceSquared), true, false, false, bInnkeeperResting, false))
			continue;

		FCollisionQueryParams Query(SCENE_QUERY_STAT(RatNoticesInnkeeper), false, Rat);
		Query.AddIgnoredActor(Resident);
		FHitResult Hit;
		const FVector Start = Rat->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const FVector End = Resident->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const bool bHasLineOfSight = !World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
		if (!ShouldBeginInnkeeperNotice(FMath::Sqrt(DistanceSquared), bHasLineOfSight,
			false, false, bInnkeeperResting, false))
			continue;

		BestDistanceSquared = DistanceSquared;
		BestInnkeeper = Resident;
	}

	if (!BestInnkeeper) return;
	InnkeeperAttentionTarget = BestInnkeeper->GetActorLocation();
	InnkeeperAttentionRemaining = InnkeeperAttentionDuration;
	InnkeeperAttentionCooldownRemaining = InnkeeperAttentionCooldown;
	IdleLookTimer = 0.f;
}

void ARatAgentAIController::UpdatePeriwinkleAttention(float DeltaSeconds)
{
	APawn* Rat = GetPawn();
	UWorld* World = GetWorld();
	if (!Rat || !World) return;

	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	PeriwinkleAttentionCooldownRemaining = FMath::Max(0.f, PeriwinkleAttentionCooldownRemaining - SafeDelta);
	PeriwinkleAttentionRemaining = FMath::Max(0.f, PeriwinkleAttentionRemaining - SafeDelta);
	if (PeriwinkleAttentionRemaining > 0.f)
	{
		if (IsActionInProgress() || IsResting())
		{
			PeriwinkleAttentionRemaining = 0.f;
			return;
		}

		const FVector Direction = PeriwinkleAttentionTarget - Rat->GetActorLocation();
		if (!Direction.IsNearlyZero())
		{
			FRotator Facing = Direction.Rotation();
			Facing.Pitch = 0.f;
			Facing.Roll = 0.f;
			Rat->SetActorRotation(FMath::RInterpTo(Rat->GetActorRotation(), Facing, SafeDelta, 2.f));
		}
		return;
	}

	if (RavenAttentionRemaining > 0.f || StoneChimeAttentionRemaining > 0.f ||
		InnkeeperAttentionRemaining > 0.f || PeriwinkleAttentionCooldownRemaining > 0.f)
	{
		PeriwinkleScanRemaining = FMath::Max(PeriwinkleScanRemaining,
			FMath::Max(RavenAttentionRemaining, StoneChimeAttentionRemaining));
		return;
	}
	PeriwinkleScanRemaining -= SafeDelta;
	if (PeriwinkleScanRemaining > 0.f) return;
	PeriwinkleScanRemaining = 1.2f;
	if (IsActionInProgress() || IsResting()) return;

	AIslandTidepoolPeriwinkle* BestSnail = nullptr;
	float BestDistanceSquared = FMath::Square(PeriwinkleNoticeRadius);
	for (TActorIterator<AIslandTidepoolPeriwinkle> It(World); It; ++It)
	{
		AIslandTidepoolPeriwinkle* Snail = *It;
		if (!IsValid(Snail)) continue;
		const float DistanceSquared = FVector::DistSquared2D(Rat->GetActorLocation(), Snail->GetActorLocation());
		if (DistanceSquared > BestDistanceSquared || !ShouldBeginPeriwinkleNotice(
			FMath::Sqrt(DistanceSquared), true, false, false, false)) continue;

		FCollisionQueryParams Query(SCENE_QUERY_STAT(RatNoticesPeriwinkle), false, Rat);
		Query.AddIgnoredActor(Snail);
		FHitResult Hit;
		const FVector Start = Rat->GetActorLocation() + FVector(0.f, 0.f, 24.f);
		const FVector End = Snail->GetActorLocation() + FVector(0.f, 0.f, 8.f);
		const bool bHasLineOfSight = !World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
		if (!ShouldBeginPeriwinkleNotice(FMath::Sqrt(DistanceSquared), bHasLineOfSight,
			false, false, false)) continue;
		BestDistanceSquared = DistanceSquared;
		BestSnail = Snail;
	}

	if (!BestSnail) return;
	PeriwinkleAttentionTarget = BestSnail->GetActorLocation();
	PeriwinkleAttentionRemaining = PeriwinkleAttentionDuration;
	PeriwinkleAttentionCooldownRemaining = PeriwinkleAttentionCooldown;
	IdleLookTimer = 0.f;
}

void ARatAgentAIController::UpdateRavenAttention(float DeltaSeconds)
{
	APawn* Rat = GetPawn();
	UWorld* World = GetWorld();
	if (!Rat || !World) return;

	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	RavenAttentionCooldownRemaining = FMath::Max(0.f, RavenAttentionCooldownRemaining - SafeDelta);
	RavenAttentionRemaining = FMath::Max(0.f, RavenAttentionRemaining - SafeDelta);
	if (StoneChimeAttentionRemaining > 0.f || PeriwinkleAttentionRemaining > 0.f || InnkeeperAttentionRemaining > 0.f)
	{
		RavenScanRemaining = FMath::Max(RavenScanRemaining, SafeDelta);
		return;
	}
	if (RavenAttentionRemaining > 0.f)
	{
		if (IsActionInProgress() || IsResting())
		{
			RavenAttentionRemaining = 0.f;
			return;
		}

		const FVector Direction = RavenAttentionTarget - Rat->GetActorLocation();
		if (!Direction.IsNearlyZero())
		{
			FRotator Facing = Direction.Rotation();
			Facing.Pitch = 0.f;
			Facing.Roll = 0.f;
			Rat->SetActorRotation(FMath::RInterpTo(Rat->GetActorRotation(), Facing, SafeDelta, 2.5f));
		}
		return;
	}

	if (RavenAttentionCooldownRemaining > 0.f)
	{
		RavenScanRemaining = FMath::Max(RavenScanRemaining, RavenAttentionCooldownRemaining);
		return;
	}
	RavenScanRemaining -= SafeDelta;
	if (RavenScanRemaining > 0.f) return;
	RavenScanRemaining = RavenScanInterval;
	if (IsActionInProgress() || IsResting()) return;

	ARavenAgentAIController* BestRaven = nullptr;
	APawn* BestRavenBody = nullptr;
	float BestDistanceSquared = FMath::Square(RavenNoticeRadius);
	for (TActorIterator<ARavenAgentAIController> It(World); It; ++It)
	{
		ARavenAgentAIController* Raven = *It;
		APawn* RavenBody = IsValid(Raven) ? Raven->GetPawn() : nullptr;
		if (!RavenBody) continue;
		const UAgentConsolidationComponent* RavenRest = RavenBody->FindComponentByClass<UAgentConsolidationComponent>();
		const bool bRavenResting = RavenRest && !RavenRest->IsAwake();
		const float DistanceSquared = FVector::DistSquared(Rat->GetActorLocation(), RavenBody->GetActorLocation());
		if (DistanceSquared > BestDistanceSquared) continue;

		FCollisionQueryParams Query(SCENE_QUERY_STAT(RatNoticesRaven), false, Rat);
		Query.AddIgnoredActor(RavenBody);
		FHitResult Hit;
		const FVector Start = Rat->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const FVector End = RavenBody->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const bool bHasLineOfSight = !World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
		if (!ShouldBeginRavenNotice(FMath::Sqrt(DistanceSquared), bHasLineOfSight,
			false, false, bRavenResting, false)) continue;

		BestDistanceSquared = DistanceSquared;
		BestRaven = Raven;
		BestRavenBody = RavenBody;
	}

	if (!BestRaven || !BestRavenBody) return;
	RavenAttentionTarget = BestRavenBody->GetActorLocation();
	RavenAttentionRemaining = RavenAttentionDuration;
	RavenAttentionCooldownRemaining = RavenAttentionCooldown;
	IdleLookTimer = 0.f;
}

void ARatAgentAIController::UpdateStoneChimeAttention(float DeltaSeconds)
{
	APawn* Rat = GetPawn();
	UWorld* World = GetWorld();
	if (!Rat || !World) return;

	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	StoneChimeAttentionCooldownRemaining = FMath::Max(0.f, StoneChimeAttentionCooldownRemaining - SafeDelta);
	StoneChimeAttentionRemaining = FMath::Max(0.f, StoneChimeAttentionRemaining - SafeDelta);
	if (StoneChimeAttentionRemaining > 0.f)
	{
		if (IsActionInProgress() || IsResting())
		{
			StoneChimeAttentionRemaining = 0.f;
			return;
		}

		const FVector Direction = StoneChimeAttentionTarget - Rat->GetActorLocation();
		if (!Direction.IsNearlyZero())
		{
			FRotator Facing = Direction.Rotation();
			Facing.Pitch = 0.f;
			Facing.Roll = 0.f;
			Rat->SetActorRotation(FMath::RInterpTo(Rat->GetActorRotation(), Facing, SafeDelta, 2.5f));
		}
		return;
	}

	if (RavenAttentionRemaining > 0.f || PeriwinkleAttentionRemaining > 0.f || InnkeeperAttentionRemaining > 0.f ||
		StoneChimeAttentionCooldownRemaining > 0.f)
	{
		StoneChimeScanRemaining = FMath::Max(RavenAttentionRemaining,
			StoneChimeAttentionCooldownRemaining);
		return;
	}
	StoneChimeScanRemaining -= SafeDelta;
	if (StoneChimeScanRemaining > 0.f) return;
	StoneChimeScanRemaining = 0.8f;
	if (IsActionInProgress() || IsResting()) return;

	AIslandListeningStonesChime* BestChime = nullptr;
	float BestDistanceSquared = FMath::Square(AIslandListeningStonesChime::AudibleRadius);
	for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
	{
		AIslandListeningStonesChime* Chime = *It;
		if (!IsValid(Chime) || !Chime->IsAudibleAt(Rat->GetActorLocation())) continue;
		const float DistanceSquared = FVector::DistSquared(Rat->GetActorLocation(), Chime->GetActorLocation());
		if (DistanceSquared > BestDistanceSquared || !ShouldBeginStoneChimeNotice(
			FMath::Sqrt(DistanceSquared), true, false, false, false)) continue;
		BestDistanceSquared = DistanceSquared;
		BestChime = Chime;
	}

	if (!BestChime) return;
	StoneChimeAttentionTarget = BestChime->GetActorLocation();
	StoneChimeAttentionRemaining = StoneChimeAttentionDuration;
	StoneChimeAttentionCooldownRemaining = StoneChimeAttentionCooldown;
	IdleLookTimer = 0.f;
}

void ARatAgentAIController::CacheAnimations(APawn* Rat)
{
	RatBody.Reset();
	CurrentAnimation = nullptr;
	if (!Rat) return;

	if (ACharacter* RatCharacter = Cast<ACharacter>(Rat)) RatBody = RatCharacter->GetMesh();
	if (!RatBody.IsValid()) return;

	IdleAnimation = LoadFoxAnimation(TEXT("ANIM_Fox_IdleBreathe"));
	LookAroundAnimation = LoadFoxAnimation(TEXT("ANIM_Fox_IdleLookAround"));
	WalkAnimation = LoadFoxAnimation(TEXT("ANIM_Fox_Walk"));
	RunAnimation = LoadFoxAnimation(TEXT("ANIM_Fox_Run"));
	SleepAnimation = LoadFoxAnimation(TEXT("ANIM_Fox_Sleeping"));
	CurrentMode = ERatLocomotionMode::Idle;
	IdleLookTimer = IdleLookInterval + FMath::FRandRange(-IdleLookJitter, IdleLookJitter);
	IdleLookRemaining = 0.f;
	UE_LOG(LogTemp, Display, TEXT("Rat locomotion on %s: idle=%s look=%s walk=%s run=%s sleep=%s"),
		*Rat->GetName(), *GetNameSafe(IdleAnimation), *GetNameSafe(LookAroundAnimation), *GetNameSafe(WalkAnimation),
		*GetNameSafe(RunAnimation), *GetNameSafe(SleepAnimation));
}

void ARatAgentAIController::UpdateAnimation(float DeltaSeconds)
{
	const APawn* Rat = GetPawn();
	if (!RatBody.IsValid() || !Rat) return;
	UpdateAnimationForMotion(DeltaSeconds, Rat->GetVelocity().Size2D(), IsResting());
}

void ARatAgentAIController::UpdateAnimationForMotion(float DeltaSeconds, float HorizontalSpeed, bool bSleeping)
{
	USkeletalMeshComponent* Body = RatBody.Get();
	if (!Body) return;

	const ERatLocomotionMode Mode = SelectLocomotionMode(HorizontalSpeed, bSleeping);
	UAnimSequence* Desired = IdleAnimation;
	bool bLoop = true;
	if (Mode != CurrentMode)
	{
		CurrentMode = Mode;
		IdleLookTimer = IdleLookInterval + FMath::FRandRange(-IdleLookJitter, IdleLookJitter);
		IdleLookRemaining = 0.f;
	}
	if (Mode == ERatLocomotionMode::Sleeping)
	{
		Desired = SleepAnimation;
	}
	else if (Mode == ERatLocomotionMode::Walking)
	{
		Desired = WalkAnimation;
	}
	else if (Mode == ERatLocomotionMode::Running)
	{
		Desired = RunAnimation;
	}
	else
	{
		if (CurrentAnimation == LookAroundAnimation && IdleLookRemaining > 0.f)
		{
			Desired = LookAroundAnimation;
			IdleLookRemaining -= FMath::Max(0.f, DeltaSeconds);
		}
		else
		{
			IdleLookRemaining = 0.f;
			IdleLookTimer -= FMath::Max(0.f, DeltaSeconds);
			if (IdleLookTimer <= 0.f && LookAroundAnimation)
			{
				Desired = LookAroundAnimation;
				bLoop = false;
				IdleLookRemaining = LookAroundAnimation->GetPlayLength();
				IdleLookTimer = IdleLookInterval + FMath::FRandRange(-IdleLookJitter, IdleLookJitter);
			}
		}
	}

	if (!Desired) Desired = IdleAnimation;
	if (!Desired || CurrentAnimation == Desired) return;

	CurrentAnimation = Desired;
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->PlayAnimation(Desired, bLoop);
}
