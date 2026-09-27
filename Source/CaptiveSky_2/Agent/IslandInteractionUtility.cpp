#include "IslandInteractionUtility.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "IslandFirefly.h"
#include "IslandListeningStonesChime.h"
#include "IslandPoolRippleEffect.h"
#include "IslandTidepoolCrab.h"
#include "IslandWeather.h"
#include "IslandWindMoteEffect.h"

FName IslandInteractionUtility::GetTargetTag(const AActor* Target)
{
	if (!Target) return NAME_None;
	if (Target->ActorHasTag(TEXT("IslandLife")))
	{
		if (Target->ActorHasTag(TEXT("Firefly"))) return FName(TEXT("Firefly"));
		if (Target->ActorHasTag(TEXT("TidepoolCrab"))) return FName(TEXT("TidepoolCrab"));
		return NAME_None;
	}
	if (!Target->ActorHasTag(TEXT("IslandLandmark"))) return NAME_None;
	for (const FName Tag : Target->Tags)
		if (Tag != FName(TEXT("IslandLandmark")) && Tag != FName(TEXT("IslandLife"))) return Tag;
	return NAME_None;
}

bool IslandInteractionUtility::CanInteract(const AActor* Observer, const AActor* Target, float MaxRange)
{
	if (!IsValid(Observer) || !IsValid(Target) || Observer == Target || !Observer->GetWorld() ||
		Observer->GetWorld() != Target->GetWorld() || Target->IsHidden() ||
		FVector::DistSquared(Observer->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(FMath::Max(0.f, MaxRange)))
	{
		return false;
	}

	FVector Start = Observer->GetActorLocation();
	FRotator ViewRotation = Observer->GetActorRotation();
	Observer->GetActorEyesViewPoint(Start, ViewRotation);
	FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 100.f);
	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;
	Target->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	if (!BoundsExtent.IsNearlyZero() && BoundsOrigin.Z > Target->GetActorLocation().Z + 25.f)
		End = BoundsOrigin;

	FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandInteractionVisibility), false, Observer);
	Query.AddIgnoredActor(Target);
	FHitResult Hit;
	return !Observer->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
}

AActor* IslandInteractionUtility::FindNearestVisibleTarget(const AActor* Observer, UWorld* World, float MaxRange)
{
	if (!IsValid(Observer) || !IsValid(World) || Observer->GetWorld() != World) return nullptr;
	AActor* Nearest = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(0.f, MaxRange));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (GetTargetTag(*It).IsNone() || !CanInteract(Observer, *It, MaxRange)) continue;
		const float DistanceSquared = FVector::DistSquared(Observer->GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			Nearest = *It;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return Nearest;
}

bool IslandInteractionUtility::CanInspect(const AActor* Observer, const AActor* Target, float MaxRange)
{
	if (!IsValid(Observer) || !IsValid(Target) || Observer == Target || !Observer->GetWorld() ||
		Observer->GetWorld() != Target->GetWorld() ||
		(Target->ActorHasTag(TEXT("IslandLife")) && Target->IsHidden()) ||
		FVector::DistSquared(Observer->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(FMath::Max(0.f, MaxRange)))
	{
		return false;
	}

	FCollisionQueryParams Query(SCENE_QUERY_STAT(AgentInspect), false, Observer);
	Query.AddIgnoredActor(Target);
	FHitResult Hit;
	return !Observer->GetWorld()->LineTraceSingleByChannel(Hit, Observer->GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Query);
}

bool IslandInteractionUtility::Perform(AActor* Observer, AActor* Target, FString& OutFact)
{
	OutFact.Reset();
	if (!IsValid(Observer) || !IsValid(Target) || !Observer->GetWorld() || Observer->GetWorld() != Target->GetWorld()) return false;
	const FName TargetTag = GetTargetTag(Target);
	UWorld* World = Observer->GetWorld();

	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("Firefly")))
	{
		if (AIslandFirefly* Firefly = Cast<AIslandFirefly>(Target)) Firefly->RespondToQuietObservation();
		OutFact = TEXT("You quietly watched a nearby firefly. Its glow briefly brightened within its ordinary pulse; it remains wild and independent. You did not touch, catch, or claim it, and it may drift away.");
		return true;
	}
	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("TidepoolCrab")))
	{
		if (AIslandTidepoolCrab* Crab = Cast<AIslandTidepoolCrab>(Target)) Crab->RespondToQuietObservation(Observer->GetActorLocation());
		OutFact = TEXT("You quietly watched a small shore crab. It scuttled a short way toward cover, paused, then resumed its usual Tideglass path. It remains wild and independent; you did not touch, catch, or claim it, and nothing persistent changed.");
		return true;
	}
	if (!Target->ActorHasTag(TEXT("IslandLandmark"))) return false;

	if (TargetTag == FName(TEXT("ListeningStones")))
	{
		FVector LocalWind = FVector::ZeroVector;
		bool bWeatherSampled = false;
		for (TActorIterator<AIslandWeather> WeatherIt(World); WeatherIt; ++WeatherIt)
		{
			LocalWind = WeatherIt->GetLocalWind(Target->GetActorLocation(), Target);
			bWeatherSampled = true;
			break;
		}
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(Target->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
		OutFact = Chime
			? bWeatherSampled
				? TEXT("Your inspection woke a quiet, layered resonance in the ListeningStones. Its pitch is tuned to the present local wind; it rings softly nearby and fades within a few seconds. The sound is synthesized locally, reveals nothing, and leaves no lasting change.")
				: TEXT("Your inspection woke a quiet, layered resonance in the ListeningStones. No IslandWeather signal was present, so it used its calm-air pitch; it rings softly nearby, reveals nothing, and leaves no lasting change.")
			: TEXT("You inspected the ListeningStones, but their short-lived resonance could not be created. No persistent change occurred.");
		if (Chime) Chime->BeginChime(LocalWind.Size2D());
		return true;
	}

	if (TargetTag == FName(TEXT("WindArch")))
	{
		bool bWindResponded = false;
		bool bVisibleMotesCreated = false;
		for (TActorIterator<AIslandWeather> WeatherIt(World); WeatherIt; ++WeatherIt)
		{
			const FVector ExistingWind = WeatherIt->GetLocalWind(Target->GetActorLocation(), Target);
			const FVector GustDirection = ExistingWind.IsNearlyZero() ? Target->GetActorForwardVector() : ExistingWind.GetSafeNormal();
			WeatherIt->AddTransientGust(Target->GetActorLocation(), GustDirection, 220.f, 1400.f, 18.f);
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AIslandWindMoteEffect* Motes = World->SpawnActor<AIslandWindMoteEffect>(Target->GetActorLocation() + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, SpawnParameters))
			{
				Motes->InitializeGust(GustDirection, 1400.f, 18.f);
				bVisibleMotesCreated = true;
			}
			bWindResponded = true;
			break;
		}
		OutFact = bWindResponded && bVisibleMotesCreated
			? TEXT("Your interaction with the WindArch created a short-lived gust in the simulated local wind. Three small illuminated motes briefly trace its changing airflow; both effects fade over eighteen seconds of Island time. Nearby residents can sense the changed wind, and the raven's flight responds to it. No lasting weather change occurred.")
			: bWindResponded
			? TEXT("Your interaction with the WindArch created a short-lived gust in the simulated local wind. It fades over eighteen seconds of Island time. Nearby residents can sense the changed wind, and the raven's flight responds to it, but its temporary visual cue could not be created. No lasting weather change occurred.")
			: TEXT("You inspected the WindArch, but no IslandWeather actor is active, so no gust was created. No lasting change occurred.");
		return true;
	}

	if (TargetTag == FName(TEXT("TideglassPool")))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector SurfaceLocation = Target->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		AIslandPoolRippleEffect* Ripple = World->SpawnActor<AIslandPoolRippleEffect>(SurfaceLocation, FRotator::ZeroRotator, SpawnParameters);
		OutFact = Ripple
			? TEXT("Your interaction sent a short ring of cool highlights across the flattened TideglassPool prototype surface. It expands and fades; it changes no permanent level state, and reveals no hidden item or reward. Stronger showers can create fainter ripples on their own; those are weather, not an effect you caused.")
			: TEXT("You inspected the TideglassPool, but the temporary surface-light response could not be created. No persistent change occurred.");
		return true;
	}

	OutFact = TEXT("You inspected a visible Island landmark. It is currently static prototype scenery: no hidden item, puzzle response, sound, or other interactive effect is implemented. Inspection is complete; returning immediately provides no new result.");
	return true;
}
