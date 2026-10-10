#include "RatAgentAIController.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

namespace
{
	constexpr float WalkSpeedThreshold = 12.f;
	constexpr float RunSpeedThreshold = 105.f;
	constexpr float IdleLookInterval = 7.5f;
	constexpr float IdleLookJitter = 2.5f;

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

void ARatAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
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
	Super::OnUnPossess();
}

void ARatAgentAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAnimation(DeltaSeconds);
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
