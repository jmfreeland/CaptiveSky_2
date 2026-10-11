#include "IslandRatSubsystem.h"

#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "IslandInnkeeperSubsystem.h"
#include "RatAgentAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

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

	const AActor* InnMarker = nullptr;
	for (TActorIterator<AActor> It(&World); It; ++It)
		if (It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("Inn"))) { InnMarker = *It; break; }
	if (!InnMarker)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island rat was not spawned: the tagged Inn landmark is missing."));
		return;
	}

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
	if (!Navigation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island rat was not spawned: no navigation system is available."));
		return;
	}

	// Ring-search outside the Inn for the nearest navigable point.
	bool bFound = false;
	FNavLocation NavLocation;
	const FVector Centre = InnMarker->GetActorLocation();
	for (const float Radius : { 700.f, 900.f, 1200.f })
	{
		for (int32 Step = 0; Step < 12 && !bFound; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(30.f * Step);
			const FVector Candidate = Centre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
			bFound = Navigation->ProjectPointToNavigation(Candidate, NavLocation, FVector(150.f, 150.f, 400.f));
		}
		if (bFound) break;
	}
	if (!bFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island rat was not spawned: no navigable point near the Inn."));
		return;
	}

	const AAutonomousAgentCharacter* Defaults = Cast<AAutonomousAgentCharacter>(BodyClass->GetDefaultObject());
	const float HalfHeight = Defaults && Defaults->GetCapsuleComponent() ? Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 14.f;
	const FTransform SpawnTransform(FRotator::ZeroRotator, NavLocation.Location + FVector(0.f, 0.f, HalfHeight + 2.f));
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
	UE_LOG(LogTemp, Log, TEXT("Spawned Island rat with stable identity %s at %s (controller %s, mesh %s)."), *AgentId, *NavLocation.Location.ToCompactString(),
		*GetNameSafe(Rat->GetController()), Rat->GetMesh() ? *GetNameSafe(Rat->GetMesh()->GetSkeletalMeshAsset()) : TEXT("none"));
}
