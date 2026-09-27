#include "Misc/AutomationTest.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandEnvironmentTest, "CaptiveSky2.Agent.IslandEnvironment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/** Adds any missing environment parameters to a collection; returns how many were added. */
	int32 EnsureEnvironmentParameters(UMaterialParameterCollection* Collection)
	{
		int32 Added = 0;
		for (const FName Name : UIslandEnvironmentSubsystem::ScalarParameterNames())
		{
			if (Collection->ScalarParameters.ContainsByPredicate([Name](const FCollectionScalarParameter& Parameter) { return Parameter.ParameterName == Name; })) continue;
			FCollectionScalarParameter& Parameter = Collection->ScalarParameters.AddDefaulted_GetRef();
			Parameter.ParameterName = Name;
			Parameter.DefaultValue = Name == TEXT("Daylight") || Name == TEXT("SunHeight") ? 1.f : Name == TEXT("IslandHour") ? 12.f : 0.f;
			++Added;
		}
		if (!Collection->VectorParameters.ContainsByPredicate([](const FCollectionVectorParameter& Parameter) { return Parameter.ParameterName == UIslandEnvironmentSubsystem::WindDirectionParameter; }))
		{
			FCollectionVectorParameter& Parameter = Collection->VectorParameters.AddDefaulted_GetRef();
			Parameter.ParameterName = UIslandEnvironmentSubsystem::WindDirectionParameter;
			Parameter.DefaultValue = FLinearColor(1.f, 0.f, 0.f, 0.f);
			++Added;
		}
		return Added;
	}
}

bool FIslandEnvironmentTest::RunTest(const FString& Parameters)
{
	// Pure rules first.
	float Wet = 0.f;
	for (int32 Second = 0; Second < 60; ++Second) Wet = UIslandEnvironmentSubsystem::StepWetness(Wet, 1.f, 0.5f, 0.f, 1.f);
	TestTrue(TEXT("A minute of heavy rain soaks the ground"), FMath::IsNearlyEqual(Wet, 1.f));
	const float AfterSun = UIslandEnvironmentSubsystem::StepWetness(1.f, 0.f, 1.f, 600.f, 120.f);
	const float AfterNight = UIslandEnvironmentSubsystem::StepWetness(1.f, 0.f, 0.f, 0.f, 120.f);
	TestTrue(TEXT("Sun and wind dry the ground faster than a calm night"), AfterSun < AfterNight && AfterNight < 1.f);
	TestTrue(TEXT("Wetness never goes negative"), UIslandEnvironmentSubsystem::StepWetness(0.1f, 0.f, 1.f, 600.f, 10000.f) == 0.f);
	TestTrue(TEXT("Golden hour peaks with the sun low but up"), UIslandEnvironmentSubsystem::GoldenHourFor(0.15f) > 0.99f);
	TestEqual(TEXT("No golden hour at noon"), UIslandEnvironmentSubsystem::GoldenHourFor(1.f), 0.f);
	TestEqual(TEXT("No golden hour after sunset"), UIslandEnvironmentSubsystem::GoldenHourFor(-0.3f), 0.f);

	// Then the published collection values, in a fixture world with its own transient collection.
	UMaterialParameterCollection* Collection = NewObject<UMaterialParameterCollection>(GetTransientPackage());
	EnsureEnvironmentParameters(Collection);
#if WITH_EDITOR
	Collection->PostEditChange();
#endif
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>();
	if (!TestNotNull(TEXT("Play worlds own an environment subsystem"), Environment)) { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false; }
	Environment->CollectionOverride = Collection;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	World->BeginPlay();
	Clock->CurrentHour = 17.f;
	Environment->Tick(0.5f);
	UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
	float Published = -1.f;
	TestTrue(TEXT("The Island hour reaches materials"), Instance && Instance->GetScalarParameterValue(TEXT("IslandHour"), Published) && FMath::IsNearlyEqual(Published, 17.f));
	TestTrue(TEXT("Golden hour is published at 17:00"), Instance->GetScalarParameterValue(TEXT("GoldenHour"), Published) && Published > 0.5f);
	TestTrue(TEXT("Rain intensity matches the weather simulation"), Instance->GetScalarParameterValue(TEXT("RainIntensity"), Published) &&
		FMath::IsNearlyEqual(Published, FMath::Clamp(Weather->SampleRainIntensity(World->GetTimeSeconds()), 0.f, 1.f), 0.001f));
	FLinearColor WindValue;
	TestTrue(TEXT("Wind direction is published as a unit vector with speed"), Instance->GetVectorParameterValue(UIslandEnvironmentSubsystem::WindDirectionParameter, WindValue) &&
		(FMath::IsNearlyZero(WindValue.A) || FMath::IsNearlyEqual(FVector(WindValue.R, WindValue.G, WindValue.B).Size(), 1.f, 0.01f)));
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#if WITH_EDITOR
// Explicit developer tool (not part of the Agent suite): creates or updates the shared collection asset.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreateEnvironmentCollectionTool, "CaptiveSky2.Tools.CreateEnvironmentCollection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCreateEnvironmentCollectionTool::RunTest(const FString& Parameters)
{
	const FString ObjectPath = UIslandEnvironmentSubsystem::CollectionPath;
	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
	const FString AssetName = FPackageName::ObjectPathToObjectName(ObjectPath);
	UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	UPackage* Package = Collection ? Collection->GetOutermost() : CreatePackage(*PackageName);
	if (!Collection)
	{
		Collection = NewObject<UMaterialParameterCollection>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Collection);
	}
	const int32 Added = EnsureEnvironmentParameters(Collection);
	AddInfo(FString::Printf(TEXT("%s: %d parameter(s) added, %d scalar and %d vector in total."), *ObjectPath, Added, Collection->ScalarParameters.Num(), Collection->VectorParameters.Num()));
	if (Added == 0) return true;
	Collection->PostEditChange();
	Package->MarkPackageDirty();
	FSavePackageArgs Save;
	Save.TopLevelFlags = RF_Public | RF_Standalone;
	return TestTrue(TEXT("Collection asset saved"), UPackage::SavePackage(Package, Collection,
		*FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()), Save));
}
#endif
