#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandListeningStonesChime.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/** A quiet, finite procedural resonance played only when a resident inspects ListeningStones. */
UCLASS()
class CAPTIVESKY_2_API AIslandListeningStonesChime : public AActor
{
	GENERATED_BODY()

public:
	AIslandListeningStonesChime();
	void BeginChime(float HorizontalWindSpeed = 0.f);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
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
