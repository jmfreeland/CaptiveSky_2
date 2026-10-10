#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandForestFox.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

/** One transient, independent fox that rests by day and forages within a small woodland-edge patch at night. */
UCLASS()
class CAPTIVESKY_2_API AIslandForestFox : public AActor
{
	GENERATED_BODY()

public:
	AIslandForestFox();

	/** The fox notices a nearby quiet observer, then trots a short way toward cover. */
	bool RespondToQuietObservation(const FVector& ObserverLocation);
	/** Resting and waking foxes are not available for a brief wildlife response. */
	bool CanRespondToQuietObservation() const { return !bResting && !bWaking; }
	/** True only during the brief look toward a nearby observer, before retreating. */
	bool IsRespondingToQuietObserver() const { return bNoticing; }
	/** The woodland schedule rests the fox through daylight and wakes it after dusk. */
	void SetResting(bool bShouldRest);
	bool IsResting() const { return bResting; }
	bool IsStartled() const { return bStartled; }
	USkeletalMeshComponent* GetFoxMesh() const { return FoxMesh; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandForestFoxTest;
	friend class FRavenPerchTest;

	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<USkeletalMeshComponent> FoxMesh;
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
	TObjectPtr<UAnimSequence> WakeAnimation;

	FVector HomeLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	float ActivityRemaining = 0.f;
	float NoticeRemaining = 0.f;
	float MoveRemaining = 0.f;
	float MoveSpeed = 0.f;
	bool bMoving = false;
	bool bNoticing = false;
	bool bStartled = false;
	bool bResting = false;
	bool bWaking = false;

	void PlayLoop(UAnimSequence* Animation);
	void BeginForaging();
	bool FindGround(const FVector& NearPoint, FVector& OutGround) const;
	bool ChooseForageTarget(FVector& OutTarget) const;
	void StartMove(const FVector& Target, bool bRun);
};
