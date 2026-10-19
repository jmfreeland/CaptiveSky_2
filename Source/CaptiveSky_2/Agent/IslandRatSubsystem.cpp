#include "IslandRatSubsystem.h"

#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "IslandInnkeeperSubsystem.h"
#include "IslandWorldStateSubsystem.h"
#include "RatAgentAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

bool UIslandRatSubsystem::FindRatHomeLocation(const TArray<FIslandWildlifeHomeRecord>& Homes, FVector& OutLocation,
	FRotator* OutEntranceRotation)
{
	const FIslandWildlifeHomeRecord* BestHome = nullptr;
	for (const FIslandWildlifeHomeRecord& Home : Homes)
	{
		if (!Home.Species.ToString().Equals(TEXT("Rat"), ESearchCase::IgnoreCase) ||
			Home.OccupantAgentId != GetAgentId() || Home.ShelterState == EIslandWildlifeShelterState::Disturbed ||
			Home.EntranceLocation.ContainsNaN() || Home.EntranceLocation.GetAbsMax() > 10000000.f ||
			!FMath::IsFinite(Home.EntranceRotation.Pitch) || !FMath::IsFinite(Home.EntranceRotation.Yaw) ||
			!FMath::IsFinite(Home.EntranceRotation.Roll))
		{
			continue;
		}

		if (!BestHome || (Home.ShelterState == EIslandWildlifeShelterState::Settled && BestHome->ShelterState != EIslandWildlifeShelterState::Settled) ||
			(Home.ShelterState == BestHome->ShelterState && Home.ShelterQuality > BestHome->ShelterQuality) ||
			(Home.ShelterState == BestHome->ShelterState && Home.ShelterQuality == BestHome->ShelterQuality && Home.Id.LexicalLess(BestHome->Id)))
		{
			BestHome = &Home;
		}
	}

	if (!BestHome) return false;
	OutLocation = BestHome->EntranceLocation;
	if (OutEntranceRotation) *OutEntranceRotation = BestHome->EntranceRotation.GetNormalized();
	return true;
}

bool UIslandRatSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandRatSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!UIslandInnkeeperSubsystem::IsIslandMapName(InWorld.GetMapName())) return;
	SpawnRat(InWorld);
}

void UIslandRatSubsystem::SpawnRat(UWorld& World)
{
	const FString AgentId = GetAgentId();
	for (TActorIterator<AAutonomousAgentCharacter> It(&World); It; ++It)
		if (It->Memory && It->Memory->GetResolvedAgentId() == AgentId) return;

	// The rat body is local Content/; without it there is nothing to spawn.
	UClass* BodyClass = LoadObject<UClass>(nullptr, GetBodyClassPath(), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!BodyClass || !BodyClass->IsChildOf(AAutonomousAgentCharacter::StaticClass())) return;

	FVector Centre = FVector::ZeroVector;
	FRotator HomeFacing = FRotator::ZeroRotator;
	const UIslandWorldStateSubsystem* WorldState = World.GetSubsystem<UIslandWorldStateSubsystem>();
	bool bHasRatHome = WorldState && FindRatHomeLocation(WorldState->GetWildlifeHomes(), Centre, &HomeFacing);
	const auto FindInnMarker = [&World]() -> const AActor*
	{
		for (TActorIterator<AActor> It(&World); It; ++It)
			if (It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("Inn"))) return *It;
		return nullptr;
	};
	if (!bHasRatHome)
	{
		const AActor* InnMarker = FindInnMarker();
		if (!InnMarker)
		{
			UE_LOG(LogTemp, Warning, TEXT("Island rat was not spawned: no matching wildlife home or tagged Inn landmark is available."));
			return;
		}
		Centre = InnMarker->GetActorLocation();
	}

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
	if (!Navigation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island rat was not spawned: no navigation system is available."));
		return;
	}

	// Search close to a registered entrance; otherwise retain the established Inn-side fallback.
	bool bFound = false;
	FNavLocation NavLocation;
	const auto TryFindNavigablePoint = [&Navigation, &NavLocation](const FVector& SearchCentre, const TArray<float>& SearchRadii)
	{
		for (const float Radius : SearchRadii)
		{
			const int32 StepCount = FMath::IsNearlyZero(Radius) ? 1 : 12;
			for (int32 Step = 0; Step < StepCount; ++Step)
			{
				const float Angle = FMath::DegreesToRadians(30.f * Step);
				const FVector Candidate = SearchCentre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
				if (Navigation->ProjectPointToNavigation(Candidate, NavLocation, FVector(150.f, 150.f, 400.f))) return true;
			}
		}
		return false;
	};
	bFound = TryFindNavigablePoint(Centre, bHasRatHome ? TArray<float>{ 0.f, 120.f, 240.f, 400.f } : TArray<float>{ 700.f, 900.f, 1200.f });
	if (!bFound && bHasRatHome)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island rat home has no nearby navigable point; using the Inn fallback."));
		bHasRatHome = false;
		HomeFacing = FRotator::ZeroRotator;
		const AActor* InnMarker = FindInnMarker();
		if (InnMarker)
		{
			Centre = InnMarker->GetActorLocation();
			bFound = TryFindNavigablePoint(Centre, { 700.f, 900.f, 1200.f });
		}
	}
	if (!bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island rat was not spawned: no navigable point near the Inn."));
		return;
	}

	const AAutonomousAgentCharacter* Defaults = Cast<AAutonomousAgentCharacter>(BodyClass->GetDefaultObject());
	const float HalfHeight = Defaults && Defaults->GetCapsuleComponent() ? Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 14.f;
	const FRotator SpawnRotation(0.f, bHasRatHome ? HomeFacing.Yaw : 0.f, 0.f);
	const FTransform SpawnTransform(SpawnRotation, NavLocation.Location + FVector(0.f, 0.f, HalfHeight + 2.f));
	AAutonomousAgentCharacter* Rat = World.SpawnActorDeferred<AAutonomousAgentCharacter>(
		BodyClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Rat) return;

	if (Rat->Memory) Rat->Memory->AgentId = AgentId;
	Rat->DisplayName = TEXT("Rat");
	Rat->Tags.AddUnique(TEXT("Rat"));
	// The placeholder Blueprint's controller class can fall back to the generic one; the rat needs its Fox-animation controller.
	Rat->AIControllerClass = ARatAgentAIController::StaticClass();
	Rat->FinishSpawning(SpawnTransform);
	if (!Rat->GetController()) Rat->SpawnDefaultController();
	UE_LOG(LogTemp, Log, TEXT("Spawned Island rat with stable identity %s at %s (%s, controller %s, mesh %s)."), *AgentId, *NavLocation.Location.ToCompactString(),
		bHasRatHome ? TEXT("registered wildlife home") : TEXT("Inn fallback"),
		*GetNameSafe(Rat->GetController()), Rat->GetMesh() ? *GetNameSafe(Rat->GetMesh()->GetSkeletalMeshAsset()) : TEXT("none"));
}
