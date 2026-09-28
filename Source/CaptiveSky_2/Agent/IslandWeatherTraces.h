#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandWeatherTraces.generated.h"

/**
 * Lets storms leave small, lasting marks that residents can find and repair. Watches the weather (read
 * only) and, once per storm, when it reaches full strength: knocks the top stone off the cairn (never
 * below the three it was found with) and tears the outer layer off every nest with more than one. The
 * marks are saved in the world state like any other lasting change; residents repair them with the
 * actions they already have (adding a cairn stone, weaving twigs).
 */
UCLASS()
class CAPTIVESKY_2_API UIslandWeatherTracesSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Storm strength at which marks are left. */
	static constexpr float MarkingStorm = 0.8f;

	/** Checks the storm at session time Now; returns true if marks were left. */
	bool EvaluateStorm(double Now);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	float SinceCheck = 0.f;
};
