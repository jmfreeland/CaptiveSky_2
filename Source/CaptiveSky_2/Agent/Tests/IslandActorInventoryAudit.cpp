// Read-only inventory of rendered character meshes in the saved Island. This helps tie
// a viewpoint capture artifact back to its authored actor without moving or saving anything.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandActorInventoryAuditTest, "CaptiveSky2.Visual.ActorInventory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandActorInventoryAuditTest::RunTest(const FString& Parameters)
{
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Candidate = Context.World();
		if (Context.WorldType == EWorldType::Editor && Candidate && Candidate->GetMapName() == TEXT("Island"))
		{
			Island = Candidate;
			break;
		}
	}
	if (!TestNotNull(TEXT("The saved Island is open in the editor world"), Island)) return false;

	int32 ActorCount = 0;
	for (TActorIterator<AActor> It(Island); It; ++It)
	{
		AActor* Actor = *It;
		TArray<USkeletalMeshComponent*> Meshes;
		Actor->GetComponents<USkeletalMeshComponent>(Meshes);
		if (Meshes.IsEmpty() && !Actor->IsA<ACharacter>()) continue;

		FString Label = Actor->GetName();
#if WITH_EDITOR
		Label = Actor->GetActorLabel();
#endif
		FString Tags;
		for (const FName Tag : Actor->Tags)
		{
			if (!Tags.IsEmpty()) Tags += TEXT(",");
			Tags += Tag.ToString();
		}
		if (Tags.IsEmpty()) Tags = TEXT("none");

		AddInfo(FString::Printf(TEXT("Actor '%s' [%s], character=%s, actor=%s, tags=%s."),
			*Label, *Actor->GetPathName(), Actor->IsA<ACharacter>() ? TEXT("yes") : TEXT("no"),
			*Actor->GetActorLocation().ToString(), *Tags));
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandActorInventoryGround), true, Actor);
		FHitResult GroundHit;
		const FVector TraceStart = Actor->GetActorLocation() + FVector(0.f, 0.f, 25.f);
		const bool bGroundHit = Island->LineTraceSingleByChannel(GroundHit, TraceStart,
			TraceStart - FVector(0.f, 0.f, 20000.f), ECC_Visibility, Query);
		const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
		FString GroundLabel = bGroundHit && GroundHit.GetActor() ? GroundHit.GetActor()->GetName() : TEXT("none");
#if WITH_EDITOR
		if (bGroundHit && GroundHit.GetActor()) GroundLabel = GroundHit.GetActor()->GetActorLabel();
#endif
		AddInfo(bGroundHit
			? FString::Printf(TEXT("  Downward visibility trace hits '%s' at %s; lowest component bound is %.1f cm above the hit."),
				*GroundLabel, *GroundHit.ImpactPoint.ToString(), ActorBounds.Min.Z - GroundHit.ImpactPoint.Z)
			: TEXT("  Downward visibility trace found no surface within 200 m; this does not prove the actor is unsupported."));
		for (const USkeletalMeshComponent* Mesh : Meshes)
		{
			const FString AssetPath = Mesh->GetSkeletalMeshAsset()
				? Mesh->GetSkeletalMeshAsset()->GetPathName() : TEXT("none");
			AddInfo(FString::Printf(TEXT("  Component '%s', mesh=%s, visible=%s, hiddenInGame=%s, location=%s, bounds=%s."),
				*Mesh->GetName(), *AssetPath, Mesh->IsVisible() ? TEXT("yes") : TEXT("no"),
				Mesh->bHiddenInGame ? TEXT("yes") : TEXT("no"), *Mesh->GetComponentLocation().ToString(),
				*Mesh->Bounds.GetBox().ToString()));
		}
		++ActorCount;
	}
	AddInfo(FString::Printf(TEXT("Inventory complete: %d character or skeletal-mesh actors; no actors were modified."), ActorCount));
	return true;
}

#endif
