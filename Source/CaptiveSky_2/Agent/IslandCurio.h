#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandCurio.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UENUM(BlueprintType)
enum class EIslandCurioKind : uint8
{
	/** One of a trail of small stones set into the ground; never changes. */
	PaleStone,
	/** Opens one step on each visit that falls on a new Island day, then stays open. */
	SeedPod,
	/** Any resident may add one stone per Island day, up to a fixed height. */
	Cairn
};

/** A strange object on the Island whose state lasts between sessions. */
USTRUCT(BlueprintType)
struct FIslandCurioRecord
{
	GENERATED_BODY()

	/** Unique tag, doubling as the move_to/interact target (e.g. PaleStone_3, Seedpod, Cairn). */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	EIslandCurioKind Kind = EIslandCurioKind::PaleStone;

	/** Ground point the object rests on. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	FVector Location = FVector::ZeroVector;

	/** Pod: 0 closed .. 3 open. Cairn: stones in the stack. Stones: unused. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	int32 State = 0;

	/** Island day of the most recent change, so some changes happen at most once a day. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	int32 LastChangedDay = -1;

	/** Stable residents who knowingly added a cairn stone; an empty list also means authorship is unknown. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	TArray<FString> Contributors;

	/** Island day a storm last knocked something off it, or -1. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Curio")
	int32 StormDamagedDay = -1;
};

/**
 * Visible form of a curio record: placeholder engine shapes, no collision. The actor sits
 * slightly above the ground so inspection line-of-sight traces never end inside the terrain.
 */
UCLASS()
class CAPTIVESKY_2_API AIslandCurio : public AActor
{
	GENERATED_BODY()

public:
	AIslandCurio();

	static constexpr float GroundClearance = 30.f;
	static constexpr int32 PodOpenState = 3;
	static constexpr int32 CairnMaxStones = 12;

	void ShowRecord(const FIslandCurioRecord& Record);
	const FIslandCurioRecord& GetRecord() const { return Shown; }
	int32 GetVisibleStoneCount() const;
	bool IsGlowing() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Island|Curio")
	TObjectPtr<UInstancedStaticMeshComponent> Stones;

	UPROPERTY(VisibleAnywhere, Category = "Island|Curio")
	TObjectPtr<UInstancedStaticMeshComponent> Husks;

	UPROPERTY(VisibleAnywhere, Category = "Island|Curio")
	TObjectPtr<UStaticMeshComponent> Seed;

	UPROPERTY(VisibleAnywhere, Category = "Island|Curio")
	TObjectPtr<UPointLightComponent> SeedGlow;

	FIslandCurioRecord Shown;
	bool bSurfaceChosen = false;
	void ChooseSurfaces();
};
