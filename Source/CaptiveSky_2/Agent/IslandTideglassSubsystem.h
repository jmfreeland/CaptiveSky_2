#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandTideglassSubsystem.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UProceduralMeshComponent;
class AStaticMeshActor;
class UStaticMeshComponent;
class AIslandDayNight;

/** Applies a weather-responsive, gently lunar-tidal water surface to Tideglass for the play session only. */
UCLASS()
class CAPTIVESKY_2_API UIslandTideglassSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* MaterialPath;
	static const TCHAR* RippleMaterialPath;
	static constexpr float TidalDayHours = 24.84f;
	static constexpr float MaximumTideOffsetCm = 14.f;
	/** Small semidiurnal tide: spring range follows new/full moon, reduced at quarter moons. */
	static float TideOffsetCm(float IslandHour, int32 IslandDay);

	/** Tests may inject a transient material; otherwise the additive project asset is loaded at play start. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** Finds the flattened sphere placed beside the map's TideglassPool marker. */
	static UStaticMeshComponent* FindPoolSurface(UWorld* World);
	/** Creates an irregular, shallow, transient water mesh over the blockout footprint. Caller owns visibility/lifetime. */
	static UProceduralMeshComponent* CreatePoolSurfaceMesh(UStaticMeshComponent* BlockoutSurface);

	/** Creates the transient water surface, applies Material, and hides the blockout until restoration. */
	bool ApplyPoolMaterial(UMaterialInterface* Material);
	/** Pushes one short-lived world-space impulse into the transient water material, when supported. */
	bool TriggerSurfaceRipple(const FVector& WorldCenter, float DurationSeconds, float RadiusCm, float Strength);
	/** Restores the blockout's prior visibility and destroys the generated surface. */
	void RestorePoolMaterial();
	bool IsApplied() const { return AppliedTo.IsValid(); }

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual void Deinitialize() override;

private:
	friend class FIslandTideglassSurfaceTest;
	friend class FIslandTideglassTideTest;
	static void TuneReadablePoolMaterial(UMaterialInstanceDynamic* Material);
	void UpdateTideSurface();
	void ApplyShoreStonePresentation();
	void RestoreShoreStonePresentation();

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> RuntimeSurface;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BlockoutSurface;

	TArray<TWeakObjectPtr<AStaticMeshActor>> HiddenShoreStoneProxies;
	TArray<bool> PreviousShoreStoneHiddenStates;
	TArray<TWeakObjectPtr<AStaticMeshActor>> ShoreStonePresentationActors;

	TWeakObjectPtr<UMeshComponent> AppliedTo;
	TWeakObjectPtr<AIslandDayNight> IslandClock;
	FVector RuntimeSurfaceBaseLocation = FVector::ZeroVector;
	float AppliedTideOffsetCm = TNumericLimits<float>::Max();
	float TideUpdateAccumulator = 0.f;
	bool bBlockoutWasVisible = true;
	bool bBlockoutWasHiddenInGame = false;
};
