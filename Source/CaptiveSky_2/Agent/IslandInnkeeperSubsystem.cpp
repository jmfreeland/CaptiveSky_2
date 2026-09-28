#include "IslandInnkeeperSubsystem.h"

#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

void UIslandInnkeeperSubsystem::SpawnInnkeeper(UWorld& World)
{
	if (AgentId.IsEmpty() || BodyClass.IsNull()) return;

	for (TActorIterator<AAutonomousAgentCharacter> It(&World); It; ++It)
		if (It->Memory && It->Memory->GetResolvedAgentId() == AgentId) return;

	AActor* InnMarker = nullptr;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("Inn")))
		{
			InnMarker = *It;
			break;
		}
	}
	if (!InnMarker)
	{
		UE_LOG(LogTemp, Warning, TEXT("Island innkeeper was not spawned: the tagged walkable Inn landmark is missing."));
		return;
	}

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
	FNavLocation NavLocation;
	if (!Navigation || !Navigation->ProjectPointToNavigation(InnMarker->GetActorLocation(), NavLocation, FVector(250.f, 250.f, 250.f)))
	{
		UE_LOG(LogTemp, Warning, TEXT("Island innkeeper was not spawned: no walkable navigation point was found at the Inn landmark."));
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
	const FTransform SpawnTransform(FRotator::ZeroRotator, NavLocation.Location + FVector(0.f, 0.f, HalfHeight + 2.f));
	AAutonomousAgentCharacter* Innkeeper = World.SpawnActorDeferred<AAutonomousAgentCharacter>(
		LoadedBodyClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Innkeeper) return;

	InitializeInnkeeper(Innkeeper, AgentId);
	Innkeeper->FinishSpawning(SpawnTransform);
	if (!Innkeeper->GetController()) Innkeeper->SpawnDefaultController();
	UE_LOG(LogTemp, Log, TEXT("Spawned Island innkeeper with stable identity %s at the walkable Inn landmark."), *AgentId);
}
