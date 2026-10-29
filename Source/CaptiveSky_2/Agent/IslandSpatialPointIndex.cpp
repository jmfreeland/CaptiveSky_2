#include "IslandSpatialPointIndex.h"

FIslandSpatialPointIndex::FIslandSpatialPointIndex(float InCellSize)
	: CellSize(FMath::IsFinite(InCellSize) && InCellSize > 1.f ? InCellSize : 1000.f)
{
}

void FIslandSpatialPointIndex::Reset()
{
	Cells.Reset();
	PointCount = 0;
}

FIntPoint FIslandSpatialPointIndex::CellFor(const FVector& Point) const
{
	return FIntPoint(FMath::FloorToInt(Point.X / CellSize), FMath::FloorToInt(Point.Y / CellSize));
}

void FIslandSpatialPointIndex::Add(const FVector& Point)
{
	if (!FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y)) return;
	Cells.FindOrAdd(CellFor(Point)).Add(Point);
	++PointCount;
}

bool FIslandSpatialPointIndex::IsWithinDistance(const FVector& Point, float Radius) const
{
	if (!FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y) ||
		!FMath::IsFinite(Radius) || Radius <= 0.f || PointCount == 0)
	{
		return false;
	}

	const FIntPoint CenterCell = CellFor(Point);
	const int32 CellRange = FMath::CeilToInt(Radius / CellSize);
	for (int32 Y = CenterCell.Y - CellRange; Y <= CenterCell.Y + CellRange; ++Y)
	{
		for (int32 X = CenterCell.X - CellRange; X <= CenterCell.X + CellRange; ++X)
		{
			const TArray<FVector>* Points = Cells.Find(FIntPoint(X, Y));
			if (!Points) continue;
			for (const FVector& Existing : *Points)
			{
				if (FVector::Dist2D(Point, Existing) < Radius)
				{
					return true;
				}
			}
		}
	}
	return false;
}
