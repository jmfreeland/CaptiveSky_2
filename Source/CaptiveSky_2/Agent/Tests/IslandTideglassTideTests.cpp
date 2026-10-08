#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "AgentBrainComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IslandDayNight.h"
#include "IslandTideglassSubsystem.h"
#include "Misc/ScopeExit.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassTideTest, "CaptiveSky2.Agent.TideglassTide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassTideTest::RunTest(const FString& Parameters)
{
	const float NewMoonHigh = UIslandTideglassSubsystem::TideOffsetCm(6.21f, 1);
	const float NewMoonLow = UIslandTideglassSubsystem::TideOffsetCm(18.63f, 1);
	TestTrue(TEXT("A new-moon high tide remains within the subtle waterline bound"),
		NewMoonHigh > 13.8f && NewMoonHigh <= UIslandTideglassSubsystem::MaximumTideOffsetCm);
	TestTrue(TEXT("The following semidiurnal low tide is almost equally below the resting level"),
		NewMoonLow < -13.8f && NewMoonLow >= -UIslandTideglassSubsystem::MaximumTideOffsetCm);
	TestTrue(TEXT("The pool begins at its mean waterline at the day boundary"),
		FMath::IsNearlyZero(UIslandTideglassSubsystem::TideOffsetCm(0.f, 1), 0.01f));

	auto FindDailyPeak = [](int32 Day)
	{
		float Peak = 0.f;
		for (float Hour = 0.f; Hour < 24.f; Hour += 0.25f)
			Peak = FMath::Max(Peak, FMath::Abs(UIslandTideglassSubsystem::TideOffsetCm(Hour, Day)));
		return Peak;
	};
	const float SpringPeak = FindDailyPeak(1);
	const float QuarterMoonPeak = FindDailyPeak(8);
	TestTrue(TEXT("New-moon spring tides have a larger range than quarter-moon neap tides"),
		SpringPeak > QuarterMoonPeak * 1.3f);

	float LargestOffset = 0.f;
	for (int32 Day = 1; Day <= 60; ++Day)
		for (float Hour = 0.f; Hour < 24.f; Hour += 0.5f)
			LargestOffset = FMath::Max(LargestOffset,
				FMath::Abs(UIslandTideglassSubsystem::TideOffsetCm(Hour, Day)));
	TestTrue(TEXT("Tide remains within its bound throughout sixty Island days"),
		LargestOffset <= UIslandTideglassSubsystem::MaximumTideOffsetCm + 0.001f);

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Transient Tideglass integration fixture world created"), World)) return false;
	if (!TestNotNull(TEXT("Engine is available for Tideglass integration fixture"), GEngine))
	{
		World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		if (UIslandTideglassSubsystem* Subsystem = World->GetSubsystem<UIslandTideglassSubsystem>())
			Subsystem->RestorePoolMaterial();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	ATargetPoint* PoolMarker = World->SpawnActor<ATargetPoint>(FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Spawn);
	AActor* PoolFootprint = World->SpawnActor<AActor>(FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Tideglass integration clock spawned"), Clock) ||
		!TestNotNull(TEXT("Tideglass integration marker spawned"), PoolMarker) ||
		!TestNotNull(TEXT("Tideglass integration footprint spawned"), PoolFootprint)) return false;
	Clock->CurrentHour = 6.21f;
	Clock->DayNumber = 1;
	PoolMarker->Tags.Add(TEXT("IslandLandmark"));
	PoolMarker->Tags.Add(TEXT("TideglassPool"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!TestNotNull(TEXT("Basic sphere loaded for the shallow pool fixture"), Sphere) ||
		!TestNotNull(TEXT("Basic material loaded for the runtime water fixture"), Material)) return false;
	UStaticMeshComponent* BlockoutSurface = NewObject<UStaticMeshComponent>(PoolFootprint);
	BlockoutSurface->SetStaticMesh(Sphere);
	BlockoutSurface->SetRelativeScale3D(FVector(6.f, 6.f, 0.01f));
	BlockoutSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoolFootprint->SetRootComponent(BlockoutSurface);
	BlockoutSurface->RegisterComponent();
	PoolFootprint->SetActorLocation(PoolMarker->GetActorLocation());

	UIslandTideglassSubsystem* Subsystem = World->GetSubsystem<UIslandTideglassSubsystem>();
	if (!TestNotNull(TEXT("Tideglass integration subsystem is available"), Subsystem)) return false;
	Subsystem->IslandClock = Clock;
	TestTrue(TEXT("Runtime water is generated from the temporary pool footprint"), Subsystem->ApplyPoolMaterial(Material));
	AActor* Resident = World->SpawnActor<AActor>(FVector(500.f, 0.f, 200.f), FRotator::ZeroRotator, Spawn);
	UAgentBrainComponent* Brain = Resident ? NewObject<UAgentBrainComponent>(Resident) : nullptr;
	if (Resident && Brain)
	{
		Resident->AddInstanceComponent(Brain);
		Brain->RegisterComponent();
		const FString HighWaterSituation = Brain->BuildSituationSummary(FAgentConversationContext());
		TestTrue(TEXT("A nearby resident can perceive Tideglass near spring high water"),
			HighWaterSituation.Contains(TEXT("near high water")) && HighWaterSituation.Contains(TEXT("cm above its mean level")));
	}
	else TestTrue(TEXT("A resident perception fixture is available"), Resident && Brain);
	if (Subsystem->RuntimeSurface)
	{
		const float BaseZ = Subsystem->RuntimeSurfaceBaseLocation.Z;
		TestTrue(TEXT("Runtime water rises to the new-moon high-tide offset"),
			FMath::IsNearlyEqual(Subsystem->RuntimeSurface->GetComponentLocation().Z - BaseZ, NewMoonHigh, 0.1f));
		Clock->CurrentHour = 18.63f;
		Subsystem->Tick(0.5f);
		TestTrue(TEXT("Tick moves the same transient surface down to low tide"),
			FMath::IsNearlyEqual(Subsystem->RuntimeSurface->GetComponentLocation().Z - BaseZ, NewMoonLow, 0.1f));
	}
	if (Brain)
	{
		const FString LowWaterSituation = Brain->BuildSituationSummary(FAgentConversationContext());
		TestTrue(TEXT("The same resident perceives Tideglass near spring low water without invented travel claims"),
			LowWaterSituation.Contains(TEXT("near low water")) && LowWaterSituation.Contains(TEXT("cm below its mean level")));
	}
	return true;
}
