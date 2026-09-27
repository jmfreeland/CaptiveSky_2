#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandInnHearthSubsystem.generated.h"

class UPointLightComponent;

/** A small, session-only hearth response powered by the inn's tagged point light. */
UCLASS()
class CAPTIVESKY_2_API UIslandInnHearthSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr float BurnDurationSeconds = 300.f;

	/** Toggle the hearth between banked and briefly lit; no saved world state is changed. */
	bool TendHearth(FString& OutFact);
	FString DescribeHearth() const;
	bool IsLit() const { return bLit; }
	float GetSecondsRemaining() const { return SecondsRemaining; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TWeakObjectPtr<UPointLightComponent> HearthLight;
	float BaseIntensity = 0.f;
	float SecondsRemaining = 0.f;
	float FlickerTime = 0.f;
	bool bLit = false;

	void SetBanked();
	void ApplyFlicker();
};
