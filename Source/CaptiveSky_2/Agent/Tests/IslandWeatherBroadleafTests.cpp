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
