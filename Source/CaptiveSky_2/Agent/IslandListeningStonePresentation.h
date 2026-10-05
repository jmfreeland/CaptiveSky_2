#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "IslandListeningStonePresentation.generated.h"

class AStaticMeshActor;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMesh;

/** Collisionless, transient render forms and a short-lived resonance at the Listening Stones. */
UCLASS()
class CAPTIVESKY_2_API AListeningStonePresentation : public AActor
{
	GENERATED_BODY()

public:
	AListeningStonePresentation();
	int32 GetStoneCount() const;
	void BeginResonance(float WindSpeed);
	float GetResonanceRemaining() const { return FMath::Max(0.f, ResonanceDuration - ResonanceElapsed); }

private:
	friend class UIslandListeningStonePresentationSubsystem;
	friend class FIslandListeningStonePresentationTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Landmark")
	TObjectPtr<UInstancedStaticMeshComponent> Stones;
	UPROPERTY(VisibleAnywhere, Category="Island|Landmark")
	TArray<TObjectPtr<UPointLightComponent>> ResonanceLights;
	float ResonanceElapsed = 0.f;
	float ResonanceDuration = 2.8f;
	float SampledWindSpeed = 0.f;
	bool BuildStoneForms(UStaticMesh* RockMesh, const FTransform& MarkerTransform,
		const TArray<AStaticMeshActor*>& Proxies);
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
