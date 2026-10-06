#include "IslandEnvironmentSubsystem.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LandscapeComponent.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/CommandLine.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/App.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

const TCHAR* UIslandEnvironmentSubsystem::CollectionPath = TEXT("/Game/Environment/MPC_IslandEnvironment.MPC_IslandEnvironment");
const TCHAR* UIslandEnvironmentSubsystem::FoliageWindCollectionPath = TEXT("/Game/PN_FoliageCollection/Materials/PN_WindParameters.PN_WindParameters");
const FName UIslandEnvironmentSubsystem::WindDirectionParameter(TEXT("WindDirection"));
const FName UIslandEnvironmentSubsystem::FoliageWindDirectionParameter(TEXT("WindDirection"));
const FName UIslandEnvironmentSubsystem::FoliageWindStrengthParameter(TEXT("WindStrength"));
const FName UIslandEnvironmentSubsystem::LandscapeWetnessParameter(TEXT("Ground Wetness"));
const TCHAR* UIslandEnvironmentSubsystem::WetLandscapeMaterialPath = TEXT("/Game/Materials/M_Island_Textured_Wet.M_Island_Textured_Wet");

const TArray<FName>& UIslandEnvironmentSubsystem::ScalarParameterNames()
{
	static const TArray<FName> Names = {
		TEXT("RainIntensity"), TEXT("Wetness"), TEXT("CloudCover"), TEXT("WindSpeed"),
		TEXT("Daylight"), TEXT("SunHeight"), TEXT("GoldenHour"), TEXT("IslandHour"), TEXT("Storm"), TEXT("LightningFlash"), TEXT("Mist"), TEXT("Indoors") };
	return Names;
}

bool UIslandEnvironmentSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandEnvironmentSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Collection = CollectionOverride ? CollectionOverride.Get() : LoadObject<UMaterialParameterCollection>(nullptr, CollectionPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	bFoliageMaterialWindEnabled = FParse::Param(FCommandLine::Get(), TEXT("IslandEnableFoliageMaterialWind")) || FoliageWindCollectionOverride != nullptr;
	if (bFoliageMaterialWindEnabled)
	{
		FoliageWindCollection = FoliageWindCollectionOverride ? FoliageWindCollectionOverride.Get()
			: LoadObject<UMaterialParameterCollection>(nullptr, FoliageWindCollectionPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!FoliageWindCollection)
			UE_LOG(LogTemp, Warning, TEXT("Island foliage material wind was requested, but its PN_WindParameters collection could not be loaded."));
	}
	UseWetLandscapeGraph();
	InitializeLandscapeMaterials();
	// Mist works through the level's height fog; a level without one gets a faint one for the session.
	for (TActorIterator<AExponentialHeightFog> It(&InWorld); It; ++It) { Fog = *It; break; }
	if (!Fog.IsValid())
	{
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Fog = InWorld.SpawnActor<AExponentialHeightFog>(FVector(0.f, 0.f, 2500.f), FRotator::ZeroRotator, Spawn);
		if (Fog.IsValid())
		{
			Fog->GetComponent()->SetFogDensity(0.004f);
			Fog->GetComponent()->SetFogHeightFalloff(0.2f);
		}
	}
	if (Fog.IsValid())
	{
		Fog->GetComponent()->SetMobility(EComponentMobility::Movable);
		BaseFogDensity = Fog->GetComponent()->FogDensity;
		BaseFogFalloff = Fog->GetComponent()->FogHeightFalloff;
	}
}

float UIslandEnvironmentSubsystem::MistFor(float InWetness, float Hour, float WindSpeed, float Rain, float InStorm)
{
	// Radiation fog: strongest around dawn, a little through the night, burning off by mid-morning.
	const float H = FMath::Fmod(FMath::Fmod(Hour, 24.f) + 24.f, 24.f);
	const float Dawn = H < 4.f ? 0.45f : H < 6.5f ? FMath::Lerp(0.45f, 1.f, (H - 4.f) / 2.5f) : H < 10.f ? FMath::Lerp(1.f, 0.f, (H - 6.5f) / 3.5f) : H >= 21.f ? 0.35f : 0.f;
	const float Calm = 1.f - FMath::SmoothStep(80.f, 320.f, WindSpeed);
	const float Ground = FMath::Clamp(InWetness, 0.f, 1.f) * Dawn * Calm;
	// Falling rain and storms leave a lighter haze whatever the hour.
	const float Haze = 0.3f * FMath::Clamp(Rain, 0.f, 1.f) + 0.25f * FMath::Clamp(InStorm, 0.f, 1.f);
	return FMath::Clamp(FMath::Max(Ground, Haze), 0.f, 1.f);
}

FString UIslandEnvironmentSubsystem::DescribeAir(float InMist)
{
	if (InMist < 0.3f) return FString();
	return InMist > 0.65f
		? TEXT(" A thick, low mist lies over the ground; distant things fade into it.")
		: TEXT(" A thin mist hangs in the air, softening distant shapes.");
}

bool UIslandEnvironmentSubsystem::IsInsideInnAt(UWorld* World, const FVector& Position, const AActor* Observer)
{
	if (!IsValid(World)) return false;

	// Require both a tagged roof overhead and enclosure in most horizontal directions. A single
	// wall, overhang, or the outdoor side of the inn must not be mistaken for an interior.
	const FVector Sample = Position + FVector(0.f, 0.f, 110.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IslandInnInterior), false);
	if (Observer) Params.AddIgnoredActor(Observer);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Sample, Sample + FVector(0.f, 0.f, 700.f), ECC_Visibility, Params) ||
		!Hit.GetActor() || !Hit.GetActor()->ActorHasTag(TEXT("IslandInn")))
	{
		return false;
	}

	static const FVector2D Directions[] = {
		FVector2D(1.f, 0.f), FVector2D(0.70710678f, 0.70710678f), FVector2D(0.f, 1.f),
		FVector2D(-0.70710678f, 0.70710678f), FVector2D(-1.f, 0.f),
		FVector2D(-0.70710678f, -0.70710678f), FVector2D(0.f, -1.f),
		FVector2D(0.70710678f, -0.70710678f)
	};
	int32 EnclosedDirections = 0;
	for (const FVector2D& Direction : Directions)
	{
		Hit = FHitResult();
		const FVector End = Sample + FVector(Direction.X, Direction.Y, 0.f) * 700.f;
		if (World->LineTraceSingleByChannel(Hit, Sample, End, ECC_Visibility, Params) &&
			Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("IslandInn")))
		{
			++EnclosedDirections;
		}
	}
	return EnclosedDirections >= 6;
}

FString UIslandEnvironmentSubsystem::DescribeInnInteriorAt(UWorld* World, const FVector& Position, const AActor* Observer)
{
	if (!IsInsideInnAt(World, Position, Observer)) return FString();
	return TEXT(" You are inside the Island inn, beneath its roof and enclosed by its walls. Local rain streaks and roof splashes are suppressed here, and the generated wind/rain ambience is softened. This does not establish complete rainproofing or a different indoor temperature.");
}

TStatId UIslandEnvironmentSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandEnvironmentSubsystem, STATGROUP_Tickables);
}

void UIslandEnvironmentSubsystem::InitializeLandscapeMaterials()
{
	if (bLandscapeMaterialsInitialized || !GetWorld()) return;
	bLandscapeMaterialsInitialized = true;
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		TArray<ULandscapeComponent*> Components;
		It->GetComponents<ULandscapeComponent>(Components);
		for (ULandscapeComponent* Component : Components)
		{
			if (!Component) continue;
			FIslandLandscapeMaterialBackup& Backup = LandscapeMaterialBackups.AddDefaulted_GetRef();
			Backup.Component = Component;
			const int32 MaterialCount = Component->GetNumMaterials();
			Backup.Materials.Reserve(MaterialCount);
			for (int32 Index = 0; Index < MaterialCount; ++Index)
			{
				UMaterialInterface* Original = Component->GetMaterial(Index);
				Backup.Materials.Add(Original);
				if (!Original) continue;
				float AuthoredWetness = 0.f;
				if (!Original->GetScalarParameterValue(LandscapeWetnessParameter, AuthoredWetness)) continue;

				int32 MaterialIndex = LandscapeOriginalMaterials.IndexOfByKey(Original);
				if (MaterialIndex == INDEX_NONE)
				{
					bool bReusedDynamic = false;
					UMaterialInstanceDynamic* Dynamic = GetOrCreateLandscapeWetnessInstance(Original, this, bReusedDynamic);
					if (!Dynamic) continue;
					MaterialIndex = LandscapeOriginalMaterials.Add(Original);
					LandscapeMaterialInstances.Add(Dynamic);
					LandscapeWetnessBaselines.Add(AuthoredWetness);
					LandscapeWetnessInstanceWasReused.Add(bReusedDynamic ? 1 : 0);
				}
				if (LandscapeMaterialInstances.IsValidIndex(MaterialIndex))
					Component->SetMaterial(Index, LandscapeMaterialInstances[MaterialIndex]);
			}
		}
	}
	ApplyLandscapeWetness();
}

void UIslandEnvironmentSubsystem::UseWetLandscapeGraph()
{
#if WITH_EDITOR
	// Each landscape component renders from its own instance baked into the map, so SetMaterial never reaches the
	// screen. Reparenting the shared instance and recaching those baked instances does.
	// Changing a parent is editor-only (standalone game asserts), so packaged builds need the wet graph authored in.
	if (!GIsEditor || !FApp::CanEverRender() || !GetWorld()) return;
	UMaterialInterface* WetGraph = LoadObject<UMaterialInterface>(nullptr, WetLandscapeMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!WetGraph) return;
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(It->LandscapeMaterial);
		if (!Instance || Instance->Parent == WetGraph) continue;
		WetSwappedInstances.Add(Instance);
		WetSwappedParents.Add(Instance->Parent);
		Instance->SetParentEditorOnly(WetGraph);
		It->UpdateAllComponentMaterialInstances();
		for (TObjectIterator<UMaterialInstanceConstant> Baked; Baked; ++Baked)
			if (Baked->GetOuter() == *It) Baked->InitStaticPermutation();
		TArray<ULandscapeComponent*> Components;
		It->GetComponents<ULandscapeComponent>(Components);
		for (ULandscapeComponent* Component : Components) Component->MarkRenderStateDirty();
	}
	if (WetSwappedInstances.Num() == 0) return;
	// Finish now rather than showing the fallback grid while a first-time landscape shader compiles.
	if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
	UE_LOG(LogTemp, Display, TEXT("IslandEnvironment: landscape now uses the wet graph %s"), WetLandscapeMaterialPath);
#endif
}

void UIslandEnvironmentSubsystem::RestoreLandscapeGraph()
{
#if WITH_EDITOR
	for (int32 Index = 0; Index < WetSwappedInstances.Num(); ++Index)
		if (WetSwappedInstances[Index]) WetSwappedInstances[Index]->SetParentEditorOnly(WetSwappedParents[Index]);
#endif
	WetSwappedInstances.Reset();
	WetSwappedParents.Reset();
}

void UIslandEnvironmentSubsystem::ApplyLandscapeWetness()
{
	if (FMath::IsNearlyEqual(Wetness, LastAppliedLandscapeWetness, 0.002f)) return;
	for (int32 Index = 0; Index < LandscapeMaterialInstances.Num(); ++Index)
		if (UMaterialInstanceDynamic* Material = LandscapeMaterialInstances[Index])
		{
			const float AuthoredWetness = LandscapeWetnessBaselines.IsValidIndex(Index) ? LandscapeWetnessBaselines[Index] : 0.f;
			Material->SetScalarParameterValue(LandscapeWetnessParameter, LandscapeWetnessValue(AuthoredWetness, Wetness));
		}
	LastAppliedLandscapeWetness = Wetness;
}

void UIslandEnvironmentSubsystem::Deinitialize()
{
	RestoreLandscapeGraph();
	for (int32 Index = 0; Index < LandscapeMaterialInstances.Num(); ++Index)
		if (LandscapeWetnessInstanceWasReused.IsValidIndex(Index) && LandscapeWetnessInstanceWasReused[Index])
			if (UMaterialInstanceDynamic* Material = LandscapeMaterialInstances[Index])
				if (LandscapeWetnessBaselines.IsValidIndex(Index))
					Material->SetScalarParameterValue(LandscapeWetnessParameter, LandscapeWetnessBaselines[Index]);
	for (const FIslandLandscapeMaterialBackup& Backup : LandscapeMaterialBackups)
	{
		if (!IsValid(Backup.Component.Get())) continue;
		for (int32 Index = 0; Index < Backup.Materials.Num(); ++Index)
			Backup.Component->SetMaterial(Index, Backup.Materials[Index]);
	}
	LandscapeMaterialBackups.Reset();
	LandscapeMaterialInstances.Reset();
	LandscapeOriginalMaterials.Reset();
	LandscapeWetnessBaselines.Reset();
	LandscapeWetnessInstanceWasReused.Reset();
	if (Fog.IsValid())
	{
		Fog->GetComponent()->SetFogDensity(BaseFogDensity);
		Fog->GetComponent()->SetFogHeightFalloff(BaseFogFalloff);
	}
	Super::Deinitialize();
}

float UIslandEnvironmentSubsystem::StepWetness(float InWetness, float Rain, float InDaylight, float WindSpeed, float Seconds)
{
	const float Dt = FMath::Max(0.f, Seconds);
	// Soaks through in under a minute of heavy rain; dries over roughly ten minutes of calm night,
	// three of sunny, breezy day.
	if (Rain > 0.05f) return FMath::Min(1.f, InWetness + Rain * Dt / 45.f);
	const float DryingRate = (0.3f + 0.7f * FMath::Clamp(InDaylight, 0.f, 1.f)) * (1.f + FMath::Clamp(WindSpeed / 600.f, 0.f, 1.f)) / 600.f;
	return FMath::Max(0.f, InWetness - DryingRate * Dt);
}

float UIslandEnvironmentSubsystem::LandscapeWetnessValue(float AuthoredWetness, float EnvironmentWetness)
{
	return FMath::Lerp(FMath::Clamp(AuthoredWetness, 0.f, 1.f), 1.f, FMath::Clamp(EnvironmentWetness, 0.f, 1.f));
}

UMaterialInstanceDynamic* UIslandEnvironmentSubsystem::GetOrCreateLandscapeWetnessInstance(UMaterialInterface* Original, UObject* Outer, bool& bOutReused)
{
	bOutReused = false;
	if (!Original) return nullptr;
	if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Original))
	{
		bOutReused = true;
		return Existing;
	}
	return UMaterialInstanceDynamic::Create(Original, Outer);
}

float UIslandEnvironmentSubsystem::GoldenHourFor(float InSunHeight)
{
	// Warm, low light: strongest with the sun just above the horizon, gone once it has set or climbed.
	return InSunHeight < -0.03f ? 0.f : FMath::Clamp(1.f - FMath::Abs(InSunHeight - 0.15f) / 0.3f, 0.f, 1.f);
}

FString UIslandEnvironmentSubsystem::DescribeGround(float InWetness, float Rain)
{
	// While it rains, the weather description already covers it; afterwards the evidence lingers.
	if (Rain > 0.05f || InWetness < 0.15f) return FString();
	return InWetness > 0.6f
		? TEXT(" The rain has passed, but the ground is still soaked and the grass and stones are dripping; it will take a while to dry.")
		: TEXT(" The ground is damp from earlier rain and slowly drying.");
}

void UIslandEnvironmentSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World) return;
	if (!bWetnessInitialized)
	{
		bWetnessInitialized = true;
		if (const UIslandWorldStateSubsystem* WorldState = World->GetSubsystem<UIslandWorldStateSubsystem>())
			if (const TOptional<float> SavedWetness = WorldState->GetSavedWetness(); SavedWetness.IsSet()) Wetness = SavedWetness.GetValue();
	}
	const double Now = World->GetTimeSeconds();

	// Sample where it matters for presentation: the viewer, else the weather actor itself.
	FVector Viewpoint = FVector::ZeroVector;
	const AActor* ViewpointActor = nullptr;
	bool bHasViewpoint = false;
	if (const APlayerController* Player = World->GetFirstPlayerController(); Player && Player->GetPawn())
	{
		ViewpointActor = Player->GetPawn();
		Viewpoint = ViewpointActor->GetActorLocation();
		bHasViewpoint = true;
	}
	RainIntensity = CloudCover = Storm = LightningFlash = 0.f;
	Wind = FVector::ZeroVector;
	FoliageWind = FVector::ZeroVector;
	float FoliageWindReferenceSpeed = 180.f;
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		if (!bHasViewpoint)
		{
			Viewpoint = It->GetActorLocation();
			ViewpointActor = *It;
			bHasViewpoint = true;
		}
		RainIntensity = FMath::Clamp(It->SampleRainIntensity(Now), 0.f, 1.f);
		CloudCover = FMath::Clamp(It->SampleCloudCover(Now), 0.f, 1.f);
		Wind = It->GetLocalWind(Viewpoint);
		FoliageWind = It->SampleWind(Viewpoint, Now);
		FoliageWindReferenceSpeed = FMath::Max(1.f, It->MaximumWindSpeed);
		Storm = FMath::Clamp(It->SampleStormIntensity(Now), 0.f, 1.f);
		LightningFlash = It->GetLightningFlash();
		break;
	}
	Indoors = IsInsideInnAt(World, Viewpoint, ViewpointActor) ? 1.f : 0.f;
	IslandHour = 12.f;
	for (TActorIterator<AIslandDayNight> It(World); It; ++It) { IslandHour = It->CurrentHour; break; }
	SunHeight = AIslandDayNight::SunHeight(IslandHour);
	Daylight = FMath::SmoothStep(-0.04f, 0.18f, SunHeight);
	GoldenHour = GoldenHourFor(SunHeight);
	Wetness = ForcedWetness >= 0.f ? FMath::Clamp(ForcedWetness, 0.f, 1.f) : StepWetness(Wetness, RainIntensity, Daylight, Wind.Size2D(), DeltaTime);
	// Mist eases toward its target rather than snapping, so fog rolls in and lifts.
	const float TargetMist = Now < ForcedMistUntil ? FMath::Clamp(ForcedMist, 0.f, 1.f) : MistFor(Wetness, IslandHour, Wind.Size2D(), RainIntensity, Storm);
	Mist = FMath::FInterpTo(Mist, TargetMist, DeltaTime, Now < ForcedMistUntil ? 2.f : 0.05f);
	if (Fog.IsValid())
	{
		// Denser and lower-lying as the mist thickens.
		Fog->GetComponent()->SetFogDensity(BaseFogDensity * (1.f + 9.f * Mist) + 0.02f * Mist);
		Fog->GetComponent()->SetFogHeightFalloff(BaseFogFalloff * (1.f + 2.f * Mist));
		// In thick mist, lamps and lightning glow through the air instead of simply fading with distance.
		const bool bVolumetric = Mist > 0.25f;
		if (Fog->GetComponent()->bEnableVolumetricFog != bVolumetric) Fog->GetComponent()->SetVolumetricFog(bVolumetric);
	}
	ApplyLandscapeWetness();
	if (bFoliageMaterialWindEnabled && FoliageWindCollection)
	{
		if (UMaterialParameterCollectionInstance* FoliageInstance = World->GetParameterCollectionInstance(FoliageWindCollection))
		{
			const float DirectionRadians = FoliageWind.SizeSquared2D() <= SMALL_NUMBER ? 0.f : FMath::Atan2(FoliageWind.Y, FoliageWind.X);
			FoliageInstance->SetScalarParameterValue(FoliageWindDirectionParameter, DirectionRadians);
			FoliageInstance->SetScalarParameterValue(FoliageWindStrengthParameter,
				FMath::Clamp(FoliageWind.Size2D() / FoliageWindReferenceSpeed, 0.f, 1.f));
		}
	}

	UMaterialParameterCollectionInstance* Instance = Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
	if (!Instance) return;
	const float Values[] = { RainIntensity, Wetness, CloudCover, static_cast<float>(Wind.Size2D()), Daylight, SunHeight, GoldenHour, IslandHour, Storm, LightningFlash, Mist, Indoors };
	static_assert(UE_ARRAY_COUNT(Values) == 12, "Keep values in the order of ScalarParameterNames.");
	const TArray<FName>& Names = ScalarParameterNames();
	for (int32 Index = 0; Index < Names.Num(); ++Index) Instance->SetScalarParameterValue(Names[Index], Values[Index]);
	const FVector Direction = Wind.GetSafeNormal();
	Instance->SetVectorParameterValue(WindDirectionParameter, FLinearColor(Direction.X, Direction.Y, Direction.Z, Wind.Size()));
}

static FAutoConsoleCommandWithWorldAndArgs GIslandWetnessCommand(
	TEXT("Island.Wetness"),
	TEXT("Developer override: hold ground wetness at an amount (0..1); no argument returns it to the weather. Not saved. Usage: Island.Wetness [amount]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UIslandEnvironmentSubsystem* Environment = World ? World->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr)
			Environment->ForcedWetness = Args.Num() > 0 ? FCString::Atof(*Args[0]) : -1.f;
	}));

static FAutoConsoleCommandWithWorldAndArgs GIslandMistCommand(
	TEXT("Island.Mist"),
	TEXT("Developer override: hold the mist at an amount (0..1) for some seconds of play (default 0.8 for 120). Not saved. Usage: Island.Mist [amount] [seconds]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIslandEnvironmentSubsystem* Environment = World ? World->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
		if (!Environment) return;
		Environment->ForcedMist = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.8f;
		Environment->ForcedMistUntil = World->GetTimeSeconds() + (Args.Num() > 1 ? FCString::Atod(*Args[1]) : 120.0);
	}));
