#pragma once

#include "CoreMinimal.h"
#include "AutonomousAgentAIController.h"
#include "RavenAgentAIController.generated.h"

UENUM(BlueprintType)
enum class ERavenLocomotionState : uint8
{
	Grounded,
	Hopping,
	TakingOff,
	Flying,
	Landing,
	Perched
};

/** Asset-independent raven locomotion; animation assets can read LocomotionState later. */
UCLASS()
class CAPTIVESKY_2_API ARavenAgentAIController : public AAutonomousAgentAIController
{
	GENERATED_BODY()

public:
	ARavenAgentAIController();

	UPROPERTY(BlueprintReadOnly, Category = "Raven|Locomotion")
	ERavenLocomotionState LocomotionState = ERavenLocomotionState::Grounded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float FlightSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float VerticalRange = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float TakeoffHeight = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float HopDistance = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raven|Locomotion")
	float HopHeight = 55.f;

	/** Tagged markers specify the centre of the upright raven capsule at rest. */
	UFUNCTION(BlueprintCallable, Category = "Raven|Locomotion")
	bool RequestPerch(FName PerchTag);

	/** Read-only collision assessment for a tagged roost marker; does not begin movement or claim ownership. */
	FString AssessRoostSite(const AActor* Site) const;
	/** Maximum visible distance for selecting an exact open-ground landing target. */
	static constexpr float GroundLandingVisibilityRange = 3500.f;

	/** Build options this body has right now (gathering twigs, weaving at the roost it is perched on). */
	FString DescribeBuildOptions() const;
	/** Keeps free flight random unless curiosity is active, then favors candidates approaching visible landmarks. */
	static FVector SelectWanderCruiseTarget(const FVector& Origin, const TArray<FVector>& RandomCandidates,
		const TArray<FVector>& VisibleLandmarks, int32 RandomFallbackIndex, bool bApplyCuriosityBias);

	UPROPERTY(BlueprintReadOnly, Category = "Raven|Nest")
	bool bCarryingTwigs = false;

protected:
	virtual bool IsActionInProgress() const override;
	virtual bool CanRest() const override;
	virtual bool CanArrangeStones() const override { return LocomotionState == ERavenLocomotionState::Grounded && !IsActionInProgress(); }
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ActOnDecision(const FAgentDecision& Decision) override;

private:
	friend class FIslandMovementProbeCommand;
	friend class FRavenPerchTest;
	friend class FIslandNestTest;
	friend class FRavenFlightTest;
	friend class FIslandCurioTest;
	friend class FIslandArrangementTest;
	friend class FIslandGuestBookTest;
	FVector MovementTarget = FVector::ZeroVector;
	FVector PerchTarget = FVector::ZeroVector;
	FVector CruiseTarget = FVector::ZeroVector;
	FVector TakeoffEscapeTarget = FVector::ZeroVector;
	FVector HopStart = FVector::ZeroVector;
	FVector HopEnd = FVector::ZeroVector;
	float HomeAltitude = 0.f;
	float HopElapsed = 0.f;
	float HopDuration = 0.55f;
	static constexpr float WanderLandmarkCuriosityChance = 0.4f;
	bool bHasMovementTarget = false;
	bool bHasTakeoffEscapeTarget = false;
	bool bTargetIsPerch = false;
	bool bApproachingPerch = false;
	bool bLandingAtArrangementSite = false;
	FVector ArrangementLandingTarget = FVector::ZeroVector;
	TWeakObjectPtr<UObject> LeftWing;
	TWeakObjectPtr<UObject> RightWing;
	FRotator LeftWingRestRotation = FRotator::ZeroRotator;
	FRotator RightWingRestRotation = FRotator::ZeroRotator;
	float WingAnimationTime = 0.f;
	/** Remaining legs of a planned flight (climb over an obstacle, cross, descend). */
	TArray<FVector> FlightWaypoints;
	/** First target for flying From -> To: To itself when the way is clear, else a climb above what blocks it. */
	FVector PlanFlightLeg(const FVector& From, const FVector& To);
	TMap<FName, double> WovenUntil;

	AActor* FindPerchedNestSite() const;
	void Build(FName Target);

	void SetFlyingMovement(bool bFlying) const;
	void BeginTakeoff(const FVector& Destination);
	bool BeginLanding(const FVector& DesiredLocation);
	void BeginGroundLandingAt(FName SiteTag);
	void BeginHop();
	bool BeginPerch();
	bool BeginPerchAt(AActor* Perch);
	bool HasSuitablePerchSupport(const AActor* Site, FHitResult* OutSupport = nullptr) const;
	static int32 SelectWindAwarePerch(const FVector& Origin, float CurrentWindSpeed,
		const TArray<FVector>& PerchLocations, const TArray<float>& PerchWindSpeeds);
	void SetGrounded();
	void CacheWingComponents(APawn* Raven);
	void UpdateWingAnimation(float DeltaSeconds);
	FVector MakeCruiseTarget(bool bForceCuriosityForProbe = false) const;
	bool TraceGround(const FVector& DesiredLocation, FVector& OutGroundLocation) const;
	bool AdvanceTowardsTarget(float DeltaSeconds);
};
