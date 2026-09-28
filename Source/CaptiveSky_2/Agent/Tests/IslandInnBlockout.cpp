// Inn blockout for the saved Island (docs/plans/inn.md, phase 1). An explicit developer tool, not a
// regular test: it picks level walkable ground on the route between the shore and the TideglassPool,
// backs up the map, and assembles a timber-and-plaster inn from engine cubes (common room
// with hearth and counter, stair to an upper room, gable roof, lit doorway). Every actor is labelled
// Inn_* and tagged IslandInn so code finds parts by tag. It then rebuilds navigation and saves.
// Rerunning does nothing while the inn exists; -InnRebuild replaces it (the map is backed up first).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EditorBuildUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "NavigationPath.h"
#include "Navigation/NavLinkProxy.h"
#include "NavigationSystem.h"
#include "UObject/SavePackage.h"

namespace
{
	/** A plain colour material saved under /Game/Inn/Materials, created on first use. */
	UMaterialInterface* InnColour(const TCHAR* Name, const FLinearColor& Colour)
	{
		const FString PackageName = FString::Printf(TEXT("/Game/Inn/Materials/MI_Inn_%s"), Name);
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
		if (UMaterialInstanceConstant* Existing = LoadObject<UMaterialInstanceConstant>(nullptr, *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet))
			return Existing;
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UPackage* Package = CreatePackage(*PackageName);
		UMaterialInstanceConstant* Instance = NewObject<UMaterialInstanceConstant>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		Instance->SetParentEditorOnly(Parent);
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Color")), Colour);
		Instance->PostEditChange();
		FAssetRegistryModule::AssetCreated(Instance);
		Package->MarkPackageDirty();
		FSavePackageArgs Save;
		Save.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, Instance, *FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()), Save);
		return Instance;
	}

	struct FInnSite
	{
		FVector Centre = FVector::ZeroVector; // ground-level centre of the footprint
		float Yaw = 0.f;                      // +X of the inn (its door) faces this way
		float LowestGround = 0.f;
		float HighestGround = 0.f;
		float GroundAtDoor = 0.f;
	};

	/** Samples the rotated footprint; fails on steep, blocked, or unwalkable ground. */
	bool SurveySite(UWorld* World, const FVector& Centre, float Yaw, FInnSite& Out)
	{
		const FRotator Facing(0.f, Yaw, 0.f);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(InnSite), true);
		float Low = TNumericLimits<float>::Max(), High = -TNumericLimits<float>::Max();
		for (int32 Ix = -2; Ix <= 2; ++Ix)
			for (int32 Iy = -2; Iy <= 2; ++Iy)
			{
				const FVector Probe = Centre + Facing.RotateVector(FVector(Ix * 280.f, Iy * 230.f, 0.f));
				FHitResult Hit;
				if (!World->LineTraceSingleByChannel(Hit, Probe + FVector(0, 0, 4000), Probe - FVector(0, 0, 6000), ECC_Visibility, Query)) return false;
				if (Hit.ImpactNormal.Z < 0.85f || Cast<APawn>(Hit.GetActor()) || (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("IslandInn")))) return false;
				Low = FMath::Min(Low, static_cast<float>(Hit.ImpactPoint.Z));
				High = FMath::Max(High, static_cast<float>(Hit.ImpactPoint.Z));
			}
		if (High - Low > 160.f) return false;
		// Nothing else built within a few metres.
		for (TActorIterator<AActor> It(World); It; ++It)
			if ((It->ActorHasTag(TEXT("IslandLandmark")) || It->ActorHasTag(TEXT("RavenPerch"))) && !It->ActorHasTag(TEXT("IslandInn")) && FVector::Dist2D(It->GetActorLocation(), Centre) < 1500.f) return false;
		FHitResult DoorHit;
		const FVector Door = Centre + Facing.RotateVector(FVector(650.f, 0.f, 0.f));
		if (!World->LineTraceSingleByChannel(DoorHit, Door + FVector(0, 0, 4000), Door - FVector(0, 0, 6000), ECC_Visibility, Query)) return false;
		if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation Walkable;
			if (!Navigation->ProjectPointToNavigation(DoorHit.ImpactPoint, Walkable, FVector(100, 100, 200))) return false;
		}
		Out = { FVector(Centre.X, Centre.Y, Low), Yaw, Low, High, static_cast<float>(DoorHit.ImpactPoint.Z) };
		return true;
	}


	/** Rebuilds navigation and reports whether a walker gets from the ListeningStones to the door and into the common room. */
	bool CheckInnAccess(UWorld* World, FAutomationTestBase& Test)
	{
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		AActor* Marker = nullptr;
		AActor* Door = nullptr;
		AActor* Bed = nullptr;
		int32 StairNavigationLinks = 0;
		FVector Stones = FVector::ZeroVector;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Inn")) && It->ActorHasTag(TEXT("IslandLandmark"))) Marker = *It;
			if (It->GetActorLabel() == TEXT("Inn_DoorStep_0")) Door = *It;
			if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnBed_1"))) Bed = *It;
			if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnStairNavigationLink"))) ++StairNavigationLinks;
			if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"))) Stones = It->GetActorLocation();
		}
		if (!Navigation || !Marker || !Bed) { Test.AddError(TEXT("No navigation system, inn marker, or tagged bed to check access with.")); return false; }
		// The Island's navmesh is statically generated: use the editor's own "Build Paths" so stored tiles regenerate.
		FEditorBuildUtils::EditorBuild(World, FBuildOptions::BuildAIPaths);
		Test.AddInfo(FString::Printf(TEXT("Navigation build %s."), Navigation->IsNavigationBuildInProgress() ? TEXT("still in progress") : TEXT("finished")));
		auto Walk = [&](const TCHAR* Label, const FVector& Goal, const FVector& Extent)
		{
			FNavLocation From, To;
			if (!Navigation->ProjectPointToNavigation(Stones, From, FVector(250, 250, 1000))) { Test.AddInfo(TEXT("ListeningStones are off the navigation mesh.")); return false; }
			if (!Navigation->ProjectPointToNavigation(Goal, To, Extent)) { Test.AddInfo(FString::Printf(TEXT("%s: no navigation near %s."), Label, *Goal.ToString())); return false; }
			const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(World, From.Location, To.Location);
			const bool bOk = Path && Path->IsValid() && !Path->IsPartial();
			Test.AddInfo(FString::Printf(TEXT("%s: goal projected to %s (%.0f cm away); path %s%s."), Label, *To.Location.ToString(), FVector::Dist(Goal, To.Location),
				bOk ? TEXT("complete, ") : Path && Path->IsValid() ? TEXT("partial, ending at ") : TEXT("missing"),
				bOk ? *FString::Printf(TEXT("%.0f m"), Path->GetPathLength() / 100.f) : Path && Path->IsValid() && Path->PathPoints.Num() > 0 ? *Path->PathPoints.Last().ToString() : TEXT("")));
			return bOk;
		};
		const FTransform Frame = Marker->GetActorTransform();
		const FVector MarkerLocal(150, -130, 100);
		if (Door) Walk(TEXT("Door step"), Door->GetActorLocation(), FVector(150, 150, 300));
		const bool bInside = Walk(TEXT("Common room"), Marker->GetActorLocation() - FVector(0, 0, 80), FVector(60, 60, 120));
		const bool bBedReachable = Walk(TEXT("Inn bed"), Bed->GetActorLocation(), FVector(250, 250, 450));
		FNavLocation BedNavGoal;
		const bool bBedTargetIsNearItsNavGoal = Navigation->ProjectPointToNavigation(Bed->GetActorLocation(), BedNavGoal, FVector(250, 250, 450)) &&
			FVector::Dist2D(Bed->GetActorLocation(), BedNavGoal.Location) <= 100.f && FMath::Abs(Bed->GetActorLocation().Z - BedNavGoal.Location.Z) <= 100.f;
		Test.TestTrue(TEXT("The tagged rest marker lies on the upstairs navmesh, not metres away from it"), bBedTargetIsNearItsNavGoal);
		Test.TestEqual(TEXT("Both disconnected stair transitions have explicit navigation links"), StairNavigationLinks, 2);
		// Spot checks in the inn's own frame, relative to the marker.
		const TPair<const TCHAR*, FVector> Spots[] = {
			{TEXT("top door step"), FVector(520, 0, 5)}, {TEXT("just inside the door"), FVector(420, 0, 5)}, {TEXT("room centre"), FVector(150, 0, 5)},
			{TEXT("under the gallery"), FVector(-250, 0, 5)}, {TEXT("upper floor"), FVector(-250, 0, 320)}, {TEXT("roof ridge"), FVector(0, 0, 900)} };
		for (const TPair<const TCHAR*, FVector>& Spot : Spots)
		{
			const FVector At = Frame.TransformPosition(Spot.Value - MarkerLocal);
			FNavLocation Near;
			FHitResult Hit;
			const bool bSurface = World->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 60), At - FVector(0, 0, 200), ECC_Visibility);
			Test.AddInfo(FString::Printf(TEXT("  %s: surface %s, nav %s"), Spot.Key,
				bSurface ? *FString::Printf(TEXT("%s at %.0f"), Hit.GetActor() ? *Hit.GetActor()->GetActorLabel() : TEXT("?"), Hit.ImpactPoint.Z) : TEXT("none"),
				Navigation->ProjectPointToNavigation(At, Near, FVector(40, 40, 80)) ? *FString::Printf(TEXT("at %.0f"), Near.Location.Z) : TEXT("none")));
		}
		// Where Aster stood (capsule centre) after signing the guest book, then found no route anywhere for 25 minutes.
		const FVector CounterStand(-100039.f, 103308.f, 2778.f - 90.f);
		FNavLocation CounterNav;
		if (Navigation->ProjectPointToNavigation(CounterStand, CounterNav, FVector(40, 40, 120)))
			Test.AddInfo(FString::Printf(TEXT("  guest-book stand: nav at %s, %.0f cm away"), *CounterNav.Location.ToString(), FVector::Dist(CounterStand, CounterNav.Location)));
		else
			Test.AddInfo(TEXT("  guest-book stand: no navigation within 40 cm sideways"));
		Walk(TEXT("Guest-book stand"), CounterStand, FVector(40, 40, 120));
		Test.TestTrue(TEXT("Grounded residents can walk into the inn"), bInside);
		Test.TestTrue(TEXT("Grounded residents can reach the tagged inn bed"), bBedReachable);
		return bInside && bBedReachable && bBedTargetIsNearItsNavGoal && StairNavigationLinks == 2;
	}

	class FInnBuilder
	{
	public:
		FInnBuilder(UWorld* InWorld, const FInnSite& InSite, UStaticMesh* InCube) : World(InWorld), Site(InSite), Cube(InCube)
		{
			Origin = FVector(Site.Centre.X, Site.Centre.Y, Site.HighestGround + 25.f); // floor level, on the plinth
			Facing = FRotator(0.f, Site.Yaw, 0.f);
			CubeBounds = Cube->GetBoundingBox();
		}

		/** A box with local centre and size (cm), in the inn's frame; LocalRotation tilts it (roof slabs). */
		AStaticMeshActor* Box(const FString& Name, const FVector& Centre, const FVector& Size, UMaterialInterface* Material, const FRotator& LocalRotation = FRotator::ZeroRotator)
		{
			const FQuat Rotation = Facing.Quaternion() * LocalRotation.Quaternion();
			const FVector Scale = Size / CubeBounds.GetSize();
			const FVector WorldCentre = Origin + Facing.RotateVector(Centre);
			FActorSpawnParameters Spawn;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(WorldCentre - Rotation.RotateVector(CubeBounds.GetCenter() * Scale), Rotation.Rotator(), Spawn);
			Actor->GetStaticMeshComponent()->SetStaticMesh(Cube);
			Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
			Actor->SetActorScale3D(Scale);
			Finish(Actor, Name);
			return Actor;
		}

		ANavLinkProxy* NavigationLink(const FString& Name, const FVector& Start, const FVector& End)
		{
			const FVector WorldStart = ToWorld(Start);
			const FVector WorldEnd = ToWorld(End);
			const FTransform Transform(FRotator::ZeroRotator, WorldStart);
			FActorSpawnParameters Spawn;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Spawn.bDeferConstruction = true;
			ANavLinkProxy* LinkActor = World->SpawnActor<ANavLinkProxy>(ANavLinkProxy::StaticClass(), Transform, Spawn);
			if (!LinkActor) return nullptr;
			FNavigationLink& Link = LinkActor->PointLinks.AddDefaulted_GetRef();
			Link.Left = FVector::ZeroVector;
			Link.Right = WorldEnd - WorldStart;
			Link.Direction = ENavLinkDirection::BothWays;
			Link.SnapRadius = 100.f;
			Link.bUseSnapHeight = true;
			Link.SnapHeight = 100.f;
			LinkActor->Tags.Add(TEXT("InnStairNavigationLink"));
			Finish(LinkActor, Name);
			LinkActor->FinishSpawning(Transform);
			return LinkActor;
		}

		struct FOpening { float Centre; float Width; float Sill; float Top; };

		/**
		 * A straight wall from local (X0,Y0) to (X1,Y1), Height tall and Thickness thick, with rectangular
		 * openings (doors, windows) cut by splitting it into solid pieces.
		 */
		void Wall(const FString& Name, const FVector2D& Start, const FVector2D& End, float Height, float Thickness, TArray<FOpening> Openings, UMaterialInterface* Material)
		{
			const FVector2D Along = (End - Start).GetSafeNormal();
			const float Length = FVector2D::Distance(Start, End);
			const float WallYaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
			Openings.Sort([](const FOpening& A, const FOpening& B) { return A.Sill < B.Sill; });
			TArray<float> Cuts = { 0.f, Length };
			for (const FOpening& Opening : Openings) { Cuts.Add(Opening.Centre - Opening.Width * 0.5f); Cuts.Add(Opening.Centre + Opening.Width * 0.5f); }
			Cuts.Sort();
			int32 Piece = 0;
			for (int32 Index = 0; Index + 1 < Cuts.Num(); ++Index)
			{
				const float A = Cuts[Index], B = Cuts[Index + 1];
				if (B - A < 1.f) continue;
				const float Mid = (A + B) * 0.5f;
				float Bottom = 0.f;
				auto Emit = [&](float From, float To)
				{
					if (To - From < 1.f) return;
					const FVector2D At = Start + Along * Mid;
					Box(FString::Printf(TEXT("%s_%02d"), *Name, Piece++), FVector(At.X, At.Y, (From + To) * 0.5f), FVector(B - A, Thickness, To - From), Material, FRotator(0.f, WallYaw, 0.f));
				};
				for (const FOpening& Opening : Openings)
				{
					if (Mid < Opening.Centre - Opening.Width * 0.5f || Mid > Opening.Centre + Opening.Width * 0.5f) continue;
					Emit(Bottom, Opening.Sill);
					Bottom = Opening.Top;
				}
				Emit(Bottom, Height);
			}
		}

		APointLight* Light(const FString& Name, const FVector& Centre, float Candela, float Radius, const FLinearColor& Colour)
		{
			FActorSpawnParameters Spawn;
			APointLight* Actor = World->SpawnActor<APointLight>(Origin + Facing.RotateVector(Centre), FRotator::ZeroRotator, Spawn);
			Actor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			Actor->PointLightComponent->SetIntensityUnits(ELightUnits::Candelas);
			Actor->PointLightComponent->SetIntensity(Candela);
			Actor->PointLightComponent->SetAttenuationRadius(Radius);
			Actor->PointLightComponent->SetLightColor(Colour);
			Finish(Actor, Name);
			return Actor;
		}

		ATargetPoint* Marker(const FString& Name, const FVector& Centre, const TArray<FName>& Tags)
		{
			ATargetPoint* Actor = World->SpawnActor<ATargetPoint>(Origin + Facing.RotateVector(Centre), Facing);
			Finish(Actor, Name);
			for (int32 Index = Tags.Num() - 1; Index >= 0; --Index) Actor->Tags.Insert(Tags[Index], 0);
			return Actor;
		}

		FVector ToWorld(const FVector& Local) const { return Origin + Facing.RotateVector(Local); }
		int32 Count = 0;

	private:
		void Finish(AActor* Actor, const FString& Name)
		{
			Actor->SetActorLabel(TEXT("Inn_") + Name);
			Actor->SetFolderPath(TEXT("Inn"));
			Actor->Tags.AddUnique(TEXT("IslandInn"));
			++Count;
		}

		UWorld* World;
		FInnSite Site;
		UStaticMesh* Cube;
		FVector Origin;
		FRotator Facing;
		FBox CubeBounds;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildInnBlockoutTool, "CaptiveSky2.Tools.BuildInnBlockout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuildInnBlockoutTool::RunTest(const FString& Parameters)
{
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() && Context.World()->GetMapName() == TEXT("Island")) Island = Context.World();
	if (!TestNotNull(TEXT("Island is the open editor map"), Island)) return false;

	TArray<AActor*> Existing;
	for (TActorIterator<AActor> It(Island); It; ++It) if (It->ActorHasTag(TEXT("IslandInn"))) Existing.Add(*It);
	const bool bRebuild = FParse::Param(FCommandLine::Get(), TEXT("InnRebuild"));
	if (Existing.Num() > 0 && !bRebuild)
	{
		AddInfo(FString::Printf(TEXT("The inn already stands (%d parts); checking access. Pass -InnRebuild to replace it."), Existing.Num()));
		return CheckInnAccess(Island, *this);
	}

	// Candidate sites along the route from the TideglassPool toward the north-east shore.
	FVector Tideglass = FVector::ZeroVector;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("TideglassPool")) && It->ActorHasTag(TEXT("IslandLandmark"))) Tideglass = It->GetActorLocation();
	if (!TestFalse(TEXT("TideglassPool landmark found"), Tideglass.IsZero())) return false;
	const FVector2D Shore(-94650.f, 119600.f);
	const FVector2D Route = (Shore - FVector2D(Tideglass)).GetSafeNormal();
	const FVector2D Side(-Route.Y, Route.X);
	const float DoorYaw = FMath::RadiansToDegrees(FMath::Atan2(Route.Y, Route.X)); // door faces arrivals from the shore
	for (AActor* Actor : Existing) Actor->SetActorEnableCollision(false); // ignore the old inn while surveying

	FInnSite Best;
	float BestScore = TNumericLimits<float>::Max();
	int32 Candidates = 0;
	for (float Distance = 2500.f; Distance <= 12000.f; Distance += 500.f)
		for (float Offset : {0.f, -700.f, 700.f, -1400.f, 1400.f})
		{
			const FVector2D At = FVector2D(Tideglass) + Route * Distance + Side * Offset;
			FInnSite Site;
			if (!SurveySite(Island, FVector(At.X, At.Y, Tideglass.Z), DoorYaw, Site)) continue;
			++Candidates;
			// Prefer level ground, then closeness to the pool so the inn stays part of the journey.
			const float Score = (Site.HighestGround - Site.LowestGround) + Distance * 0.02f + FMath::Abs(Offset) * 0.01f;
			if (Score < BestScore) { BestScore = Score; Best = Site; }
		}
	if (!TestTrue(TEXT("Found level walkable ground for the inn on the shore route"), Candidates > 0)) return false;
	AddInfo(FString::Printf(TEXT("Chose inn site %s (door yaw %.0f°, ground varies %.0f cm) from %d candidates."), *Best.Centre.ToString(), Best.Yaw, Best.HighestGround - Best.LowestGround, Candidates));

	const FString MapFile = FPackageName::LongPackageNameToFilename(Island->GetOutermost()->GetName(), FPackageName::GetMapPackageExtension());
	const FString Backup = FPaths::ProjectSavedDir() / TEXT("MapBackups") / FString::Printf(TEXT("Island_%s.umap"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	if (!TestTrue(TEXT("Map backed up before building"), IFileManager::Get().Copy(*Backup, *MapFile) == COPY_OK)) return false;
	for (AActor* Actor : Existing) Island->EditorDestroyActor(Actor, true);

	// Sharp engine cubes: the prototyping chamfer cube balloons its rounded edges when stretched thin.
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* Oak = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Wood_Oak.M_Wood_Oak"));
	UMaterialInterface* Stone = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Props/Materials/M_Rock.M_Rock"));
	UMaterialInterface* Plaster = InnColour(TEXT("Plaster"), FLinearColor(0.62f, 0.55f, 0.43f));
	UMaterialInterface* Timber = InnColour(TEXT("Timber"), FLinearColor(0.09f, 0.055f, 0.035f));
	UMaterialInterface* Shingle = InnColour(TEXT("Shingle"), FLinearColor(0.16f, 0.09f, 0.07f));
	if (!Oak) Oak = Timber;
	if (!Stone) Stone = InnColour(TEXT("Stone"), FLinearColor(0.3f, 0.29f, 0.27f));

	FInnBuilder Inn(Island, Best, Cube);
	constexpr float L = 1000.f, W = 800.f, T = 30.f, H1 = 200.f, H = 600.f;
	const float PlinthDepth = Best.HighestGround + 25.f - Best.LowestGround + 40.f;

	// Stone plinth and oak floor.
	Inn.Box(TEXT("Plinth"), FVector(0, 0, -PlinthDepth * 0.5f), FVector(L + 60, W + 60, PlinthDepth), Stone);
	Inn.Box(TEXT("Floor"), FVector(0, 0, 2), FVector(L - 2 * T, W - 2 * T, 4), Oak);

	// Plaster walls: door facing the shore, windows on both long sides and above the door.
	const TArray<FInnBuilder::FOpening> SideWindows = { {250, 120, 100, 220}, {750, 120, 100, 220}, {300, 110, 400, 500}, {700, 110, 400, 500} };
	Inn.Wall(TEXT("WallFront"), FVector2D(L / 2 - T / 2, -W / 2), FVector2D(L / 2 - T / 2, W / 2), H, T, { {W / 2, 150, 0, 235}, {W / 2, 120, 400, 500} }, Plaster);
	Inn.Wall(TEXT("WallBack"), FVector2D(-L / 2 + T / 2, -W / 2), FVector2D(-L / 2 + T / 2, W / 2), H, T, {}, Plaster);
	Inn.Wall(TEXT("WallLeft"), FVector2D(-L / 2 + T, -W / 2 + T / 2), FVector2D(L / 2 - T, -W / 2 + T / 2), H, T, SideWindows, Plaster);
	Inn.Wall(TEXT("WallRight"), FVector2D(-L / 2 + T, W / 2 - T / 2), FVector2D(L / 2 - T, W / 2 - T / 2), H, T, SideWindows, Plaster);

	// Dark half-timbering standing just proud of the plaster: corner posts, a belt at the upper floor, a top plate.
	for (const FVector2D Corner : { FVector2D(1, 1), FVector2D(1, -1), FVector2D(-1, 1), FVector2D(-1, -1) })
		Inn.Box(FString::Printf(TEXT("Post_%s%s"), Corner.X > 0 ? TEXT("F") : TEXT("B"), Corner.Y > 0 ? TEXT("R") : TEXT("L")), FVector(Corner.X * (L / 2 - 5), Corner.Y * (W / 2 - 5), H / 2), FVector(40, 40, H), Timber);
	for (const float Z : { H1, H - 10.f })
	{
		const FString Level = Z < H / 2 ? TEXT("Belt") : TEXT("Plate");
		Inn.Box(Level + TEXT("_Front"), FVector(L / 2 + 4, 0, Z), FVector(16, W + 20, 22), Timber);
		Inn.Box(Level + TEXT("_Back"), FVector(-L / 2 - 4, 0, Z), FVector(16, W + 20, 22), Timber);
		Inn.Box(Level + TEXT("_Left"), FVector(0, -W / 2 - 4, Z), FVector(L + 20, 16, 22), Timber);
		Inn.Box(Level + TEXT("_Right"), FVector(0, W / 2 + 4, Z), FVector(L + 20, 16, 22), Timber);
	}
	for (const float X : { -250.f, 250.f })
		for (const float Y : { -1.f, 1.f })
			Inn.Box(FString::Printf(TEXT("Brace_%s%s"), X > 0 ? TEXT("F") : TEXT("B"), Y > 0 ? TEXT("R") : TEXT("L")), FVector(X, Y * (W / 2 + 4), (H1 + H) / 2), FVector(18, 14, 300), Timber, FRotator(30.f * Y, 0, 0));

	// Gable roof along the long axis with a generous overhang, and stepped plaster gable ends.
	const float Pitch = 35.f, HalfSpan = W / 2 + 70.f, Rise = HalfSpan * FMath::Tan(FMath::DegreesToRadians(Pitch));
	const float Slope = HalfSpan / FMath::Cos(FMath::DegreesToRadians(Pitch));
	for (const float Slant : { -1.f, 1.f })
		Inn.Box(Slant < 0 ? TEXT("RoofLeft") : TEXT("RoofRight"), FVector(0, Slant * HalfSpan / 2, H + Rise / 2 + 8), FVector(L + 120, Slope + 10, 18), Shingle, FRotator(0, 0, Slant * Pitch));
	Inn.Box(TEXT("RoofRidge"), FVector(0, 0, H + Rise + 14), FVector(L + 130, 30, 24), Timber);
	for (int32 Step = 0; Step < 4; ++Step)
	{
		const float Width = (W - T) * (1.f - (Step + 0.5f) / 4.f);
		const float Z = H + Rise * (Step + 0.5f) / 4.f;
		Inn.Box(FString::Printf(TEXT("GableFront_%d"), Step), FVector(L / 2 - T / 2, 0, Z), FVector(T, Width, Rise / 4), Plaster);
		Inn.Box(FString::Printf(TEXT("GableBack_%d"), Step), FVector(-L / 2 + T / 2, 0, Z), FVector(T, Width, Rise / 4), Plaster);
	}

	// Stone chimney through the back wall, with a masonry hearth frame around an open firebox.
	Inn.Box(TEXT("Chimney"), FVector(-L / 2 - 40, 0, (H + Rise + 150) / 2), FVector(130, 200, H + Rise + 150), Stone);
	const float HearthX = -L / 2 + T + 40;
	Inn.Box(TEXT("HearthBase"), FVector(HearthX, 0, 15), FVector(80, 240, 30), Stone);
	Inn.Box(TEXT("HearthPier_Left"), FVector(HearthX, -100, 76), FVector(80, 40, 92), Stone);
	Inn.Box(TEXT("HearthPier_Right"), FVector(HearthX, 100, 76), FVector(80, 40, 92), Stone);
	Inn.Box(TEXT("HearthBack"), FVector(HearthX - 30, 0, 70.5f), FVector(20, 160, 81), Stone);
	Inn.Box(TEXT("HearthLintel"), FVector(HearthX, 0, 121), FVector(80, 240, 20), Stone);
	Inn.Marker(TEXT("Hearth"), FVector(HearthX + 10, 0, 70), { TEXT("InnHearth") });
	Inn.Box(TEXT("HearthMantel"), FVector(-L / 2 + T + 55, 0, 140), FVector(60, 280, 18), Oak);

	// Upper loft with a stairwell opening; the landing meets the loft beyond the last tread.
	Inn.Box(TEXT("UpperFloorBack"), FVector(-285, 0, H1 + 8), FVector(370, W - 2 * T, 16), Oak);
	Inn.Box(TEXT("UpperFloorFrontLeft"), FVector(25, -230, H1 + 8), FVector(250, 280, 16), Oak);
	Inn.Box(TEXT("UpperFloorFrontRight"), FVector(25, 230, H1 + 8), FVector(250, 280, 16), Oak);
	constexpr int32 StairSteps = 5;
	constexpr float StairRun = 100.f;
	for (int32 Step = 0; Step < StairSteps; ++Step)
	{
		const float Top = (Step + 1) * H1 / StairSteps;
		const float X = -90.f + (StairSteps - Step - 0.5f) * StairRun;
		Inn.Box(FString::Printf(TEXT("Stair_%02d"), Step), FVector(X, 0.f, Top * 0.5f), FVector(StairRun, 140.f, Top), Oak);
	}
	Inn.Box(TEXT("UpperLanding"), FVector(-70.f, 0.f, H1 + 8.f), FVector(200.f, 140.f, 16.f), Oak);
	// Recast leaves a tiny nav gap at the floor-to-first-tread and last-tread-to-landing edges.
	// These short bidirectional links bridge only those physical stair transitions.
	Inn.NavigationLink(TEXT("StairLink_Foot"), FVector(300.f, 0.f, 4.f), FVector(360.f, 0.f, 40.f));
	Inn.NavigationLink(TEXT("StairLink_Crest"), FVector(60.f, 0.f, 160.f), FVector(-70.f, 0.f, H1 + 16.f));
	Inn.Box(TEXT("GalleryRail"), FVector(150, -230, H1 + 60), FVector(8, 280, 90), Timber);

	// Common room: counter, two tables with benches. Upper room: a bed.
	AStaticMeshActor* Counter = Inn.Box(TEXT("Counter"), FVector(180, -W / 2 + T + 50, 55), FVector(320, 70, 110), Oak);
	Counter->Tags.Insert(TEXT("InnCounter"), 0);
	// Tables sit well back from the door so the entry stays walkable around them.
	for (const float X : { 250.f, -40.f })
	{
		const FString Name = FString::Printf(TEXT("Table_%s"), X > 100 ? TEXT("Front") : TEXT("Back"));
		const float TableY = X > 100.f ? 190.f : 70.f;
		Inn.Box(Name, FVector(X, TableY, 38), FVector(190, 90, 76), Oak);
		Inn.Box(Name + TEXT("_BenchL"), FVector(X, TableY - 90, 23), FVector(190, 34, 46), Oak);
		Inn.Box(Name + TEXT("_BenchR"), FVector(X, TableY + 90, 23), FVector(190, 34, 46), Oak);
	}
	AStaticMeshActor* Bed = Inn.Box(TEXT("Bed_1"), FVector(-330, -200, H1 + 16 + 25), FVector(200, 100, 50), Oak);
	Bed->Tags.Add(TEXT("InnBedFurniture_1"));
	// Place the rest target on clear loft floor beside the bed, off its collision footprint.
	Inn.Marker(TEXT("BedRestSpot_1"), FVector(-160, -200, H1 + 16), { TEXT("InnBed_1") });

	// Steps up to the door if the plinth stands proud of the ground there.
	const float DoorRise = (Best.HighestGround + 25.f) - Best.GroundAtDoor;
	const int32 StepCount = FMath::Clamp(FMath::CeilToInt(DoorRise / 18.f), 0, 10);
	for (int32 Step = 0; Step < StepCount; ++Step)
	{
		const float Top = -DoorRise + (Step + 1) * DoorRise / StepCount;
		Inn.Box(FString::Printf(TEXT("DoorStep_%d"), Step), FVector(L / 2 + 30 + (StepCount - Step - 0.5f) * 32.f, 0, (Top - DoorRise - 20) / 2), FVector(32, 220, Top + DoorRise + 20), Stone);
	}

	// Warm light: the hearth fire and a lantern by the door, so the doorway reads from the path at dusk.
	Inn.Light(TEXT("HearthLight"), FVector(-L / 2 + T + 110, 0, 70), 40.f, 1200.f, FLinearColor(1.f, 0.52f, 0.22f))->Tags.Insert(TEXT("InnHearthLight"), 0);
	for (const float X : { 250.f, -40.f })
	{
		const FString Name = FString::Printf(TEXT("TableLamp_%s"), X > 100 ? TEXT("Front") : TEXT("Back"));
		const float Z = X > 100 ? 280.f : H1 - 50.f; // the back lamp hangs under the upper floor
		// Lamp housings sit right against their lights; letting them cast shadows would black out the walls.
		Inn.Box(Name, FVector(X, 70, Z + 12), FVector(20, 20, 24), Timber)->GetStaticMeshComponent()->SetCastShadow(false);
		Inn.Light(Name + TEXT("Light"), FVector(X, 70, Z - 8), 25.f, 900.f, FLinearColor(1.f, 0.6f, 0.3f));
	}
	Inn.Box(TEXT("Lantern"), FVector(L / 2 + 30, 120, 250), FVector(22, 22, 30), Timber)->GetStaticMeshComponent()->SetCastShadow(false);
	Inn.Light(TEXT("DoorLantern"), FVector(L / 2 + 45, 120, 245), 12.f, 700.f, FLinearColor(1.f, 0.62f, 0.3f))->Tags.Insert(TEXT("InnDoorLantern"), 0);

	// A landmark marker inside the common room, so residents can perceive and walk to the inn.
	// Clear floor between the counter and the benches, so the walkable goal isn't eaten by furniture margins.
	Inn.Marker(TEXT("Marker"), FVector(150, -130, 100), { TEXT("Inn"), TEXT("IslandLandmark") });
	AddInfo(FString::Printf(TEXT("Placed %d inn parts; door at %s."), Inn.Count, *Inn.ToWorld(FVector(L / 2, 0, 0)).ToString()));

	CheckInnAccess(Island, *this);
	return TestTrue(TEXT("Map with the inn saved"), FEditorFileUtils::SaveLevel(Island->PersistentLevel));
}

#endif
