#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "IslandDayNight.h"
#include "IslandInteractionUtility.h"
#include "IslandPoolRippleEffect.h"
#include "IslandTideglassSubsystem.h"
#include "IslandTidepoolMinnows.h"
#include "IslandWeather.h"
#include "RavenAgentAIController.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandMinnowTest, "CaptiveSky2.Agent.TidepoolMinnows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandMinnowTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Calm shallows allow ordinary school movement"), FMath::IsNearlyEqual(AIslandTidepoolMinnows::RainMovementScale(0.f), 1.f));
	TestTrue(TEXT("Heavy rain gently tightens, but does not stop, the school"),
		AIslandTidepoolMinnows::RainMovementScale(1.f) > 0.f && AIslandTidepoolMinnows::RainMovementScale(1.f) < 1.f);
	TestTrue(TEXT("The rain response changes smoothly and monotonically"),
		AIslandTidepoolMinnows::RainMovementScale(0.8f) < AIslandTidepoolMinnows::RainMovementScale(0.4f));

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Minnow ecology fixture world created"), World) || !TestNotNull(TEXT("Engine is available for minnow fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AIslandDayNight* Clock = World->SpawnActor<AIslandDayNight>(Spawn);
	AIslandWeather* Weather = World->SpawnActor<AIslandWeather>(Spawn);
	AActor* Habitat = World->SpawnActor<AActor>(FVector(1000.f, 2000.f, 300.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Day-night clock spawned"), Clock) || !TestNotNull(TEXT("Weather actor spawned"), Weather) || !TestNotNull(TEXT("Tideglass habitat marker spawned"), Habitat))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Clock->CurrentHour = 12.f;
	Clock->DayNumber = 1;
	Habitat->Tags.Add(TEXT("TideglassPool"));
	AStaticMeshActor* PoolSurfaceActor = World->SpawnActor<AStaticMeshActor>(Habitat->GetActorLocation(), FRotator::ZeroRotator, Spawn);
	UStaticMesh* PoolFootprint = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!TestNotNull(TEXT("Synthetic Tideglass footprint actor spawned"), PoolSurfaceActor) ||
		!TestNotNull(TEXT("Engine pool-footprint sphere is available"), PoolFootprint))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	PoolSurfaceActor->GetStaticMeshComponent()->SetStaticMesh(PoolFootprint);
	PoolSurfaceActor->SetActorScale3D(FVector(4.f, 4.f, 0.1f));
	PoolSurfaceActor->GetStaticMeshComponent()->UpdateBounds();
	Weather->RefreshNightEcology();
	AIslandTidepoolMinnows* School = Weather->DayMinnowSchool.Get();
	if (!TestNotNull(TEXT("One daytime school appears at Tideglass"), School))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	School->Weather = Weather;
	School->IslandClock = Clock;
	School->CachePoolSwimmingBounds();
	School->ConfigureAppearance();
	School->UpdateSchool(0.f);
	TestTrue(TEXT("The school derives a swim boundary from the tagged pool's flattened sphere"), School->bHasPoolSwimmingBounds);
	auto AreFishInsidePoolFootprint = [&School]()
	{
		if (!School->bHasPoolSwimmingBounds) return false;
		for (UProceduralMeshComponent* Minnow : School->Fish)
		{
			if (!Minnow) return false;
			const FVector Location = Minnow->GetComponentLocation();
			const FVector2D NormalizedOffset(
				(Location.X - School->PoolSurfaceBoundsOrigin.X) / School->PoolSwimmingRadii.X,
				(Location.Y - School->PoolSurfaceBoundsOrigin.Y) / School->PoolSwimmingRadii.Y);
			if (NormalizedOffset.Size() > 1.001f) return false;
		}
		return true;
	};
	auto IsInsideStartleRippleFootprint = [&School](const FVector& WorldLocation)
	{
		if (!School->bHasPoolSwimmingBounds) return false;
		const float SafeRadiusX = FMath::Max(1.f, School->PoolSwimmingRadii.X - AIslandTidepoolMinnows::StartleRippleEdgeClearanceCm);
		const float SafeRadiusY = FMath::Max(1.f, School->PoolSwimmingRadii.Y - AIslandTidepoolMinnows::StartleRippleEdgeClearanceCm);
		const FVector2D NormalizedOffset(
			(WorldLocation.X - School->PoolSurfaceBoundsOrigin.X) / SafeRadiusX,
			(WorldLocation.Y - School->PoolSurfaceBoundsOrigin.Y) / SafeRadiusY);
		return NormalizedOffset.Size() <= 1.001f;
	};
	TestTrue(TEXT("Every fish begins within the measured shallow-water footprint"), AreFishInsidePoolFootprint());
	Weather->RefreshNightEcology();
	TestTrue(TEXT("Repeated ecology refresh reuses rather than duplicates the school"), Weather->DayMinnowSchool.Get() == School);
	TestTrue(TEXT("The school is wild life, not a landmark, pet, or nest site"),
		School->ActorHasTag(TEXT("IslandLife")) && School->ActorHasTag(TEXT("MinnowSchool")) &&
		!School->ActorHasTag(TEXT("IslandLandmark")) && !School->ActorHasTag(TEXT("RavenNestSite")));
	TestEqual(TEXT("The shallow-water school has a small bounded population"), School->Fish.Num(), 5);
	TestEqual(TEXT("Each fish has one animated forked caudal fin"), School->Tails.Num(), School->Fish.Num());
	TestEqual(TEXT("Each fish has a dorsal fin and paired pectoral fins"), School->BodyFins.Num(), School->Fish.Num());
	TestEqual(TEXT("Each fish has one visual-only dorsal flash mark"), School->DorsalMarks.Num(), School->Fish.Num());
	TArray<FLinearColor> FishColors;
	for (int32 Index = 0; Index < School->Fish.Num(); ++Index)
	{
		UProceduralMeshComponent* Minnow = School->Fish[Index];
		UProceduralMeshComponent* Tail = School->Tails.IsValidIndex(Index) ? School->Tails[Index] : nullptr;
		UProceduralMeshComponent* Fins = School->BodyFins.IsValidIndex(Index) ? School->BodyFins[Index] : nullptr;
		UProceduralMeshComponent* DorsalMark = School->DorsalMarks.IsValidIndex(Index) ? School->DorsalMarks[Index] : nullptr;
		const FProcMeshSection* BodySection = Minnow ? Minnow->GetProcMeshSection(0) : nullptr;
		const FProcMeshSection* TailSection = Tail ? Tail->GetProcMeshSection(0) : nullptr;
		const FProcMeshSection* FinsSection = Fins ? Fins->GetProcMeshSection(0) : nullptr;
		const FProcMeshSection* MarkSection = DorsalMark ? DorsalMark->GetProcMeshSection(0) : nullptr;
		const FProcMeshSection* EyeSection = DorsalMark ? DorsalMark->GetProcMeshSection(1) : nullptr;
		const FProcMeshSection* PupilSection = DorsalMark ? DorsalMark->GetProcMeshSection(2) : nullptr;
		TestTrue(TEXT("Tapered minnow bodies are procedural, visual-only, and nonblocking"),
			BodySection && BodySection->ProcVertexBuffer.Num() == 86 && BodySection->ProcIndexBuffer.Num() == 504 &&
			Minnow->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Minnow->CastShadow);
		if (BodySection)
		{
			const FBox BodyBounds = BodySection->SectionLocalBox;
			TestTrue(TEXT("The low-poly body tapers at both the tail stock and snout"),
				BodyBounds.Min.X <= -8.f && BodyBounds.Max.X >= 8.8f &&
				BodyBounds.Min.Y <= -1.9f && BodyBounds.Max.Y >= 1.9f &&
				BodyBounds.Min.Z <= -1.9f && BodyBounds.Max.Z >= 1.9f);
			auto MaxRingRadius = [BodySection](int32 FirstVertex)
			{
				float Radius = 0.f;
				for (int32 Segment = 0; Segment < 12; ++Segment)
				{
					const FVector& Position = BodySection->ProcVertexBuffer[FirstVertex + Segment].Position;
					Radius = FMath::Max(Radius, FVector2D(Position.Y, Position.Z).Size());
				}
				return Radius;
			};
			const float TailStockRadius = MaxRingRadius(1);
			const float MidBodyRadius = MaxRingRadius(1 + 3 * 12);
			const float SnoutBaseRadius = MaxRingRadius(1 + 6 * 12);
			TestTrue(TEXT("The tail stock and snout are substantially narrower than the fuller midsection"),
				TailStockRadius < MidBodyRadius * 0.35f && SnoutBaseRadius < MidBodyRadius * 0.35f);
		}
		TestTrue(TEXT("Forked tail fins are visible procedural meshes without collision or shadows"),
			Tail && TailSection && Tail->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Tail->CastShadow);
		TestTrue(TEXT("Dorsal and pectoral fins are visible procedural meshes without collision or shadows"),
			Fins && FinsSection && Fins->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Fins->CastShadow);
		TestTrue(TEXT("The dorsal and paired pectoral fins are double-sided triangles"),
			FinsSection && FinsSection->ProcVertexBuffer.Num() == 18 && FinsSection->ProcIndexBuffer.Num() == 18);
		TestTrue(TEXT("A narrow dorsal flash follows the fish's back without collision or shadows"),
			DorsalMark && MarkSection && MarkSection->ProcVertexBuffer.Num() == 14 && MarkSection->ProcIndexBuffer.Num() == 36 &&
			DorsalMark->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !DorsalMark->CastShadow &&
			MarkSection->SectionLocalBox.Min.X <= -4.8f && MarkSection->SectionLocalBox.Max.X >= 4.8f &&
			MarkSection->SectionLocalBox.Min.Y <= -0.34f && MarkSection->SectionLocalBox.Max.Y >= 0.34f);
		TestTrue(TEXT("Paired eyes sit on both head flanks as small visual-only mesh details"),
			EyeSection && EyeSection->ProcVertexBuffer.Num() == 18 && EyeSection->ProcIndexBuffer.Num() == 48 &&
			EyeSection->SectionLocalBox.Min.X > 3.9f && EyeSection->SectionLocalBox.Max.X < 5.f &&
			EyeSection->SectionLocalBox.Min.Y < -1.f && EyeSection->SectionLocalBox.Max.Y > 1.f &&
			PupilSection && PupilSection->ProcVertexBuffer.Num() == 18 && PupilSection->ProcIndexBuffer.Num() == 48);
		TestTrue(TEXT("Each tail fin is positioned just behind the body tail stock"),
			Tail && Tail->GetRelativeLocation().X <= -7.5f);
		if (TailSection)
		{
			float MinTailX = BIG_NUMBER;
			float MinTailZ = BIG_NUMBER;
			float MaxTailZ = -BIG_NUMBER;
			for (const FProcMeshVertex& Vertex : TailSection->ProcVertexBuffer)
			{
				MinTailX = FMath::Min(MinTailX, Vertex.Position.X);
				MinTailZ = FMath::Min(MinTailZ, Vertex.Position.Z);
				MaxTailZ = FMath::Max(MaxTailZ, Vertex.Position.Z);
			}
			TestTrue(TEXT("The caudal fin has two vertical lobes extending behind its narrow stalk"),
				TailSection->ProcVertexBuffer.Num() == 12 && MinTailX <= -3.1f && MinTailZ <= -3.2f && MaxTailZ >= 3.2f);
		if (FinsSection)
		{
			float MinFinY = BIG_NUMBER;
			float MaxFinY = -BIG_NUMBER;
			float MaxFinZ = -BIG_NUMBER;
			for (const FProcMeshVertex& Vertex : FinsSection->ProcVertexBuffer)
			{
				MinFinY = FMath::Min(MinFinY, Vertex.Position.Y);
				MaxFinY = FMath::Max(MaxFinY, Vertex.Position.Y);
				MaxFinZ = FMath::Max(MaxFinZ, Vertex.Position.Z);
			}
			TestTrue(TEXT("Pectoral fins extend laterally and the dorsal fin clears the body surface"),
				MinFinY <= -4.2f && MaxFinY >= 4.2f && MaxFinZ >= 4.25f);
		}
		}
		UMaterialInstanceDynamic* FishMaterial = Minnow ? Cast<UMaterialInstanceDynamic>(Minnow->GetMaterial(0)) : nullptr;
		if (FishMaterial)
		{
			const FLinearColor FishColor = FishMaterial->K2_GetVectorParameterValue(TEXT("Color"));
			FishColors.Add(FishColor);
			TestTrue(TEXT("Minnow palette stays subdued enough to hold its color in bright shallow-water light"),
				FMath::Max(FishColor.R, FMath::Max(FishColor.G, FishColor.B)) <= 0.4f);
			UMaterialInstanceDynamic* FinMaterial = Tail ? Cast<UMaterialInstanceDynamic>(Tail->GetMaterial(0)) : nullptr;
			const FLinearColor FinColor = FinMaterial ? FinMaterial->K2_GetVectorParameterValue(TEXT("Color")) : FLinearColor::Black;
			TestTrue(TEXT("Tail and body fins share a darker, stable accent material"),
				FinMaterial && Fins && Tail->GetMaterial(0) == Fins->GetMaterial(0) &&
				FinColor.Equals(FishColor * 0.55f, 0.001f));
			UMaterialInstanceDynamic* MarkMaterial = DorsalMark ? Cast<UMaterialInstanceDynamic>(DorsalMark->GetMaterial(0)) : nullptr;
			const FLinearColor MarkColor = MarkMaterial ? MarkMaterial->K2_GetVectorParameterValue(TEXT("Color")) : FLinearColor::Black;
			TestTrue(TEXT("Dorsal marks add a restrained warm contrast without changing the natural body palette"),
				MarkMaterial && FMath::Max(MarkColor.R, FMath::Max(MarkColor.G, MarkColor.B)) >
				FMath::Max(FishColor.R, FMath::Max(FishColor.G, FishColor.B)) + 0.1f &&
				MarkColor.R <= 0.65f && MarkColor.G <= 0.65f);
			UMaterialInstanceDynamic* EyeMaterial = DorsalMark ? Cast<UMaterialInstanceDynamic>(DorsalMark->GetMaterial(1)) : nullptr;
			UMaterialInstanceDynamic* PupilMaterial = DorsalMark ? Cast<UMaterialInstanceDynamic>(DorsalMark->GetMaterial(2)) : nullptr;
			const FLinearColor IrisColor = EyeMaterial ? EyeMaterial->K2_GetVectorParameterValue(TEXT("Color")) : FLinearColor::Black;
			const FLinearColor PupilColor = PupilMaterial ? PupilMaterial->K2_GetVectorParameterValue(TEXT("Color")) : FLinearColor::White;
			TestTrue(TEXT("Amber irises and dark pupils add a restrained, readable fish-face cue"),
				EyeMaterial && PupilMaterial && IrisColor.R > 0.75f && IrisColor.G > 0.5f && IrisColor.B < 0.4f &&
				FMath::Max(PupilColor.R, FMath::Max(PupilColor.G, PupilColor.B)) < 0.1f);
		}
	}
	TestEqual(TEXT("Every fish receives an individual visual color"), FishColors.Num(), School->Fish.Num());
	const float SpringHighOffset = UIslandTideglassSubsystem::TideOffsetCm(6.21f, 1);
	const float SpringLowOffset = UIslandTideglassSubsystem::TideOffsetCm(18.63f, 1);
	Clock->CurrentHour = 6.21f;
	School->UpdateSchool(0.f);
	const float HighWaterFishZ = School->Fish[0]->GetComponentLocation().Z;
	const float HighWaterSwimHeight = HighWaterFishZ - School->GetActorLocation().Z - SpringHighOffset;
	Clock->CurrentHour = 18.63f;
	School->UpdateSchool(0.f);
	const float LowWaterFishZ = School->Fish[0]->GetComponentLocation().Z;
	const float LowWaterSwimHeight = LowWaterFishZ - School->GetActorLocation().Z - SpringLowOffset;
	const FProcMeshSection* TideTestBody = School->Fish[0]->GetProcMeshSection(0);
	const float FishHalfHeight = TideTestBody ? TideTestBody->SectionLocalBox.GetExtent().Z : 0.f;
	TestTrue(TEXT("The school rises and falls with Tideglass between spring high and low water"),
		FMath::IsNearlyEqual(LowWaterFishZ - HighWaterFishZ, SpringLowOffset - SpringHighOffset, 0.01f));
	TestTrue(TEXT("At spring high tide the fish center stays within 1.3 cm of the surface"),
		HighWaterSwimHeight >= 0.3f && HighWaterSwimHeight <= 1.3f);
	TestTrue(TEXT("At spring low tide the fish center stays within 1.3 cm of the surface"),
		LowWaterSwimHeight >= 0.3f && LowWaterSwimHeight <= 1.3f);
	TestTrue(TEXT("The fish body straddles the surface instead of hovering wholly above it"),
		FishHalfHeight > HighWaterSwimHeight && FishHalfHeight > LowWaterSwimHeight);
	Clock->CurrentHour = 12.f;
	School->UpdateSchool(0.f);
	int32 DistinctColorCount = 0;
	for (int32 Index = 0; Index < FishColors.Num(); ++Index)
	{
		bool bSeenColor = false;
		for (int32 PriorIndex = 0; PriorIndex < Index; ++PriorIndex)
			if (FishColors[Index].Equals(FishColors[PriorIndex], 0.001f)) { bSeenColor = true; break; }
		if (!bSeenColor) ++DistinctColorCount;
	}
	TestEqual(TEXT("The small school uses stable, distinct natural color variants"), DistinctColorCount, School->Fish.Num());
	if (School->Tails.Num() == School->Fish.Num())
	{
		const FRotator StableTailPose = School->Tails[0]->GetRelativeRotation();
		School->Tick(0.f);
		TestTrue(TEXT("Repeated update at the same swim phase keeps a stable tail pose"), School->Tails[0]->GetRelativeRotation().Equals(StableTailPose, 0.001f));
		School->Tick(0.2f);
		TestFalse(TEXT("Tail fins wag visibly as the fish swim"), School->Tails[0]->GetRelativeRotation().Equals(StableTailPose, 0.25f));
	}

	School->Tick(0.35f);
	const FVector BeforeScatter = [&School]()
	{
		FVector Center = FVector::ZeroVector;
		for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) Center += Minnow->GetRelativeLocation();
		return Center / School->Fish.Num();
	}();
	ACharacter* Visitor = World->SpawnActor<ACharacter>(School->GetActorLocation() - FVector(300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Quiet observer spawned near the pool"), Visitor))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Visitors and residents resolve one stable school target"), IslandInteractionUtility::GetTargetTag(School), FName(TEXT("MinnowSchool")));
	TestTrue(TEXT("The nearby school is visible to a visitor"), IslandInteractionUtility::CanInteract(Visitor, School));
	FVector ExpectedStartleRippleLocation = FVector::ZeroVector;
	float NearestFishDistanceSquared = TNumericLimits<float>::Max();
	for (UProceduralMeshComponent* FishBody : School->Fish)
	{
		if (!FishBody) continue;
		const float DistanceSquared = FVector::DistSquared2D(FishBody->GetComponentLocation(), Visitor->GetActorLocation());
		if (DistanceSquared < NearestFishDistanceSquared)
		{
			NearestFishDistanceSquared = DistanceSquared;
			const FVector FishLocation = FishBody->GetComponentLocation();
			const FVector SafeRippleCenter = School->GetActorTransform().TransformPosition(
				School->ClampToPoolSwimmingBounds(School->GetActorTransform().InverseTransformPosition(FishLocation),
					AIslandTidepoolMinnows::StartleRippleEdgeClearanceCm));
			ExpectedStartleRippleLocation = FVector(SafeRippleCenter.X, SafeRippleCenter.Y,
				School->GetActorLocation().Z + School->GetTideOffsetCm() - 24.f);
		}
	}
	FString VisitorFact;
	TestTrue(TEXT("Quiet observation is a valid visitor response"), IslandInteractionUtility::Perform(Visitor, School, VisitorFact));
	TestTrue(TEXT("The response truthfully describes scattering without capture or persistence"),
		VisitorFact.Contains(TEXT("scattered from your quiet attention")) && VisitorFact.Contains(TEXT("wild and uncaught")) && VisitorFact.Contains(TEXT("nothing persistent changed")));
	AIslandPoolRippleEffect* ObserverStartleRipple = nullptr;
	int32 ObserverStartleRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("MinnowStartleImpact"))) continue;
		ObserverStartleRipple = *It;
		++ObserverStartleRippleCount;
	}
	TestEqual(TEXT("One quiet observation creates one short-lived school-startle cue"), ObserverStartleRippleCount, 1);
	if (ObserverStartleRipple)
	{
		TestTrue(TEXT("Nearby shore wildlife can recognize the cue as a minnow surface disturbance"),
			ObserverStartleRipple->ActorHasTag(TEXT("MinnowImpact")) && ObserverStartleRipple->ActorHasTag(TEXT("MinnowStartleImpact")));
		TestTrue(TEXT("The startle ripple is brief, broad enough to notice, and contained near the fish in the shallows"),
			FMath::IsNearlyEqual(ObserverStartleRipple->DurationSeconds, 1.15f) &&
			FMath::IsNearlyEqual(ObserverStartleRipple->SurfaceRadius, 72.f) &&
			ObserverStartleRipple->PeakLightIntensity > 1.35f && ObserverStartleRipple->PeakLightIntensity <= 5.f);
		TestTrue(TEXT("A startle response renders a visible continuous surface ring in addition to its moving highlights"),
			ObserverStartleRipple->StartleRing && ObserverStartleRipple->StartleRing->IsVisible() &&
			ObserverStartleRipple->StartleRing->GetProcMeshSection(0) != nullptr &&
			ObserverStartleRipple->StartleRingMaterial &&
			ObserverStartleRipple->StartleRing->GetRelativeLocation().Z >= 32.f);
		const FProcMeshSection* StartleRingSection = ObserverStartleRipple->StartleRing
			? ObserverStartleRipple->StartleRing->GetProcMeshSection(0) : nullptr;
		TestTrue(TEXT("The surface ring has a broad enough band to remain legible over moving water"),
			StartleRingSection && StartleRingSection->ProcVertexBuffer.Num() >= 2 &&
			FVector::Dist(StartleRingSection->ProcVertexBuffer[0].Position, StartleRingSection->ProcVertexBuffer[1].Position) >= 0.09f);
		TestTrue(TEXT("The cue stays near the nearest fish while keeping its full ring inside the safe water footprint"),
			ObserverStartleRipple->GetActorLocation().Equals(ExpectedStartleRippleLocation, 0.1f));
		TestTrue(TEXT("The observer-triggered ring's full radius clears the pool edge"),
			IsInsideStartleRippleFootprint(ObserverStartleRipple->GetActorLocation()));
		ObserverStartleRipple->Tick(0.5f);
		TestTrue(TEXT("The visible surface ring expands while the startle cue fades"),
			ObserverStartleRipple->StartleRing->GetRelativeScale3D().X > 12.f);
		School->RespondToQuietObservation(Visitor->GetActorLocation());
		int32 RepeatedStartleRippleCount = 0;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("MinnowStartleImpact"))) ++RepeatedStartleRippleCount;
		TestEqual(TEXT("A repeated quiet look cannot stack a startle cue inside its short cooldown"), RepeatedStartleRippleCount, 1);
		ObserverStartleRipple->Tags.Remove(TEXT("MinnowStartleImpact"));
		ObserverStartleRipple->Destroy();
	}
	School->Tick(0.7f);
	FVector ScatteredCenter = FVector::ZeroVector;
	for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) ScatteredCenter += Minnow->GetRelativeLocation();
	ScatteredCenter /= School->Fish.Num();
	TestTrue(TEXT("The school visibly fans away from a quiet observer"), ScatteredCenter.X > BeforeScatter.X + 80.f);
	TestTrue(TEXT("The full quiet scatter keeps every fish within the pool's shallow-water footprint"), AreFishInsidePoolFootprint());
	School->Tick(2.f);
	FVector RegroupedCenter = FVector::ZeroVector;
	for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) RegroupedCenter += Minnow->GetRelativeLocation();
	RegroupedCenter /= School->Fish.Num();
	TestTrue(TEXT("Minnows naturally regroup within their small local habitat"), RegroupedCenter.Size2D() < 180.f && FMath::IsNearlyZero(School->ScatterRemaining));

	Habitat->Tags.Add(TEXT("IslandLandmark"));
	FVector CircleCenter = FVector::ZeroVector;
	float OriginalCircleRadius = 0.f;
	for (UProceduralMeshComponent* Minnow : School->Fish)
	{
		if (!Minnow) continue;
		CircleCenter += Minnow->GetRelativeLocation();
	}
	CircleCenter /= School->Fish.Num();
	for (UProceduralMeshComponent* Minnow : School->Fish)
		if (Minnow) OriginalCircleRadius += FVector::Dist2D(Minnow->GetRelativeLocation(), CircleCenter);
	OriginalCircleRadius /= School->Fish.Num();
	TestTrue(TEXT("The close observer has a clear view of the minnow school"), IslandInteractionUtility::CanInspect(Visitor, School, 800.f));
	FString PoolFact;
	TestTrue(TEXT("A visible Tideglass landmark interaction reaches its nearby school"), IslandInteractionUtility::Perform(Visitor, Habitat, PoolFact));
	TestTrue(TEXT("The pool truthfully reports a brief, uncapturable wildlife response"),
		PoolFact.Contains(TEXT("widened its circle of motion")) && PoolFact.Contains(TEXT("wild and uncaught")) && PoolFact.Contains(TEXT("no permanent level state changes")));
	TestTrue(TEXT("The school accepts one short water-ripple response"), School->SurfacePulseRemaining > 0.f && School->SurfacePulseRemaining <= 1.2f);
	School->Tick(0.3f);
	CircleCenter = FVector::ZeroVector;
	for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) CircleCenter += Minnow->GetRelativeLocation();
	CircleCenter /= School->Fish.Num();
	float ExpandedCircleRadius = 0.f;
	for (UProceduralMeshComponent* Minnow : School->Fish)
		if (Minnow) ExpandedCircleRadius += FVector::Dist2D(Minnow->GetRelativeLocation(), CircleCenter);
	ExpandedCircleRadius /= School->Fish.Num();
	TestTrue(TEXT("The school visibly widens rather than fleeing the pool"), ExpandedCircleRadius > OriginalCircleRadius * 1.4f);
	TestTrue(TEXT("A surface pulse widens the orbit without carrying fish onto the surrounding shore"), AreFishInsidePoolFootprint());
	School->Tick(0.9f);
	TestTrue(*FString::Printf(TEXT("The surface response decays back to the normal orbit (%.4f seconds remain)"), School->SurfacePulseRemaining),
		FMath::IsNearlyZero(School->SurfacePulseRemaining, 0.01f));
	TestFalse(TEXT("The brief response cooldown prevents immediate retriggering"), School->RespondToSurfaceRipple());
	School->Tick(1.8f);
	Visitor->SetActorLocation(Habitat->GetActorLocation() + FVector(1000.f, 0.f, 0.f));
	FString DistantPoolFact;
	TestTrue(TEXT("The pool itself still responds independently of who can see its fish"), IslandInteractionUtility::Perform(Visitor, Habitat, DistantPoolFact));
	TestFalse(TEXT("A distant observer is not told the minnow school reacted to their interaction"), DistantPoolFact.Contains(TEXT("widened its circle of motion")));
	CircleCenter = FVector::ZeroVector;
	for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) CircleCenter += Minnow->GetRelativeLocation();
	CircleCenter /= School->Fish.Num();
	float PreWindCircleRadius = 0.f;
	for (UProceduralMeshComponent* Minnow : School->Fish)
		if (Minnow) PreWindCircleRadius += FVector::Dist2D(Minnow->GetRelativeLocation(), CircleCenter);
	PreWindCircleRadius /= School->Fish.Num();

	AIslandPoolRippleEffect* WindRipple = World->SpawnActor<AIslandPoolRippleEffect>(School->GetActorLocation() + FVector(40.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("A nearby wind ripple spawns beside the shallow school"), WindRipple))
	{
		WindRipple->ConfigureAsWindImpact(120.f);
		School->RippleCheckRemaining = 0.f;
		School->Tick(0.36f);
		TestTrue(TEXT("A natural nearby wind ripple briefly widens the school"),
			School->SurfacePulseRemaining > 0.f && School->SurfacePulseRemaining <= 1.2f);
		School->Tick(0.3f);
		CircleCenter = FVector::ZeroVector;
		for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) CircleCenter += Minnow->GetRelativeLocation();
		CircleCenter /= School->Fish.Num();
		ExpandedCircleRadius = 0.f;
		for (UProceduralMeshComponent* Minnow : School->Fish)
			if (Minnow) ExpandedCircleRadius += FVector::Dist2D(Minnow->GetRelativeLocation(), CircleCenter);
		ExpandedCircleRadius /= School->Fish.Num();
		TestTrue(*FString::Printf(TEXT("The natural ripple produces readable widening (%.1f cm vs %.1f cm before; pulse alpha %.2f)"),
			ExpandedCircleRadius, PreWindCircleRadius, School->GetSurfacePulseAlpha()),
			ExpandedCircleRadius > PreWindCircleRadius * 1.4f);
		WindRipple->Destroy();
	}
	School->Tick(3.f);
	TestTrue(TEXT("The wind response settles back into the normal orbit"), FMath::IsNearlyZero(School->SurfacePulseRemaining));

	AIslandPoolRippleEffect* DistantRainRipple = World->SpawnActor<AIslandPoolRippleEffect>(School->GetActorLocation() + FVector(700.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("A distant rain ripple spawns outside the school's reach"), DistantRainRipple))
	{
		DistantRainRipple->ConfigureAsRainImpact();
		School->RippleCheckRemaining = 0.f;
		School->Tick(0.36f);
		TestTrue(TEXT("A distant natural rain ripple does not disturb the school"), FMath::IsNearlyZero(School->SurfacePulseRemaining));
		DistantRainRipple->Destroy();
	}
	AIslandPoolRippleEffect* NearbyVisitorRipple = World->SpawnActor<AIslandPoolRippleEffect>(School->GetActorLocation() + FVector(40.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("An untyped nearby visitor ripple is distinguishable from weather"), NearbyVisitorRipple))
	{
		School->RippleCheckRemaining = 0.f;
		School->Tick(0.36f);
		TestTrue(TEXT("The ambient weather sensor ignores ordinary interaction ripples"), FMath::IsNearlyZero(School->SurfacePulseRemaining));
		NearbyVisitorRipple->Destroy();
	}

	School->ScatterRemaining = 0.f;
	School->SurfacePulseRemaining = 0.f;
	School->SurfacePulseCooldownRemaining = 0.f;
	AIslandPoolRippleEffect* UnintendedRipple = nullptr;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
		if (It->ActorHasTag(TEXT("MinnowImpact"))) { UnintendedRipple = *It; break; }
	if (UnintendedRipple) UnintendedRipple->Destroy();
	School->SurfaceBreakRemaining = 0.f;
	School->TryCreateSurfaceBreak(0.8f);
	TestTrue(TEXT("Heavy rain postpones rather than stacks a fish surface break"),
		FMath::IsNearlyEqual(School->SurfaceBreakRemaining, 4.f));
	School->SurfaceBreakRemaining = 0.f;
	School->TryCreateSurfaceBreak(0.f);
	AIslandPoolRippleEffect* MinnowRipple = nullptr;
	int32 MinnowRippleCount = 0;
	for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MinnowImpact"))) { MinnowRipple = *It; ++MinnowRippleCount; }
	}
	TestEqual(TEXT("One calm surface break creates exactly one transient ripple"), MinnowRippleCount, 1);
	if (MinnowRipple)
	{
		TestTrue(TEXT("The fish ripple is short, small, and visually restrained"),
			FMath::IsNearlyEqual(MinnowRipple->DurationSeconds, 0.95f) &&
			FMath::IsNearlyEqual(MinnowRipple->SurfaceRadius, 48.f) &&
			MinnowRipple->PeakLightIntensity <= 0.7f);
		const UProceduralMeshComponent* SourceFish = School->Fish[0];
		TestTrue(TEXT("The ripple originates under the source fish at the shallow surface"), SourceFish &&
			FVector::Dist2D(MinnowRipple->GetActorLocation(), SourceFish->GetComponentLocation()) < 1.f &&
			FMath::IsNearlyEqual(MinnowRipple->GetActorLocation().Z, School->GetActorLocation().Z + School->GetTideOffsetCm() - 24.f));
		School->CheckForNaturalSurfaceRipple();
		TestTrue(TEXT("The minnow school ignores its own surface break instead of feeding back"),
			FMath::IsNearlyZero(School->SurfacePulseRemaining));
		TestTrue(TEXT("The next surface break is delayed to keep wildlife effects sparse"),
			FMath::IsNearlyEqual(School->SurfaceBreakRemaining, 19.f));
		Visitor->SetActorLocation(School->GetActorLocation() + FVector(300.f, 0.f, 0.f));
		ARavenAgentAIController* CueObserverController = World->SpawnActor<ARavenAgentAIController>(Spawn);
		if (TestNotNull(TEXT("A resident controller can inspect its immediate wildlife context"), CueObserverController))
		{
			CueObserverController->Possess(Visitor);
			World->GetTimerManager().ClearAllTimersForObject(CueObserverController);
			const FString NearbyContext = CueObserverController->DescribeActionState();
			TestTrue(TEXT("The cue remains available through the longest normal decision interval and timer slack"),
				FMath::IsNearlyEqual(School->SurfaceBreakContextRemaining, AIslandTidepoolMinnows::SurfaceBreakContextLifetime));
			TestTrue(TEXT("A nearby resident receives the faded surface-break cue in its next action context"),
				NearbyContext.Contains(TEXT("Nearby transient wildlife cue")) && NearbyContext.Contains(TEXT("ripple has faded")));
			Visitor->SetActorLocation(School->GetActorLocation() + FVector(1600.f, 0.f, 0.f));
			TestFalse(TEXT("The same transient cue is omitted outside the habitat radius"),
				CueObserverController->DescribeActionState().Contains(TEXT("Nearby transient wildlife cue")));
			Visitor->SetActorLocation(School->GetActorLocation() + FVector(300.f, 0.f, 0.f));
			School->SurfaceBreakContextRemaining = 0.1f;
			School->Tick(0.2f);
			TestFalse(TEXT("The cue expires from resident context without leaving memory or world state"),
				CueObserverController->DescribeActionState().Contains(TEXT("Nearby transient wildlife cue")));
			CueObserverController->UnPossess();
			CueObserverController->Destroy();
		}
		MinnowRipple->Destroy();
		School->ScatterRemaining = 0.f;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("MinnowStartleImpact"))) It->Destroy();
	}

	ARavenAgentAIController* FlybyController = World->SpawnActor<ARavenAgentAIController>(Spawn);
	ACharacter* RavenBody = World->SpawnActor<ACharacter>(School->GetActorLocation() + FVector(-300.f, 0.f, 1000.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Raven controller spawned for overhead ecology"), FlybyController) && TestNotNull(TEXT("Raven body spawned for overhead ecology"), RavenBody))
	{
		FlybyController->Possess(RavenBody);
		World->GetTimerManager().ClearAllTimersForObject(FlybyController);
		FlybyController->LocomotionState = ERavenLocomotionState::Flying;
		School->Tick(0.4f);
		TestTrue(TEXT("High flight remains outside the fish school's disturbance height"), FMath::IsNearlyZero(School->ScatterRemaining));
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-3000.f, 0.f, 400.f));
		School->Tick(0.4f);
		TestTrue(TEXT("Distant flight remains outside the Tideglass disturbance radius"), FMath::IsNearlyZero(School->ScatterRemaining));
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-300.f, 0.f, 400.f));
		School->ScatterRemaining = 0.f;
		School->RavenFlybyCooldownRemaining = 0.f;
		School->RavenCheckRemaining = 0.f;
		School->RavenPresenceLatch.Reset();
		School->ScatterSurfaceCueCooldownRemaining = 0.f;
		FVector ExpectedRavenRippleLocation = FVector::ZeroVector;
		NearestFishDistanceSquared = TNumericLimits<float>::Max();
		for (UProceduralMeshComponent* FishBody : School->Fish)
		{
			if (!FishBody) continue;
			const float DistanceSquared = FVector::DistSquared2D(FishBody->GetComponentLocation(), RavenBody->GetActorLocation());
			if (DistanceSquared < NearestFishDistanceSquared)
			{
				NearestFishDistanceSquared = DistanceSquared;
				const FVector FishLocation = FishBody->GetComponentLocation();
				const FVector SafeRippleCenter = School->GetActorTransform().TransformPosition(
					School->ClampToPoolSwimmingBounds(School->GetActorTransform().InverseTransformPosition(FishLocation),
						AIslandTidepoolMinnows::StartleRippleEdgeClearanceCm));
				ExpectedRavenRippleLocation = FVector(SafeRippleCenter.X, SafeRippleCenter.Y,
					School->GetActorLocation().Z + School->GetTideOffsetCm() - 24.f);
			}
		}
		FVector BeforeFlyby = FVector::ZeroVector;
		for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) BeforeFlyby += Minnow->GetRelativeLocation();
		BeforeFlyby /= School->Fish.Num();
		School->Tick(0.4f);
		TestTrue(TEXT("A low raven glide over the shallows briefly startles the school"), School->ScatterRemaining > 1.9f && School->ScatterRemaining <= 2.4f);
		AIslandPoolRippleEffect* RavenStartleRipple = nullptr;
		int32 RavenStartleRippleCount = 0;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
		{
			if (!It->ActorHasTag(TEXT("MinnowStartleImpact"))) continue;
			RavenStartleRipple = *It;
			++RavenStartleRippleCount;
		}
		TestEqual(TEXT("A low Raven pass creates one transient surface cue for the scattering school"), RavenStartleRippleCount, 1);
		if (RavenStartleRipple)
		{
			TestTrue(TEXT("The Raven-triggered cue is tide-locked near the nearest fish and inset from the bank"),
				RavenStartleRipple->GetActorLocation().Equals(ExpectedRavenRippleLocation, 0.1f));
			TestTrue(TEXT("The Raven-triggered ring's full radius clears the pool edge"),
				IsInsideStartleRippleFootprint(RavenStartleRipple->GetActorLocation()));
			School->CheckForNearbyRavenDisturbance();
			int32 RepeatedRavenRippleCount = 0;
			for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
				if (It->ActorHasTag(TEXT("MinnowStartleImpact"))) ++RepeatedRavenRippleCount;
			TestEqual(TEXT("The same Raven approach does not stack overlapping startle cues"), RepeatedRavenRippleCount, 1);
			RavenStartleRipple->Destroy();
		}
		School->Tick(0.7f);
		FVector DuringFlyby = FVector::ZeroVector;
		for (UProceduralMeshComponent* Minnow : School->Fish) if (Minnow) DuringFlyby += Minnow->GetRelativeLocation();
		DuringFlyby /= School->Fish.Num();
		TestTrue(TEXT("Minnows fan away from the low overhead approach"), DuringFlyby.X > BeforeFlyby.X + 80.f);
		School->Tick(3.f);
		TestTrue(TEXT("The school regroups without being repeatedly startled during one close pass"),
			FMath::IsNearlyZero(School->ScatterRemaining) && School->RavenFlybyCooldownRemaining > 0.f);
		FlybyController->LocomotionState = ERavenLocomotionState::Flying;
		School->RavenFlybyCooldownRemaining = 0.f;
		School->ScatterSurfaceCueCooldownRemaining = 0.f;
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-3000.f, 0.f, 400.f));
		School->RavenCheckRemaining = 0.f;
		School->Tick(0.06f);
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-300.f, 0.f, 400.f));
		School->Tick(0.06f);
		TestTrue(TEXT("A one-tick low Raven pass is detected by the per-tick ecology poll"),
			School->ScatterRemaining > 0.f && School->RavenFlybyCooldownRemaining > 0.f);
		School->ScatterRemaining = 0.f;
		School->RavenFlybyCooldownRemaining = 0.f;
		School->RavenCheckRemaining = 0.f;
		School->ScatterSurfaceCueCooldownRemaining = 0.f;
		for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("MinnowStartleImpact"))) It->Destroy();
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-3000.f, 0.f, 400.f));
		FlybyController->LocomotionState = ERavenLocomotionState::Perched;
		School->RavenFlybyCooldownRemaining = 0.f;
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-900.f, 0.f, 100.f));
		School->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A settled raven at the far side of the pool does not disturb the school"),
			FMath::IsNearlyZero(School->ScatterRemaining));
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-300.f, 0.f, 700.f));
		School->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A nearby settled raven well above the shallows does not disturb the school"),
			FMath::IsNearlyZero(School->ScatterRemaining));
		const FVector PerchedRavenLocation = School->GetActorLocation() + FVector(-300.f, 0.f, 100.f);
		RavenBody->SetActorLocation(PerchedRavenLocation);
		School->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A nearby perched raven briefly scatters the minnow school"),
			School->ScatterRemaining > 0.f && School->ScatterRemaining <= 2.4f);
		TestTrue(TEXT("The school fans away from a settled raven"), School->ScatterDirection.X > 0.95f);
		School->ScatterRemaining = 0.f;
		School->RavenFlybyCooldownRemaining = 0.f;
		School->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("A lingering perched raven does not repeatedly scatter the school"),
			FMath::IsNearlyZero(School->ScatterRemaining));
		FlybyController->LocomotionState = ERavenLocomotionState::Flying;
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(0.f, 0.f, 900.f));
		School->CheckForNearbyRavenDisturbance();
		FlybyController->LocomotionState = ERavenLocomotionState::Perched;
		RavenBody->SetActorLocation(PerchedRavenLocation);
		School->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("Changing Raven flight state within the outer ring does not rearm the school"),
			FMath::IsNearlyZero(School->ScatterRemaining));
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-900.f, 0.f, 100.f));
		School->CheckForNearbyRavenDisturbance();
		RavenBody->SetActorLocation(PerchedRavenLocation);
		School->CheckForNearbyRavenDisturbance();
		TestTrue(TEXT("Leaving the outer ring and returning rearms the minnow response"),
			School->ScatterRemaining > 0.f && School->ScatterRemaining <= 2.4f);
		School->ScatterRemaining = 0.f;
		School->RavenFlybyCooldownRemaining = 0.f;
		FlybyController->UnPossess();
	}

	ARavenAgentAIController* ResidentController = World->SpawnActor<ARavenAgentAIController>(Spawn);
	ACharacter* Resident = World->SpawnActor<ACharacter>(School->GetActorLocation() + FVector(0.f, 100.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Resident inspection controller spawned"), ResidentController) && TestNotNull(TEXT("Resident body spawned"), Resident))
	{
		ResidentController->Possess(Resident);
		ResidentController->InspectTarget(TEXT("MinnowSchool"));
		TestTrue(TEXT("A resident can choose to inspect the school through its ordinary wildlife path"),
			ResidentController->DescribeActionState().Contains(TEXT("scattered from your quiet attention")));
		ResidentController->UnPossess();
	}

	Clock->CurrentHour = 19.f;
	Weather->RefreshNightEcology();
	TestTrue(TEXT("The daytime school leaves the habitat after dusk"), School->IsActorBeingDestroyed() && !Weather->DayMinnowSchool.IsValid());
	Clock->CurrentHour = 6.f;
	Weather->RefreshNightEcology();
	TestTrue(TEXT("One school returns at dawn without duplicating"), Weather->DayMinnowSchool.IsValid());
	AIslandTidepoolMinnows* DawnSchool = Weather->DayMinnowSchool.Get();
	Weather->RefreshNightEcology();
	TestTrue(TEXT("Dawn refresh preserves the same single school"), Weather->DayMinnowSchool.Get() == DawnSchool);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
