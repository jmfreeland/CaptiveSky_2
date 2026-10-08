#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandListeningStonePresentation.generated.h"

class AStaticMeshActor;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMesh;
class AIslandWeather;

/** Collisionless, transient render forms and a short-lived resonance at the Listening Stones. */
UCLASS()
class CAPTIVESKY_2_API AListeningStonePresentation : public AActor
{
	GENERATED_BODY()

public:
	AListeningStonePresentation();
	int32 GetStoneCount() const;
	void BeginResonance(float WindSpeed);
	float GetResonanceRemaining() const { return bIsResonating ? FMath::Max(0.f, ResonanceDuration - ResonanceElapsed) : 0.f; }
	/** Natural resonance is reserved for a meaningful, rising outdoor gust, not steady background wind. */
	static bool ShouldResonateForWind(float PreviousSpeed, float CurrentSpeed);

private:
	friend class UIslandListeningStonePresentationSubsystem;
	friend class FIslandListeningStonePresentationTest;
	static constexpr float StoneHeightRatio = 0.58f;
	UPROPERTY(VisibleAnywhere, Category="Island|Landmark")
	TObjectPtr<UInstancedStaticMeshComponent> Stones;
	UPROPERTY(VisibleAnywhere, Category="Island|Landmark")
	TArray<TObjectPtr<UPointLightComponent>> ResonanceLights;
	float ResonanceElapsed = 0.f;
	float ResonanceDuration = 2.8f;
	float SampledWindSpeed = 0.f;
	bool bIsResonating = false;
	float AmbientWindCheckAccumulator = 0.f;
	float LastAmbientWindSpeed = 0.f;
	double LastAmbientChimeAt = -1000.0;
	bool bHasAmbientWindSample = false;
	TWeakObjectPtr<AIslandWeather> Weather;
	bool BuildStoneForms(UStaticMesh* RockMesh, const FTransform& MarkerTransform,
		const TArray<AStaticMeshActor*>& Proxies);
	virtual void BeginPlay() override;
	void CheckForNaturalGust(float DeltaSeconds);
	void ObserveAmbientWind(float CurrentSpeed);
	void BeginNaturalResonance(float WindSpeed);
	virtual void Tick(float DeltaSeconds) override;
};

/** Replaces only the three cube visuals during Game/PIE. Original map actors remain authoritative. */
UCLASS()
class CAPTIVESKY_2_API UIslandListeningStonePresentationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static bool FindStoneProxies(UWorld* World, AActor*& OutMarker, TArray<AStaticMeshActor*>& OutProxies);
	void NotifyChime(float WindSpeed);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	friend class FIslandListeningStonePresentationTest;
	TWeakObjectPtr<AListeningStonePresentation> PresentationActor;
	TArray<TWeakObjectPtr<AStaticMeshActor>> HiddenProxies;
	TArray<bool> PreviousProxyVisibility;
	void ApplyPresentation(UWorld* World);
	void RestorePresentation();
};
