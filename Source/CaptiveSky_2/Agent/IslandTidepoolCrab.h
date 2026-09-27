#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandTidepoolCrab.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/** Small, untargetable shore life: independently scuttles near its Tideglass habitat. */
UCLASS()
class CAPTIVESKY_2_API AIslandTidepoolCrab : public AActor
{
	GENERATED_BODY()

public:
	AIslandTidepoolCrab();

	/** Briefly scuttle away from a quiet observer; never changes ownership or saved state. */
	void RespondToQuietObservation(const FVector& ObserverLocation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	friend class FIslandNightEcologyTest;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TObjectPtr<UStaticMeshComponent> Shell;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Claws;
	UPROPERTY(VisibleAnywhere, Category="Island|Ecology")
	TArray<TObjectPtr<UStaticMeshComponent>> Eyes;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseShapeMaterial;

	FVector HomeLocation = FVector::ZeroVector;
	FVector ScurryDirection = FVector::ZeroVector;
	float Phase = 0.f;
	float ScurryRemaining = 0.f;
	FVector ResolveGroundPath(const FVector& Start, const FVector& Desired) const;
};
