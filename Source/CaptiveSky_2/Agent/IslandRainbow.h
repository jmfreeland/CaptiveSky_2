#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandRainbow.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** What the sky and ground are doing, as far as a rainbow is concerned. */
struct FIslandRainbowInputs
{
	/** Sine of the sun's elevation, -1..1. */
	float SunHeight = 0.f;
	float Rain = 0.f;
	float Wetness = 0.f;
	float CloudCover = 0.f;
	float Storm = 0.f;
};

/**
 * How visible a rainbow is, 0..1. Needs a low-to-middling sun (a bow only shows while the sun is
 * under about 42 degrees), ground still wet from a shower, a sky that has opened up, and no storm.
 * Pure.
 */
CAPTIVESKY_2_API float ComputeRainbowStrength(const FIslandRainbowInputs& Inputs);

/** The little arc of sky the bow sits in: a plane facing the viewer, shaded by an optional local material. */
UCLASS()
class CAPTIVESKY_2_API AIslandRainbowActor : public AActor
{
	GENERATED_BODY()
public:
	AIslandRainbowActor();
	/** False when the optional rainbow material asset is missing; the actor then stays hidden. */
	bool HasMaterial() const { return Material != nullptr; }
	void Apply(const FVector& ViewerLocation, const FVector& Axis, float Intensity);

	/** Distance (cm) from the viewer to the bow's plane; the bow's centre is the anti-solar point there. */
	static constexpr float Distance = 40000.f;
	/** Full width of the plane, cm; wide enough for the 42 degree bow and the fainter 51 degree one. */
	static constexpr float PlaneSize = 2.7f * Distance;
	/** Optional local asset built by Scripts/Create-IslandRainbowMaterial.py. */
	static const TCHAR* MaterialPath;

private:
	UPROPERTY(VisibleAnywhere, Category="Island|Rainbow")
	TObjectPtr<UStaticMeshComponent> Plane;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
};

/**
 * A rainbow in the sky opposite the sun when the sun comes out over wet ground after a shower. It
 * follows the viewer, fades in and out over about twenty seconds, and is mentioned to residents.
 * Console: Island.Rainbow <0..1> forces a strength for testing; a negative value follows the weather.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandRainbowSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	/** Unit direction from the viewer toward the anti-solar point (below the horizon while the sun is up). Pure. */
	static FVector AntiSolarAxis(float IslandHour);
	/** What a resident notices; empty when there is no bow. Pure. */
	static FString DescribeRainbow(float Strength);

	float GetStrength() const { return Strength; }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TWeakObjectPtr<AIslandRainbowActor> Actor;
	float Strength = 0.f;
};
