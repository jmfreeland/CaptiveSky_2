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
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

const TCHAR* UIslandEnvironmentSubsystem::CollectionPath = TEXT("/Game/Environment/MPC_IslandEnvironment.MPC_IslandEnvironment");
const FName UIslandEnvironmentSubsystem::WindDirectionParameter(TEXT("WindDirection"));
const FName UIslandEnvironmentSubsystem::LandscapeWetnessParameter(TEXT("Ground Wetness"));

const TArray<FName>& UIslandEnvironmentSubsystem::ScalarParameterNames()
{
	static const TArray<FName> Names = {
		TEXT("RainIntensity"), TEXT("Wetness"), TEXT("CloudCover"), TEXT("WindSpeed"),
		TEXT("Daylight"), TEXT("SunHeight"), TEXT("GoldenHour"), TEXT("IslandHour"), TEXT("Storm"), TEXT("LightningFlash") };
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
	InitializeLandscapeMaterials();
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
					UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(Original, this);
					if (!Dynamic) continue;
					MaterialIndex = LandscapeOriginalMaterials.Add(Original);
					LandscapeMaterialInstances.Add(Dynamic);
					LandscapeWetnessBaselines.Add(FMath::Clamp(AuthoredWetness, 0.f, 1.f));
				}
				if (LandscapeMaterialInstances.IsValidIndex(MaterialIndex))
					Component->SetMaterial(Index, LandscapeMaterialInstances[MaterialIndex]);
			}
		}
	}
	ApplyLandscapeWetness();
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
	if (const APlayerController* Player = World->GetFirstPlayerController(); Player && Player->GetPawn()) Viewpoint = Player->GetPawn()->GetActorLocation();
	RainIntensity = CloudCover = Storm = LightningFlash = 0.f;
	Wind = FVector::ZeroVector;
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		if (Viewpoint.IsZero()) Viewpoint = It->GetActorLocation();
		RainIntensity = FMath::Clamp(It->SampleRainIntensity(Now), 0.f, 1.f);
		CloudCover = FMath::Clamp(It->SampleCloudCover(Now), 0.f, 1.f);
		Wind = It->GetLocalWind(Viewpoint);
		Storm = FMath::Clamp(It->SampleStormIntensity(Now), 0.f, 1.f);
		LightningFlash = It->GetLightningFlash();
		break;
	}
	IslandHour = 12.f;
	for (TActorIterator<AIslandDayNight> It(World); It; ++It) { IslandHour = It->CurrentHour; break; }
	SunHeight = AIslandDayNight::SunHeight(IslandHour);
	Daylight = FMath::SmoothStep(-0.04f, 0.18f, SunHeight);
	GoldenHour = GoldenHourFor(SunHeight);
	Wetness = StepWetness(Wetness, RainIntensity, Daylight, Wind.Size2D(), DeltaTime);
	ApplyLandscapeWetness();

	UMaterialParameterCollectionInstance* Instance = Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
	if (!Instance) return;
	const float Values[] = { RainIntensity, Wetness, CloudCover, static_cast<float>(Wind.Size2D()), Daylight, SunHeight, GoldenHour, IslandHour, Storm, LightningFlash };
	static_assert(UE_ARRAY_COUNT(Values) == 10, "Keep values in the order of ScalarParameterNames.");
	const TArray<FName>& Names = ScalarParameterNames();
	for (int32 Index = 0; Index < Names.Num(); ++Index) Instance->SetScalarParameterValue(Names[Index], Values[Index]);
	const FVector Direction = Wind.GetSafeNormal();
	Instance->SetVectorParameterValue(WindDirectionParameter, FLinearColor(Direction.X, Direction.Y, Direction.Z, Wind.Size()));
}
