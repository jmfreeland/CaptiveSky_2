#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandArrangement.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

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

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	FDateTime CreatedUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Island|Arrangement")
	TArray<FIslandArrangementResponse> Responses;
};

/**
 * Visible form of an arranging site's work: flattened engine spheres in the chosen form, with
 * three small stones per response set in an arc outside it. Older work darkens toward a mossy
 * tone over the first week of Island days. Purely visual: no collision.
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

	static int32 StoneCountFor(EIslandArrangementForm Form);
	/** Fresh pale stone at age 0, fully moss-darkened at DaysToWeather. */
	static FLinearColor WeatheredTint(int32 AgeDays);

	void ShowSite(const FIslandArrangementSite& Site, int32 Today);
	int32 GetVisibleStoneCount() const;
	int32 GetVisibleForageTwigCount() const;
	bool HasForageableTwigs() const;
	bool GatherForageableTwigs();
	FLinearColor GetCurrentTint() const { return CurrentTint; }
	FName GetSiteId() const { return SiteId; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Island|Arrangement")
	TObjectPtr<UInstancedStaticMeshComponent> Stones;

	UPROPERTY(VisibleAnywhere, Category = "Island|Forage")
	TObjectPtr<UInstancedStaticMeshComponent> ForageTwigs;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Surface;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ForageSurface;

	FName SiteId;

	FLinearColor CurrentTint = FLinearColor::White;
	bool bForageAvailable = true;

	void ShowForageTwigs();
};
