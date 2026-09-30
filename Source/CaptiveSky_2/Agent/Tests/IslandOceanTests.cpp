#include "Misc/AutomationTest.h"
#include "IslandOceanSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/Material.h"

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

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
