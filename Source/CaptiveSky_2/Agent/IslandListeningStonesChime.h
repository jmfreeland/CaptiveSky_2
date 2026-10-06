#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandListeningStonesChime.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

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
	void BeginChime(float HorizontalWindSpeed = 0.f);
	/** Short-lived listener fact for nearby resident situation summaries; empty outside the audible window. */
	FString DescribeForListener(const FVector& ListenerLocation) const;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	friend class FIslandListeningStonePresentationTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Interaction")
	TObjectPtr<UAudioComponent> AudioComponent;
	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ChimeWave;
	float ElapsedSeconds = 0.f;
	float DurationSeconds = 2.8f;
	float SampledWindSpeed = 0.f;
	float AppliedPitchRatio = 1.f;
	static float CalculateWindPitchRatio(float HorizontalWindSpeed);
	void BuildChimeWave(float PitchRatio);
};
