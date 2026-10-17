#pragma once

#include "CoreMinimal.h"
#include "AutonomousAgentAIController.h"
#include "RatAgentAIController.generated.h"

class USkeletalMeshComponent;
class UAnimSequence;

enum class ERatLocomotionMode : uint8
{
	Idle,
	Walking,
	Running,
	Sleeping
};

/** Drives the Fox-derived Rat body with the compatible Animal Variety Pack sequences. */
UCLASS()
class CAPTIVESKY_2_API ARatAgentAIController : public AAutonomousAgentAIController
{
	GENERATED_BODY()

public:
	ARatAgentAIController();

	/** Pure speed/rest mapping shared by the runtime animation selector and regression test. */
	static ERatLocomotionMode SelectLocomotionMode(float HorizontalSpeed, bool bSleeping);
	/** A quiet rat may give a nearby visible raven one brief glance, never during an action or rest. */
	static bool ShouldBeginRavenNotice(float DistanceCm, bool bHasLineOfSight, bool bRatBusy,
		bool bRatResting, bool bRavenResting, bool bAlreadyNoticing);
	static constexpr float RavenNoticeRadius = 800.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FRatAnimationAssetsTest;
	TWeakObjectPtr<USkeletalMeshComponent> RatBody;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> IdleAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LookAroundAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> WalkAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> RunAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> SleepAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentAnimation;
	ERatLocomotionMode CurrentMode = ERatLocomotionMode::Idle;
	float IdleLookTimer = 0.f;
	float IdleLookRemaining = 0.f;
	float RavenScanRemaining = 0.f;
	float RavenAttentionRemaining = 0.f;
	float RavenAttentionCooldownRemaining = 0.f;
	FVector RavenAttentionTarget = FVector::ZeroVector;

	void CacheAnimations(APawn* Rat);
	void UpdateAnimation(float DeltaSeconds);
	void UpdateRavenAttention(float DeltaSeconds);
	void UpdateAnimationForMotion(float DeltaSeconds, float HorizontalSpeed, bool bSleeping);
};
