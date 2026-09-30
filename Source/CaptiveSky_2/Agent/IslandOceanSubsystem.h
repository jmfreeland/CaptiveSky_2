#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandOceanSubsystem.generated.h"

class AStaticMeshActor;
class UMaterialInterface;

/**
 * Gives the level's ocean plane (a static mesh actor named or labelled "OceanPlane") the Single Layer Water
 * material /Game/Materials/M_IslandOcean for the session, then puts the authored material back at end play.
 * The level asset is never edited. Without the material or the plane it does nothing.
 * Build the material with Scripts/Create-IslandOceanMaterial.py.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandOceanSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* MaterialPath;
	static const FName OceanActorName;

	/** Tests supply a material here; empty loads MaterialPath. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MaterialOverride;

	static AStaticMeshActor* FindOceanPlane(UWorld* World);

	/** Swaps the ocean plane's first material. False when there is no plane or no material. Safe to call twice. */
	bool ApplyOceanMaterial(UMaterialInterface* Material);
	void RestoreOceanMaterial();
	bool IsApplied() const { return AppliedTo.IsValid(); }

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalMaterial;

	TWeakObjectPtr<AStaticMeshActor> AppliedTo;
};
