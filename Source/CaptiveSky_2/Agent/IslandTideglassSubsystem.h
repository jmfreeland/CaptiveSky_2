#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandTideglassSubsystem.generated.h"

class UMaterialInterface;
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

	/** Changes only the runtime component material. The authored material is restored at teardown. */
	bool ApplyPoolMaterial(UMaterialInterface* Material);
	void RestorePoolMaterial();
	bool IsApplied() const { return AppliedTo.IsValid(); }

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalMaterial;

	TWeakObjectPtr<UStaticMeshComponent> AppliedTo;
};
