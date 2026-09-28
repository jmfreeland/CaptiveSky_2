#include "Misc/AutomationTest.h"
#include "IslandInnkeeperSubsystem.h"
#include "AutonomousAgentCharacter.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "IslandInteractionUtility.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandInnkeeperSpawnTest, "CaptiveSky2.Agent.IslandInnkeeperSpawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandInnkeeperSpawnTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The Island package name is accepted"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("/Game/Maps/Island")));
	TestTrue(TEXT("A PIE-prefixed Island map name is accepted"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("UEDPIE_2_Island")));
	TestFalse(TEXT("A different map does not receive the innkeeper"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("/Game/Maps/Other")));
	TestFalse(TEXT("A similarly named map is not mistaken for Island"), UIslandInnkeeperSubsystem::IsIslandMapName(TEXT("Island_Test")));

	const UIslandInnkeeperSubsystem* Defaults = GetDefault<UIslandInnkeeperSubsystem>();
	if (!TestNotNull(TEXT("Innkeeper subsystem defaults are available"), Defaults)) return false;
	TestEqual(TEXT("The innkeeper has an independent stable identity"), Defaults->AgentId, FString(TEXT("Agent_Innkeeper_01")));
	UClass* BodyClass = Defaults->BodyClass.LoadSynchronous();
	if (!TestNotNull(TEXT("The configured shared placeholder body is loadable"), BodyClass)) return false;
	TestTrue(TEXT("The configured body uses the shared autonomous-agent architecture"), BodyClass->IsChildOf(AAutonomousAgentCharacter::StaticClass()));
	const AAutonomousAgentCharacter* ConfiguredBodyDefaults = Cast<AAutonomousAgentCharacter>(BodyClass->GetDefaultObject());
	if (!TestNotNull(TEXT("The configured body exposes the actual capsule dimensions"), ConfiguredBodyDefaults)) return false;
	const float ConfiguredHalfHeight = ConfiguredBodyDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Isolated innkeeper fixture world created"), World)) return false;
	AActor* InnMarker = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator);
	AActor* HearthMarker = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator(0.f, 30.f, 0.f));
	if (!TestNotNull(TEXT("Inn landmark fixture created"), InnMarker) || !TestNotNull(TEXT("Hearth landmark fixture created"), HearthMarker))
	{
		World->DestroyWorld(false);
		return false;
	}
	const FVector HearthSideStart = UIslandInnkeeperSubsystem::GetSpawnCandidate(InnMarker, HearthMarker);
	TestTrue(TEXT("The preferred start is within interaction range of the hearth"), FVector::Dist(HearthSideStart, HearthMarker->GetActorLocation()) < 400.f);
	TestEqual(TEXT("A missing hearth falls back to the Inn landmark"), UIslandInnkeeperSubsystem::GetSpawnCandidate(InnMarker, nullptr), InnMarker->GetActorLocation());
	const TArray<FVector> SpawnCandidates = UIslandInnkeeperSubsystem::GetSpawnCandidates(InnMarker, HearthMarker);
	TestEqual(TEXT("Hearth-side placement searches nearby clear alternatives before the landmark"), SpawnCandidates.Num(), 8);
	if (SpawnCandidates.Num() == 8)
	{
		TestEqual(TEXT("The closest hearth-side candidate remains first"), SpawnCandidates[0], HearthSideStart);
		TestTrue(TEXT("Nearby alternatives fan around furniture while retaining hearth interaction range"),
			FVector::Dist(SpawnCandidates[1] + FVector(0.f, 0.f, ConfiguredHalfHeight + 2.f), HearthMarker->GetActorLocation()) <= IslandInteractionUtility::DefaultInteractionRange &&
			FVector::Dist(SpawnCandidates[2] + FVector(0.f, 0.f, ConfiguredHalfHeight + 2.f), HearthMarker->GetActorLocation()) <= IslandInteractionUtility::DefaultInteractionRange &&
			!SpawnCandidates[1].Equals(SpawnCandidates[2]) && !SpawnCandidates[3].Equals(SpawnCandidates[4]));
		TestEqual(TEXT("The tagged Inn landmark remains the final placement fallback"), SpawnCandidates.Last(), InnMarker->GetActorLocation());
	}
	const TArray<FVector> LandmarkOnlyCandidates = UIslandInnkeeperSubsystem::GetSpawnCandidates(InnMarker, nullptr);
	TestEqual(TEXT("Without a hearth only the tagged Inn landmark is considered"), LandmarkOnlyCandidates.Num(), 1);
	if (LandmarkOnlyCandidates.Num() == 1)
		TestEqual(TEXT("Without a hearth the candidate is the tagged Inn landmark"), LandmarkOnlyCandidates[0], InnMarker->GetActorLocation());
	World->DestroyActor(InnMarker);
	World->DestroyActor(HearthMarker);

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AAutonomousAgentCharacter* Resident = World->SpawnActor<AAutonomousAgentCharacter>(BodyClass, FTransform::Identity, Spawn);
	if (!TestNotNull(TEXT("Placeholder body spawns without starting a world or making a model request"), Resident))
	{
		World->DestroyWorld(false);
		return false;
	}

	UIslandInnkeeperSubsystem::InitializeInnkeeper(Resident, Defaults->AgentId);
	if (!TestNotNull(TEXT("The resident has its own memory component"), Resident->Memory.Get()) ||
		!TestNotNull(TEXT("The resident has its own autonomous brain component"), Resident->Brain.Get()))
	{
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("The independent memory directory identity is assigned before BeginPlay"), Resident->Memory->GetResolvedAgentId(), FString(TEXT("Agent_Innkeeper_01")));
	Resident->RegisterApproachTargetTags();
	TestTrue(TEXT("The stable movement tag retains the full Agent_ identity"), Resident->ActorHasTag(TEXT("ApproachAgent_Agent_Innkeeper_01")));
	TestTrue(TEXT("The shortened movement tag resolves the model's common Agent_ omission"), Resident->ActorHasTag(TEXT("ApproachAgent_Innkeeper_01")));
	Resident->RegisterApproachTargetTags();
	TestEqual(TEXT("Registering aliases is idempotent"), Resident->Tags.FilterByPredicate([](const FName& Tag) { return Tag == FName(TEXT("ApproachAgent_Innkeeper_01")); }).Num(), 1);
	TestEqual(TEXT("The resident uses its friendly display name"), Resident->GetAgentDisplayName(), FString(TEXT("Innkeeper")));
	Resident->DisplayName.Reset();
	TestEqual(TEXT("Residents without an authored display name retain their actor label"), Resident->GetAgentDisplayName(), Resident->GetActorNameOrLabel());
	Resident->DisplayName = TEXT("Innkeeper");
	TestTrue(TEXT("The resident is clearly identifiable by role"), Resident->ActorHasTag(TEXT("IslandInnkeeper")));
	TestTrue(TEXT("The memory file is isolated in the resident's own agent folder"), Resident->Memory->GetMemoryFilePath().Contains(TEXT("Agent_Innkeeper_01")));
	TestTrue(TEXT("The independent identity document loads from its own agent folder"), Resident->Memory->LoadAgentDocument(TEXT("identity.md")).Contains(TEXT("made a home in the inn")));
	TestTrue(TEXT("The independent personality document loads from its own agent folder"), Resident->Memory->LoadAgentDocument(TEXT("personality.md")).Contains(TEXT("welcoming without insisting on company")));
	TestFalse(TEXT("Fixture tests never dispatch an LLM request"), Resident->Brain->bRequestInFlight);

	World->DestroyWorld(false);
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Island = Context.World();
		if (Context.WorldType != EWorldType::Editor || !Island || Island->GetMapName() != TEXT("Island")) continue;

		AActor* IslandInn = nullptr;
		AActor* IslandHearth = nullptr;
		AActor* IslandDoor = nullptr;
		for (TActorIterator<AActor> It(Island); It; ++It)
		{
			if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("Inn"))) IslandInn = *It;
			if (It->ActorHasTag(TEXT("InnHearth"))) IslandHearth = *It;
			if (It->ActorHasTag(TEXT("InnDoorLantern"))) IslandDoor = *It;
		}
		if (!TestNotNull(TEXT("The saved Island has a tagged inn landmark"), IslandInn) ||
			!TestNotNull(TEXT("The saved Island has a tagged hearth"), IslandHearth) ||
			!TestNotNull(TEXT("The saved Island has a tagged doorway"), IslandDoor)) continue;

		const AAutonomousAgentCharacter* BodyDefaults = Cast<AAutonomousAgentCharacter>(BodyClass->GetDefaultObject());
		if (!TestNotNull(TEXT("The configured body has capsule dimensions for the map audit"), BodyDefaults)) continue;
		const float Radius = BodyDefaults->GetCapsuleComponent()->GetScaledCapsuleRadius();
		const float HalfHeight = BodyDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FNavLocation SpawnLocation;
		int32 CandidateIndex = INDEX_NONE;
		int32 SelectedExtraClearanceCm = INDEX_NONE;
		const TArray<FVector> IslandCandidates = UIslandInnkeeperSubsystem::GetSpawnCandidates(IslandInn, IslandHearth);
		const bool bFoundRealMapStart = UIslandInnkeeperSubsystem::FindSpawnLocation(*Island, IslandCandidates,
			IslandDoor->GetActorLocation(), Radius, HalfHeight, IslandInn, IslandHearth, SpawnLocation, CandidateIndex, SelectedExtraClearanceCm);
		TestTrue(TEXT("The production selector finds a capsule-clear, complete-path start in the saved Inn"), bFoundRealMapStart);
		if (bFoundRealMapStart)
		{
			TestTrue(TEXT("The selected start stays in the room-side candidates, not the distant Inn-marker fallback"),
				CandidateIndex >= 0 && CandidateIndex < IslandCandidates.Num() - 1);
			TestTrue(TEXT("The selected start remains within five metres of the hearth"),
				FVector::Dist2D(SpawnLocation.Location, IslandHearth->GetActorLocation()) <= 500.f);
			TestTrue(TEXT("The selected resident body remains within the real hearth interaction range"),
				FVector::Dist(SpawnLocation.Location + FVector(0.f, 0.f, HalfHeight + 2.f), IslandHearth->GetActorLocation()) <=
				IslandInteractionUtility::DefaultInteractionRange);
			TestTrue(TEXT("The selected hearth-side start has the strongest available capsule clearance"), SelectedExtraClearanceCm >= 50);
			AddInfo(FString::Printf(TEXT("Saved Island innkeeper spawn selector chose candidate %d/%d at %s, %d cm extra capsule clearance, %.0f cm from the hearth."),
				CandidateIndex, IslandCandidates.Num() - 1, *SpawnLocation.Location.ToCompactString(), SelectedExtraClearanceCm,
				FVector::Dist2D(SpawnLocation.Location, IslandHearth->GetActorLocation())));

			UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Island);
			FNavLocation DoorGoal;
			const bool bHasDoorGoal = Navigation && Navigation->ProjectPointToNavigation(IslandDoor->GetActorLocation(),
				DoorGoal, FVector(350.f, 350.f, 500.f));
			TestTrue(TEXT("The production door marker projects to navigation for candidate diagnostics"), bHasDoorGoal);
			FCollisionQueryParams ClearanceQuery(SCENE_QUERY_STAT(InnkeeperSpawnAudit), false);
			ClearanceQuery.AddIgnoredActor(IslandInn);
			ClearanceQuery.AddIgnoredActor(IslandHearth);
			if (bHasDoorGoal)
			{
				for (int32 Index = 0; Index < IslandCandidates.Num(); ++Index)
				{
					const TArray<FVector> SingleCandidate = { IslandCandidates[Index] };
					FNavLocation CandidateLocation;
					int32 SingleIndex = INDEX_NONE;
					int32 SingleExtraClearanceCm = INDEX_NONE;
					const bool bCandidateValid = UIslandInnkeeperSubsystem::FindSpawnLocation(*Island, SingleCandidate,
						IslandDoor->GetActorLocation(), Radius, HalfHeight, IslandInn, IslandHearth, CandidateLocation, SingleIndex,
						SingleExtraClearanceCm);
					if (!bCandidateValid)
					{
						AddInfo(FString::Printf(TEXT("Island innkeeper candidate %d: blocked, unprojectable, or no complete door route."), Index));
						continue;
					}
					const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(Island, CandidateLocation.Location, DoorGoal.Location);
					const float BodyToHearthDistance = FVector::Dist(CandidateLocation.Location + FVector(0.f, 0.f, HalfHeight + 2.f),
						IslandHearth->GetActorLocation());
					AddInfo(FString::Printf(TEXT("Island innkeeper candidate %d: complete %.0f cm door path, up to %d cm extra capsule clearance, %.0f cm body-to-hearth (%s interaction)."),
						Index, Path ? Path->GetPathLength() : 0.f, SingleExtraClearanceCm, BodyToHearthDistance,
						BodyToHearthDistance <= IslandInteractionUtility::DefaultInteractionRange ? TEXT("in") : TEXT("out of")));
				}
			}
		}
	}
	return true;
}
