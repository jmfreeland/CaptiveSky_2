// Copyright Epic Games, Inc. All Rights Reserved.

#include "IslandVegetationUtility.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"

namespace
{
	struct FNearbyVegetationInstance
	{
		double DistanceSquared = TNumericLimits<double>::Max();
		FVector Location = FVector::ZeroVector;
	};

	int32 GetSeasonalMeadowFlowerSpeciesIndex(FName ComponentName)
	{
		static const FName GenericFlowerComponents[] =
		{
			TEXT("IslandMeadowFlowerA"), TEXT("IslandMeadowFlowerB"), TEXT("IslandMeadowFlowerC"), TEXT("IslandMeadowFlowerD"),
			TEXT("IslandMeadowFlowerE"), TEXT("IslandMeadowFlowerF"), TEXT("IslandMeadowFlowerG"), TEXT("IslandMeadowFlowerH")
		};
		for (int32 SpeciesIndex = 0; SpeciesIndex < UE_ARRAY_COUNT(GenericFlowerComponents); ++SpeciesIndex)
			if (ComponentName == GenericFlowerComponents[SpeciesIndex]) return SpeciesIndex;
		if (ComponentName == TEXT("IslandMeadowBluebells")) return AIslandWeather::WoodlandBluebellSpeciesIndex;
		if (ComponentName == TEXT("IslandTideglassMarshMarigolds")) return AIslandWeather::TideglassMarshMarigoldSpeciesIndex;
		if (ComponentName == TEXT("IslandTideglassYellowFlagIrises")) return AIslandWeather::TideglassYellowFlagIrisSpeciesIndex;
		return INDEX_NONE;
	}

	bool IsVisibleFromObserver(const AActor* Observer, const AIslandWeather* Weather,
		const FVector& ViewStart, const FVector& InstanceLocation, float BoundsHeight,
		FName TraceStat)
	{
		if (!Observer || !Weather || !Observer->GetWorld()) return false;
		FVector End = InstanceLocation + FVector(0.f, 0.f, FMath::Max(35.f, BoundsHeight * 0.55f));
		FCollisionQueryParams Params(TraceStat, false, Observer);
		Params.AddIgnoredActor(Weather);
		FHitResult Hit;
		return !Observer->GetWorld()->LineTraceSingleByChannel(Hit, ViewStart, End, ECC_Visibility, Params);
	}

	bool FindNearestVisibleInstance(const AActor* Observer, const AIslandWeather* Weather,
		const FVector& ViewStart, const UHierarchicalInstancedStaticMeshComponent* Foliage,
		const UStaticMesh* Mesh, const TArray<int32>& InstanceIndices, float SearchRadiusSquared,
		FNearbyVegetationInstance& OutInstance)
	{
		if (!Foliage || !Mesh || InstanceIndices.IsEmpty()) return false;
		TArray<FNearbyVegetationInstance> Candidates;
		Candidates.Reserve(FMath::Min(InstanceIndices.Num(), 16));
		for (const int32 InstanceIndex : InstanceIndices)
		{
			FTransform Transform;
			if (!Foliage->GetInstanceTransform(InstanceIndex, Transform, true)) continue;
			const FVector Location = Transform.GetLocation();
			const double DistanceSquared = FVector::DistSquared(Observer->GetActorLocation(), Location);
			if (DistanceSquared <= SearchRadiusSquared) Candidates.Add({ DistanceSquared, Location });
		}
		Candidates.Sort([](const FNearbyVegetationInstance& A, const FNearbyVegetationInstance& B)
			{ return A.DistanceSquared < B.DistanceSquared; });

		constexpr int32 MaxVisibilityChecks = 3;
		const int32 CheckCount = FMath::Min(Candidates.Num(), MaxVisibilityChecks);
		for (int32 Index = 0; Index < CheckCount; ++Index)
		{
			if (IsVisibleFromObserver(Observer, Weather, ViewStart, Candidates[Index].Location,
				Mesh->GetBounds().BoxExtent.Z, TEXT("VisitorVegetationVisibility")))
			{
				OutInstance = Candidates[Index];
				return true;
			}
		}
		return false;
	}

	bool FindNearestVisibleInstance(const AActor* Observer, const AIslandWeather* Weather,
		const FVector& ViewStart, const UInstancedSkinnedMeshComponent* Trees,
		const USkeletalMesh* Mesh, float SearchRadiusSquared, FNearbyVegetationInstance& OutInstance)
	{
		if (!Trees || !Mesh || Trees->GetInstanceCount() <= 0) return false;
		TArray<FNearbyVegetationInstance> Candidates;
		Candidates.Reserve(FMath::Min(Trees->GetInstanceCount(), 16));
		for (int32 InstanceIndex = 0; InstanceIndex < Trees->GetInstanceCount(); ++InstanceIndex)
		{
			FTransform Transform;
			const FPrimitiveInstanceId InstanceId = Trees->GetInstanceId(InstanceIndex);
			if (!Trees->GetInstanceTransform(InstanceId, Transform, true)) continue;
			const FVector Location = Transform.GetLocation();
			const double DistanceSquared = FVector::DistSquared(Observer->GetActorLocation(), Location);
			if (DistanceSquared <= SearchRadiusSquared) Candidates.Add({ DistanceSquared, Location });
		}
		Candidates.Sort([](const FNearbyVegetationInstance& A, const FNearbyVegetationInstance& B)
			{ return A.DistanceSquared < B.DistanceSquared; });

		constexpr int32 MaxVisibilityChecks = 3;
		const int32 CheckCount = FMath::Min(Candidates.Num(), MaxVisibilityChecks);
		for (int32 Index = 0; Index < CheckCount; ++Index)
		{
			if (IsVisibleFromObserver(Observer, Weather, ViewStart, Candidates[Index].Location,
				Mesh->GetBounds().BoxExtent.Z, TEXT("VisitorBroadleafVisibility")))
			{
				OutInstance = Candidates[Index];
				return true;
			}
		}
		return false;
	}

	void AddOrUpdatePatch(TArray<FIslandVegetationPatch>& Patches, const FString& DisplayName,
		float DistanceCm, const FVector& Location, float BloomProgress = -1.f)
	{
		if (DisplayName.IsEmpty()) return;
		if (FIslandVegetationPatch* Existing = Patches.FindByPredicate(
			[&DisplayName](const FIslandVegetationPatch& Patch) { return Patch.DisplayName == DisplayName; }))
		{
			if (DistanceCm < Existing->DistanceCm)
			{
				Existing->DistanceCm = DistanceCm;
				Existing->Location = Location;
				Existing->bHasLocation = true;
				Existing->BloomProgress = BloomProgress;
			}
			return;
		}
		Patches.Add({ DisplayName, DistanceCm, Location, true, BloomProgress });
	}
}

FString IslandVegetationUtility::DescribeStaticVegetation(const UStaticMesh& Mesh, FName ComponentName)
{
	if (ComponentName == TEXT("IslandMeadowBluebells")) return TEXT("woodland bluebells");
	if (ComponentName == TEXT("IslandTideglassMarshMarigolds")) return TEXT("Tideglass marsh marigolds");
	if (ComponentName == TEXT("IslandTideglassYellowFlagIrises")) return TEXT("yellow-flag irises");
	const FString Name = Mesh.GetName();
	const FString LowerName = Name.ToLower();
	const FString LowerPath = Mesh.GetPathName().ToLower();
	if (LowerPath.Contains(TEXT("/flowermesh/")) || LowerName.StartsWith(TEXT("flower_"))) return TEXT("wildflowers");
	if (LowerPath.Contains(TEXT("/groundplantmesh/")) || LowerName.StartsWith(TEXT("ground_"))) return TEXT("low groundcover");
	if (LowerPath.Contains(TEXT("/pn_interactivespruceforest/")) || LowerName.StartsWith(TEXT("spruce_"))) return TEXT("spruce trees");
	if (LowerName.StartsWith(TEXT("festuca_")) || LowerName.StartsWith(TEXT("phalaris_"))) return TEXT("grasses");

	FString DisplayName = Name;
	DisplayName.RemoveFromStart(TEXT("SM_"));
	DisplayName.RemoveFromStart(TEXT("SKM_"));
	DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));
	return DisplayName + TEXT(" vegetation");
}

FString IslandVegetationUtility::DescribeBroadleafTree(const USkeletalMesh& Mesh)
{
	const FString LowerName = Mesh.GetName().ToLower();
	if (LowerName.Contains(TEXT("norway_maple"))) return TEXT("Norway maple trees");
	if (LowerName.Contains(TEXT("european_aspen"))) return TEXT("European aspen trees");
	if (LowerName.Contains(TEXT("european_beech"))) return TEXT("European beech trees");
	if (LowerName.Contains(TEXT("black_alder"))) return TEXT("black alder trees");
	if (LowerName.Contains(TEXT("common_hazel"))) return TEXT("common hazel trees");

	FString DisplayName = Mesh.GetName();
	DisplayName.RemoveFromStart(TEXT("SKM_"));
	DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));
	return DisplayName + TEXT(" trees");
}

TArray<FIslandVegetationPatch> IslandVegetationUtility::FindNearbyVisiblePatches(
	const AActor* Observer, float SearchRadiusCm, int32 MaxPatches)
{
	TArray<FIslandVegetationPatch> Patches;
	if (!IsValid(Observer) || !Observer->GetWorld() || !FMath::IsFinite(SearchRadiusCm) || SearchRadiusCm <= 0.f || MaxPatches <= 0)
		return Patches;

	UWorld* World = Observer->GetWorld();
	const double SearchRadiusSquared = FMath::Square(static_cast<double>(SearchRadiusCm));
	FVector ViewStart = Observer->GetActorLocation();
	FRotator ViewRotation = Observer->GetActorRotation();
	Observer->GetActorEyesViewPoint(ViewStart, ViewRotation);
	double IslandDayProgress = 0.0;
	bool bHasIslandDayProgress = false;
	for (TActorIterator<AIslandDayNight> DayNightIt(World); DayNightIt; ++DayNightIt)
	{
		if (!IsValid(*DayNightIt) || DayNightIt->DayNumber < 1 || !FMath::IsFinite(DayNightIt->CurrentHour)) continue;
		IslandDayProgress = DayNightIt->DayNumber + AIslandDayNight::WrapHour(DayNightIt->CurrentHour) / 24.0;
		bHasIslandDayProgress = true;
		break;
	}

	for (TActorIterator<AIslandWeather> WeatherIt(World); WeatherIt; ++WeatherIt)
	{
		AIslandWeather* Weather = *WeatherIt;
		TArray<UActorComponent*> FoliageComponents;
		Weather->GetComponents(UHierarchicalInstancedStaticMeshComponent::StaticClass(), FoliageComponents);
		for (UActorComponent* Component : FoliageComponents)
		{
			UHierarchicalInstancedStaticMeshComponent* Foliage = Cast<UHierarchicalInstancedStaticMeshComponent>(Component);
			const UStaticMesh* Mesh = Foliage ? Foliage->GetStaticMesh() : nullptr;
			if (!Foliage || !Mesh || !Foliage->IsVisible() || Foliage->bHiddenInGame || Foliage->GetInstanceCount() <= 0) continue;

			const TArray<int32> Indices = Foliage->GetInstancesOverlappingSphere(Observer->GetActorLocation(), SearchRadiusCm, true);
			FNearbyVegetationInstance Nearest;
			if (!FindNearestVisibleInstance(Observer, Weather, ViewStart, Foliage, Mesh, Indices,
				static_cast<float>(SearchRadiusSquared), Nearest)) continue;
			const FName ComponentName = Foliage->GetFName();
			const int32 FlowerSpeciesIndex = GetSeasonalMeadowFlowerSpeciesIndex(ComponentName);
			const float BloomProgress = FlowerSpeciesIndex != INDEX_NONE && bHasIslandDayProgress
				? AIslandWeather::GetMeadowFlowerSpeciesBloomProgress(FlowerSpeciesIndex, IslandDayProgress) : -1.f;
			AddOrUpdatePatch(Patches, DescribeStaticVegetation(*Mesh, ComponentName),
				FMath::Sqrt(static_cast<float>(Nearest.DistanceSquared)), Nearest.Location, BloomProgress);
		}

		TArray<UActorComponent*> TreeComponents;
		Weather->GetComponents(UInstancedSkinnedMeshComponent::StaticClass(), TreeComponents);
		for (UActorComponent* Component : TreeComponents)
		{
			UInstancedSkinnedMeshComponent* Trees = Cast<UInstancedSkinnedMeshComponent>(Component);
			const USkeletalMesh* Mesh = Trees ? Cast<USkeletalMesh>(Trees->GetSkinnedAsset()) : nullptr;
			if (!Trees || !Mesh || !Trees->IsVisible() || Trees->bHiddenInGame || Trees->GetInstanceCount() <= 0) continue;

			FNearbyVegetationInstance Nearest;
			if (!FindNearestVisibleInstance(Observer, Weather, ViewStart, Trees, Mesh,
				static_cast<float>(SearchRadiusSquared), Nearest)) continue;
			AddOrUpdatePatch(Patches, DescribeBroadleafTree(*Mesh),
				FMath::Sqrt(static_cast<float>(Nearest.DistanceSquared)), Nearest.Location);
		}
	}

	Patches.Sort([](const FIslandVegetationPatch& A, const FIslandVegetationPatch& B)
		{ return A.DistanceCm < B.DistanceCm; });
	if (Patches.Num() > MaxPatches) Patches.SetNum(MaxPatches);
	return Patches;
}

FString IslandVegetationUtility::FormatVisitorObservation(
	const TArray<FIslandVegetationPatch>& Patches, const AActor* Observer)
{
	if (Patches.IsEmpty()) return FString();
	TArray<FString> Details;
	for (const FIslandVegetationPatch& Patch : Patches)
	{
		if (Patch.DisplayName.IsEmpty()) continue;
		FString Detail = Patch.DisplayName;
		if (IsValid(Observer) && Patch.bHasLocation && FMath::IsFinite(Patch.DistanceCm) && Patch.DistanceCm > 0.f &&
			FMath::IsFinite(Patch.Location.X) && FMath::IsFinite(Patch.Location.Y) &&
			FMath::IsFinite(Patch.Location.Z))
		{
			const FVector Offset = Patch.Location - Observer->GetActorLocation();
			const FVector Forward = Observer->GetActorForwardVector();
			const FVector Right = Observer->GetActorRightVector();
			const float ForwardAmount = FVector::DotProduct(Offset, Forward);
			const float RightAmount = FVector::DotProduct(Offset, Right);
			const FString Direction = FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount)
				? (ForwardAmount >= 0.f ? TEXT("ahead of you") : TEXT("behind you"))
				: (RightAmount >= 0.f ? TEXT("to your right") : TEXT("to your left"));
			const int32 DistanceMetres = FMath::Max(1, FMath::RoundToInt(Patch.DistanceCm / 100.f));
			Detail += FString::Printf(TEXT(" about %d metres %s"), DistanceMetres, *Direction);
		}
		else
		{
			Detail += TEXT(" nearby");
		}
		if (Patch.BloomProgress >= 0.6f)
			Detail += TEXT(" are in bloom");
		else if (Patch.BloomProgress >= 0.08f)
			Detail += TEXT(" are beginning to bloom");
		Details.AddUnique(MoveTemp(Detail));
	}
	if (Details.IsEmpty()) return FString();

	const FString Plants = FString::Join(Details, TEXT(", "));
	return FString::Printf(TEXT("You pause to notice %s. You leave every plant growing, rooted, and undisturbed."), *Plants);
}
