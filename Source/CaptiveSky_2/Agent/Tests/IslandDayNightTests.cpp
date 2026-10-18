#include "Misc/AutomationTest.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "ProceduralMeshComponent.h"
#include "IslandDayNight.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWeather.h"
#include "Engine/World.h"
#include "IslandWorldStateSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandClockTest, "CaptiveSky2.Agent.DayNight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIslandClockTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Midnight wraps"), AIslandDayNight::WrapHour(24), 0.f);
	TestEqual(TEXT("Negative preview wraps"), AIslandDayNight::WrapHour(-1), 23.f);
	TestTrue(TEXT("Sun rises at six"), FMath::IsNearlyZero(AIslandDayNight::SunHeight(6)));
	TestTrue(TEXT("Sun overhead at noon"), FMath::IsNearlyEqual(AIslandDayNight::SunHeight(12), 1.f));
	TestTrue(TEXT("Sun below horizon at midnight"), FMath::IsNearlyEqual(AIslandDayNight::SunHeight(0), -1.f));
	TestEqual(TEXT("A high daytime sun receives no artificial twilight fill"), AIslandDayNight::TwilightFillAmount(12.f), 0.f);
	TestTrue(TEXT("Low golden-hour sun receives a partial, bounded cool fill"),
		AIslandDayNight::TwilightFillAmount(17.f) > 0.5f && AIslandDayNight::TwilightFillAmount(17.f) < 1.f &&
		AIslandDayNight::TwilightFillStrength <= 0.6f);
	TestTrue(TEXT("The twilight fill reaches full strength only near the horizon"),
		AIslandDayNight::TwilightFillAmount(18.f) > AIslandDayNight::TwilightFillAmount(17.5f));
	TestTrue(TEXT("Golden-hour ambient light lifts shadows at 17:00"),
		AIslandDayNight::GoldenHourSkylightScale(17.f) > 1.f &&
		AIslandDayNight::GoldenHourSkylightScale(17.f) < 1.f + AIslandDayNight::GoldenHourSkylightBoost);
	TestTrue(TEXT("The golden-hour fill grows smoothly toward sunset"),
		AIslandDayNight::GoldenHourSkylightScale(17.5f) > AIslandDayNight::GoldenHourSkylightScale(17.f));
	TestEqual(TEXT("Night ambient brightness is unchanged by the golden-hour lift"),
		AIslandDayNight::GoldenHourSkylightScale(20.f), 1.f);
	const float QuarterMoonHour = static_cast<float>((AIslandDayNight::LunarCycleDays * 0.25 - 7.0) * 24.0);
	const float FullMoonHour = static_cast<float>((AIslandDayNight::LunarCycleDays * 0.5 - 14.0) * 24.0);
	const float NextNewMoonHour = static_cast<float>((AIslandDayNight::LunarCycleDays - 29.0) * 24.0);
	TestTrue(TEXT("Day one begins at a new moon"), FMath::IsNearlyZero(AIslandDayNight::LunarIllumination(1, 0.f), 0.001f));
	TestTrue(TEXT("A quarter cycle has half illumination"), FMath::IsNearlyEqual(AIslandDayNight::LunarIllumination(8, QuarterMoonHour), 0.5f, 0.001f));
	TestEqual(TEXT("A new moon still lights the night"), AIslandDayNight::MoonlightScale(0.f), AIslandDayNight::NewMoonLightFloor);
	TestEqual(TEXT("A full moon gives full moonlight"), AIslandDayNight::MoonlightScale(1.f), 1.f);
	TestTrue(TEXT("Half a lunar cycle reaches a full moon"), FMath::IsNearlyEqual(AIslandDayNight::LunarIllumination(15, FullMoonHour), 1.f, 0.001f));
	TestTrue(TEXT("The 29.53-day cycle returns continuously to new moon"),
		FMath::IsNearlyZero(AIslandDayNight::LunarIllumination(30, NextNewMoonHour), 0.001f));
	TestTrue(TEXT("A new moon rides with the sun: up at noon, down at midnight"),
		AIslandDayNight::MoonHeight(1, 12.f) > 0.99f && AIslandDayNight::MoonHeight(1, 0.f) < -0.99f);
	TestTrue(TEXT("A near-full moon is high at midnight and down at noon"),
		AIslandDayNight::MoonHeight(15, 0.f) > 0.9f && AIslandDayNight::MoonHeight(15, 12.f) < -0.9f);
	TestTrue(TEXT("Thick cloud hides a risen moon at night"), AIslandDayNight::IsMoonHiddenByCloud(0.7f, 1.f, 0.95f));
	TestFalse(TEXT("A clear sky shows the moon"), AIslandDayNight::IsMoonHiddenByCloud(0.7f, 1.f, 0.1f));
	TestFalse(TEXT("A set moon is not described as hidden"), AIslandDayNight::IsMoonHiddenByCloud(-0.5f, 1.f, 0.95f));
	TestFalse(TEXT("Daylight cloud does not mention the moon"), AIslandDayNight::IsMoonHiddenByCloud(0.7f, 0.f, 0.95f));
	TestEqual(TEXT("Cycle start is a new moon"), FString(AIslandDayNight::LunarPhaseName(0.f)), FString(TEXT("new moon")));
	TestEqual(TEXT("A quarter of the way is first quarter"), FString(AIslandDayNight::LunarPhaseName(0.25f)), FString(TEXT("first quarter")));
	TestEqual(TEXT("Half way is a full moon"), FString(AIslandDayNight::LunarPhaseName(0.5f)), FString(TEXT("full moon")));
	TestEqual(TEXT("Late cycle is a waning crescent"), FString(AIslandDayNight::LunarPhaseName(0.88f)), FString(TEXT("waning crescent")));
	TestEqual(TEXT("The end of the cycle wraps back to new moon"), FString(AIslandDayNight::LunarPhaseName(0.97f)), FString(TEXT("new moon")));
	TestEqual(TEXT("Non-finite progress is harmless"), FString(AIslandDayNight::LunarPhaseName(NAN)), FString(TEXT("new moon")));
	TestTrue(TEXT("Lunar phase advances smoothly across Island midnight"),
		FMath::Abs(AIslandDayNight::LunarPhaseProgress(2, 0.f) - AIslandDayNight::LunarPhaseProgress(1, 23.99f)) < 0.001f);
	TestTrue(TEXT("Clear weather leaves daylight unchanged"), FMath::IsNearlyEqual(AIslandDayNight::CloudSunlightTransmission(0.f), 1.f) && FMath::IsNearlyEqual(AIslandDayNight::CloudSkylightTransmission(0.f), 1.f));
	TestTrue(TEXT("Overcast modestly softens direct sunlight"), FMath::IsNearlyEqual(AIslandDayNight::CloudSunlightTransmission(1.f), 0.76f));
	TestTrue(TEXT("Overcast gently softens ambient skylight"), FMath::IsNearlyEqual(AIslandDayNight::CloudSkylightTransmission(1.f), 0.70f));
	TestTrue(TEXT("Clear skies preserve full moonlight while overcast blocks most direct moonlight"),
		FMath::IsNearlyEqual(AIslandDayNight::CloudMoonlightTransmission(0.f), 1.f) &&
		FMath::IsNearlyEqual(AIslandDayNight::CloudMoonlightTransmission(1.f), AIslandDayNight::OvercastMoonlightFloor));
	TestTrue(TEXT("Clouds attenuate moonlight smoothly and more strongly than daylight"),
		AIslandDayNight::CloudMoonlightTransmission(0.75f) < AIslandDayNight::CloudMoonlightTransmission(0.25f) &&
		AIslandDayNight::CloudMoonlightTransmission(0.75f) < AIslandDayNight::CloudSkylightTransmission(0.75f));
	TestTrue(TEXT("Moonlight transmission clamps invalid cloud-cover bounds"),
		FMath::IsNearlyEqual(AIslandDayNight::CloudMoonlightTransmission(-1.f), 1.f) &&
		FMath::IsNearlyEqual(AIslandDayNight::CloudMoonlightTransmission(2.f), AIslandDayNight::OvercastMoonlightFloor));
	TestTrue(TEXT("Cloud illumination remains bounded for out-of-range input"), AIslandDayNight::CloudSunlightTransmission(2.f) == AIslandDayNight::CloudSunlightTransmission(1.f));

	if (!TestNotNull(TEXT("Engine is available for the lighting fixture"), GEngine)) return false;
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Lighting fixture world created"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(Spawn);
	ASkyLight* Sky = World->SpawnActor<ASkyLight>(Spawn);
	if (!TestNotNull(TEXT("Weather spawned in lighting fixture"), Weather) || !TestNotNull(TEXT("Clock spawned in lighting fixture"), Clock) || !TestNotNull(TEXT("Sun spawned in lighting fixture"), Sun) || !TestNotNull(TEXT("Sky spawned in lighting fixture"), Sky))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Clock->Sun = Sun;
	Clock->Sky = Sky;
	TestNotNull(TEXT("The shared clock owns a procedural starfield"), Clock->Starfield);
	TestNotNull(TEXT("The shared clock owns a night fill light"), Clock->Starlight.Get());
	auto VerifyForwardLightSelection = [this, Sun, Clock](const TCHAR* Phase)
	{
		TArray<UDirectionalLightComponent*> Lights;
		Lights.Add(Cast<UDirectionalLightComponent>(Sun->GetLightComponent()));
		Lights.Add(Clock->Moon.Get());
		Lights.Add(Clock->Starlight.Get());
		Lights.Remove(nullptr);
		TSet<int32> Priorities;
		UDirectionalLightComponent* BrightestLight = nullptr;
		for (UDirectionalLightComponent* Light : Lights)
		{
			Priorities.Add(Light->ForwardShadingPriority);
			if (!BrightestLight || Light->Intensity > BrightestLight->Intensity) BrightestLight = Light;
		}
		TestEqual(FString::Printf(TEXT("%s assigns a unique forward priority to each Island directional light"), Phase),
			Priorities.Num(), Lights.Num());
		if (BrightestLight)
			TestEqual(FString::Printf(TEXT("%s gives the brightest directional light the unique top priority"), Phase),
				BrightestLight->ForwardShadingPriority, Lights.Num() - 1);
	};
	if (Clock->Starfield)
	{
		TestEqual(TEXT("The starfield has six progressive twilight groups"), Clock->Starfield->GetNumSections(), AIslandDayNight::NightStarSectionCount);
		int32 ProceduralStarVertices = 0;
		float SmallestStarWidth = TNumericLimits<float>::Max();
		float LargestStarWidth = 0.f;
		for (int32 SectionIndex = 0; SectionIndex < AIslandDayNight::NightStarSectionCount; ++SectionIndex)
			if (const FProcMeshSection* Section = Clock->Starfield->GetProcMeshSection(SectionIndex))
			{
				ProceduralStarVertices += Section->ProcVertexBuffer.Num();
				for (int32 VertexIndex = 0; VertexIndex + 3 < Section->ProcVertexBuffer.Num(); VertexIndex += 4)
				{
					const float StarWidth = FVector::Dist(Section->ProcVertexBuffer[VertexIndex].Position,
						Section->ProcVertexBuffer[VertexIndex + 1].Position);
					SmallestStarWidth = FMath::Min(SmallestStarWidth, StarWidth);
					LargestStarWidth = FMath::Max(LargestStarWidth, StarWidth);
				}
			}
		TestEqual(TEXT("The seeded starfield contains the complete fixed population"), ProceduralStarVertices, AIslandDayNight::NightStarCount * 4);
		const float MinimumStarPixelsAt1080p = (SmallestStarWidth / AIslandDayNight::NightStarShellRadius) *
			(180.f / PI) * (1920.f / 90.f);
		TestTrue(TEXT("Gameplay-scale star cards are no longer sub-pixel at 1080p/90-degree FOV"), MinimumStarPixelsAt1080p >= 1.f);
		TestTrue(TEXT("Random star cards remain within the authored size envelope"),
			SmallestStarWidth >= AIslandDayNight::NightStarHalfSizeMin * 2.f &&
			LargestStarWidth <= AIslandDayNight::NightStarHalfSizeMax * 2.f);
	}
	Clock->CurrentHour = 12.f;
	Clock->DayNumber = 1;
	Clock->DaySunIntensity = 10.f;
	Clock->UpdateLighting();
	VerifyForwardLightSelection(TEXT("Noon"));
	const float NoonSkyIntensity = Sky->GetLightComponent()->Intensity;
	bool bNoStarGroupsAtNoon = true;
	if (Clock->Starfield)
		for (int32 SectionIndex = 0; SectionIndex < AIslandDayNight::NightStarSectionCount; ++SectionIndex)
		{
			const FProcMeshSection* Section = Clock->Starfield->GetProcMeshSection(SectionIndex);
			bNoStarGroupsAtNoon &= Section && !Section->bSectionVisible;
		}
	TestTrue(TEXT("Daylight keeps all star groups hidden"), bNoStarGroupsAtNoon);
	TestEqual(TEXT("Default moon illumination is tuned for a visible but dark night"), Clock->MoonIntensity, 1.5f);
	TestTrue(TEXT("Fixed-exposure night fill keeps a readable skylight floor"), AIslandDayNight::NightSkylightFloor >= 2.5f);
	Clock->CurrentHour = 17.f;
	Clock->UpdateLighting();
	TestTrue(TEXT("Golden hour adds ambient skylight above the ordinary daytime level"),
		Sky->GetLightComponent()->Intensity > NoonSkyIntensity);
	TestTrue(TEXT("Golden-hour skylight follows its bounded scale"),
		FMath::IsNearlyEqual(Sky->GetLightComponent()->Intensity,
			NoonSkyIntensity * AIslandDayNight::GoldenHourSkylightScale(17.f), 0.001f));
	Clock->CurrentHour = 18.5f;
	Clock->UpdateLighting();
	VerifyForwardLightSelection(TEXT("Dusk"));
	int32 VisibleStarGroupsAtDusk = 0;
	if (Clock->Starfield)
		for (int32 SectionIndex = 0; SectionIndex < AIslandDayNight::NightStarSectionCount; ++SectionIndex)
			if (const FProcMeshSection* Section = Clock->Starfield->GetProcMeshSection(SectionIndex); Section && Section->bSectionVisible)
				++VisibleStarGroupsAtDusk;
	TestTrue(TEXT("Twilight reveals only the first portion of the star groups"),
		VisibleStarGroupsAtDusk > 0 && VisibleStarGroupsAtDusk < AIslandDayNight::NightStarSectionCount);
	Clock->CurrentHour = 0.f;
	Clock->UpdateLighting();
	VerifyForwardLightSelection(TEXT("New moon"));
	bool bFirstTwilightGroupVisible = false;
	if (Clock->Starfield)
		if (const FProcMeshSection* FirstStarGroup = Clock->Starfield->GetProcMeshSection(0))
			bFirstTwilightGroupVisible = FirstStarGroup->bSectionVisible;
	TestTrue(TEXT("The first star group appears after sunset"), bFirstTwilightGroupVisible);
	if (Clock->Starlight)
	{
		TestTrue(TEXT("A new-moon midnight is still lit by the night fill"), Clock->Starlight->Intensity > 0.f);
		TestTrue(TEXT("The night fill shines down from the sky, not up from below the horizon"), Clock->Starlight->GetForwardVector().Z < 0.f);
		TestFalse(TEXT("The night fill casts no shadows"), Clock->Starlight->CastShadows);
		Clock->CurrentHour = 17.5f;
		Clock->UpdateLighting();
		const float ExpectedTwilightFill = Clock->StarlightIntensity * AIslandDayNight::TwilightFillStrength *
			AIslandDayNight::TwilightFillAmount(Clock->CurrentHour);
		TestTrue(TEXT("The shadowless sky fill softly lifts the low-sun transition"),
			FMath::IsNearlyEqual(Clock->Starlight->Intensity, ExpectedTwilightFill, 0.001f) && ExpectedTwilightFill > 0.f);
		Clock->CurrentHour = 12.f;
		Clock->UpdateLighting();
		TestTrue(TEXT("The sky fill turns fully off again at noon"), Clock->Starlight->Intensity <= 0.001f);
		Clock->CurrentHour = 0.f;
		Clock->UpdateLighting();
	}
	const float NewMoonOffset = FMath::Abs(FMath::FindDeltaAngleDegrees(Sun->GetActorRotation().Pitch, Clock->Moon->GetComponentRotation().Pitch));
	TestTrue(TEXT("Near new moon, the moon follows the sun's arc"), NewMoonOffset < 2.f);
	const FString NewMoonDescription = Clock->DescribeTime();
	TestTrue(TEXT("Time observations describe a waxing phase with very faint illumination"),
		NewMoonDescription.Contains(TEXT("waxing")) && NewMoonDescription.Contains(TEXT("very faint")));
	Clock->DayNumber = 15;
	Clock->UpdateLighting();
	VerifyForwardLightSelection(TEXT("Full moon"));
	const float FullMoonOffset = FMath::Abs(FMath::FindDeltaAngleDegrees(Sun->GetActorRotation().Pitch, Clock->Moon->GetComponentRotation().Pitch));
	TestTrue(TEXT("Near full moon, the moon moves opposite the sun's arc"), FullMoonOffset > 170.f);
	const FString FullMoonDescription = Clock->DescribeTime();
	TestTrue(TEXT("Time observations describe a waxing phase with bright illumination"),
		FullMoonDescription.Contains(TEXT("waxing")) && FullMoonDescription.Contains(TEXT("bright")));
	Clock->CurrentHour = 12.f;
	Clock->MoonIntensity = 0.f;
	float MinimumCover = 2.f;
	float MaximumCover = -1.f;
	int32 ClearSeed = 0;
	int32 OvercastSeed = 0;
	for (int32 Seed = -1000; Seed <= 1000; ++Seed)
	{
		Weather->WeatherSeed = Seed;
		const float Cover = Weather->SampleCloudCover(World->GetTimeSeconds());
		if (Cover < MinimumCover) { MinimumCover = Cover; ClearSeed = Seed; }
		if (Cover > MaximumCover) { MaximumCover = Cover; OvercastSeed = Seed; }
	}
	Weather->WeatherSeed = ClearSeed;
	Clock->UpdateLighting();
	const float ClearSun = Sun->GetLightComponent()->Intensity;
	const float ClearSky = Sky->GetLightComponent()->Intensity;
	Weather->WeatherSeed = OvercastSeed;
	Clock->UpdateLighting();
	TestTrue(TEXT("Overcast cloud signal dims the actual directional sunlight"), Sun->GetLightComponent()->Intensity < ClearSun * 0.8f);
	TestTrue(TEXT("Overcast cloud signal dims the actual skylight"), Sky->GetLightComponent()->Intensity < ClearSky * 0.75f);
	Weather->WeatherSeed = ClearSeed;
	Clock->CurrentHour = 0.f;
	Clock->DayNumber = 15;
	Clock->MoonIntensity = 1.5f;
	Clock->UpdateLighting();
	bool bAllStarGroupsVisible = true;
	if (Clock->Starfield)
		for (int32 SectionIndex = 0; SectionIndex < AIslandDayNight::NightStarSectionCount; ++SectionIndex)
		{
			const FProcMeshSection* Section = Clock->Starfield->GetProcMeshSection(SectionIndex);
			bAllStarGroupsVisible &= Section && Section->bSectionVisible;
		}
	TestTrue(TEXT("A dark full-moon midnight reveals every star group"), bAllStarGroupsVisible);
	TestTrue(TEXT("Midnight extinguishes direct sunlight"), Sun->GetLightComponent()->Intensity <= 0.001f);
	TestTrue(TEXT("A clear near-full-moon midnight keeps moonlight active"), FMath::IsNearlyEqual(Clock->Moon->Intensity,
		Clock->MoonIntensity * AIslandDayNight::MoonlightScale(AIslandDayNight::LunarIllumination(Clock->DayNumber, Clock->CurrentHour)) *
		AIslandDayNight::CloudMoonlightTransmission(Weather->SampleCloudCover(World->GetTimeSeconds())), 0.001f));
	const float ClearMoonlight = Clock->Moon->Intensity;
	Weather->WeatherSeed = OvercastSeed;
	Clock->UpdateLighting();
	TestTrue(TEXT("Overcast clouds attenuate the actual moonlight component"), Clock->Moon->Intensity < ClearMoonlight);
	Weather->WeatherSeed = ClearSeed;
	Clock->UpdateLighting();
	TestTrue(TEXT("The ambient sky retains the tested low-light visibility floor"),
		FMath::IsNearlyEqual(Sky->GetLightComponent()->Intensity,
			AIslandDayNight::NightSkylightFloor * AIslandDayNight::CloudSkylightTransmission(Weather->SampleCloudCover(World->GetTimeSeconds())), 0.001f));
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandClockPersistenceTest, "CaptiveSky2.Agent.DayNightPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIslandClockPersistenceTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Engine is available for the clock fixture"), GEngine)) return false;
	const FString StateFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / TEXT("IslandClock") / TEXT("WorldState.json"));
	IFileManager::Get().Delete(*StateFile, false, true, true);
	// An empty File leaves the fixture world on its default storage, which should be none.
	auto StartSession = [](UWorld*& OutWorld, const FString& File, bool bResume) -> AIslandDayNight*
	{
		const UWorld::InitializationValues Init = UWorld::InitializationValues()
			.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
			.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
		OutWorld = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(OutWorld);
		UIslandWorldStateSubsystem* State = OutWorld->GetSubsystem<UIslandWorldStateSubsystem>();
		if (State) State->StorageFileOverride = File;
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		OutWorld->SpawnActor<AIslandWeather>(Spawn);
		AIslandDayNight* Clock = OutWorld->SpawnActor<AIslandDayNight>(Spawn);
		Clock->StartHour = 9.f;
		Clock->bResumeSavedTime = bResume;
		OutWorld->BeginPlay();
		// Fixture worlds have no game mode to begin play for actors, so start the clock directly.
		Clock->DispatchBeginPlay();
		return Clock;
	};
	auto EndSession = [](UWorld* World, AIslandDayNight* Clock)
	{
		// Fixture actors are never initialized for play, so the engine skips EndPlay; run its save directly.
		Clock->PersistHour();
		Clock->Destroy();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	UWorld* World = nullptr;
	AIslandDayNight* Clock = StartSession(World, FString(), true);
	const UIslandWorldStateSubsystem* FixtureState = World->GetSubsystem<UIslandWorldStateSubsystem>();
	TestTrue(TEXT("Code-created worlds have no lasting storage"), FixtureState && FixtureState->GetStorageFilePath().IsEmpty());
	TestEqual(TEXT("Without saved time the clock uses Start Hour"), Clock->CurrentHour, 9.f);
	EndSession(World, Clock);

	Clock = StartSession(World, StateFile, true);
	TestEqual(TEXT("First session starts at Start Hour"), Clock->CurrentHour, 9.f);
	UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
	UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>();
	TestTrue(TEXT("The first session records lingering wetness"), State && State->SaveWetness(0.74f));
	if (Environment) Environment->Tick(0.f);
	TestTrue(TEXT("The environment starts from its saved wetness"), Environment && FMath::IsNearlyEqual(Environment->GetWetness(), 0.74f));
	Clock->CurrentHour = 21.5f;
	Clock->DayNumber = 4;
	EndSession(World, Clock);
	TestTrue(TEXT("Ending play saves the Island hour"), FPaths::FileExists(StateFile));
	FString SavedState;
	TestTrue(TEXT("Saved Island state can be inspected by the headless gateway"), FFileHelper::LoadFileToString(SavedState, *StateFile));
	TestTrue(TEXT("The environment snapshot is timestamped"), SavedState.Contains(TEXT("\"observed_utc\"")));
	TestTrue(TEXT("Saved weather includes rain, clouds, mist, and wind components"),
		SavedState.Contains(TEXT("\"rain_intensity\"")) && SavedState.Contains(TEXT("\"cloud_cover\"")) &&
		SavedState.Contains(TEXT("\"mist\"")) && SavedState.Contains(TEXT("\"wind_speed_cm_s\"")) &&
		SavedState.Contains(TEXT("\"wind_x_cm_s\"")) && SavedState.Contains(TEXT("\"wind_y_cm_s\"")));

	Clock = StartSession(World, StateFile, true);
	TestTrue(TEXT("Next session resumes where the last one ended"), FMath::IsNearlyEqual(Clock->CurrentHour, 21.5f, 0.01f));
	TestEqual(TEXT("Next session resumes on the same Island day"), Clock->DayNumber, 4);
	Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>();
	if (Environment) Environment->Tick(0.f);
	TestTrue(TEXT("Lingering wet ground survives the session boundary"), Environment && FMath::IsNearlyEqual(Environment->GetWetness(), 0.74f));
	Clock->CurrentHour = 23.99f;
	Clock->Tick(10.f);
	TestEqual(TEXT("Midnight begins a new Island day"), Clock->DayNumber, 5);
	Clock->CurrentHour = 21.5f;
	Clock->Tick(61.f);
	const float AfterTick = Clock->CurrentHour;
	Clock->CurrentHour = 3.f;
	State->LoadAndSpawn();
	TestTrue(TEXT("Clock saves periodically during play, not only at the end"), State->GetSavedHour().IsSet() && FMath::IsNearlyEqual(State->GetSavedHour().GetValue(), AfterTick, 0.01f));
	TestTrue(TEXT("Wetness remains available in the shared world-state file"), State->GetSavedWetness().IsSet() && FMath::IsNearlyEqual(State->GetSavedWetness().GetValue(), 0.74f));
	Clock->bResumeSavedTime = false;
	EndSession(World, Clock);

	Clock = StartSession(World, StateFile, false);
	TestEqual(TEXT("Opting out of resume starts at Start Hour"), Clock->CurrentHour, 9.f);
	EndSession(World, Clock);
	IFileManager::Get().Delete(*StateFile, false, true, true);
	return true;
}
