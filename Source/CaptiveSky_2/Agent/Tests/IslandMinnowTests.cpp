#include "Misc/AutomationTest.h"
#include "AutonomousAgentAIController.h"
#include "IslandDayNight.h"
#include "IslandInteractionUtility.h"
#include "IslandPoolRippleEffect.h"
#include "IslandTideglassSubsystem.h"
#include "IslandTidepoolMinnows.h"
#include "IslandWeather.h"
#include "RavenAgentAIController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

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
	School->ConfigureAppearance();
	Weather->RefreshNightEcology();
	TestTrue(TEXT("Repeated ecology refresh reuses rather than duplicates the school"), Weather->DayMinnowSchool.Get() == School);
	TestTrue(TEXT("The school is wild life, not a landmark, pet, or nest site"),
		School->ActorHasTag(TEXT("IslandLife")) && School->ActorHasTag(TEXT("MinnowSchool")) &&
		!School->ActorHasTag(TEXT("IslandLandmark")) && !School->ActorHasTag(TEXT("RavenNestSite")));
	TestEqual(TEXT("The shallow-water school has a small bounded population"), School->Fish.Num(), 5);
	TestEqual(TEXT("Each fish has one articulated-looking tail fin"), School->Tails.Num(), School->Fish.Num());
	TArray<FLinearColor> FishColors;
	for (int32 Index = 0; Index < School->Fish.Num(); ++Index)
	{
		UStaticMeshComponent* Minnow = School->Fish[Index];
		UStaticMeshComponent* Tail = School->Tails.IsValidIndex(Index) ? School->Tails[Index] : nullptr;
		TestTrue(TEXT("Minnows are visual-only and nonblocking"), Minnow && Minnow->GetStaticMesh() && Minnow->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Minnow->CastShadow);
		TestTrue(TEXT("Tail fins are visible meshes without collision or shadows"), Tail && Tail->GetStaticMesh() && Tail->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Tail->CastShadow);
		TestTrue(TEXT("Each tail fin is positioned beyond the rear of its fish instead of embedded near the center"),
			Tail && Tail->GetRelativeLocation().X <= -40.f);
		UMaterialInstanceDynamic* FishMaterial = Minnow ? Cast<UMaterialInstanceDynamic>(Minnow->GetMaterial(0)) : nullptr;
		if (FishMaterial)
		{
			const FLinearColor FishColor = FishMaterial->K2_GetVectorParameterValue(TEXT("Color"));
			FishColors.Add(FishColor);
			TestTrue(TEXT("Minnow palette stays subdued enough to hold its color in bright shallow-water light"),
				FMath::Max(FishColor.R, FMath::Max(FishColor.G, FishColor.B)) <= 0.4f);
			TestTrue(TEXT("Each tail shares its fish's stable color"), Tail && Tail->GetMaterial(0) == FishMaterial);
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
	const float FishHalfHeight = School->Fish[0]->GetStaticMesh()->GetBounds().BoxExtent.Z * School->Fish[0]->GetRelativeScale3D().Z;
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
		for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) Center += Minnow->GetRelativeLocation();
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
	FString VisitorFact;
	TestTrue(TEXT("Quiet observation is a valid visitor response"), IslandInteractionUtility::Perform(Visitor, School, VisitorFact));
	TestTrue(TEXT("The response truthfully describes scattering without capture or persistence"),
		VisitorFact.Contains(TEXT("scattered from your quiet attention")) && VisitorFact.Contains(TEXT("wild and uncaught")) && VisitorFact.Contains(TEXT("nothing persistent changed")));
	School->Tick(0.7f);
	FVector ScatteredCenter = FVector::ZeroVector;
	for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) ScatteredCenter += Minnow->GetRelativeLocation();
	ScatteredCenter /= School->Fish.Num();
	TestTrue(TEXT("The school visibly fans away from a quiet observer"), ScatteredCenter.X > BeforeScatter.X + 80.f);
	School->Tick(2.f);
	FVector RegroupedCenter = FVector::ZeroVector;
	for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) RegroupedCenter += Minnow->GetRelativeLocation();
	RegroupedCenter /= School->Fish.Num();
	TestTrue(TEXT("Minnows naturally regroup within their small local habitat"), RegroupedCenter.Size2D() < 180.f && FMath::IsNearlyZero(School->ScatterRemaining));

	Habitat->Tags.Add(TEXT("IslandLandmark"));
	FVector CircleCenter = FVector::ZeroVector;
	float OriginalCircleRadius = 0.f;
	for (UStaticMeshComponent* Minnow : School->Fish)
	{
		if (!Minnow) continue;
		CircleCenter += Minnow->GetRelativeLocation();
	}
	CircleCenter /= School->Fish.Num();
	for (UStaticMeshComponent* Minnow : School->Fish)
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
	for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) CircleCenter += Minnow->GetRelativeLocation();
	CircleCenter /= School->Fish.Num();
	float ExpandedCircleRadius = 0.f;
	for (UStaticMeshComponent* Minnow : School->Fish)
		if (Minnow) ExpandedCircleRadius += FVector::Dist2D(Minnow->GetRelativeLocation(), CircleCenter);
	ExpandedCircleRadius /= School->Fish.Num();
	TestTrue(TEXT("The school visibly widens rather than fleeing the pool"), ExpandedCircleRadius > OriginalCircleRadius * 1.4f);
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
	for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) CircleCenter += Minnow->GetRelativeLocation();
	CircleCenter /= School->Fish.Num();
	float PreWindCircleRadius = 0.f;
	for (UStaticMeshComponent* Minnow : School->Fish)
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
		for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) CircleCenter += Minnow->GetRelativeLocation();
		CircleCenter /= School->Fish.Num();
		ExpandedCircleRadius = 0.f;
		for (UStaticMeshComponent* Minnow : School->Fish)
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
		const UStaticMeshComponent* SourceFish = School->Fish[0];
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
	}

	ARavenAgentAIController* FlybyController = World->SpawnActor<ARavenAgentAIController>(Spawn);
	ACharacter* RavenBody = World->SpawnActor<ACharacter>(School->GetActorLocation() + FVector(-300.f, 0.f, 1000.f), FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Raven controller spawned for overhead ecology"), FlybyController) && TestNotNull(TEXT("Raven body spawned for overhead ecology"), RavenBody))
	{
		FlybyController->Possess(RavenBody);
		FlybyController->LocomotionState = ERavenLocomotionState::Flying;
		School->Tick(0.4f);
		TestTrue(TEXT("High flight remains outside the fish school's disturbance height"), FMath::IsNearlyZero(School->ScatterRemaining));
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-3000.f, 0.f, 400.f));
		School->Tick(0.4f);
		TestTrue(TEXT("Distant flight remains outside the Tideglass disturbance radius"), FMath::IsNearlyZero(School->ScatterRemaining));
		RavenBody->SetActorLocation(School->GetActorLocation() + FVector(-300.f, 0.f, 400.f));
		FVector BeforeFlyby = FVector::ZeroVector;
		for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) BeforeFlyby += Minnow->GetRelativeLocation();
		BeforeFlyby /= School->Fish.Num();
		School->Tick(0.4f);
		TestTrue(TEXT("A low raven glide over the shallows briefly startles the school"), School->ScatterRemaining > 1.9f && School->ScatterRemaining <= 2.4f);
		School->Tick(0.7f);
		FVector DuringFlyby = FVector::ZeroVector;
		for (UStaticMeshComponent* Minnow : School->Fish) if (Minnow) DuringFlyby += Minnow->GetRelativeLocation();
		DuringFlyby /= School->Fish.Num();
		TestTrue(TEXT("Minnows fan away from the low overhead approach"), DuringFlyby.X > BeforeFlyby.X + 80.f);
		School->Tick(3.f);
		TestTrue(TEXT("The school regroups without being repeatedly startled during one close pass"),
			FMath::IsNearlyZero(School->ScatterRemaining) && School->RavenFlybyCooldownRemaining > 0.f);
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
