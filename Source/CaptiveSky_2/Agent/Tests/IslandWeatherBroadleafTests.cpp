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
	TestEqual(TEXT("The first grove-field quarter selects Norway maple"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.125f), 0);
	TestEqual(TEXT("The second grove-field quarter selects European aspen"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.375f), 1);
	TestEqual(TEXT("The third grove-field quarter selects European beech"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.625f), 2);
	TestEqual(TEXT("The final grove-field quarter selects black alder"),
		AIslandWeather::GetBroadleafSpeciesIndex(0.875f), 3);
	TestEqual(TEXT("A field at the upper endpoint stays within the last species"),
		AIslandWeather::GetBroadleafSpeciesIndex(1.f), 3);
	TestEqual(TEXT("A field below zero stays within the first species"),
		AIslandWeather::GetBroadleafSpeciesIndex(-0.1f), 0);
	TestEqual(TEXT("A non-finite grove field resolves deterministically"),
		AIslandWeather::GetBroadleafSpeciesIndex(std::numeric_limits<float>::quiet_NaN()), 0);
	TestEqual(TEXT("The first broadleaf in a maple grove starts with maple"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(0, 0), 0);
	TestEqual(TEXT("The second broadleaf in a grove rotates to aspen"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(0, 1), 1);
	TestEqual(TEXT("The third broadleaf in a grove rotates to beech"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(0, 2), 2);
	TestEqual(TEXT("The fourth broadleaf in a grove rotates to alder"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(0, 3), 3);
	TestEqual(TEXT("Species rotation wraps deterministically for larger groves"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(0, 4), 0);
	TestEqual(TEXT("Rotation begins with the grove's selected species"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(3, 1), 0);
	TestEqual(TEXT("Negative tree ordinals do not rotate before the grove's selected species"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(2, -1), 2);

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
		TestEqual(TEXT("Black Alder uses the same bounded 80 m fade start"), CullStart, 8000);
		TestEqual(TEXT("Black Alder uses the same bounded 125 m cull end"), CullEnd, 12500);
	}

	const TCHAR* BlackAlderAssetPath = TEXT("/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/Tree_Black_Alder_01_A.Tree_Black_Alder_01_A");
	TestNotNull(TEXT("The approved Black Alder A skeletal mesh is available"),
		LoadObject<USkeletalMesh>(nullptr, BlackAlderAssetPath));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherBroadleafMixingTest,
	"CaptiveSky2.Agent.IslandWeather.BroadleafMixing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherBroadleafMixingTest::RunTest(const FString& Parameters)
{
	for (int32 GroveSpecies = 0; GroveSpecies < 4; ++GroveSpecies)
	{
		int32 SpeciesCounts[4] = {};
		for (int32 TreeOrdinal = 0; TreeOrdinal < 8; ++TreeOrdinal)
		{
			const int32 SelectedSpecies = AIslandWeather::GetBroadleafSpeciesIndexInGrove(GroveSpecies, TreeOrdinal);
			TestTrue(TEXT("Within-grove species rotation stays inside the four-species range"),
				SelectedSpecies >= 0 && SelectedSpecies < 4);
			if (SelectedSpecies >= 0 && SelectedSpecies < 4) ++SpeciesCounts[SelectedSpecies];
		}

		for (int32 SpeciesIndex = 0; SpeciesIndex < 4; ++SpeciesIndex)
			TestEqual(TEXT("An eight-tree mixed-grove sample contains two of each resolved species"),
				SpeciesCounts[SpeciesIndex], 2);
	}

	TestEqual(TEXT("An invalid low grove species clamps to the first resolved species"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(-1, 0), 0);
	TestEqual(TEXT("An invalid high grove species clamps to the last resolved species"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(4, 0), 3);
	TestEqual(TEXT("Negative tree ordinals do not rotate before the grove's selected species"),
		AIslandWeather::GetBroadleafSpeciesIndexInGrove(2, -1), 2);
	return true;
}
