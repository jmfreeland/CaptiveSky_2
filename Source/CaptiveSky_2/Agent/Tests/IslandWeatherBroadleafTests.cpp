#include "Misc/AutomationTest.h"
#include "IslandWeather.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherBroadleafDiversityTest,
	"CaptiveSky2.Agent.IslandWeather.BroadleafDiversity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherBroadleafDiversityTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The first habitat-field quarter selects Norway maple"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.125f), 0);
	TestEqual(TEXT("The second habitat-field quarter selects European aspen"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.375f), 1);
	TestEqual(TEXT("The third habitat-field quarter selects European beech"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.625f), 2);
	TestEqual(TEXT("The final habitat-field quarter selects black alder"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.875f), 3);
	TestEqual(TEXT("A field at the upper endpoint stays within the last species"),
		AIslandWeather::GetBroadleafSpeciesIndex(1.f), 3);
	TestEqual(TEXT("A field below zero stays within the first species"),
		AIslandWeather::GetBroadleafSpeciesIndex(-0.1f), 0);
	TestEqual(TEXT("A non-finite habitat field resolves deterministically"),
		AIslandWeather::GetBroadleafSpeciesIndex(std::numeric_limits<float>::quiet_NaN()), 0);
	TestEqual(TEXT("Four resolved species share the 3,600-tree budget evenly"),
		AIslandWeather::GetBroadleafSpeciesLimit(4), 900);
	TestEqual(TEXT("Three resolved species preserve the former 1,200-tree shares"),
		AIslandWeather::GetBroadleafSpeciesLimit(3), 1200);
	TestEqual(TEXT("A single resolved species can use the full fixed population budget"),
		AIslandWeather::GetBroadleafSpeciesLimit(1), AIslandWeather::BroadleafTreePopulationBudget);
	TestEqual(TEXT("No resolved species admits no broadleaf instances"),
		AIslandWeather::GetBroadleafSpeciesLimit(0), 0);

	const AIslandWeather* Defaults = GetDefault<AIslandWeather>();
	const UInstancedSkinnedMeshComponent* BlackAlders = Defaults ? Defaults->IslandBlackAlders : nullptr;
	TestNotNull(TEXT("Black Alder uses its own instanced-skinned foliage component"), BlackAlders);
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

	const TCHAR* BlackAlderAssetPath = TEXT("/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/Tree_Black_Alder_01_A.Tree_Black_Alder_01_A");
	TestNotNull(TEXT("The approved Black Alder A skeletal mesh is available"),
		LoadObject<USkeletalMesh>(nullptr, BlackAlderAssetPath));
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
	TestEqual(TEXT("Height samples below range clamp to the species minimum"),
		AIslandWeather::GetBroadleafTreeHeight(3, -1.f), 900.f);
	TestEqual(TEXT("Non-finite height samples resolve to the profile midpoint"),
		AIslandWeather::GetBroadleafTreeHeight(0, std::numeric_limits<float>::quiet_NaN()), 1250.f);
	TestEqual(TEXT("Invalid species indices do not produce a tree height"),
		AIslandWeather::GetBroadleafTreeHeight(4, 0.5f), 0.f);
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
	TestTrue(TEXT("A sampled habitat field resolves to one of the four broadleaf species"),
		PatchSpecies >= 0 && PatchSpecies < 4);
	TestEqual(TEXT("Nearby trees remain in the same smoothed habitat patch"),
		AIslandWeather::GetBroadleafSpeciesIndexAt(FVector(500.f, 700.f, 0.f), HabitatSeed), PatchSpecies);
	TestEqual(TEXT("A second nearby tree remains in that habitat patch"),
		AIslandWeather::GetBroadleafSpeciesIndexAt(FVector(1000.f, 1000.f, 0.f), HabitatSeed), PatchSpecies);
	TestEqual(TEXT("Repeated position and seed resolve deterministically"),
		AIslandWeather::GetBroadleafSpeciesIndexAt(PatchOrigin, HabitatSeed), PatchSpecies);
	int32 DistantSpecies = PatchSpecies;
	for (int32 Step = 1; Step <= 200 && DistantSpecies == PatchSpecies; ++Step)
		DistantSpecies = AIslandWeather::GetBroadleafSpeciesIndexAt(FVector(Step * 5000.f, 0.f, 0.f), HabitatSeed);
	TestTrue(TEXT("The smooth habitat field forms different broadleaf communities across the landscape"),
		DistantSpecies >= 0 && DistantSpecies < 4 && DistantSpecies != PatchSpecies);
	return true;
}
