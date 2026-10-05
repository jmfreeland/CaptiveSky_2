#include "IslandInteractionUtility.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "IslandGuestBook.h"
#include "IslandArrangement.h"
#include "IslandInnHearthSubsystem.h"
#include "IslandFirefly.h"
#include "IslandForestStag.h"
#include "IslandListeningStonesChime.h"
#include "IslandListeningStonePresentation.h"
#include "IslandPoolRippleEffect.h"
#include "IslandTidepoolCrab.h"
#include "IslandTideglassDragonfly.h"
#include "IslandTidepoolMinnows.h"
#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandWindMoteEffect.h"

FName IslandInteractionUtility::GetTargetTag(const AActor* Target)
{
	if (!Target) return NAME_None;
	if (Target->IsA<AIslandGuestBook>()) return FName(TEXT("GuestBook"));
	if (Target->IsA<AIslandArrangement>()) return FName(TEXT("IslandArrangement"));
	if (Target->ActorHasTag(TEXT("IslandLife")))
	{
		if (Target->ActorHasTag(TEXT("WoodlandDeer"))) return FName(TEXT("WoodlandDeer"));
		if (Target->ActorHasTag(TEXT("Firefly"))) return FName(TEXT("Firefly"));
		if (Target->ActorHasTag(TEXT("TidepoolCrab"))) return FName(TEXT("TidepoolCrab"));
		if (Target->ActorHasTag(TEXT("TideglassDragonfly"))) return FName(TEXT("TideglassDragonfly"));
		if (Target->ActorHasTag(TEXT("MinnowSchool"))) return FName(TEXT("MinnowSchool"));
		return NAME_None;
	}
	if (Target->ActorHasTag(TEXT("IslandInn")) && Target->ActorHasTag(TEXT("InnHearth"))) return FName(TEXT("InnHearth"));
	if (!Target->ActorHasTag(TEXT("IslandLandmark"))) return NAME_None;
	for (const FName Tag : Target->Tags)
		if (Tag != FName(TEXT("IslandLandmark")) && Tag != FName(TEXT("IslandLife"))) return Tag;
	return NAME_None;
}

bool IslandInteractionUtility::IsMovementTargetAllowed(const AActor* Target)
{
	return IsValid(Target) && !Target->ActorHasTag(TEXT("IslandLife"));
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
	if (TargetTag == FName(TEXT("GuestBook")))
	{
		const UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
		if (!State)
		{
			OutFact = TEXT("The inn guest book cannot be read here. Nothing changed.");
			return false;
		}

		const TArray<FIslandGuestBookEntry>& Entries = State->GetGuestBookEntries();
		if (Entries.IsEmpty())
		{
			OutFact = TEXT("The guest book's pages are blank. Nothing was written.");
			return true;
		}

		OutFact = TEXT("The inn guest book, latest entries:\n");
		const int32 First = FMath::Max(0, Entries.Num() - 3);
		for (int32 Index = First; Index < Entries.Num(); ++Index)
		{
			const FIslandGuestBookEntry& Entry = Entries[Index];
			FString Line = Entry.Line.Left(34);
			if (Entry.Line.Len() > 34) Line += TEXT("...");
			if (Index > First) OutFact += TEXT("\n");
			OutFact += FString::Printf(TEXT("Day %d - %s: %s"), Entry.Day, *Entry.AgentId.Left(12), *Line);
		}
		return true;
	}

	if (TargetTag == FName(TEXT("InnHearth")) && Target->ActorHasTag(TEXT("IslandInn")))
	{
		if (UIslandInnHearthSubsystem* Hearth = World->GetSubsystem<UIslandInnHearthSubsystem>())
			return Hearth->TendHearth(OutFact);
		OutFact = TEXT("You inspected the inn hearth, but no hearth response is active. Nothing changed.");
		return false;
	}

	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("Firefly")))
	{
		if (AIslandFirefly* Firefly = Cast<AIslandFirefly>(Target)) Firefly->RespondToQuietObservation();
		OutFact = TEXT("You quietly watched a nearby firefly. Its glow briefly brightened within its ordinary pulse; it remains wild and independent. You did not touch, catch, or claim it, and it may drift away.");
		return true;
	}
	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("WoodlandDeer")))
	{
		if (AIslandForestStag* Deer = Cast<AIslandForestStag>(Target)) Deer->RespondToQuietObservation(Observer->GetActorLocation());
		OutFact = TEXT("A wild stag lifted its head, bounded a short way toward the nearby trees, then settled back into grazing. It remains independent; you did not touch, follow, feed, or claim it, and nothing persistent changed.");
		return true;
	}
	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("TidepoolCrab")))
	{
		if (AIslandTidepoolCrab* Crab = Cast<AIslandTidepoolCrab>(Target)) Crab->RespondToQuietObservation(Observer->GetActorLocation());
		OutFact = TEXT("You quietly watched a small shore crab. It scuttled a short way toward cover, paused, then resumed its usual Tideglass path. It remains wild and independent; you did not touch, catch, or claim it, and nothing persistent changed.");
		return true;
	}
	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("TideglassDragonfly")))
	{
		if (AIslandTideglassDragonfly* Dragonfly = Cast<AIslandTideglassDragonfly>(Target)) Dragonfly->RespondToQuietObservation(Observer->GetActorLocation());
		OutFact = TEXT("You quietly watched a Tideglass dragonfly hover above the shore. It darted a little way off, then resumed circling near the pool; it remains wild and independent. You did not touch, catch, or claim it, and nothing persistent changed.");
		return true;
	}
	if (Target->ActorHasTag(TEXT("IslandLife")) && TargetTag == FName(TEXT("MinnowSchool")))
	{
		if (AIslandTidepoolMinnows* Minnows = Cast<AIslandTidepoolMinnows>(Target)) Minnows->RespondToQuietObservation(Observer->GetActorLocation());
		OutFact = TEXT("A small school of minnows flicked through the Tideglass shallows, scattered from your quiet attention, then began circling back together. They remain wild and uncaught; nothing persistent changed.");
		return true;
	}
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
		bool bVisibleFireflyResponse = false;
		if (Chime)
		{
			Chime->BeginChime(LocalWind.Size2D());
			if (UIslandListeningStonePresentationSubsystem* Presentation = World->GetSubsystem<UIslandListeningStonePresentationSubsystem>())
				Presentation->NotifyChime(LocalWind.Size2D());
			for (TActorIterator<AIslandFirefly> FireflyIt(World); FireflyIt; ++FireflyIt)
			{
				if (FireflyIt->RespondToSoftChime(Chime) &&
					CanInspect(Observer, *FireflyIt, AIslandListeningStonesChime::AudibleRadius))
				{
					bVisibleFireflyResponse = true;
				}
			}
		}
		OutFact = Chime
			? bWeatherSampled
				? TEXT("Your inspection woke a quiet, layered resonance in the ListeningStones. Its pitch is tuned to the present local wind; it rings softly nearby and fades within a few seconds. The sound is synthesized locally, reveals nothing, and leaves no lasting change.")
				: TEXT("Your inspection woke a quiet, layered resonance in the ListeningStones. No IslandWeather signal was present, so it used its calm-air pitch; it rings softly nearby, reveals nothing, and leaves no lasting change.")
			: TEXT("You inspected the ListeningStones, but their short-lived resonance could not be created. No persistent change occurred.");
		if (bVisibleFireflyResponse)
		{
			OutFact += TEXT(" In clear view, a nearby wild firefly answered with a small glow lift; it remains uncaught and unchanged.");
		}
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
		bool bNearbyMinnowsResponded = false;
		if (Ripple)
		{
			for (TActorIterator<AIslandTidepoolMinnows> It(World); It; ++It)
			{
				if (FVector::DistSquared(It->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(800.f) ||
					!CanInspect(Observer, *It, 800.f))
					continue;
				bNearbyMinnowsResponded = It->RespondToSurfaceRipple();
				break;
			}
		}
		OutFact = Ripple
			? bNearbyMinnowsResponded
				? TEXT("Your interaction sent a short ring of cool highlights across the flattened TideglassPool prototype surface. The nearby minnow school briefly widened its circle of motion, then returned to its usual path. Both responses fade; the fish remain wild and uncaught, no permanent level state changes, and no hidden item or reward is revealed. Stronger showers can create fainter ripples on their own; those are weather, not an effect you caused.")
				: TEXT("Your interaction sent a short ring of cool highlights across the flattened TideglassPool prototype surface. It expands and fades; it changes no permanent level state, and reveals no hidden item or reward. Stronger showers can create fainter ripples on their own; those are weather, not an effect you caused.")
			: TEXT("You inspected the TideglassPool, but the temporary surface-light response could not be created. No persistent change occurred.");
		return true;
	}

	if (TargetTag == FName(TEXT("IslandArrangement")))
	{
		const AIslandArrangement* Arrangement = Cast<AIslandArrangement>(Target);
		const UIslandWorldStateSubsystem* State = World->GetSubsystem<UIslandWorldStateSubsystem>();
		const FIslandArrangementSite* Site = Arrangement && State ? State->FindArrangementSite(Arrangement->GetSiteId()) : nullptr;
		if (!Site)
		{
			OutFact = TEXT("You looked over the stone-arranging ground, but no shared site record was available. Nothing changed.");
			return false;
		}
		if (!Site->bHasWork)
		{
			OutFact = FString::Printf(TEXT("This open patch near the ListeningStones has no arrangement yet. You could use build with target %s if you wish; inspecting the empty ground changes nothing."),
				*Site->Id.ToString());
			return true;
		}

		const int32 AgeDays = FMath::Max(0, UIslandWorldStateSubsystem::CurrentIslandDay(World) - Site->Day);
		const TCHAR* Weathering = AgeDays <= 0 ? TEXT("freshly placed") : AgeDays < 4 ? TEXT("a little weathered") : TEXT("mossy and settled");
		OutFact = FString::Printf(TEXT("You looked closely at a %s made from %d small stones; it is %s. %d small arcs of stones answer it. The maker, title, and private meaning are not visible here. Looking changes nothing; the separate build action is how a resident can leave a response."),
			*UIslandWorldStateSubsystem::FormName(Site->Form), AIslandArrangement::StoneCountFor(Site->Form), Weathering, Site->Responses.Num());
		return true;
	}

	if (!Target->ActorHasTag(TEXT("IslandLandmark"))) return false;
	OutFact = TEXT("You inspected a visible Island landmark. It is currently static prototype scenery: no hidden item, puzzle response, sound, or other interactive effect is implemented. Inspection is complete; returning immediately provides no new result.");
	return true;
}
