#include "Misc/AutomationTest.h"
#include "IslandWeather.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherMeadowHabitatTest,
	"CaptiveSky2.Agent.IslandWeather.MeadowHabitat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherMeadowHabitatTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Meadow habitat retains full lush-community coverage"),
		FMath::IsNearlyEqual(AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(0.f, 0.f, 0.f), 1.f));
	TestTrue(TEXT("Nested open habitat leaves readable glades"),
		FMath::IsNearlyEqual(AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(1.f, 1.f, 1.f), 0.08f));
	TestTrue(TEXT("Three habitat scales create stronger glade-to-lush contrast"),
		AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(0.1f, 0.1f, 0.1f) -
		AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(0.9f, 0.9f, 0.9f) > 0.7f);
	TestTrue(TEXT("Invalid meadow fields resolve to a bounded neutral density"),
		FMath::IsFinite(AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(
			std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity())));
	const FVector SamplePosition(-99500.f, 100300.f, 0.f);
	const float RepeatedSample = AIslandWeather::SampleLandscapeCommunityFieldAtScale(SamplePosition, 0x18ac70e5, 18000.f);
	TestTrue(TEXT("Seeded meadow fields are deterministic"),
		FMath::IsNearlyEqual(RepeatedSample,
			AIslandWeather::SampleLandscapeCommunityFieldAtScale(SamplePosition, 0x18ac70e5, 18000.f)));
	TestTrue(TEXT("Different meadow seeds form distinct communities"),
		!FMath::IsNearlyEqual(RepeatedSample,
			AIslandWeather::SampleLandscapeCommunityFieldAtScale(SamplePosition, 0x7cb25431, 18000.f), 0.001f));

	double AverageMeadowAcceptance = 0.0;
	constexpr int32 MeadowFieldSamples = 128;
	constexpr float MeadowWorldMin = -201600.f;
	constexpr float MeadowWorldSpan = 403200.f;
	for (int32 Y = 0; Y < MeadowFieldSamples; ++Y)
		for (int32 X = 0; X < MeadowFieldSamples; ++X)
		{
			const FVector Position(MeadowWorldMin + (X + 0.5f) * MeadowWorldSpan / MeadowFieldSamples,
				MeadowWorldMin + (Y + 0.5f) * MeadowWorldSpan / MeadowFieldSamples, 0.f);
			const float Macro = AIslandWeather::SampleLandscapeCommunityFieldAtScale(Position, 0x18ac70e5, 18000.f);
			const float Meso = AIslandWeather::SampleLandscapeCommunityFieldAtScale(Position, 0x7cb25431, 6000.f);
			const float Micro = AIslandWeather::SampleLandscapeCommunityFieldAtScale(Position, 0x4cf5ad43, 900.f);
			AverageMeadowAcceptance += AIslandWeather::CalculateLandscapeMeadowAcceptanceChance(Macro, Meso, Micro);
		}
	AverageMeadowAcceptance /= FMath::Square(MeadowFieldSamples);
	TestTrue(TEXT("Nested spatial fields preserve near-46% mean candidate acceptance over the Island bounds"),
		FMath::IsNearlyEqual(static_cast<float>(AverageMeadowAcceptance), 0.46f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherBroadleafDiversityTest,
	"CaptiveSky2.Agent.IslandWeather.BroadleafDiversity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherBroadleafDiversityTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The first habitat-field fifth selects Norway maple"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.1f), 0);
	TestEqual(TEXT("The second habitat-field fifth selects European aspen"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.3f), 1);
	TestEqual(TEXT("The middle habitat-field fifth selects European beech"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.5f), 2);
	TestEqual(TEXT("The fourth habitat-field fifth selects black alder"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.7f), 3);
	TestEqual(TEXT("The final habitat-field fifth selects common hazel"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.9f), 4);
	TestEqual(TEXT("A field at the upper endpoint stays within the last species"),
		AIslandWeather::GetBroadleafSpeciesIndex(1.f), 4);
	TestEqual(TEXT("A field below zero stays within the first species"),
		AIslandWeather::GetBroadleafSpeciesIndex(-0.1f), 0);
	TestEqual(TEXT("A non-finite habitat field resolves deterministically"),
		AIslandWeather::GetBroadleafSpeciesIndex(std::numeric_limits<float>::quiet_NaN()), 0);
	const FVector VariantSamplePosition(-98250.f, 104125.f, 0.f);
	const int32 VariantSample = AIslandWeather::GetBroadleafVariantIndexAt(VariantSamplePosition, 0x5a17);
	TestTrue(TEXT("The local canopy field selects only A or B"), VariantSample == 0 || VariantSample == 1);
	TestEqual(TEXT("A/B canopy assignment is stable for a position and seed"),
		AIslandWeather::GetBroadleafVariantIndexAt(VariantSamplePosition, 0x5a17), VariantSample);
	TestEqual(TEXT("A-only diagnostic baseline maps every requested variant to A"),
		AIslandWeather::ResolveBroadleafVariantIndex(1, false), 0);
	TestEqual(TEXT("Default comparison profile preserves requested B variants"),
		AIslandWeather::ResolveBroadleafVariantIndex(1, true), 1);
	TestEqual(TEXT("A variants stay A in both comparison profiles"),
		AIslandWeather::ResolveBroadleafVariantIndex(0, true), 0);
	int32 VariantCounts[2] = {};
	for (int32 GridY = -4; GridY <= 4; ++GridY)
	{
		for (int32 GridX = -4; GridX <= 4; ++GridX)
		{
			const FVector Sample = FVector(-100000.f, 100000.f, 0.f) + FVector(GridX * 37500.f, GridY * 37500.f, 0.f);
			++VariantCounts[AIslandWeather::GetBroadleafVariantIndexAt(Sample, 0x5a17)];
		}
	}
	TestTrue(TEXT("The representative Island-scale field uses both canopy silhouettes"),
		VariantCounts[0] >= 16 && VariantCounts[0] <= 65 &&
		VariantCounts[1] >= 16 && VariantCounts[1] <= 65);
	TestEqual(TEXT("Five resolved species share the 3,600-tree budget evenly"),
		AIslandWeather::GetBroadleafSpeciesLimit(5), 720);
	TestEqual(TEXT("Four resolved species retain 900-tree shares"),
		AIslandWeather::GetBroadleafSpeciesLimit(4), 900);
	TestEqual(TEXT("Three resolved species preserve the former 1,200-tree shares"),
		AIslandWeather::GetBroadleafSpeciesLimit(3), 1200);
	TestEqual(TEXT("A single resolved species can use the full fixed population budget"),
		AIslandWeather::GetBroadleafSpeciesLimit(1), AIslandWeather::BroadleafTreePopulationBudget);
	TestEqual(TEXT("No resolved species admits no broadleaf instances"),
		AIslandWeather::GetBroadleafSpeciesLimit(0), 0);
	const float SharedLayerInnerRadius = FMath::Max(
		FMath::Max(AIslandWeather::WoodlandMatureTreeInnerRadiusCm, AIslandWeather::WoodlandSaplingInnerRadiusCm),
		FMath::Max(FMath::Max(AIslandWeather::WoodlandShrubInnerRadiusCm, AIslandWeather::WoodlandFlowerInnerRadiusCm),
			AIslandWeather::WoodlandCoppiceInnerRadiusCm));
	const float SharedLayerOuterRadius = FMath::Min(
		FMath::Min(AIslandWeather::WoodlandMatureTreeOuterRadiusCm, AIslandWeather::WoodlandSaplingOuterRadiusCm),
		FMath::Min(FMath::Min(AIslandWeather::WoodlandShrubOuterRadiusCm, AIslandWeather::WoodlandFlowerOuterRadiusCm),
			AIslandWeather::WoodlandCoppiceOuterRadiusCm));
	TestTrue(TEXT("Mature trees, saplings, shrubs, flowers and coppice share a mixed radial habitat"),
		SharedLayerInnerRadius < SharedLayerOuterRadius);
	const float AreaMedianRadius = AIslandWeather::SampleUniformAnnulusRadius(0.5f, 0.f, 1800.f);
	TestTrue(TEXT("The median sample bisects mature-grove area rather than radius"),
		FMath::IsNearlyEqual(AreaMedianRadius, FMath::Sqrt(FMath::Square(1800.f) * 0.5f), 0.01f));
	TestEqual(TEXT("Annulus sampling honors its inner radius"),
		AIslandWeather::SampleUniformAnnulusRadius(0.f, 800.f, 2800.f), 800.f);
	TestEqual(TEXT("Annulus sampling honors its outer radius"),
		AIslandWeather::SampleUniformAnnulusRadius(1.f, 800.f, 2800.f), 2800.f);
	TestEqual(TEXT("Annulus sampling clamps values below the inner boundary"),
		AIslandWeather::SampleUniformAnnulusRadius(-1.f, 800.f, 2800.f), 800.f);
	TestEqual(TEXT("Annulus sampling clamps values above the outer boundary"),
		AIslandWeather::SampleUniformAnnulusRadius(2.f, 800.f, 2800.f), 2800.f);
	TestEqual(TEXT("Non-finite annulus samples resolve safely"),
		AIslandWeather::SampleUniformAnnulusRadius(std::numeric_limits<float>::quiet_NaN(), 800.f, 2800.f), 0.f);
	const FVector GroveScalePosition(-104250.f, 98250.f, 0.f);
	const float GroveRadiusScale = AIslandWeather::GetWoodlandGroveRadiusScaleAt(GroveScalePosition, 0x3c6ef372);
	TestTrue(TEXT("Grove footprint scale stays inside its restrained 0.82..1.18 range"),
		GroveRadiusScale >= 0.82f && GroveRadiusScale <= 1.18f);
	TestEqual(TEXT("Grove footprint variation is stable for the same world position and seed"),
		AIslandWeather::GetWoodlandGroveRadiusScaleAt(GroveScalePosition, 0x3c6ef372), GroveRadiusScale);
	TestTrue(TEXT("Invalid grove positions use the neutral footprint scale"),
		FMath::IsNearlyEqual(AIslandWeather::GetWoodlandGroveRadiusScaleAt(
			FVector(std::numeric_limits<float>::quiet_NaN(), 0.f, 0.f), 0x3c6ef372), 1.f));
	float MinimumGroveScale = MAX_flt;
	float MaximumGroveScale = -MAX_flt;
	for (int32 GridY = -8; GridY <= 8; ++GridY)
		for (int32 GridX = -8; GridX <= 8; ++GridX)
		{
			const FVector Sample = GroveScalePosition + FVector(GridX * 5000.f, GridY * 5000.f, 0.f);
			const float Scale = AIslandWeather::GetWoodlandGroveRadiusScaleAt(Sample, 0x3c6ef372);
			MinimumGroveScale = FMath::Min(MinimumGroveScale, Scale);
			MaximumGroveScale = FMath::Max(MaximumGroveScale, Scale);
		}
	TestTrue(TEXT("Nearby Island-scale grove footprints do not all repeat the same radius"),
		MaximumGroveScale - MinimumGroveScale >= 0.12f);

	const AIslandWeather* Defaults = GetDefault<AIslandWeather>();
	const UInstancedSkinnedMeshComponent* BlackAlders = Defaults ? Defaults->IslandBlackAlders : nullptr;
	const UInstancedSkinnedMeshComponent* BlackAldersB = Defaults ? Defaults->IslandBlackAldersVariantB : nullptr;
	const UInstancedSkinnedMeshComponent* CommonHazels = Defaults ? Defaults->IslandCommonHazels : nullptr;
	TestNotNull(TEXT("Black Alder uses its own instanced-skinned foliage component"), BlackAlders);
	TestNotNull(TEXT("Black Alder B uses a separate instanced-skinned foliage component"), BlackAldersB);
	TestNotNull(TEXT("Common Hazel uses an instanced-skinned foliage component"), CommonHazels);
	if (CommonHazels)
	{
		TestEqual(TEXT("Common Hazel foliage has no collision"),
			CommonHazels->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Common Hazel foliage does not affect navigation"), CommonHazels->CanEverAffectNavigation());
		int32 CullStart = 0;
		int32 CullEnd = 0;
		CommonHazels->GetCullDistances(CullStart, CullEnd);
		TestEqual(TEXT("Common Hazel joins the established broadleaf fade range at 80 m"), CullStart, 8000);
		TestEqual(TEXT("Common Hazel stays inside the established 125 m cull end"), CullEnd, 12500);
	}
	if (BlackAlders)
	{
		TestEqual(TEXT("Black Alder foliage has no collision"),
			BlackAlders->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Black Alder foliage does not affect navigation"), BlackAlders->CanEverAffectNavigation());
		int32 CullStart = 0;
		int32 CullEnd = 0;
		BlackAlders->GetCullDistances(CullStart, CullEnd);
		TestEqual(TEXT("Black Alder begins its distinct mid-distance fade at 120 m"), CullStart, 12000);
		TestEqual(TEXT("Black Alder stays inside the spruce horizon with a 220 m cull end"), CullEnd, 22000);
	}
	if (BlackAlders && BlackAldersB)
	{
		int32 VariantBCullStart = 0;
		int32 VariantBCullEnd = 0;
		BlackAldersB->GetCullDistances(VariantBCullStart, VariantBCullEnd);
		TestEqual(TEXT("Black Alder B keeps the A variant's 120 m fade start"), VariantBCullStart, 12000);
		TestEqual(TEXT("Black Alder B keeps the A variant's 220 m cull end"), VariantBCullEnd, 22000);
		TestEqual(TEXT("Black Alder B has no collision"), BlackAldersB->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Black Alder B does not affect navigation"), BlackAldersB->CanEverAffectNavigation());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherBroadleafNaniteReadinessAssetsTest,
	"CaptiveSky2.Agent.IslandWeather.BroadleafNaniteReadinessAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherBroadleafNaniteReadinessAssetsTest::RunTest(const FString& Parameters)
{
	const TCHAR* BroadleafAssemblyAssetPaths[] =
	{
		TEXT("/Game/Megaplant_Library/Tree_Norway_Maple/Tree_Norway_Maple_Forest_01/Tree_Norway_Maple_Forest_01_A.Tree_Norway_Maple_Forest_01_A"),
		TEXT("/Game/Megaplant_Library/Tree_Norway_Maple/Tree_Norway_Maple_Forest_01/Tree_Norway_Maple_Forest_01_B.Tree_Norway_Maple_Forest_01_B"),
		TEXT("/Game/Megaplant_Library/Tree_European_Aspen/Tree_European_Aspen_01/Tree_European_Aspen_01_A.Tree_European_Aspen_01_A"),
		TEXT("/Game/Megaplant_Library/Tree_European_Aspen/Tree_European_Aspen_01/Tree_European_Aspen_01_B.Tree_European_Aspen_01_B"),
		TEXT("/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_A.Tree_European_Beech_01_A"),
		TEXT("/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_B.Tree_European_Beech_01_B"),
		TEXT("/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/Tree_Black_Alder_01_A.Tree_Black_Alder_01_A"),
		TEXT("/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/Tree_Black_Alder_01_B.Tree_Black_Alder_01_B"),
		TEXT("/Game/Megaplant_Library/Tree_Common_Hazel/Tree_Common_Hazel_01/Tree_Common_Hazel_01_A.Tree_Common_Hazel_01_A")
	};
	for (const TCHAR* AssetPath : BroadleafAssemblyAssetPaths)
	{
		USkeletalMesh* BroadleafAsset = LoadObject<USkeletalMesh>(nullptr, AssetPath);
		TestNotNull(FString::Printf(TEXT("Broadleaf skeletal mesh is available: %s"), AssetPath), BroadleafAsset);
		if (BroadleafAsset)
		{
			TestTrue(FString::Printf(TEXT("Broadleaf mesh has Nanite enabled: %s"), AssetPath),
				BroadleafAsset->IsNaniteEnabled());
			TestTrue(FString::Printf(TEXT("Broadleaf mesh is a Nanite Assembly: %s"), AssetPath),
				BroadleafAsset->IsNaniteAssembly());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherBroadleafHeightProfilesTest,
	"CaptiveSky2.Agent.IslandWeather.BroadleafHeightProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherBroadleafHeightProfilesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Norway maple occupies the shorter lower-canopy height band"),
		AIslandWeather::GetBroadleafTreeHeight(0, 0.f), 1000.f);
	TestEqual(TEXT("European aspen reaches the taller upper-canopy height band"),
		AIslandWeather::GetBroadleafTreeHeight(1, 1.f), 1800.f);
	TestEqual(TEXT("European beech retains an overlapping mid-canopy profile"),
		AIslandWeather::GetBroadleafTreeHeight(2, 0.5f), 1325.f);
	TestEqual(TEXT("Black alder retains a lower moisture-edge canopy profile"),
		AIslandWeather::GetBroadleafTreeHeight(3, 1.f), 1400.f);
	TestEqual(TEXT("Common hazel occupies a distinct lower woodland-canopy band"),
		AIslandWeather::GetBroadleafTreeHeight(4, 0.f), 450.f);
	TestEqual(TEXT("Common hazel reaches the installed mesh's approximately five-metre silhouette"),
		AIslandWeather::GetBroadleafTreeHeight(4, 1.f), 700.f);
	TestEqual(TEXT("Height samples below range clamp to the species minimum"),
		AIslandWeather::GetBroadleafTreeHeight(3, -1.f), 900.f);
	TestEqual(TEXT("Non-finite height samples resolve to the profile midpoint"),
		AIslandWeather::GetBroadleafTreeHeight(0, std::numeric_limits<float>::quiet_NaN()), 1250.f);
	TestEqual(TEXT("Invalid species indices do not produce a tree height"),
		AIslandWeather::GetBroadleafTreeHeight(5, 0.5f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherBroadleafHabitatSelectionTest,
	"CaptiveSky2.Agent.IslandWeather.BroadleafHabitatSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherBroadleafHabitatSelectionTest::RunTest(const FString& Parameters)
{
	constexpr int32 HabitatSeed = 12345;
	const FVector PatchOrigin(0.f, 0.f, 0.f);
	const int32 PatchSpecies = AIslandWeather::GetBroadleafSpeciesIndexAt(PatchOrigin, HabitatSeed);
	TestTrue(TEXT("A sampled habitat field resolves to one of the five broadleaf species"),
		PatchSpecies >= 0 && PatchSpecies < 5);
	TestEqual(TEXT("Nearby trees remain in the same smoothed habitat patch"),
		AIslandWeather::GetBroadleafSpeciesIndexAt(FVector(500.f, 700.f, 0.f), HabitatSeed), PatchSpecies);
	TestEqual(TEXT("A second nearby tree remains in that habitat patch"),
		AIslandWeather::GetBroadleafSpeciesIndexAt(FVector(1000.f, 1000.f, 0.f), HabitatSeed), PatchSpecies);
	TestEqual(TEXT("Repeated position and seed resolve deterministically"),
		AIslandWeather::GetBroadleafSpeciesIndexAt(PatchOrigin, HabitatSeed), PatchSpecies);
	// Sample a 3 km coordinate neighborhood around the Island's authored landmark
	// center. This checks the species field at scene scale, not which samples are
	// valid land; the former test only marched along one axis out to 10 km, well
	// beyond the playable landscape. The placement path salts WeatherSeed with
	// this value before sampling, so use the same field here.
	int32 SpeciesCounts[AIslandWeather::BroadleafTreeSpeciesCount] = {};
	const FVector IslandCenter(-100000.f, 100000.f, 0.f);
	const int32 PlacementSeed = HabitatSeed ^ 0x3b67ae85;
	for (int32 GridY = -4; GridY <= 4; ++GridY)
	{
		for (int32 GridX = -4; GridX <= 4; ++GridX)
		{
			const FVector Sample = IslandCenter + FVector(GridX * 37500.f, GridY * 37500.f, 0.f);
			const int32 Species = AIslandWeather::GetBroadleafSpeciesIndexAt(Sample, PlacementSeed);
			if (Species >= 0 && Species < AIslandWeather::BroadleafTreeSpeciesCount)
				++SpeciesCounts[Species];
		}
	}
	// Keep all five species present and avoid one habitat label dominating this
	// representative 81-point Island-scale sample. This measures the smoothed
	// field, not valid land or final placed-tree counts.
	for (int32 Species = 0; Species < AIslandWeather::BroadleafTreeSpeciesCount; ++Species)
	{
		TestTrue(FString::Printf(TEXT("Broadleaf species %d is selectable in the Island-scale habitat field"), Species),
			SpeciesCounts[Species] > 0);
		TestTrue(FString::Printf(TEXT("Broadleaf species %d retains a meaningful share of the representative Island-scale field"), Species),
			SpeciesCounts[Species] >= 5 && SpeciesCounts[Species] <= 28);
	}
	return true;
}
