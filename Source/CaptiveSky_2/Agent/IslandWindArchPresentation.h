#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandWindArchPresentation.generated.h"

class AStaticMeshActor;
class AIslandWindMoteEffect;
class AIslandWeather;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/** Transient rock forms that give the Wind Arch a weathered, assembled silhouette. */
UCLASS()
class CAPTIVESKY_2_API AWindArchStonework : public AActor
{
	GENERATED_BODY()

public:
	AWindArchStonework();

	int32 GetStoneCount() const;
	/** Stronger natural winds may create one nearby transient motes pass after the cooldown. */
	static bool ShouldEmitNaturalWindMotes(float WindSpeed, float CooldownRemaining);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class UIslandWindArchPresentationSubsystem;
	friend class FIslandWindArchPresentationTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Landmark")
	TObjectPtr<UInstancedStaticMeshComponent> Stones;
	TWeakObjectPtr<AIslandWeather> Weather;
	TWeakObjectPtr<AIslandWindMoteEffect> AmbientMotes;
	float AmbientMoteCooldownRemaining = 0.f;
	static constexpr float AmbientMoteWindThreshold = 105.f;
	static constexpr float AmbientMoteCooldownSeconds = 30.f;

	static float GetPillarStoneScale(const FVector& LocalHalfExtent, const FVector& RockHalfExtent);
	static int32 GetPillarLayerCount(const FVector& LocalHalfExtent, const FVector& RockHalfExtent);
	bool BuildStonework(UStaticMesh* RockMesh, const FTransform& MarkerTransform,
		const TArray<AStaticMeshActor*>& Pillars, const AStaticMeshActor* Beam);
	void CheckForAmbientWind();
};

/** Replaces only the saved Wind Arch's cube visuals during Game/PIE; never saves map edits. */
UCLASS()
class CAPTIVESKY_2_API UIslandWindArchPresentationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Finds the three cube proxies by their tagged marker-relative transforms without changing them. */
	static bool FindWindArchProxies(UWorld* World, AActor*& OutMarker, TArray<AStaticMeshActor*>& OutPillars,
		AStaticMeshActor*& OutBeam);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	friend class FIslandWindArchPresentationTest;
	TWeakObjectPtr<AWindArchStonework> StoneworkActor;
	TArray<TWeakObjectPtr<AStaticMeshActor>> HiddenProxies;
	TArray<bool> PreviousProxyVisibility;

	void ApplyPresentation(UWorld* World);
	void RestorePresentation();
};
