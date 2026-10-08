#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandDew.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/** What the sky and ground are doing, as far as morning dew is concerned. */
struct FIslandDewInputs
{
	/** Sine of the sun's elevation, -1..1. */
	float SunHeight = 0.f;
	/** Island hour, 0..24. Dew is a morning thing; evening light on dry grass does not glitter. */
	float Hour = 12.f;
	float Rain = 0.f;
	float Wetness = 0.f;
	float CloudCover = 0.f;
	float Storm = 0.f;
};

/**
 * How much dew glitters on the grass, 0..1. Needs a low morning sun to catch the beads, a sky open
 * enough to let it through, no rain falling and no storm. Wet ground carries more dew than dry. Pure.
 */
CAPTIVESKY_2_API float ComputeDewStrength(const FIslandDewInputs& Inputs);

struct FIslandDewGlint
{
	FVector Position = FVector::ZeroVector;
	float Size = 1.f;
	bool bPlaced = false;
};

/** A scatter of tiny glints near the ground around the viewer, twinkled by a local additive material. */
UCLASS()
class CAPTIVESKY_2_API AIslandDewActor : public AActor
{
	GENERATED_BODY()
public:
	AIslandDewActor();

	static constexpr int32 MaxGlints = 360;
	/** Glints live in a disc this wide (cm) around the viewer. */
	static constexpr float Radius = 1100.f;
	/** Optional local asset built by Scripts/Create-IslandDewMaterial.py. */
	static const TCHAR* MaterialPath;

	/** How many glints a dew strength calls for. Pure. */
	static int32 DesiredCount(float Strength);
	/** Edge length (cm) of a glint at this distance from the viewer: far ones are larger so they still read. Pure. */
	static float GlintSize(float Distance);

	/** False when the optional material asset is missing; the actor then stays hidden. */
	bool HasMaterial() const { return Material != nullptr; }
	float GetStrength() const { return CurrentStrength; }
	/** Finds the nearest active glint that could plausibly be noticed from Origin. */
	bool FindNearestGlint(const FVector& Origin, float MaxDistance, FVector& OutLocation) const;
	/** Keep Strength's worth of glints scattered around Viewer, re-seating any that fall out of range. */
	void Advance(const FVector& Viewer, float Strength);

private:
	UPROPERTY(VisibleAnywhere, Category="Island|Dew")
	TObjectPtr<UInstancedStaticMeshComponent> Glints;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
	TArray<FIslandDewGlint> Slots;
	FRandomStream Random;
	float CurrentStrength = 0.f;
	float LastStrength = -1.f;

	float GroundHeightAt(const FVector2D& XY, float ReferenceZ) const;
	void Seat(int32 Slot, const FVector& Viewer);
};

/**
 * Dew glittering on the grass in the low morning sun. It fades in and out over about twenty seconds
 * and is mentioned to residents. Console: Island.Dew <0..1> forces a strength for testing; a
 * negative value follows the weather.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandDewSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	/** What a resident notices; empty when there is no dew. Pure. */
	static FString DescribeDew(float Strength);

	float GetStrength() const { return Strength; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TWeakObjectPtr<AIslandDewActor> Actor;
	float Strength = 0.f;
};
