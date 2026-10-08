#include "IslandWeatherTraces.h"
#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandWrack.h"
#include "EngineUtils.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWeatherTraces, Log, All);

bool UIslandWeatherTracesSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UIslandWeatherTracesSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandWeatherTracesSubsystem, STATGROUP_Tickables);
}

void UIslandWeatherTracesSubsystem::Tick(float DeltaTime)
{
	SinceCheck += DeltaTime;
	if (SinceCheck < 5.f || !GetWorld()) return;
	SinceCheck = 0.f;
	EvaluateStorm(GetWorld()->GetTimeSeconds());
}

bool UIslandWeatherTracesSubsystem::EvaluateStorm(double Now)
{
	UWorld* World = GetWorld();
	UIslandWorldStateSubsystem* WorldState = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!WorldState) return false;
	for (TActorIterator<AIslandWeather> It(World); It; ++It)
	{
		if (It->SampleStormIntensity(Now) < MarkingStorm) return false;
		TArray<FString> Marks;
		if (!WorldState->ApplyStormMarks(It->WeatherTimeOffset + Now, UIslandWorldStateSubsystem::CurrentIslandDay(World), Marks)) return false;
		for (const FString& Mark : Marks) UE_LOG(LogIslandWeatherTraces, Log, TEXT("Storm mark: %s"), *Mark);
		if (UIslandWrackSubsystem* Wrack = World->GetSubsystem<UIslandWrackSubsystem>())
			Wrack->DepositAfterStorm(UIslandWorldStateSubsystem::CurrentIslandDay(World));
		return Marks.Num() > 0;
	}
	return false;
}
