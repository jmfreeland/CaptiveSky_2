#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandGrade.generated.h"

class UPostProcessComponent;

/** What the world is doing, as far as colour is concerned; all 0..1. */
struct FIslandGradeInputs
{
	float Daylight = 1.f;
	float GoldenHour = 0.f;
	float Storm = 0.f;
	float Rain = 0.f;
	float Wetness = 0.f;
	float Mist = 0.f;
	float CloudCover = 0.f;
};

/** A gentle colour grade. Neutral is saturation 1, gain (1,1,1), contrast 1. */
struct FIslandGrade
{
	float Saturation = 1.f;
	FVector3f Gain = FVector3f(1.f, 1.f, 1.f);
	float Contrast = 1.f;
};

/** The grade a world state calls for. Strength 0 gives neutral; 1 is the intended subtle look. Pure. */
CAPTIVESKY_2_API FIslandGrade ComputeIslandGrade(const FIslandGradeInputs& Inputs, float Strength);

/** Carries the grade as an unbound post-process; the subsystem moves its settings. */
UCLASS()
class CAPTIVESKY_2_API AIslandGradeActor : public AActor
{
	GENERATED_BODY()
public:
	AIslandGradeActor();
	void Apply(const FIslandGrade& Grade, float BlendWeight);

private:
	UPROPERTY(VisibleAnywhere, Category="Island|Grade")
	TObjectPtr<UPostProcessComponent> Post;
};

/**
 * A slow, subtle colour grade that follows the world: warm at golden hour, blue at night, cooler and
 * quieter in storms, richer on wet ground. Reads the environment subsystem; nothing is saved.
 * Console: Island.Grade <0..2> scales it (0 turns it off, 1 is default).
 */
UCLASS()
class CAPTIVESKY_2_API UIslandGradeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	/** Seconds the grade takes to settle most of the way to a new target. */
	static constexpr float SettleSeconds = 6.f;
	/** Moves Current toward Target by Seconds of settling. Pure. */
	static FIslandGrade Settle(const FIslandGrade& Current, const FIslandGrade& Target, float Seconds);

	const FIslandGrade& GetCurrent() const { return Current; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TWeakObjectPtr<AIslandGradeActor> Actor;
	FIslandGrade Current;
	float SinceUpdate = 0.f;
	bool bSnap = true;
};
