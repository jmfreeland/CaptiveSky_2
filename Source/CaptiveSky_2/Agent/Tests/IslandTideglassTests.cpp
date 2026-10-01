#include "Misc/AutomationTest.h"
#include "Agent/IslandTideglassSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassSurfaceTest, "CaptiveSky2.Agent.IslandTideglass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassSurfaceTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* PoolWater = LoadObject<UMaterialInterface>(nullptr, UIslandTideglassSubsystem::MaterialPath);
	if (PoolWater)
		TestEqual(TEXT("The prototype water remains a stable opaque surface over the shallow blockout mesh"),
			PoolWater->GetBlendMode(), BLEND_Opaque);
	else
		AddInfo(TEXT("Optional generated material is absent; use Scripts/Create-TideglassPoolMaterial.py before play to enable the visual swap."));

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	UIslandTideglassSubsystem* Tideglass = World->GetSubsystem<UIslandTideglassSubsystem>();
	if (!TestNotNull(TEXT("The Tideglass subsystem exists in a game world"), Tideglass))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	UMaterial* AuthoredBlockout = NewObject<UMaterial>(GetTransientPackage(), TEXT("TestTideglassBlockout"));
	UMaterial* PreviewWater = NewObject<UMaterial>(GetTransientPackage(), TEXT("TestTideglassWater"));
	TestFalse(TEXT("No Tideglass marker or surface means no material is applied"), Tideglass->ApplyPoolMaterial(PreviewWater));

	ATargetPoint* Marker = World->SpawnActor<ATargetPoint>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Synthetic Tideglass marker spawns"), Marker))
	{
		Marker->Tags.AddUnique(TEXT("TideglassPool"));
		AStaticMeshActor* SurfaceActor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (TestNotNull(TEXT("Synthetic pool surface actor spawns"), SurfaceActor) && TestNotNull(TEXT("Engine sphere mesh is available"), Sphere))
		{
			UStaticMeshComponent* Surface = SurfaceActor->GetStaticMeshComponent();
			Surface->SetStaticMesh(Sphere);
			SurfaceActor->SetActorScale3D(FVector(4.f, 4.f, 0.1f));
			Surface->SetMaterial(0, AuthoredBlockout);
			TestTrue(TEXT("The subsystem finds the flattened sphere beside the tagged pool"), Tideglass->FindPoolSurface(World) == Surface);
			Tideglass->MaterialOverride = PreviewWater;
			Tideglass->OnWorldBeginPlay(*World);
			TestTrue(TEXT("Play start applies the transient pool-water material"), Tideglass->IsApplied());
			UProceduralMeshComponent* RuntimeWater = Cast<UProceduralMeshComponent>(Tideglass->AppliedTo.Get());
			TestNotNull(TEXT("Play start creates an organic procedural water surface"), RuntimeWater);
			if (RuntimeWater)
			{
				const FProcMeshSection* WaterSection = RuntimeWater->GetProcMeshSection(0);
				TestTrue(TEXT("The water surface follows the saved blockout component transform"),
					RuntimeWater->GetComponentLocation().Equals(Surface->GetComponentLocation(), 1.f));
				TestTrue(TEXT("The water surface contains a 64-segment double-ring mesh"),
					WaterSection && WaterSection->ProcVertexBuffer.Num() == 129 && WaterSection->ProcIndexBuffer.Num() == 576);
				TestTrue(TEXT("The pool's top faces use the winding Unreal renders from above"),
					WaterSection && WaterSection->ProcIndexBuffer.Num() >= 3 && WaterSection->ProcIndexBuffer[0] == 0 &&
					WaterSection->ProcIndexBuffer[1] == 2 && WaterSection->ProcIndexBuffer[2] == 1);
				TestTrue(TEXT("The generated water surface receives the transient material"), RuntimeWater->GetMaterial(0) == PreviewWater);
				TestTrue(TEXT("The generated water surface does not replace the blockout's collision"),
					RuntimeWater->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				TestTrue(TEXT("The old sphere is hidden only while the procedural surface is active"), Surface->bHiddenInGame);
			}
			TestTrue(TEXT("Repeated runtime application is safe"), Tideglass->ApplyPoolMaterial(PreviewWater));
			Tideglass->RestorePoolMaterial();
			TestTrue(TEXT("Teardown restores the authored blockout material"), Surface->GetMaterial(0) == AuthoredBlockout);
			TestTrue(TEXT("Teardown restores the blockout component visibility"), Surface->IsVisible());
			TestFalse(TEXT("Teardown restores the blockout sphere visibility"), Surface->bHiddenInGame);
			TestNull(TEXT("Teardown destroys the transient procedural water surface"), Tideglass->RuntimeSurface.Get());
			TestFalse(TEXT("No Tideglass material remains applied after restore"), Tideglass->IsApplied());
		}
	}

	bool bFoundEditorIsland = false;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* EditorWorld = Context.World();
		if (Context.WorldType != EWorldType::Editor || !EditorWorld || EditorWorld->GetMapName() != TEXT("Island")) continue;
		bFoundEditorIsland = true;
		TestNotNull(TEXT("The saved Island contains the tagged shallow-pool surface"),
			UIslandTideglassSubsystem::FindPoolSurface(EditorWorld));
		break;
	}
	TestTrue(TEXT("The saved Island map is open for the real-surface assertion"), bFoundEditorIsland);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
