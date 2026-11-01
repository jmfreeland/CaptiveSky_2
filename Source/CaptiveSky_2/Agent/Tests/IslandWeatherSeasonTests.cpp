#include "Misc/AutomationTest.h"
#include "IslandWeather.h"
#include "IslandDayNight.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherSeasonalFlowersTest,
	"CaptiveSky2.Agent.IslandWeather.SeasonalFlowers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherSeasonalFlowersTest::RunTest(const FString& Parameters)
{
	using Weather = AIslandWeather;
	TestTrue(TEXT("Woodland bluebells bloom through spring"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 1) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 90));
	TestFalse(TEXT("Woodland bluebells rest after spring"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 91));
	TestTrue(TEXT("Tideglass marsh marigolds bloom in spring"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassMarshMarigoldSpeciesIndex, 46));
	TestFalse(TEXT("Tideglass marsh marigolds rest from summer onward"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassMarshMarigoldSpeciesIndex, 136));
	TestTrue(TEXT("Yellow-flag iris spans spring and summer"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 90) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 136) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 180));
	TestFalse(TEXT("Yellow-flag iris rests in autumn"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 181));
	TestFalse(TEXT("Bluebells rest through winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 316));
	TestFalse(TEXT("Marsh marigolds rest through winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassMarshMarigoldSpeciesIndex, 316));
	TestFalse(TEXT("Yellow-flag iris rests through winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 316));
	TestTrue(TEXT("The annual calendar wraps back to spring"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex,
			AIslandDayNight::CalendarDaysPerYear + 1));
	TestTrue(TEXT("Unclassified meadow species remain available year-round"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(0, 316) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(7, 136));
	return true;
}
