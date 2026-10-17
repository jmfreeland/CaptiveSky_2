#include "Misc/AutomationTest.h"
#include "Agent/IslandInstancedSkinnedFoliage.h"
#include "Animation/AnimSequenceTransformProviderData.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "DynamicWindData.h"
#include "DynamicWindSkeletalData.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandInstancedSkinnedFoliageProviderTest,
	"CaptiveSky2.Agent.InstancedSkinnedFoliageProvider",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandInstancedSkinnedFoliageProviderTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Transient foliage world is created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandInstancedSkinnedFoliage* Host = World->SpawnActor<AIslandInstancedSkinnedFoliage>(
		FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Instanced-skeletal foliage host spawns"), Host) ||
		!TestNotNull(TEXT("The host owns its instanced-skeletal component"), Host ? Host->Instances.Get() : nullptr))
	{
		return false;
	}

	UAnimSequenceTransformProviderData* Provider = NewObject<UAnimSequenceTransformProviderData>(
		Host->Instances, NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("A transient animation transform-provider fixture is created"), Provider)) return false;

	Host->TransformProvider = Provider;
	Host->RebuildInstances();
	TestTrue(TEXT("Rebuilding the foliage host applies its configured transform provider"),
		Host->Instances->GetTransformProvider() == Provider);

	Host->TransformProvider = nullptr;
	Host->RebuildInstances();
	TestNull(TEXT("Rebuilding after clearing the provider removes the prior component provider"),
		Host->Instances->GetTransformProvider());

	USkeletalMesh* Beech = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_A.Tree_European_Beech_01_A"));
	if (Beech)
	{
		const UDynamicWindSkeletalData* WindData = Cast<UDynamicWindSkeletalData>(
			Beech->GetAssetUserDataOfClass(UDynamicWindSkeletalData::StaticClass()));
		TestNotNull(TEXT("Installed PVE Beech keeps its DynamicWind skeletal profile"), WindData);
		if (WindData && WindData->bIsEnabled)
		{
			Host->SourceMesh = Beech;
			Host->RebuildInstances();
			TestTrue(TEXT("An enabled PVE wind profile automatically selects UE's DynamicWind transform provider"),
				Host->TransformProvider && Host->TransformProvider->IsA<UDynamicWindData>() &&
				Host->Instances->GetTransformProvider() == Host->TransformProvider);
		}
		else if (WindData)
		{
			AddInfo(TEXT("The installed Beech carries DynamicWind data, but it is disabled; the host correctly leaves it in ref pose."));
		}
	}
	else
	{
		AddInfo(TEXT("The installed Beech Megaplant is absent in this test project; the generic provider lifecycle checks still ran."));
	}
	return true;
}
