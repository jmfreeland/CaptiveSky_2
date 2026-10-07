#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandForestStag.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
class AIslandLightning;
class AIslandListeningStonesChime;
class ARavenAgentAIController;

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
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> WakeAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LookAroundAnimation;

	FVector HomeLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	float ActivityRemaining = 0.f;
	float WakeRemaining = 0.f;
	float ThunderCheckRemaining = 0.f;
	float RavenCheckRemaining = 0.f;
	float RavenFlybyCooldownRemaining = 0.f;
	float ListeningStonesCheckRemaining = 0.f;
	float ListeningStonesCooldownRemaining = 0.f;
	float ListeningRemaining = 0.f;
	float MoveSpeed = 0.f;
	bool bMoving = false;
	bool bWakingUp = false;
	bool bListeningToChime = false;
	bool bStartled = false;
	bool bResting = false;
	TWeakObjectPtr<AIslandLightning> LastHeardThunder;
	TWeakObjectPtr<AIslandListeningStonesChime> LastHeardChime;

	bool FindGround(const FVector& NearPoint, FVector& OutGround) const;
	bool ChooseWanderTarget(FVector& OutTarget) const;
	void CheckForNearbyRavenFlyby();
	void CheckForNearbyListeningStonesChime();
	void StartMove(const FVector& Target, bool bRun);
	void PlayLoop(UAnimSequence* Animation);
	void BeginGrazing();
	void CheckForNearbyThunder();
};
