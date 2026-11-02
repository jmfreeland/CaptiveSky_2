#include "Misc/AutomationTest.h"
#include "IslandWeather.h"
#include "IslandDayNight.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandWeatherSeasonalFlowersTest,
	"CaptiveSky2.Agent.IslandWeather.SeasonalFlowers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandWeatherSeasonalFlowersTest::RunTest(const FString& Parameters)
{
	using Weather = AIslandWeather;
	TestTrue(TEXT("Woodland bluebells bloom through spring"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 1) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 90));
	TestTrue(TEXT("Bluebells ease from buds into spring bloom over an Island day"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::WoodlandBluebellSpeciesIndex, 1.5), 0.5f, 0.001f) &&
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::WoodlandBluebellSpeciesIndex, 2.0), 1.f, 0.001f));
	TestTrue(TEXT("Bluebells ease back to seed-head rest as spring ends"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::WoodlandBluebellSpeciesIndex, 90.5), 0.5f, 0.001f) &&
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::WoodlandBluebellSpeciesIndex, 91.0), 0.f, 0.001f));
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
	TestTrue(TEXT("Yellow-flag iris eases into and out of its longer season"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::TideglassYellowFlagIrisSpeciesIndex, 1.5), 0.5f, 0.001f) &&
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::TideglassYellowFlagIrisSpeciesIndex, 180.5), 0.5f, 0.001f));
	TestFalse(TEXT("Yellow-flag iris rests in autumn"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 181));
	TestFalse(TEXT("Bluebells rest through winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex, 316));
	TestFalse(TEXT("Marsh marigolds rest through winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassMarshMarigoldSpeciesIndex, 316));
	TestFalse(TEXT("Yellow-flag iris rests through winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::TideglassYellowFlagIrisSpeciesIndex, 316));
	TestTrue(TEXT("The annual bloom curve wraps smoothly back into spring"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(Weather::WoodlandBluebellSpeciesIndex,
			AIslandDayNight::CalendarDaysPerYear + 1.5), 0.5f, 0.001f) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(Weather::WoodlandBluebellSpeciesIndex,
			AIslandDayNight::CalendarDaysPerYear + 1));
	TestTrue(TEXT("Generic meadow flowers share the spring flush"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(0, 1) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(3, 90));
	TestFalse(TEXT("Short spring flowers rest by summer"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(3, 91));
	TestTrue(TEXT("Generic flower calendars overlap through summer and autumn"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(2, 136) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(7, 230));
	TestFalse(TEXT("All generic meadow flowers rest in winter"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(0, 316) &&
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(7, 271));
	TestTrue(TEXT("A generic meadow form eases into bloom across three Island days"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(0, 2.5), 0.5f, 0.001f) &&
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(0, 4.0), 1.f, 0.001f));
	TestTrue(TEXT("Late-season meadow forms open gradually rather than appearing as one synchronized patch"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(7, 134.5), 0.5f, 0.001f));
	TestTrue(TEXT("Unknown meadow indices retain the year-round compatibility fallback"),
		Weather::DoesMeadowFlowerSpeciesBloomInSeason(11, 316) &&
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(11, 1.5), 1.f, 0.001f));
	TestTrue(TEXT("A non-finite calendar sample fails closed for seasonal specialists"),
		FMath::IsNearlyEqual(Weather::GetMeadowFlowerSpeciesBloomProgress(
			Weather::WoodlandBluebellSpeciesIndex, std::numeric_limits<double>::infinity()), 0.f, 0.001f));
	return true;
}
