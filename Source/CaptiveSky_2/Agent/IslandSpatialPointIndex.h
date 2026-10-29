#pragma once

#include "CoreMinimal.h"

/**
 * Incremental 2D point index for deterministic foliage clearance checks.
 * Candidate queries still use the caller's exact radius and Dist2D comparison;
 * the grid only skips points that cannot possibly be within that radius.
 */
class CAPTIVESKY_2_API FIslandSpatialPointIndex
{
public:
	explicit FIslandSpatialPointIndex(float InCellSize = 1000.f);

	void Reset();
	void Add(const FVector& Point);
	bool IsWithinDistance(const FVector& Point, float Radius) const;
	int32 Num() const { return PointCount; }

private:
	FIntPoint CellFor(const FVector& Point) const;

	float CellSize = 1000.f;
	int32 PointCount = 0;
	TMap<FIntPoint, TArray<FVector>> Cells;
};
