#include "Misc/AutomationTest.h"
#include "AgentBrainComponent.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Components/BoxComponent.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
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
	UMaterialParameterCollection* IslandCollection = LoadObject<UMaterialParameterCollection>(nullptr, UIslandEnvironmentSubsystem::CollectionPath);
	TestNotNull(TEXT("The shared Island environment collection loads"), IslandCollection);
	if (IslandCollection)
	{
		TestTrue(TEXT("The saved shared collection now exposes the Indoors scalar"),
			IslandCollection->ScalarParameters.ContainsByPredicate([](const FCollectionScalarParameter& Parameter) { return Parameter.ParameterName == TEXT("Indoors"); }));
	}

	// Pure rules first.
	float Wet = 0.f;
	for (int32 Second = 0; Second < 60; ++Second) Wet = UIslandEnvironmentSubsystem::StepWetness(Wet, 1.f, 0.5f, 0.f, 1.f);
	TestTrue(TEXT("A minute of heavy rain soaks the ground"), FMath::IsNearlyEqual(Wet, 1.f));
	const float AfterSun = UIslandEnvironmentSubsystem::StepWetness(1.f, 0.f, 1.f, 600.f, 120.f);
	const float AfterNight = UIslandEnvironmentSubsystem::StepWetness(1.f, 0.f, 0.f, 0.f, 120.f);
	TestTrue(TEXT("Sun and wind dry the ground faster than a calm night"), AfterSun < AfterNight && AfterNight < 1.f);
	TestTrue(TEXT("Wetness never goes negative"), UIslandEnvironmentSubsystem::StepWetness(0.1f, 0.f, 1.f, 600.f, 10000.f) == 0.f);
	TestTrue(TEXT("Dry weather preserves the material's authored landscape value"), FMath::IsNearlyEqual(UIslandEnvironmentSubsystem::LandscapeWetnessValue(0.15f, 0.f), 0.15f));
	TestTrue(TEXT("A fully soaked environment drives the landscape to fully wet"), FMath::IsNearlyEqual(UIslandEnvironmentSubsystem::LandscapeWetnessValue(0.15f, 1.f), 1.f));
	TestTrue(TEXT("Partial environment wetness blends from the authored baseline"), FMath::IsNearlyEqual(UIslandEnvironmentSubsystem::LandscapeWetnessValue(0.15f, 0.5f), 0.575f, 0.001f));
	TestTrue(TEXT("Golden hour peaks with the sun low but up"), UIslandEnvironmentSubsystem::GoldenHourFor(0.15f) > 0.99f);
	TestEqual(TEXT("No golden hour at noon"), UIslandEnvironmentSubsystem::GoldenHourFor(1.f), 0.f);
	TestEqual(TEXT("No golden hour after sunset"), UIslandEnvironmentSubsystem::GoldenHourFor(-0.3f), 0.f);
	TestTrue(TEXT("Residents are not told about wet ground during rain"), UIslandEnvironmentSubsystem::DescribeGround(1.f, 0.8f).IsEmpty());
	TestTrue(TEXT("Soaked ground lingers after rain"), UIslandEnvironmentSubsystem::DescribeGround(0.9f, 0.f).Contains(TEXT("still soaked")));
	TestTrue(TEXT("Damp ground is noticed while drying"), UIslandEnvironmentSubsystem::DescribeGround(0.3f, 0.f).Contains(TEXT("damp")));
	TestTrue(TEXT("Dry ground goes unmentioned"), UIslandEnvironmentSubsystem::DescribeGround(0.05f, 0.f).IsEmpty());
	TestTrue(TEXT("A calm dawn after a wet night brings thick low mist"), UIslandEnvironmentSubsystem::MistFor(0.9f, 6.5f, 30.f, 0.f, 0.f) > 0.8f);
	TestTrue(TEXT("Wind keeps the mist from forming"), UIslandEnvironmentSubsystem::MistFor(0.9f, 6.5f, 400.f, 0.f, 0.f) < 0.05f);
	TestEqual(TEXT("A dry afternoon is clear"), UIslandEnvironmentSubsystem::MistFor(0.f, 15.f, 30.f, 0.f, 0.f), 0.f);
	TestTrue(TEXT("Mist burns off by late morning"), UIslandEnvironmentSubsystem::MistFor(0.9f, 11.f, 30.f, 0.f, 0.f) < 0.05f);
	TestTrue(TEXT("Rain and storms leave a haze"), UIslandEnvironmentSubsystem::MistFor(0.f, 15.f, 400.f, 1.f, 1.f) > 0.5f);
	TestTrue(TEXT("Residents notice thick mist"), UIslandEnvironmentSubsystem::DescribeAir(0.8f).Contains(TEXT("thick, low mist")));
	TestTrue(TEXT("Clear air goes unmentioned"), UIslandEnvironmentSubsystem::DescribeAir(0.1f).IsEmpty());
	UMaterialInterface* IslandLandscapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/MI_Island_Landscape.MI_Island_Landscape"));
	TestNotNull(TEXT("Island landscape uses a material with an environment wetness control"), IslandLandscapeMaterial);
	if (IslandLandscapeMaterial)
	{
		UMaterialInstanceDynamic* WetLandscape = UMaterialInstanceDynamic::Create(IslandLandscapeMaterial, GetTransientPackage());
		TestNotNull(TEXT("Landscape wetness can be driven through a reversible dynamic material instance"), WetLandscape);
		if (WetLandscape)
		{
			bool bReusedDynamic = false;
			UMaterialInstanceDynamic* ReusedLandscape = UIslandEnvironmentSubsystem::GetOrCreateLandscapeWetnessInstance(WetLandscape, GetTransientPackage(), bReusedDynamic);
			TestTrue(TEXT("An already-dynamic landscape material is reused rather than wrapped in another MID"), bReusedDynamic && ReusedLandscape == WetLandscape);
			bool bCreatedDynamic = true;
			UMaterialInstanceDynamic* CreatedLandscape = UIslandEnvironmentSubsystem::GetOrCreateLandscapeWetnessInstance(IslandLandscapeMaterial, GetTransientPackage(), bCreatedDynamic);
			TestTrue(TEXT("An authored constant instance still receives a new wetness MID"), CreatedLandscape && !bCreatedDynamic);
			WetLandscape->SetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, 0.73f);
			float MaterialWetness = -1.f;
			TestTrue(TEXT("Landscape material accepts the simulated wetness value"),
				WetLandscape->GetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, MaterialWetness) && FMath::IsNearlyEqual(MaterialWetness, 0.73f));
		}
	}

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
	// A small collision fixture exercises the inn's actual tagged roof/wall evidence. It never
	// starts a play session or calls an agent model.
	auto SpawnInnBox = [World](const FVector& Centre, const FVector& Extent)
	{
		AActor* Part = World->SpawnActor<AActor>(Centre, FRotator::ZeroRotator);
		if (!Part) return static_cast<AActor*>(nullptr);
		Part->Tags.Add(TEXT("IslandInn"));
		UBoxComponent* Box = NewObject<UBoxComponent>(Part);
		Part->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Part->SetActorLocation(Centre, false, nullptr, ETeleportType::TeleportPhysics);
		return Part;
	};
	SpawnInnBox(FVector(450.f, 0.f, 200.f), FVector(20.f, 500.f, 200.f));
	SpawnInnBox(FVector(-450.f, 0.f, 200.f), FVector(20.f, 500.f, 200.f));
	SpawnInnBox(FVector(0.f, 450.f, 200.f), FVector(500.f, 20.f, 200.f));
	SpawnInnBox(FVector(0.f, -450.f, 200.f), FVector(500.f, 20.f, 200.f));
	AActor* InnRoof = SpawnInnBox(FVector(0.f, 0.f, 500.f), FVector(500.f, 500.f, 20.f));
	TestTrue(TEXT("Tagged walls and an overhead roof establish an enclosed inn interior"), UIslandEnvironmentSubsystem::IsInsideInnAt(World, FVector::ZeroVector));
	const FString Interior = UIslandEnvironmentSubsystem::DescribeInnInteriorAt(World, FVector::ZeroVector);
	TestTrue(TEXT("Resident interior report states the visual response and does not promise warmth or complete rainproofing"),
		Interior.Contains(TEXT("beneath its roof")) && Interior.Contains(TEXT("Local rain streaks")) && Interior.Contains(TEXT("different indoor temperature")));
	AActor* Resident = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	UAgentBrainComponent* ResidentBrain = Resident ? NewObject<UAgentBrainComponent>(Resident) : nullptr;
	TestNotNull(TEXT("A resident brain is available for the perception check"), ResidentBrain);
	if (Resident && ResidentBrain)
	{
		Resident->AddInstanceComponent(ResidentBrain);
		ResidentBrain->RegisterComponent();
		TestTrue(TEXT("The resident's actual perception summary receives the interior evidence"), ResidentBrain->BuildSituationSummary(FAgentConversationContext()).Contains(TEXT("inside the Island inn")));
	}
	TestFalse(TEXT("A point outside the roof is not perceived as indoors"), UIslandEnvironmentSubsystem::IsInsideInnAt(World, FVector(900.f, 0.f, 0.f)));
	Clock->CurrentHour = 17.f;
	Environment->Tick(0.5f);
	UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
	float Published = -1.f;
	TestTrue(TEXT("The environment collection publishes Indoors for the enclosed viewer"),
		Instance && Instance->GetScalarParameterValue(TEXT("Indoors"), Published) && FMath::IsNearlyEqual(Published, 1.f));
	if (InnRoof)
	{
		InnRoof->SetActorEnableCollision(false);
		if (UPrimitiveComponent* RoofPrimitive = Cast<UPrimitiveComponent>(InnRoof->GetRootComponent()))
			RoofPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Walls without a tagged roof are not enough to claim an inn interior"), UIslandEnvironmentSubsystem::IsInsideInnAt(World, FVector::ZeroVector));
		Environment->Tick(0.5f);
		TestTrue(TEXT("The environment collection clears Indoors when the roof is absent"),
			Instance && Instance->GetScalarParameterValue(TEXT("Indoors"), Published) && FMath::IsNearlyEqual(Published, 0.f));
	}
	TestTrue(TEXT("The Island hour reaches materials"), Instance && Instance->GetScalarParameterValue(TEXT("IslandHour"), Published) && FMath::IsNearlyEqual(Published, 17.f));
	TestTrue(TEXT("Golden hour is published at 17:00"), Instance->GetScalarParameterValue(TEXT("GoldenHour"), Published) && Published > 0.5f);
	TestTrue(TEXT("Rain intensity matches the weather simulation"), Instance->GetScalarParameterValue(TEXT("RainIntensity"), Published) &&
		FMath::IsNearlyEqual(Published, FMath::Clamp(Weather->SampleRainIntensity(World->GetTimeSeconds()), 0.f, 1.f), 0.001f));
	FLinearColor WindValue;
	TestTrue(TEXT("Wind direction is published as a unit vector with speed"), Instance->GetVectorParameterValue(UIslandEnvironmentSubsystem::WindDirectionParameter, WindValue) &&
		(FMath::IsNearlyZero(WindValue.A) || FMath::IsNearlyEqual(FVector(WindValue.R, WindValue.G, WindValue.B).Size(), 1.f, 0.01f)));
	// Mist drives the height fog: a fixture level without fog gets one, and forced mist thickens it.
	TArray<AActor*> Fogs;
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It) Fogs.Add(*It);
	if (TestEqual(TEXT("A level without height fog gets one for the session"), Fogs.Num(), 1))
	{
		UExponentialHeightFogComponent* FogComponent = CastChecked<AExponentialHeightFog>(Fogs[0])->GetComponent();
		const float ClearDensity = FogComponent->FogDensity;
		Environment->ForcedMist = 1.f;
		Environment->ForcedMistUntil = World->GetTimeSeconds() + 60.0;
		for (int32 Step = 0; Step < 20; ++Step) Environment->Tick(0.5f);
		TestTrue(TEXT("Mist thickens the fog"), Environment->GetMist() > 0.9f && FogComponent->FogDensity > ClearDensity * 5.f);
		TestTrue(TEXT("Mist is published to materials"), Instance->GetScalarParameterValue(TEXT("Mist"), Published) && Published > 0.9f);
	}
	UMaterialInstanceDynamic* ReusedLandscapeForTeardown = IslandLandscapeMaterial
		? UMaterialInstanceDynamic::Create(IslandLandscapeMaterial, GetTransientPackage()) : nullptr;
	if (ReusedLandscapeForTeardown)
	{
		ReusedLandscapeForTeardown->SetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, 0.83f);
		Environment->LandscapeOriginalMaterials.Add(ReusedLandscapeForTeardown);
		Environment->LandscapeMaterialInstances.Add(ReusedLandscapeForTeardown);
		Environment->LandscapeWetnessBaselines.Add(0.15f);
		Environment->LandscapeWetnessInstanceWasReused.Add(1);
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	float RestoredWetness = -1.f;
	TestTrue(TEXT("World teardown restores the reused landscape MID's authored wetness baseline"),
		ReusedLandscapeForTeardown && ReusedLandscapeForTeardown->GetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, RestoredWetness) &&
		FMath::IsNearlyEqual(RestoredWetness, 0.15f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandLandscapeAssignmentTest, "CaptiveSky2.Agent.IslandLandscapeAssignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandLandscapeAssignmentTest::RunTest(const FString& Parameters)
{
	int32 IslandWorldCount = 0;
	int32 LandscapeActorCount = 0;
	int32 LandscapeComponentCount = 0;
	int32 AssignedMaterialCount = 0;
	bool bUsesIslandLandscapeInstance = false;
	TSet<FString> AssignedMaterialPaths;
	TMap<FName, int32> LayerComponentCounts;

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Island = Context.World();
		if (Context.WorldType != EWorldType::Editor || !Island || Island->GetMapName() != TEXT("Island")) continue;
		++IslandWorldCount;
		for (TActorIterator<ALandscapeProxy> It(Island); It; ++It)
		{
			++LandscapeActorCount;
			TArray<ULandscapeComponent*> Components;
			It->GetComponents<ULandscapeComponent>(Components);
			for (const ULandscapeComponent* Component : Components)
			{
				if (!Component) continue;
				++LandscapeComponentCount;
				for (const FWeightmapLayerAllocationInfo& Layer : Component->GetWeightmapLayerAllocations())
					if (Layer.LayerInfo && !Layer.LayerInfo->GetLayerName().IsNone()) ++LayerComponentCounts.FindOrAdd(Layer.LayerInfo->GetLayerName());
				for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
				{
					const UMaterialInterface* Material = Component->GetMaterial(MaterialIndex);
					if (!Material) continue;
					++AssignedMaterialCount;
					const FString MaterialPath = Material->GetPathName();
					AssignedMaterialPaths.Add(MaterialPath);
					if (MaterialPath == TEXT("/Game/Materials/MI_Island_Landscape.MI_Island_Landscape"))
						bUsesIslandLandscapeInstance = true;
				}
			}
		}
	}

	TArray<FString> SortedMaterialPaths = AssignedMaterialPaths.Array();
	SortedMaterialPaths.Sort();
	TArray<FName> SortedLayerNames;
	LayerComponentCounts.GetKeys(SortedLayerNames);
	SortedLayerNames.Sort(FNameLexicalLess());
	FString LayerSummary;
	for (const FName LayerName : SortedLayerNames)
	{
		if (!LayerSummary.IsEmpty()) LayerSummary += TEXT(", ");
		LayerSummary += FString::Printf(TEXT("%s on %d component(s)"), *LayerName.ToString(), LayerComponentCounts.FindRef(LayerName));
	}
	FString MaterialInstanceSummary(TEXT("unavailable"));
	if (const UMaterialInstance* Instance = LoadObject<UMaterialInstance>(nullptr, TEXT("/Game/Materials/MI_Island_Landscape.MI_Island_Landscape")))
	{
		TArray<FString> ScalarOverrides;
		for (const FScalarParameterValue& Value : Instance->ScalarParameterValues)
			ScalarOverrides.Add(FString::Printf(TEXT("%s=%.3f"), *Value.ParameterInfo.Name.ToString(), Value.ParameterValue));
		TArray<FString> TextureOverrides;
		for (const FTextureParameterValue& Value : Instance->TextureParameterValues)
			if (Value.ParameterValue)
				TextureOverrides.Add(FString::Printf(TEXT("%s=%s"), *Value.ParameterInfo.Name.ToString(), *Value.ParameterValue->GetPathName()));
		MaterialInstanceSummary = FString::Printf(TEXT("parent=%s; scalar overrides=[%s]; texture overrides=[%s]"),
			Instance->Parent ? *Instance->Parent->GetPathName() : TEXT("none"),
			*FString::Join(ScalarOverrides, TEXT(", ")), *FString::Join(TextureOverrides, TEXT(", ")));
	}
	AddInfo(FString::Printf(TEXT("Saved Island landscape audit: %d editor map(s), %d landscape actor(s), %d component(s), %d assigned material slot(s). Distinct materials: %s. Allocated painted layers: %s"),
		IslandWorldCount, LandscapeActorCount, LandscapeComponentCount, AssignedMaterialCount,
		*FString::Join(SortedMaterialPaths, TEXT(", ")), LayerSummary.IsEmpty() ? TEXT("none") : *LayerSummary));
	AddInfo(FString::Printf(TEXT("Configured landscape instance: %s"), *MaterialInstanceSummary));
	TestTrue(TEXT("The saved Island editor map is loaded for the material audit"), IslandWorldCount > 0);
	TestTrue(TEXT("The saved Island has landscape components with assigned materials"), LandscapeComponentCount > 0 && AssignedMaterialCount > 0);
	TestTrue(TEXT("The Island landscape uses its configured MI_Island_Landscape instance"), bUsesIslandLandscapeInstance);
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
