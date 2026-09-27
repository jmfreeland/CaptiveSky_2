// Map audit for the saved Island: finds props near the landmark route that float above whatever
// is beneath them. Informational by default (warnings, never a failure). With -GroundingFix, props
// hovering a little above ground are lowered onto it and the map is saved after a backup copy is
// written to Saved/MapBackups/. Mid-air objects are only reported; they need a human decision.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

namespace
{
	/** Height of the actor's lowest point above the first solid thing beneath its footprint, or unset if nothing is below. */
	TOptional<float> GapBeneath(UWorld* World, AActor* Actor, const FBox& Bounds)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandGroundingAudit), true, Actor);
		const FVector Centre = Bounds.GetCenter();
		const FVector Extent = Bounds.GetExtent() * 0.6f;
		// Centre plus four inner corners: resting on any of them counts as supported.
		const FVector2D Probes[] = { {0, 0}, {Extent.X, Extent.Y}, {-Extent.X, Extent.Y}, {Extent.X, -Extent.Y}, {-Extent.X, -Extent.Y} };
		TOptional<float> Smallest;
		for (const FVector2D& Probe : Probes)
		{
			// Start above the prop (which is ignored) so partly buried props still find the surface they sit in.
			const FVector Start(Centre.X + Probe.X, Centre.Y + Probe.Y, Bounds.Max.Z + 50.f);
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 20000.f), ECC_Visibility, Query)) continue;
			const float Gap = Bounds.Min.Z - Hit.ImpactPoint.Z;
			if (!Smallest.IsSet() || Gap < Smallest.GetValue()) Smallest = Gap;
		}
		return Smallest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandGroundingAuditTest, "CaptiveSky2.Visual.Grounding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGroundingAuditTest::RunTest(const FString& Parameters)
{
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() && Context.World()->GetMapName() == TEXT("Island")) Island = Context.World();
	if (!TestNotNull(TEXT("Island is the open editor map"), Island)) return false;

	FVector Centre = FVector::ZeroVector;
	int32 Landmarks = 0;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("IslandLandmark")) || It->ActorHasTag(TEXT("RavenPerch"))) { Centre += It->GetActorLocation(); ++Landmarks; }
	if (!TestTrue(TEXT("The Island has landmarks to audit around"), Landmarks > 0)) return false;
	Centre /= Landmarks;

	constexpr float AuditRadius = 6000.f;   // the landmark route and its surroundings
	constexpr float HoverTolerance = 3.f;   // resting contact, allowing for mesh bounds padding
	constexpr float MidAir = 150.f;         // clearly not resting on anything
	const bool bFix = FParse::Param(FCommandLine::Get(), TEXT("GroundingFix"));
	TArray<TPair<AActor*, float>> ToLower;
	int32 Checked = 0;
	for (TActorIterator<AActor> It(Island); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->HasAnyFlags(RF_Transient) || Actor->IsEditorOnly() || FVector::Dist2D(Actor->GetActorLocation(), Centre) > AuditRadius) continue;
		// Only visible, reasonably sized geometry and bodies; skies, volumes and huge terrain pieces are skipped.
		TArray<UPrimitiveComponent*> Primitives;
		Actor->GetComponents(Primitives);
		const bool bVisibleBody = Primitives.ContainsByPredicate([](const UPrimitiveComponent* Component)
			{ return (Component->IsA<UStaticMeshComponent>() || Component->IsA<USkeletalMeshComponent>()) && Component->IsVisible() && !Component->bHiddenInGame; });
		if (!bVisibleBody) continue;
		const FBox Bounds = Actor->GetComponentsBoundingBox(true);
		if (!Bounds.IsValid || Bounds.GetSize().GetMax() > 5000.f) continue;
		++Checked;
		// Resting on (or sunk into) something counts as grounded; only a positive gap is a problem.
		const TOptional<float> Gap = GapBeneath(Island, Actor, Bounds);
		if (!Gap.IsSet() || Gap.GetValue() <= HoverTolerance) continue;
		const FString Label = Actor->GetActorLabel();
		if (Gap.GetValue() >= MidAir)
		{
			AddWarning(FString::Printf(TEXT("Mid-air: %s (%s) at %s is %.0f cm above anything beneath it; needs a human decision."), *Label, *Actor->GetClass()->GetName(), *Actor->GetActorLocation().ToString(), Gap.GetValue()));
			continue;
		}
		AddWarning(FString::Printf(TEXT("Hovering: %s (%s) at %s floats %.1f cm above its support."), *Label, *Actor->GetClass()->GetName(), *Actor->GetActorLocation().ToString(), Gap.GetValue()));
		ToLower.Add({Actor, Gap.GetValue()});
	}
	AddInfo(FString::Printf(TEXT("Audited %d visible props within %.0f m of the landmarks; %d hover slightly."), Checked, AuditRadius / 100.f, ToLower.Num()));
	if (!bFix || ToLower.Num() == 0) return true;

	// Back up the saved map before changing it; Content/ is not in git.
	const FString MapFile = FPackageName::LongPackageNameToFilename(Island->GetOutermost()->GetName(), FPackageName::GetMapPackageExtension());
	const FString Backup = FPaths::ProjectSavedDir() / TEXT("MapBackups") / FString::Printf(TEXT("Island_%s.umap"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	if (!TestTrue(TEXT("Map backed up before fixing"), IFileManager::Get().Copy(*Backup, *MapFile) == COPY_OK)) return false;
	AddInfo(FString::Printf(TEXT("Backed up %s to %s"), *MapFile, *Backup));
	for (const TPair<AActor*, float>& Entry : ToLower)
	{
		Entry.Key->Modify();
		Entry.Key->SetActorLocation(Entry.Key->GetActorLocation() - FVector(0, 0, Entry.Value));
		AddInfo(FString::Printf(TEXT("Lowered %s by %.1f cm onto its support."), *Entry.Key->GetActorLabel(), Entry.Value));
	}
	TestTrue(TEXT("Grounded map saved"), FEditorFileUtils::SaveLevel(Island->PersistentLevel));
	return true;
}

#endif
