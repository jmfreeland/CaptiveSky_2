#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentRestPresentationComponent.generated.h"

class ACharacter;
class UAgentConsolidationComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EAgentRestPosture : uint8
{
	GroundedSettle,
	PerchedBird
};

/** Body-specific visual posture driven by the shared consciousness lifecycle. */
UCLASS(ClassGroup=(Agent), meta=(BlueprintSpawnableComponent))
class CAPTIVESKY_2_API UAgentRestPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentRestPresentationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Agent|Rest")
	EAgentRestPosture RestPosture = EAgentRestPosture::GroundedSettle;

	/** Bind explicitly for transient/test actors; embodied characters bind automatically at BeginPlay. */
	void BindToConsciousness(UAgentConsolidationComponent* Consciousness);
	void SetRestPosture(EAgentRestPosture NewPosture);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class FRavenPerchTest;
	UFUNCTION()
	void HandleConsciousStateChanged(EAgentConsciousState PreviousState, EAgentConsciousState NewState);

	TWeakObjectPtr<UAgentConsolidationComponent> BoundConsciousness;
	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<USkeletalMeshComponent> BodyMesh;
	FTransform AwakeMeshTransform = FTransform::Identity;
	FTransform StartMeshTransform = FTransform::Identity;
	FTransform TargetMeshTransform = FTransform::Identity;
	float TransitionElapsed = 0.f;
	float TransitionDuration = 0.45f;
	bool bHasCapturedAwakeTransform = false;
	bool bAnimatingMesh = false;
	bool bHasCapturedCrouchCapability = false;
	bool bOriginalCanCrouch = false;
	bool bWasCrouchedBeforeRest = false;

	void ApplyConsciousState(EAgentConsciousState State);
};
