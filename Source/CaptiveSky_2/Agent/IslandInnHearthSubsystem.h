#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandInnHearthSubsystem.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/** A small, session-only hearth response powered by the inn's tagged point light. */
UCLASS()
class CAPTIVESKY_2_API UIslandInnHearthSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr float BurnDurationSeconds = 300.f;
	static constexpr float WarmthCueRadius = 350.f;

	/** Toggle the hearth between banked and briefly lit; no saved world state is changed. */
	bool TendHearth(FString& OutFact);
	FString DescribeHearth() const;
	bool IsLit() const { return bLit; }
	float GetSecondsRemaining() const { return SecondsRemaining; }
	float GetWarmthFactorAt(const FVector& Location) const;
	FString DescribeWarmthAt(const FVector& Location) const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	friend class FIslandInnHearthTest;
	TWeakObjectPtr<UPointLightComponent> HearthLight;
	TWeakObjectPtr<AActor> HearthAnchor;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> FlameMeshes;
	TArray<FTransform> FlameBaseTransforms;
	float BaseIntensity = 0.f;
	float SecondsRemaining = 0.f;
	float FlickerTime = 0.f;
	bool bLit = false;

	void SetBanked();
	void ApplyFlicker();
	void InitializeFlames(AActor* Anchor);
};
