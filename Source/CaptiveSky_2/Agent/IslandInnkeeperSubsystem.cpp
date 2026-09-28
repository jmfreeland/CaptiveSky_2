#include "IslandInnkeeperSubsystem.h"

#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Misc/PackageName.h"

bool UIslandInnkeeperSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UIslandInnkeeperSubsystem::IsIslandMapName(const FString& MapName)
{
	FString ShortName = FPackageName::GetShortName(MapName);
	if (ShortName.StartsWith(TEXT("UEDPIE_")))
	{
		const int32 InstanceSeparator = ShortName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
		if (InstanceSeparator == INDEX_NONE) return false;
		ShortName = ShortName.Mid(InstanceSeparator + 1);
	}
	return ShortName.Equals(TEXT("Island"), ESearchCase::IgnoreCase);
}

void UIslandInnkeeperSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsIslandMapName(InWorld.GetMapName())) return;
	SpawnInnkeeper(InWorld);
}

void UIslandInnkeeperSubsystem::InitializeInnkeeper(AAutonomousAgentCharacter* Agent, const FString& InAgentId)
{
	if (!IsValid(Agent)) return;
	if (Agent->Memory) Agent->Memory->AgentId = InAgentId;
	Agent->DisplayName = TEXT("Innkeeper");
	Agent->Tags.AddUnique(TEXT("IslandInnkeeper"));
}

FVector UIslandInnkeeperSubsystem::GetSpawnCandidate(const AActor* InnMarker, const AActor* HearthMarker)
{
	if (HearthMarker)
	{
		// The marker is in the firebox. Stand just inside the room, close enough to tend the hearth
		// immediately without spawning in its collision or making the resident walk through it.
		return HearthMarker->GetActorLocation() + HearthMarker->GetActorForwardVector() * 220.f;
	}
	return InnMarker ? InnMarker->GetActorLocation() : FVector::ZeroVector;
}

TArray<FVector> UIslandInnkeeperSubsystem::GetSpawnCandidates(const AActor* InnMarker, const AActor* HearthMarker)
{
	TArray<FVector> Candidates;
	if (HearthMarker)
	{
		const FVector Forward = HearthMarker->GetActorForwardVector();
		const FVector Right = HearthMarker->GetActorRightVector();
		const FVector Hearth = HearthMarker->GetActorLocation();
		// Start near the hearth, then fan toward the clear centre/door side of the room. The
		// runtime selector still checks capsule clearance and a complete path before spawning.
		Candidates.Add(Hearth + Forward * 220.f);
		Candidates.Add(Hearth + Forward * 360.f + Right * 220.f);
		Candidates.Add(Hearth + Forward * 360.f - Right * 220.f);
		Candidates.Add(Hearth + Forward * 500.f);
		Candidates.Add(Hearth + Forward * 700.f);
	}
	if (InnMarker) Candidates.Add(InnMarker->GetActorLocation());
	return Candidates;
}

void UIslandInnkeeperSubsystem::SpawnInnkeeper(UWorld& World)
{
	if (AgentId.IsEmpty() || BodyClass.IsNull()) return;

	for (TActorIterator<AAutonomousAgentCharacter> It(&World); It; ++It)
		if (It->Memory && It->Memory->GetResolvedAgentId() == AgentId) return;

	AActor* InnMarker = nullptr;
	AActor* HearthMarker = nullptr;
	AActor* DoorMarker = nullptr;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("Inn")))
		{
			InnMarker = *It;
		}
		if (It->ActorHasTag(TEXT("InnHearth"))) HearthMarker = *It;
		if (It->ActorHasTag(TEXT("InnDoorLantern"))) DoorMarker = *It;
	}
	if (!InnMarker)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island innkeeper was not spawned: the tagged walkable Inn landmark is missing."));
		return;
	}

	UClass* LoadedBodyClass = BodyClass.LoadSynchronous();
	if (!LoadedBodyClass || !LoadedBodyClass->IsChildOf(AAutonomousAgentCharacter::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("Island innkeeper was not spawned: configured body class is missing or is not an autonomous-agent character."));
		return;
	}

	const AAutonomousAgentCharacter* Defaults = Cast<AAutonomousAgentCharacter>(LoadedBodyClass->GetDefaultObject());
	const float HalfHeight = Defaults && Defaults->GetCapsuleComponent()
		? Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 96.f;
	const float Radius = Defaults && Defaults->GetCapsuleComponent()
		? Defaults->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 42.f;

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
	FNavLocation DoorLocation;
	const FVector DoorCandidate = DoorMarker ? DoorMarker->GetActorLocation() : InnMarker->GetActorLocation();
	if (!Navigation || !Navigation->ProjectPointToNavigation(DoorCandidate, DoorLocation, FVector(350.f, 350.f, 500.f)))
	{
		UE_LOG(LogTemp, Warning, TEXT("Island innkeeper was not spawned: no walkable destination was found at the Inn door or landmark."));
		return;
	}

	FCollisionQueryParams ClearanceQuery(SCENE_QUERY_STAT(InnkeeperSpawnClearance), false);
	ClearanceQuery.AddIgnoredActor(InnMarker);
	if (HearthMarker) ClearanceQuery.AddIgnoredActor(HearthMarker);
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FNavLocation NavLocation;
	bool bFoundClearRoute = false;
	for (const FVector& Candidate : GetSpawnCandidates(InnMarker, HearthMarker))
	{
		FNavLocation Projected;
		if (!Navigation->ProjectPointToNavigation(Candidate, Projected, FVector(250.f, 250.f, 250.f))) continue;
		const FVector CapsuleCentre = Projected.Location + FVector(0.f, 0.f, HalfHeight + 2.f);
		if (World.OverlapBlockingTestByChannel(CapsuleCentre, FQuat::Identity, ECC_Pawn, Capsule, ClearanceQuery)) continue;

		const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(&World, Projected.Location, DoorLocation.Location);
		if (!Path || !Path->IsValid() || Path->IsPartial()) continue;

		NavLocation = Projected;
		bFoundClearRoute = true;
		break;
	}
	if (!bFoundClearRoute)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island innkeeper was not spawned: no capsule-clear hearth/Inn position has a complete path to the door."));
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, NavLocation.Location + FVector(0.f, 0.f, HalfHeight + 2.f));
	AAutonomousAgentCharacter* Innkeeper = World.SpawnActorDeferred<AAutonomousAgentCharacter>(
		LoadedBodyClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Innkeeper) return;

	InitializeInnkeeper(Innkeeper, AgentId);
	Innkeeper->FinishSpawning(SpawnTransform);
	if (!Innkeeper->GetController()) Innkeeper->SpawnDefaultController();
	UE_LOG(LogTemp, Log, TEXT("Spawned Island innkeeper with stable identity %s at capsule-clear, door-reachable location %s."), *AgentId, *NavLocation.Location.ToCompactString());
}
