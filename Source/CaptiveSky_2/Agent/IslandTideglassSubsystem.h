#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandTideglassSubsystem.generated.h"

class UMaterialInterface;
class UMeshComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;

/** Applies a weather-responsive water surface to Tideglass for the play session only. */
UCLASS()
class CAPTIVESKY_2_API UIslandTideglassSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* MaterialPath;

	/** Tests may inject a transient material; otherwise the additive project asset is loaded at play start. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** Finds the flattened sphere placed beside the map's TideglassPool marker. */
	static UStaticMeshComponent* FindPoolSurface(UWorld* World);
	/** Creates an irregular, shallow, transient water mesh over the blockout footprint. Caller owns visibility/lifetime. */
	static UProceduralMeshComponent* CreatePoolSurfaceMesh(UStaticMeshComponent* BlockoutSurface);

	/** Creates the transient water surface, applies Material, and hides the blockout until restoration. */
	bool ApplyPoolMaterial(UMaterialInterface* Material);
	/** Restores the blockout's prior visibility and destroys the generated surface. */
	void RestorePoolMaterial();
	bool IsApplied() const { return AppliedTo.IsValid(); }

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	friend class FIslandTideglassSurfaceTest;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> RuntimeSurface;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BlockoutSurface;

	TWeakObjectPtr<UMeshComponent> AppliedTo;
	bool bBlockoutWasVisible = true;
	bool bBlockoutWasHiddenInGame = false;
};
