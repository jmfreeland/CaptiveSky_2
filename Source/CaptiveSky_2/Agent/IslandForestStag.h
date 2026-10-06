#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandForestStag.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
class AIslandLightning;

/** A single wild stag that grazes and roams a small, bounded patch near Wind Arch. */
UCLASS()
class CAPTIVESKY_2_API AIslandForestStag : public AActor
{
	GENERATED_BODY()

public:
	AIslandForestStag();

	/** Startle briefly, moving away from a quiet observer before returning to grazing. */
	void RespondToQuietObservation(const FVector& ObserverLocation);
	/** Sleep in place at night; wake into the ordinary grazing routine in daylight. */
	void SetResting(bool bShouldRest);
	bool IsResting() const { return bResting; }
	bool IsStartled() const { return bStartled; }
	USkeletalMeshComponent* GetDeerMesh() const { return DeerMesh; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandForestStagTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<USkeletalMeshComponent> DeerMesh;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> GrazeAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> WalkAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> RunAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> SleepAnimation;

	FVector HomeLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	float ActivityRemaining = 0.f;
	float ThunderCheckRemaining = 0.f;
	float MoveSpeed = 0.f;
	bool bMoving = false;
	bool bStartled = false;
	bool bResting = false;
	TWeakObjectPtr<AIslandLightning> LastHeardThunder;

	bool FindGround(const FVector& NearPoint, FVector& OutGround) const;
	bool ChooseWanderTarget(FVector& OutTarget) const;
	void StartMove(const FVector& Target, bool bRun);
	void PlayLoop(UAnimSequence* Animation);
	void BeginGrazing();
	void CheckForNearbyThunder();
};
