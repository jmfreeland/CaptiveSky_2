#include "Misc/AutomationTest.h"
#include "IslandOceanSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "CaptiveSky_2Character.h"
#include "IslandPoolRippleEffect.h"
#include "IslandOceanTestCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterSubsystem.h"
#include "WaterZoneActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandOceanTest, "CaptiveSky2.Agent.IslandOcean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandOceanTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandOceanSubsystem* Ocean = World->GetSubsystem<UIslandOceanSubsystem>();
	if (!TestNotNull(TEXT("The ocean subsystem exists in a game world"), Ocean))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	UMaterial* Water = NewObject<UMaterial>(GetTransientPackage(), TEXT("TestWater"));
	UMaterial* Flat = NewObject<UMaterial>(GetTransientPackage(), TEXT("TestFlat"));
	TestFalse(TEXT("No ocean plane, nothing to apply"), Ocean->ApplyOceanMaterial(Water));

	FActorSpawnParameters Spawn;
	Spawn.Name = UIslandOceanSubsystem::OceanActorName;
	AStaticMeshActor* Plane = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("The ocean plane spawns"), Plane))
	{
		Plane->GetStaticMeshComponent()->SetMaterial(0, Flat);
		TestTrue(TEXT("The plane is found by name"), UIslandOceanSubsystem::FindOceanPlane(World) == Plane);
		TestFalse(TEXT("A missing material is refused"), Ocean->ApplyOceanMaterial(nullptr));
		TestTrue(TEXT("The water material is applied"), Ocean->ApplyOceanMaterial(Water));
		TestTrue(TEXT("The plane now carries it"), Plane->GetStaticMeshComponent()->GetMaterial(0) == Water);
		TestTrue(TEXT("Applying twice is harmless"), Ocean->ApplyOceanMaterial(Water));
		Ocean->RestoreOceanMaterial();
		TestTrue(TEXT("The authored material comes back"), Plane->GetStaticMeshComponent()->GetMaterial(0) == Flat);
		TestFalse(TEXT("Nothing stays applied after a restore"), Ocean->IsApplied());
		Plane->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Ocean->OnWorldBeginPlay(*World);
		TestEqual(TEXT("World startup disables stale collision on the hidden legacy ocean plane"),
			Plane->GetStaticMeshComponent()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}

	AIslandOceanTestCharacter* Player = World->SpawnActor<AIslandOceanTestCharacter>();
	if (TestNotNull(TEXT("The player character spawns for water-movement checks"), Player))
	{
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
		const float OriginalFlySpeed = Movement->MaxFlySpeed;
		APlayerController* SwimController = World->SpawnActor<APlayerController>();
		if (TestNotNull(TEXT("The water-movement test controller spawns"), SwimController))
		{
			SwimController->Possess(Player);
			TestTrue(TEXT("The test controller possesses the swimming character"), Player->GetController() == SwimController);
		}

		Player->EnterSurfaceSwimming(100.f);
		TestTrue(TEXT("Entering surface water enables the player-only swim state"), Player->IsSurfaceSwimming());
		TestEqual(TEXT("Surface swimming uses gravity-free movement"), Movement->MovementMode, MOVE_Flying);
		TestEqual(TEXT("Surface swimming applies its tuned speed"), Movement->MaxFlySpeed, 260.f);
		TestEqual(TEXT("The tuned speed is active in flying swim mode"), Movement->GetMaxSpeed(), 260.f);

		if (SwimController)
		{
			SwimController->SetControlRotation(FRotator(0.f, 90.f, 0.f));
			Player->DoMove(0.f, 1.f);
			const FVector RightwardInput = Movement->GetPendingInputVector();
			TestTrue(TEXT("Swimming accepts forward input as horizontal movement"), RightwardInput.Size2D() > 0.99f);
			TestTrue(TEXT("Camera-relative swim input follows a rightward heading"),
				FVector::DotProduct(RightwardInput.GetSafeNormal2D(), FVector::RightVector) > 0.99f);
			TestTrue(TEXT("Horizontal swim input has no unintended vertical component"), FMath::Abs(RightwardInput.Z) < KINDA_SMALL_NUMBER);
			Movement->ConsumeInputVector();

			SwimController->SetControlRotation(FRotator::ZeroRotator);
			Player->DoMove(0.f, 1.f);
			const FVector ForwardInput = Movement->GetPendingInputVector();
			TestTrue(TEXT("Swimming preserves forward movement on the world X axis"),
				ForwardInput.X > 0.99f && FMath::Abs(ForwardInput.Y) < KINDA_SMALL_NUMBER && FMath::Abs(ForwardInput.Z) < KINDA_SMALL_NUMBER);
		}

		Player->LeaveSurfaceSwimming(MOVE_Falling);
		TestFalse(TEXT("Leaving water clears the swim state"), Player->IsSurfaceSwimming());
		TestEqual(TEXT("Leaving water restores the prior flight speed"), Movement->MaxFlySpeed, OriginalFlySpeed);
		TestEqual(TEXT("Leaving water resumes falling"), Movement->MovementMode, MOVE_Falling);

		Player->EnterSurfaceSwimming(100.f);
		Player->Jump();
		TestFalse(TEXT("Jumping exits surface swimming"), Player->IsSurfaceSwimming());
		TestEqual(TEXT("Jumping from the surface returns to falling movement"), Movement->MovementMode, MOVE_Falling);
	}

	AWaterZone* WaterZone = World->SpawnActor<AWaterZone>();
	AWaterBodyOcean* OceanBody = World->SpawnActor<AWaterBodyOcean>(FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator);
	UWaterSubsystem* WaterSubsystem = UWaterSubsystem::GetWaterSubsystem(World);
	UWaterBodyComponent* OceanComponent = OceanBody ? OceanBody->GetWaterBodyComponent() : nullptr;
	if (TestNotNull(TEXT("The transient game world has a Water subsystem"), WaterSubsystem) &&
		TestNotNull(TEXT("The native ocean body has a water component"), OceanComponent) &&
		TestNotNull(TEXT("The native ocean has a water zone"), WaterZone))
	{
		WaterZone->SetZoneExtent(FVector2D(50000.f, 50000.f));
		WaterZone->SetRenderTargetResolution(FIntPoint(256, 256));
		WaterZone->AddWaterBodyComponent(OceanComponent);
		WaterZone->Update();
		WaterSubsystem->SetOceanBodyComponent(OceanComponent);

		FVector SurfaceLocation = FVector::ZeroVector;
		FVector SurfaceNormal = FVector::UpVector;
		FVector WaterVelocity = FVector::ZeroVector;
		float WaterDepth = 0.f;
		const bool bSurfaceQuerySucceeded = OceanComponent->GetWaterSurfaceInfoAtLocation(
			FVector::ZeroVector, SurfaceLocation, SurfaceNormal, WaterVelocity, WaterDepth, false);
		TestTrue(TEXT("The native ocean answers a surface query in its zone"), bSurfaceQuerySucceeded);

		if (bSurfaceQuerySucceeded)
		{
			AActor* ShoreGround = World->SpawnActor<AActor>(FVector(2000.f, 0.f, SurfaceLocation.Z - 60.f), FRotator::ZeroRotator);
			UBoxComponent* ShoreFloor = NewObject<UBoxComponent>(ShoreGround, TEXT("ShoreFloor"));
			ShoreGround->SetRootComponent(ShoreFloor);
			ShoreFloor->SetBoxExtent(FVector(250.f, 250.f, 10.f));
			ShoreFloor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			ShoreFloor->SetCollisionObjectType(ECC_WorldStatic);
			ShoreFloor->SetCollisionResponseToAllChannels(ECR_Block);
			ShoreFloor->RegisterComponent();
			ShoreFloor->SetWorldLocation(FVector(2000.f, 0.f, SurfaceLocation.Z - 60.f));
			FHitResult ShoreFloorProbe;
			TestTrue(TEXT("The transient shallow floor is queryable as world static"),
				World->LineTraceSingleByChannel(ShoreFloorProbe,
					FVector(2000.f, 0.f, SurfaceLocation.Z + 20.f),
					FVector(2000.f, 0.f, SurfaceLocation.Z - 55.f), ECC_WorldStatic));

			AIslandOceanTestCharacter* ShallowWaterPlayer = World->SpawnActor<AIslandOceanTestCharacter>(
				FVector(2000.f, 0.f, SurfaceLocation.Z + 12.f), FRotator::ZeroRotator);
			if (TestNotNull(TEXT("The player for the shallow shore exit check spawns"), ShallowWaterPlayer))
			{
				ShallowWaterPlayer->EnterSurfaceSwimming(SurfaceLocation.Z);
				ShallowWaterPlayer->Tick(1.f / 60.f);
				TestFalse(TEXT("A swimmer wades out when shallow walkable ground is beneath them"),
					ShallowWaterPlayer->IsSurfaceSwimming());
				TestEqual(TEXT("A shallow shore exit restores walking movement"),
					ShallowWaterPlayer->GetCharacterMovement()->MovementMode, MOVE_Walking);
				TestTrue(TEXT("The shallow shore exit places the capsule above the floor"),
					ShallowWaterPlayer->GetActorLocation().Z - ShallowWaterPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() >=
					ShoreFloor->GetComponentLocation().Z + ShoreFloor->GetScaledBoxExtent().Z);
			}

			AIslandOceanTestCharacter* WaterPlayer = World->SpawnActor<AIslandOceanTestCharacter>(
				FVector(0.f, 0.f, SurfaceLocation.Z + 90.f), FRotator::ZeroRotator);
			if (TestNotNull(TEXT("The player for the real surface-query check spawns"), WaterPlayer))
			{
				WaterPlayer->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
				WaterPlayer->Tick(1.f / 60.f);
				TestTrue(TEXT("A falling player enters surface swimming from the real ocean query"), WaterPlayer->IsSurfaceSwimming());

				TArray<AIslandPoolRippleEffect*> SwimWakes;
				for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
					if (It->ActorHasTag(TEXT("PlayerSwimImpact"))) SwimWakes.Add(*It);
				TestEqual(TEXT("A stationary swimmer does not emit a wake"), SwimWakes.Num(), 0);

				WaterPlayer->GetCharacterMovement()->Velocity = FVector(120.f, 0.f, 0.f);
				WaterPlayer->Tick(1.f / 60.f);
				TestTrue(TEXT("The open-water swimmer remains in surface-swim mode while moving"),
					WaterPlayer->IsSurfaceSwimming());
				TestTrue(TEXT("The open-water swim speed remains above the wake threshold"),
					WaterPlayer->GetCharacterMovement()->Velocity.Size2D() >= 80.f);
				for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
					if (It->ActorHasTag(TEXT("PlayerSwimImpact"))) SwimWakes.AddUnique(*It);
				TestEqual(TEXT("Moving across the ocean leaves one short water wake"), SwimWakes.Num(), 1);
				if (SwimWakes.Num() == 1)
				{
					TestTrue(TEXT("The wake trails behind the player's swimming direction"),
						SwimWakes[0]->GetActorLocation().X < WaterPlayer->GetActorLocation().X);
					TestTrue(TEXT("The wake's thin procedural ring is visible"),
						SwimWakes[0]->StartleRing && SwimWakes[0]->StartleRing->IsVisible());
					TestTrue(TEXT("The visible wake ring has a surface material"),
						SwimWakes[0]->StartleRing && SwimWakes[0]->StartleRing->GetMaterial(0) != nullptr);
					TestEqual(TEXT("A player wake does not add active point-light draw cost"),
						SwimWakes[0]->PeakLightIntensity, 0.f);
					TestTrue(TEXT("The wake uses the player-only ripple type"),
						SwimWakes[0]->ActorHasTag(TEXT("PlayerSwimImpact")) &&
						!SwimWakes[0]->ActorHasTag(TEXT("MinnowImpact")) &&
						!SwimWakes[0]->ActorHasTag(TEXT("RainImpact")) &&
						!SwimWakes[0]->ActorHasTag(TEXT("WindImpact")));
				}

				WaterPlayer->Tick(1.f / 60.f);
				int32 WakeCountDuringCooldown = 0;
				for (TActorIterator<AIslandPoolRippleEffect> It(World); It; ++It)
					if (It->ActorHasTag(TEXT("PlayerSwimImpact"))) ++WakeCountDuringCooldown;
				TestEqual(TEXT("A swimming wake is rate-limited while the player keeps moving"), WakeCountDuringCooldown, 1);
			}

			AIslandOceanTestCharacter* GroundedWaterPlayer = World->SpawnActor<AIslandOceanTestCharacter>(
				FVector(0.f, 0.f, SurfaceLocation.Z + 96.f), FRotator::ZeroRotator);
			if (TestNotNull(TEXT("The player for the grounded water-contact check spawns"), GroundedWaterPlayer))
			{
				GroundedWaterPlayer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				GroundedWaterPlayer->Tick(1.f / 60.f);
				TestTrue(TEXT("A grounded player at the ocean surface enters swimming instead of walking across water"),
					GroundedWaterPlayer->IsSurfaceSwimming());
				GroundedWaterPlayer->Tick(1.f / 60.f);
				TestTrue(TEXT("A swimmer without shallow ground remains swimming in open water"),
					GroundedWaterPlayer->IsSurfaceSwimming());
			}

			AActor* WadingGround = World->SpawnActor<AActor>(FVector(3000.f, 0.f, SurfaceLocation.Z - 60.f), FRotator::ZeroRotator);
			UBoxComponent* WadingFloor = NewObject<UBoxComponent>(WadingGround, TEXT("WadingFloor"));
			WadingGround->SetRootComponent(WadingFloor);
			WadingFloor->SetBoxExtent(FVector(250.f, 250.f, 10.f));
			WadingFloor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			WadingFloor->SetCollisionObjectType(ECC_WorldStatic);
			WadingFloor->SetCollisionResponseToAllChannels(ECR_Block);
			WadingFloor->RegisterComponent();
			WadingFloor->SetWorldLocation(FVector(3000.f, 0.f, SurfaceLocation.Z - 60.f));
			AIslandOceanTestCharacter* WadingPlayer = World->SpawnActor<AIslandOceanTestCharacter>(
				FVector(3000.f, 0.f, SurfaceLocation.Z + 46.f), FRotator::ZeroRotator);
			if (TestNotNull(TEXT("The player for the shallow-water walking check spawns"), WadingPlayer))
			{
				WadingPlayer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				WadingPlayer->Tick(1.f / 60.f);
				TestFalse(TEXT("A grounded player on shallow submerged terrain does not oscillate into surface swimming"),
					WadingPlayer->IsSurfaceSwimming());
				TestEqual(TEXT("Shallow submerged terrain keeps normal walking movement"),
					WadingPlayer->GetCharacterMovement()->MovementMode, MOVE_Walking);
			}
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
