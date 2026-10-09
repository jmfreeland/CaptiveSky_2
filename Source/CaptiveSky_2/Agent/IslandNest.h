#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandNest.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Visible form of a persistent nest record. Low-poly engine primitives form a shallow woven
 * bowl with short tapered twig ends; their arrangement is seeded by the site tag so the same
 * nest looks the same every session. Purely visual: no collision or perch-support changes.
 */
UCLASS()
class CAPTIVESKY_2_API AIslandNest : public AActor
{
	GENERATED_BODY()

public:
	AIslandNest();

	static constexpr int32 TwigsPerLayer = 7;
	static constexpr int32 FoundationTwigCount = 5;
	static constexpr int32 StormDebrisTwigCount = 5;

	void SetWoven(FName InSiteTag, int32 InLayers, bool bShowStormDebris = false);
	int32 GetWovenLayers() const { return WovenLayers; }
	int32 GetVisibleTwigCount() const;
	int32 GetVisibleTwigTipCount() const;
	int32 GetVisibleFallenTwigCount() const;

private:
	friend class FIslandNestTest;

	UPROPERTY(VisibleAnywhere, Category = "Island|Nest")
	TObjectPtr<UInstancedStaticMeshComponent> Twigs;

	UPROPERTY(VisibleAnywhere, Category = "Island|Nest")
	TObjectPtr<UInstancedStaticMeshComponent> TwigTips;

	UPROPERTY(VisibleAnywhere, Category = "Island|Nest")
	TObjectPtr<UInstancedStaticMeshComponent> FallenTwigs;

	FName SiteTag;
	int32 WovenLayers = 0;
	bool bSurfaceChosen = false;
};
