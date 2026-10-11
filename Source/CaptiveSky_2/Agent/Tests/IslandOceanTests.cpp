#include "Misc/AutomationTest.h"
#include "IslandOceanSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "CaptiveSky_2Character.h"
#include "IslandOceanTestCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterSubsystem.h"
#include "WaterZoneActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandOceanTest, "CaptiveSky2.Agent.IslandOcean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandOceanTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
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
			AIslandOceanTestCharacter* WaterPlayer = World->SpawnActor<AIslandOceanTestCharacter>(
				FVector(0.f, 0.f, SurfaceLocation.Z + 90.f), FRotator::ZeroRotator);
			if (TestNotNull(TEXT("The player for the real surface-query check spawns"), WaterPlayer))
			{
				WaterPlayer->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
				WaterPlayer->Tick(1.f / 60.f);
				TestTrue(TEXT("A falling player enters surface swimming from the real ocean query"), WaterPlayer->IsSurfaceSwimming());
			}

			AIslandOceanTestCharacter* GroundedWaterPlayer = World->SpawnActor<AIslandOceanTestCharacter>(
				FVector(0.f, 0.f, SurfaceLocation.Z + 96.f), FRotator::ZeroRotator);
			if (TestNotNull(TEXT("The player for the grounded water-contact check spawns"), GroundedWaterPlayer))
			{
				GroundedWaterPlayer->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				GroundedWaterPlayer->Tick(1.f / 60.f);
				TestTrue(TEXT("A grounded player at the ocean surface enters swimming instead of walking across water"),
					GroundedWaterPlayer->IsSurfaceSwimming());
			}
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
