#include "IslandEnvironmentSubsystem.h"
#include "IslandDayNight.h"
#include "IslandWeather.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

const TCHAR* UIslandEnvironmentSubsystem::CollectionPath = TEXT("/Game/Environment/MPC_IslandEnvironment.MPC_IslandEnvironment");
const FName UIslandEnvironmentSubsystem::WindDirectionParameter(TEXT("WindDirection"));

const TArray<FName>& UIslandEnvironmentSubsystem::ScalarParameterNames()
{
	static const TArray<FName> Names = {
		TEXT("RainIntensity"), TEXT("Wetness"), TEXT("CloudCover"), TEXT("WindSpeed"),
		TEXT("Daylight"), TEXT("SunHeight"), TEXT("GoldenHour"), TEXT("IslandHour") };
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
}

TStatId UIslandEnvironmentSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandEnvironmentSubsystem, STATGROUP_Tickables);
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
	const double Now = World->GetTimeSeconds();

	// Sample where it matters for presentation: the viewer, else the weather actor itself.
	FVector Viewpoint = FVector::ZeroVector;
	if (const APlayerController* Player = World->GetFirstPlayerController(); Player && Player->GetPawn()) Viewpoint = Player->GetPawn()->GetActorLocation();
	RainIntensity = CloudCover = 0.f;
	Wind = FVector::ZeroVector;
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		if (Viewpoint.IsZero()) Viewpoint = It->GetActorLocation();
		RainIntensity = FMath::Clamp(It->SampleRainIntensity(Now), 0.f, 1.f);
		CloudCover = FMath::Clamp(It->SampleCloudCover(Now), 0.f, 1.f);
		Wind = It->GetLocalWind(Viewpoint);
		break;
	}
	IslandHour = 12.f;
	for (TActorIterator<AIslandDayNight> It(World); It; ++It) { IslandHour = It->CurrentHour; break; }
	SunHeight = AIslandDayNight::SunHeight(IslandHour);
	Daylight = FMath::SmoothStep(-0.04f, 0.18f, SunHeight);
	GoldenHour = GoldenHourFor(SunHeight);
	Wetness = StepWetness(Wetness, RainIntensity, Daylight, Wind.Size2D(), DeltaTime);

	UMaterialParameterCollectionInstance* Instance = Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
	if (!Instance) return;
	const float Values[] = { RainIntensity, Wetness, CloudCover, static_cast<float>(Wind.Size2D()), Daylight, SunHeight, GoldenHour, IslandHour };
	const TArray<FName>& Names = ScalarParameterNames();
	for (int32 Index = 0; Index < Names.Num(); ++Index) Instance->SetScalarParameterValue(Names[Index], Values[Index]);
	const FVector Direction = Wind.GetSafeNormal();
	Instance->SetVectorParameterValue(WindDirectionParameter, FLinearColor(Direction.X, Direction.Y, Direction.Z, Wind.Size()));
}
