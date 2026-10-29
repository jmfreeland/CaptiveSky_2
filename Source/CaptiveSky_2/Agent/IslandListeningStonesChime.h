#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandListeningStonesChime.generated.h"

class UAudioComponent;
class USoundWaveProcedural;
class APawn;

enum class EIslandChimeCause : uint8
{
	Unspecified,
	NaturalGust,
	VisitorApproach,
	ResidentApproach,
	ResidentListening,
	Inspection
};

/** A quiet, finite procedural resonance triggered by a natural gust or resident interaction at ListeningStones. */
UCLASS()
class CAPTIVESKY_2_API AIslandListeningStonesChime : public AActor
{
	GENERATED_BODY()

public:
	AIslandListeningStonesChime();
	static constexpr float AttenuationInnerRadius = 350.f;
	static constexpr float AttenuationFalloffDistance = 750.f;
	static constexpr float AudibleRadius = AttenuationInnerRadius + AttenuationFalloffDistance;
	static constexpr float ResidentContextLifetimeSeconds = 305.f;
	void BeginChime(float HorizontalWindSpeed = 0.f, EIslandChimeCause Cause = EIslandChimeCause::Unspecified,
		const APawn* QuietResident = nullptr);
	void BeginChime(const FVector& HorizontalWind, EIslandChimeCause Cause = EIslandChimeCause::Unspecified,
		const APawn* QuietResident = nullptr);
	/** Whether a nearby animal can still physically hear the tone. */
	bool IsAudibleAt(const FVector& ListenerLocation) const;
	/** Fading or just-faded tone fact for nearby residents; no enduring discovery is implied. */
	FString DescribeForListener(const FVector& ListenerLocation) const;
	/** Sparse private memory policy: only nearby autonomous residents who can hear a tone retain it. */
	static bool ShouldRecordResidentObservation(bool bPlayerControlled, bool bHasMemory, bool bAudible, bool bHasRecentSimilarMemory);
	static FString ResidentObservationText(EIslandChimeCause Cause);
	/** A quiet-listening practice is remembered only by a distinct autonomous resident with clear, nearby sight of the listener. */
	static bool ShouldRecordQuietPractice(bool bPlayerControlled, bool bHasMemory, bool bIsSubject, bool bWithinWitnessRange,
		bool bHasLineOfSight, bool bFacingSubject, bool bHasRecentSimilarMemory, bool bHasSubjectIdentity);
	static FString QuietPracticeObservationText(const FString& SubjectAgentId);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	friend class FIslandListeningStonePresentationTest;
	friend class FIslandListeningStoneResidentMemoryTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Interaction")
	TObjectPtr<UAudioComponent> AudioComponent;
	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ChimeWave;
	float ElapsedSeconds = 0.f;
	float DurationSeconds = 2.8f;
	double ContextExpiresAt = 0.0;
	float SampledWindSpeed = 0.f;
	float AppliedPitchRatio = 1.f;
	EIslandChimeCause Cause = EIslandChimeCause::Unspecified;
	static float CalculateWindPitchRatio(float HorizontalWindSpeed);
	static float CalculateWindDirectionPitchRatio(const FVector& HorizontalWind, const FVector& LandmarkForward);
	void BeginChimeWithPitch(float HorizontalWindSpeed, float PitchRatio, EIslandChimeCause InCause,
		const APawn* QuietResident);
	void RecordNearbyResidentObservations(const APawn* QuietResident);
	void RecordQuietPracticeWitnesses(const APawn* QuietResident);
	void BuildChimeWave(float PitchRatio);
};
