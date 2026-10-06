#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandArrangement.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class AIslandDayNight;

/** The small vocabulary of shapes residents can arrange stones into; the layout itself is computed. */
UENUM(BlueprintType)
enum class EIslandArrangementForm : uint8
{
	Ring,
	Line,
	Spiral,
	Pair
};

/** A later resident's addition to someone else's arrangement. */
USTRUCT(BlueprintType)
struct FIslandArrangementResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FString AgentId;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	int32 Day = 1;

	/** What the responder meant by it, in their own words. Known only to them unless they say it aloud. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FString Intent;
};

/**
 * One level patch of ground near the ListeningStones where a resident may arrange stones.
 * A site holds at most one work; later residents may add a few responses around it.
 */
USTRUCT(BlueprintType)
struct FIslandArrangementSite
{
	GENERATED_BODY()

	/** Unique build target, e.g. ArrangingGround_2. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	bool bHasWork = false;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	EIslandArrangementForm Form = EIslandArrangementForm::Ring;

	/** Deterministic layout seed so the work looks the same every session. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	int32 Seed = 0;

	/** Stable visible motif seed shared by a work and any descendants that transform it. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	int32 MotifSeed = 0;

	/** The maker's own title and intent; only the maker is reminded of these. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FString Title;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FString Intent;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FString MakerAgentId;

	/** Island day the work was made; its age drives weathering. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	int32 Day = 1;

	/** Island day the fallen-twig bundle was last gathered; an empty site renews on a later Island day. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Forage")
	int32 ForageGatheredDay = 0;

	/** Public lineage: an observed earlier work that genuinely influenced this transformed form. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FName InfluenceSiteId;

	/** Stable resident IDs who have visibly encountered this work; bounded by the world-state subsystem. */
	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	TArray<FString> ObservedBy;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FDateTime CreatedUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	TArray<FIslandArrangementResponse> Responses;
};

/**
 * Visible form of an arranging site's work: flattened engine spheres in the chosen form, with
 * three small stones per response set in an arc outside it. Older work darkens toward a mossy
 * tone over the first week of Island days; after that a faint lichen takes hold on the stones and,
 * over the next two weeks, begins to glow after dark. Purely visual: no collision.
 */
UCLASS()
class CAPTIVESKY_2_API AIslandArrangement : public AActor
{
	GENERATED_BODY()

public:
	AIslandArrangement();

	static constexpr int32 MaxResponses = 3;
	static constexpr int32 StonesPerResponse = 3;
	static constexpr int32 DaysToWeather = 7;
	/** Island days after weathering is complete for the lichen to reach its full night glow. */
	static constexpr int32 DaysToGlow = 14;
	static constexpr float LichenLightIntensity = 5.f;

	static int32 StoneCountFor(EIslandArrangementForm Form);
	/** Fresh pale stone at age 0, fully moss-darkened at DaysToWeather. */
	static FLinearColor WeatheredTint(int32 AgeDays);
	/** 0 until the work is fully weathered, then rises smoothly to 1 at DaysToWeather + DaysToGlow. */
	static float LichenGlow(int32 AgeDays);

	void ShowSite(const FIslandArrangementSite& Site, int32 Today);
	int32 GetVisibleStoneCount() const;
	int32 GetVisibleMotifCount() const;
	int32 GetVisibleForageTwigCount() const;
	bool HasForageableTwigs() const;
	bool GatherForageableTwigs();
	FLinearColor GetCurrentTint() const { return CurrentTint; }
	FName GetSiteId() const { return SiteId; }
	int32 GetVisibleLichenCount() const;
	/** Current glow after age and the hour: 0 by day or while the work is young, up to 1 on a deep night. */
	float GetLichenGlow() const { return LichenLevel; }
	float GetLichenLightIntensity() const;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Island|Arrangement")
	TObjectPtr<UInstancedStaticMeshComponent> Stones;

	UPROPERTY(VisibleAnywhere, Category = "Island|Arrangement")
	TObjectPtr<UInstancedStaticMeshComponent> MotifStones;

	UPROPERTY(VisibleAnywhere, Category = "Island|Arrangement")
	TObjectPtr<UInstancedStaticMeshComponent> Lichen;

	UPROPERTY(VisibleAnywhere, Category = "Island|Arrangement")
	TObjectPtr<UPointLightComponent> LichenLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LichenSurface;

	UPROPERTY(VisibleAnywhere, Category = "Island|Forage")
	TObjectPtr<UInstancedStaticMeshComponent> ForageTwigs;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Surface;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MotifSurface;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ForageSurface;

	FName SiteId;

	FLinearColor CurrentTint = FLinearColor::White;
	bool bForageAvailable = true;
	bool bHasWork = false;
	int32 WorkDay = 1;
	int32 ShownDay = 1;
	float LichenLevel = 0.f;
	TWeakObjectPtr<AIslandDayNight> DayNight;

	void UpdateLichen();
	friend class FIslandLichenTest;
	friend class FIslandArrangementTest;

	void ShowForageTwigs();
};
