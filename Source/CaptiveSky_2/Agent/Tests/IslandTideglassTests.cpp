#include "Misc/AutomationTest.h"
#include "Agent/IslandTideglassSubsystem.h"
#include "Components/StaticMeshComponent.h"
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
			TestTrue(TEXT("The pool surface receives the transient material"), Surface->GetMaterial(0) == PreviewWater);
			TestTrue(TEXT("Repeated runtime application is safe"), Tideglass->ApplyPoolMaterial(PreviewWater));
			Tideglass->RestorePoolMaterial();
			TestTrue(TEXT("Teardown restores the authored blockout material"), Surface->GetMaterial(0) == AuthoredBlockout);
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
