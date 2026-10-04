// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AgentLLMTypes.h"
#include "AutonomousAgentAIController.generated.h"

class UNavigationPath;
class UNavigationSystemV1;
class ACharacter;
struct FNavAgentProperties;
struct FNavLocation;

/**
 * Drives an AAutonomousAgentCharacter's think/act loop directly in C++: polls
 * UAgentBrainComponent::RequestDecision on a timer and turns the resulting
 * FAgentDecision into a movement/navigation command.
 *
 * This does NOT use StateTree, unlike ACombatAIController/ASideScrollingAIController
 * elsewhere in this project -- FStateTreeAgentDecideTask (see AgentStateTreeUtility.h)
 * exists for a future StateTree-driven version, but building that graph requires the
 * StateTree editor UI. This controller is the interim path to get the agent thinking
 * and moving today.
 */
UCLASS(Abstract)
class CAPTIVESKY_2_API AAutonomousAgentAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAutonomousAgentAIController();

	// How often the agent asks its brain for a new decision while no request is already in flight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float ThinkIntervalSeconds = 60.f;

	// Radius used to pick a random reachable point for the Wander action.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float WanderRadius = 2000.f;

	/** A resident approaches another body to conversation distance without seeking its capsule centre. */
	static constexpr float ResidentApproachStandOffDistance = 350.f;
	static constexpr float ResidentApproachAltitudeOffset = 180.f;
	static FVector BuildResidentApproachPoint(const FVector& MoverLocation, const FVector& TargetLocation);
	/** Horizontal stand-off point when the target is elevated above the mover's walkable plane. */
	static FVector BuildGroundedResidentApproachPoint(const FVector& MoverLocation, const FVector& TargetLocation);
	/** Eight nearby ground stand-offs, starting on the mover's side of an elevated resident. */
	static TArray<FVector> BuildGroundedResidentApproachCandidates(const FVector& MoverLocation, const FVector& TargetLocation);
	/** Raven locomotion state overrides walking movement mode while perched. */
	static bool IsElevatedResidentForApproach(const ACharacter* TargetCharacter);
	/** Finds a complete, in-range ground route to a conversational stand-off beside an elevated resident. */
	static bool FindGroundedResidentApproachGoal(UNavigationSystemV1* Navigation, const FVector& MoverLocation,
		const FVector& TargetLocation, const FNavAgentProperties& AgentProperties, float CapsuleHalfHeight,
		float SpeakingRadius, FNavLocation& OutStart, FNavLocation& OutGoal, AActor* PathfindingContext = nullptr);
	/** Fallback lifetime cap applies only to bounded play; continuous mode is governed by the shared session budget. */
	static constexpr int32 BoundedAutonomousRequestLimit = 30;
	static bool IsAutonomousRequestLimitReached(int32 RequestCount, bool bContinuousPlay);
	static constexpr float WanderMinimumDistance = 100.f;
	static constexpr float WanderAcceptanceRadius = 50.f;
	static constexpr float WanderFrontierProgressWeight = 4000.f;
	static constexpr float WanderLandmarkProgressWeight = 4000.f;
	static bool IsUsableWanderPath(const UNavigationPath* Path, const FVector& Origin, const FVector& Goal);
	static bool IsWanderPathPhysicallyClear(const UWorld* World, const UNavigationPath* Path, const APawn* Pawn);
	/** Larger scores mean a wander goal is farther from the resident's recent successful destinations. */
	static float WanderNoveltyScore(const FVector& Candidate, const TArray<FVector>& RecentDestinations);
	/** Extends an explicit wander's explored frontier, with local novelty as a tie-breaker. */
	static float WanderFrontierScore(const FVector& Candidate, const FVector& ExplorationOrigin,
		float FurthestExploredDistance, const TArray<FVector>& RecentDestinations);
	/** Rewards a wander candidate only when it moves toward a currently visible nearby landmark. */
	static float WanderLandmarkProgressScore(const FVector& Candidate, const FVector& Origin,
		const TArray<FVector>& VisibleLandmarks);
	/** Hidden, unidentified or recently inspected landmarks do not attract an explicit wander. */
	static bool IsWanderLandmarkEligible(const AActor* Landmark, const TMap<FName, double>& RecentInspections, double Now);
	/** Prefer a nearby floor-level walking point, sampling around blocked fixtures before an elevated fallback. */
	static bool ProjectGroundedTarget(UNavigationSystemV1* Navigation, const FVector& Target, const FNavAgentProperties& AgentProperties, FNavLocation& OutLocation);

	FString DescribeActionState() const;
	static double BackgroundDelay(int32 Repeats, double BaseSeconds);

protected:
	bool CanFollowWanderCuriosityToward(const AActor* Landmark, double Now) const;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	virtual void ActOnDecision(const FAgentDecision& Decision);
	virtual bool IsActionInProgress() const;
	virtual bool CanRest() const;
	/** Whether this body is settled enough to arrange stones by hand, beak, or otherwise. */
	virtual bool CanArrangeStones() const;
	void ArrangeStones(const FAgentDecision& Decision);
	void WriteGuestBook(const FAgentDecision& Decision);
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
	void ReportAction(const FString& Outcome);
	bool IsResting() const;
	bool TryRest(FName RequestedRestSite = NAME_None);

private:
	friend class FIslandMovementProbeCommand;
	friend class FAgentBlockedGroundMoveTest;
	friend class FRavenPerchTest;
	friend class FIslandNightEcologyTest;
	friend class FIslandMinnowTest;
	friend class FIslandInnHearthTest;
	friend class FIslandInnRestTest;
	FTimerHandle ThinkTimerHandle;
	FString LastActionOutcome = TEXT("No completed action yet.");
	FString LastActionKey;
	int32 RepeatedActions = 0;
	int32 BoundedAutonomousRequests = 0;
	TArray<FVector> RecentWanderDestinations;
	FVector WanderExplorationOrigin = FVector::ZeroVector;
	float FurthestWanderDistance = 0.f;
	bool bCurrentMoveIsWander = false;
	FName PendingGroundMoveTargetName;
	double NextThinkAt = 0;
	double NextRestAt = 0;
	TMap<FName, double> InspectedUntil;
	void InspectTarget(FName Target);

	// Bound to the possessed character's Brain->OnDecisionReady.
	UFUNCTION()
	void HandleDecisionReady(const FAgentDecision& Decision);

	// Timer callback: asks the brain to decide, if it isn't already working on one.
	void Think();

};
