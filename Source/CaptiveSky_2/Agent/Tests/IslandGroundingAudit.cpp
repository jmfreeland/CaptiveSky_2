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
		// Centre, four inner corners, and both ends of the long axis (so a beam finds the pillars under
		// its ends): resting on any of them counts as supported.
		const FVector Full = Bounds.GetExtent() * 0.95f;
		const FVector2D LongEnd = Full.X >= Full.Y ? FVector2D(Full.X, 0) : FVector2D(0, Full.Y);
		const FVector2D Probes[] = { {0, 0}, {Extent.X, Extent.Y}, {-Extent.X, Extent.Y}, {Extent.X, -Extent.Y}, {-Extent.X, -Extent.Y}, LongEnd, -LongEnd };
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
	TArray<TPair<AActor*, FBox>> Audited;
	int32 Checked = 0;
	// First gather every visible prop, so a prop resting on another (a beam on pillars) can find it.
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
		Audited.Add({Actor, Bounds});
	}
	for (const TPair<AActor*, FBox>& Entry : Audited)
	{
		AActor* Actor = Entry.Key;
		const FBox& Bounds = Entry.Value;
		++Checked;
		// Resting on (or sunk into) something counts as grounded; only a positive gap is a problem.
		TOptional<float> Gap = GapBeneath(Island, Actor, Bounds);
		// Traces can slip past narrow supports; also measure to the highest prop wholly beneath this one's outline.
		for (const TPair<AActor*, FBox>& Other : Audited)
		{
			const FBox& Below = Other.Value;
			if (Other.Key == Actor || Below.Max.Z > Bounds.Min.Z + HoverTolerance || Below.Max.X < Bounds.Min.X || Below.Min.X > Bounds.Max.X ||
				Below.Max.Y < Bounds.Min.Y || Below.Min.Y > Bounds.Max.Y) continue;
			const float ToTop = Bounds.Min.Z - Below.Max.Z;
			if (!Gap.IsSet() || ToTop < Gap.GetValue()) Gap = ToTop;
		}
		if (!Gap.IsSet() || Gap.GetValue() <= HoverTolerance) continue;
		const FString Label = Actor->GetActorLabel();
		if (Gap.GetValue() >= MidAir)
		{
			AddWarning(FString::Printf(TEXT("Mid-air: %s (%s) at %s is %.0f cm above anything beneath it; needs a human decision. Bounds %s to %s."), *Label, *Actor->GetClass()->GetName(), *Actor->GetActorLocation().ToString(), Gap.GetValue(), *Bounds.Min.ToString(), *Bounds.Max.ToString()));
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

namespace
{
	AActor* FindByLabel(UWorld* World, const TCHAR* Label)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
			if (It->GetActorLabel() == Label) return *It;
		return nullptr;
	}

	/** Moves Actor so its component bounds' minimum corner (on the given axes) lands at Target. */
	void AlignBoundsMin(AActor* Actor, float TargetMinZ, TOptional<float> TargetCentreX = {})
	{
		const FBox Bounds = Actor->GetComponentsBoundingBox(true);
		FVector Offset(0.f, 0.f, TargetMinZ - Bounds.Min.Z);
		if (TargetCentreX.IsSet()) Offset.X = TargetCentreX.GetValue() - Bounds.GetCenter().X;
		Actor->SetActorLocation(Actor->GetActorLocation() + Offset);
	}
}

// One-off repair for the placeholder WindArch: stretch the grounded pillars upward to one level top that
// clears the raven's hover point at the landmark, and make the beam span both pillars and rest on them.
// Backs up the map to Saved/MapBackups/ before saving. Safe to rerun: an already-sound arch is left alone.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairWindArchTool, "CaptiveSky2.Tools.RepairWindArch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRepairWindArchTool::RunTest(const FString& Parameters)
{
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() && Context.World()->GetMapName() == TEXT("Island")) Island = Context.World();
	if (!TestNotNull(TEXT("Island is the open editor map"), Island)) return false;
	AActor* PillarA = FindByLabel(Island, TEXT("WindArch_Pillar_A"));
	AActor* PillarB = FindByLabel(Island, TEXT("WindArch_Pillar_B"));
	AActor* Beam = FindByLabel(Island, TEXT("WindArch_Beam"));
	if (!TestTrue(TEXT("WindArch pillars and beam exist"), PillarA && PillarB && Beam)) return false;

	const FBox A = PillarA->GetComponentsBoundingBox(true);
	const FBox B = PillarB->GetComponentsBoundingBox(true);
	FBox Span = Beam->GetComponentsBoundingBox(true);
	// The raven flies to 1.8 m above the WindArch marker; the beam must clear that with room to spare.
	float Clearance = 0.f;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("WindArch")) && It->ActorHasTag(TEXT("IslandLandmark"))) Clearance = static_cast<float>(It->GetActorLocation().Z) + 350.f;
	const float Top = FMath::Max3(static_cast<float>(A.Max.Z), static_cast<float>(B.Max.Z), Clearance);
	const float Left = FMath::Min(A.Min.X, B.Min.X);
	const float Right = FMath::Max(A.Max.X, B.Max.X);
	const bool bLevel = FMath::IsNearlyEqual(A.Max.Z, Top, 2.f) && FMath::IsNearlyEqual(B.Max.Z, Top, 2.f);
	const bool bSpans = Span.Min.X <= Left + 2.f && Span.Max.X >= Right - 2.f;
	const bool bResting = FMath::IsNearlyEqual(Span.Min.Z, Top, 2.f);
	AddInfo(FString::Printf(TEXT("Pillar tops %.1f / %.1f; beam spans X %.0f..%.0f (pillars %.0f..%.0f), beam bottom %.1f."), A.Max.Z, B.Max.Z, Span.Min.X, Span.Max.X, Left, Right, Span.Min.Z));
	if (bLevel && bSpans && bResting) { AddInfo(TEXT("The WindArch is already sound; nothing changed.")); return true; }

	const FString MapFile = FPackageName::LongPackageNameToFilename(Island->GetOutermost()->GetName(), FPackageName::GetMapPackageExtension());
	const FString Backup = FPaths::ProjectSavedDir() / TEXT("MapBackups") / FString::Printf(TEXT("Island_%s.umap"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	if (!TestTrue(TEXT("Map backed up before repair"), IFileManager::Get().Copy(*Backup, *MapFile) == COPY_OK)) return false;

	// Stretch the shorter pillar upward from its base until both tops are level.
	for (AActor* Pillar : {PillarA, PillarB})
	{
		const FBox Bounds = Pillar->GetComponentsBoundingBox(true);
		if (Bounds.Max.Z >= Top - 0.5f) continue;
		Pillar->Modify();
		FVector Scale = Pillar->GetActorScale3D();
		Scale.Z *= (Top - Bounds.Min.Z) / FMath::Max(1.f, Bounds.Max.Z - Bounds.Min.Z);
		Pillar->SetActorScale3D(Scale);
		AlignBoundsMin(Pillar, Bounds.Min.Z);
		AddInfo(FString::Printf(TEXT("Stretched %s to %.1f cm tall, base unchanged."), *Pillar->GetActorLabel(), Top - Bounds.Min.Z));
	}
	// Lengthen the beam along X to cover both pillars, centre it over them, and rest it on their tops.
	Beam->Modify();
	FVector BeamScale = Beam->GetActorScale3D();
	BeamScale.X *= (Right - Left) / FMath::Max(1.f, Span.Max.X - Span.Min.X);
	Beam->SetActorScale3D(BeamScale);
	AlignBoundsMin(Beam, Top, (Left + Right) * 0.5f);
	Span = Beam->GetComponentsBoundingBox(true);
	AddInfo(FString::Printf(TEXT("Beam now spans X %.0f..%.0f and rests at %.1f. Backup: %s"), Span.Min.X, Span.Max.X, Span.Min.Z, *Backup));
	return TestTrue(TEXT("Repaired map saved"), FEditorFileUtils::SaveLevel(Island->PersistentLevel));
}

#endif
