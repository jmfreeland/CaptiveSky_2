// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class USkeletalMesh;
class UStaticMesh;

struct FIslandVegetationPatch
{
	FString DisplayName;
	float DistanceCm = 0.f;
};

/** Shared, read-only vegetation naming and nearby-patch observation helpers. */
namespace IslandVegetationUtility
{
	FString DescribeStaticVegetation(const UStaticMesh& Mesh, FName ComponentName);
	FString DescribeBroadleafTree(const USkeletalMesh& Mesh);

	TArray<FIslandVegetationPatch> FindNearbyVisiblePatches(const AActor* Observer,
		float SearchRadiusCm, int32 MaxPatches = 3);
	FString FormatVisitorObservation(const TArray<FIslandVegetationPatch>& Patches);
}
