#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandWildlifeHome.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;

/** Visual-only materialization of a canonical wildlife-home record. */
UCLASS()
class CAPTIVESKY_2_API AIslandWildlifeHome : public AActor
{
	GENERATED_BODY()

public:
	AIslandWildlifeHome();

	/** Applies the supported species' authored visual to the record's entrance anchor. */
	bool ApplyHomeRecord(const FIslandWildlifeHomeRecord& Record);

	FName GetHomeId() const { return HomeId; }
	FName GetSpecies() const { return Species; }
	UStaticMeshComponent* GetShelterMesh() const { return ShelterMesh; }
	UInstancedStaticMeshComponent* GetCacheDisplayMesh() const { return CacheDisplayMesh; }

	/** RatHollow's open entrance is 117 cm behind the authored ground-centre origin on local -X. */
	static constexpr float RatHollowEntranceOffsetCm = 117.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Island|Wildlife Home")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Island|Wildlife Home")
	TObjectPtr<UStaticMeshComponent> ShelterMesh;

	UPROPERTY(VisibleAnywhere, Category = "Island|Wildlife Home")
	TObjectPtr<UInstancedStaticMeshComponent> CacheDisplayMesh;

	FName HomeId;
	FName Species;
};
