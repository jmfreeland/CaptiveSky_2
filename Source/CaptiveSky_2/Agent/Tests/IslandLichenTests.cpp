#include "Misc/AutomationTest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IslandArrangement.h"
#include "IslandDayNight.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandLichenTest, "CaptiveSky2.Agent.IslandLichen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandLichenTest::RunTest(const FString& Parameters)
{
	using Arr = AIslandArrangement;
	TestEqual(TEXT("Noon is not night"), AIslandDayNight::NightAmount(12.f), 0.f);
	TestEqual(TEXT("Midnight is full night"), AIslandDayNight::NightAmount(0.f), 1.f);
	TestTrue(TEXT("Dusk is part way"), AIslandDayNight::NightAmount(18.5f) > 0.f && AIslandDayNight::NightAmount(18.5f) < 1.f);

	TestEqual(TEXT("A new work has no lichen glow"), Arr::LichenGlow(0), 0.f);
	TestEqual(TEXT("Lichen waits for the weathering week"), Arr::LichenGlow(Arr::DaysToWeather), 0.f);
	TestEqual(TEXT("A negative age is clamped"), Arr::LichenGlow(-3), 0.f);
	TestTrue(TEXT("Lichen strengthens with age"), Arr::LichenGlow(Arr::DaysToWeather + 4) < Arr::LichenGlow(Arr::DaysToWeather + 9));
	TestEqual(TEXT("Lichen reaches full glow"), Arr::LichenGlow(Arr::DaysToWeather + Arr::DaysToGlow), 1.f);
	TestEqual(TEXT("Lichen glow stays capped"), Arr::LichenGlow(500), 1.f);

	if (!TestNotNull(TEXT("Engine is available"), GEngine)) return false;
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Lichen fixture world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	Arr* Work = World->SpawnActor<Arr>(Spawn);
	if (TestNotNull(TEXT("Clock spawned"), Clock) && TestNotNull(TEXT("Arrangement spawned"), Work))
	{
		FIslandArrangementSite Site;
		Site.Id = TEXT("ArrangingGround_Test");
		Site.bHasWork = true;
		Site.Form = EIslandArrangementForm::Ring;
		Site.Seed = 41;
		Site.Day = 1;

		Clock->DayNumber = 30;
		Clock->CurrentHour = 0.f;
		Work->ShowSite(Site, 30);
		TestEqual(TEXT("One lichen patch per stone"), Work->GetVisibleLichenCount(), Work->GetVisibleStoneCount());
		TestTrue(TEXT("Old work glows on a deep night"), FMath::IsNearlyEqual(Work->GetLichenGlow(), 1.f));
		TestEqual(TEXT("The lichen lights the ground"), Work->GetLichenLightIntensity(), Arr::LichenLightIntensity);
		TestTrue(TEXT("The lichen patches are shown"), Work->Lichen->IsVisible());

		Clock->CurrentHour = 12.f;
		Work->UpdateLichen();
		TestEqual(TEXT("The glow is off by day"), Work->GetLichenGlow(), 0.f);
		TestEqual(TEXT("No lichen light by day"), Work->GetLichenLightIntensity(), 0.f);

		Site.Day = 27;
		Clock->CurrentHour = 0.f;
		Work->ShowSite(Site, 30);
		TestEqual(TEXT("A young work does not glow at night"), Work->GetLichenGlow(), 0.f);
		TestFalse(TEXT("A young work shows no lichen"), Work->Lichen->IsVisible());

		Clock->DayNumber = 27 + Arr::DaysToWeather + 8;
		Work->UpdateLichen();
		TestTrue(TEXT("The same work grows lichen as Island days pass"), Work->GetLichenGlow() > 0.f);
		TestTrue(TEXT("Its patches appear"), Work->Lichen->IsVisible());

		FIslandArrangementSite Empty;
		Empty.Id = TEXT("ArrangingGround_Empty");
		Work->ShowSite(Empty, Clock->DayNumber);
		TestEqual(TEXT("A site with no work has no lichen"), Work->GetVisibleLichenCount(), 0);
		TestEqual(TEXT("A site with no work gives no light"), Work->GetLichenLightIntensity(), 0.f);
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
