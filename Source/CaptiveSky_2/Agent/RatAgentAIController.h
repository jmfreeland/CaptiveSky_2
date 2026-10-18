#pragma once

#include "CoreMinimal.h"
#include "AutonomousAgentAIController.h"
#include "RatAgentAIController.generated.h"

class USkeletalMeshComponent;
class UAnimSequence;
class AIslandListeningStonesChime;
class AIslandTidepoolPeriwinkle;

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
	/** A live, audible landmark tone earns one brief glance from an otherwise idle rat. */
	static bool ShouldBeginStoneChimeNotice(float DistanceCm, bool bChimeAudible, bool bRatBusy,
		bool bRatResting, bool bAlreadyNoticing);
	/** An idle rat may briefly look at a nearby, visible tidepool snail. */
	static bool ShouldBeginPeriwinkleNotice(float DistanceCm, bool bHasLineOfSight, bool bRatBusy,
		bool bRatResting, bool bAlreadyNoticing);
	/** An idle rat may briefly acknowledge the awake Innkeeper nearby, without interrupting either resident. */
	static bool ShouldBeginInnkeeperNotice(float DistanceCm, bool bHasLineOfSight, bool bRatBusy,
		bool bRatResting, bool bInnkeeperResting, bool bAlreadyNoticing);
	/** Whether the rat's current animation state is awake rather than resting. */
	bool IsAwake() const { return CurrentMode != ERatLocomotionMode::Sleeping; }
	static constexpr float RavenNoticeRadius = 800.f;
	static constexpr float PeriwinkleNoticeRadius = 420.f;
	static constexpr float InnkeeperNoticeRadius = 650.f;

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
	float StoneChimeScanRemaining = 0.f;
	float StoneChimeAttentionRemaining = 0.f;
	float StoneChimeAttentionCooldownRemaining = 0.f;
	FVector StoneChimeAttentionTarget = FVector::ZeroVector;
	float PeriwinkleScanRemaining = 0.f;
	float PeriwinkleAttentionRemaining = 0.f;
	float PeriwinkleAttentionCooldownRemaining = 0.f;
	FVector PeriwinkleAttentionTarget = FVector::ZeroVector;
	float InnkeeperScanRemaining = 0.f;
	float InnkeeperAttentionRemaining = 0.f;
	float InnkeeperAttentionCooldownRemaining = 0.f;
	FVector InnkeeperAttentionTarget = FVector::ZeroVector;

	void CacheAnimations(APawn* Rat);
	void UpdateAnimation(float DeltaSeconds);
	void UpdateRavenAttention(float DeltaSeconds);
	void UpdateStoneChimeAttention(float DeltaSeconds);
	void UpdatePeriwinkleAttention(float DeltaSeconds);
	void UpdateInnkeeperAttention(float DeltaSeconds);
	void UpdateAnimationForMotion(float DeltaSeconds, float HorizontalSpeed, bool bSleeping);
};
