#include "Misc/AutomationTest.h"
#include "Agent/IslandPoolRippleEffect.h"
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
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandTideglassSurfaceTest, "CaptiveSky2.Agent.IslandTideglass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandTideglassSurfaceTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* PoolWater = LoadObject<UMaterialInterface>(nullptr, UIslandTideglassSubsystem::MaterialPath);
	UMaterialInterface* RippleWater = LoadObject<UMaterialInterface>(nullptr, UIslandTideglassSubsystem::RippleMaterialPath);
	if (PoolWater)
		TestEqual(TEXT("The prototype water remains a stable opaque surface over the shallow blockout mesh"),
			PoolWater->GetBlendMode(), BLEND_Opaque);
	else if (!RippleWater)
		AddInfo(TEXT("Optional generated material is absent; use Scripts/Create-TideglassPoolMaterial.py before play to enable the visual swap."));
	if (RippleWater)
		TestEqual(TEXT("The generated ripple water material remains opaque"), RippleWater->GetBlendMode(), BLEND_Opaque);
	else
		AddInfo(TEXT("Optional material-driven ripple asset is absent; set CAPTIVESKY_TIDEGLASS_MATERIAL_NAME=M_TideglassPool_Ripple and run Scripts/Create-TideglassPoolMaterial.py."));

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
		TArray<AStaticMeshActor*> ShoreStoneProxies;
		if (TestNotNull(TEXT("Synthetic pool surface actor spawns"), SurfaceActor) && TestNotNull(TEXT("Engine sphere mesh is available"), Sphere))
		{
			UStaticMeshComponent* Surface = SurfaceActor->GetStaticMeshComponent();
			Surface->SetStaticMesh(Sphere);
			SurfaceActor->SetActorScale3D(FVector(4.f, 4.f, 0.1f));
			Surface->SetMaterial(0, AuthoredBlockout);

			for (int32 Index = 0; Index < 4; ++Index)
			{
				const float Angle = Index * UE_PI * 0.5f;
				AStaticMeshActor* Proxy = World->SpawnActor<AStaticMeshActor>(
					FVector(FMath::Cos(Angle) * 320.f, FMath::Sin(Angle) * 320.f, 0.f), FRotator::ZeroRotator);
				if (Proxy)
				{
					Proxy->GetStaticMeshComponent()->SetStaticMesh(Sphere);
					Proxy->SetActorScale3D(FVector(0.45f, 0.45f, 0.35f));
					Proxy->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					ShoreStoneProxies.Add(Proxy);
				}
			}
			TestEqual(TEXT("All four synthetic shore-stone collision proxies spawn"), ShoreStoneProxies.Num(), 4);

			TestTrue(TEXT("The subsystem finds the flattened sphere beside the tagged pool"), Tideglass->FindPoolSurface(World) == Surface);
			UMaterialInterface* StartupMaterial = RippleWater ? RippleWater : PoolWater ? PoolWater : PreviewWater;
			Tideglass->MaterialOverride = (RippleWater || PoolWater) ? nullptr : PreviewWater;
			World->BeginPlay();
			TestTrue(TEXT("Normal Game-world BeginPlay applies its default asset or isolated preview fallback"), Tideglass->IsApplied());
			UProceduralMeshComponent* RuntimeWater = Cast<UProceduralMeshComponent>(Tideglass->AppliedTo.Get());
			TestNotNull(TEXT("Play start creates an organic procedural water surface"), RuntimeWater);
			if (RuntimeWater)
			{
				UMaterialInstanceDynamic* RuntimeMaterial = Cast<UMaterialInstanceDynamic>(RuntimeWater->GetMaterial(0));
				TestNotNull(TEXT("The play-session water uses a transient material instance"), RuntimeMaterial);
				TestTrue(TEXT("The transient water material keeps the configured base asset"),
					RuntimeMaterial && RuntimeMaterial->Parent == StartupMaterial);
				if ((RippleWater || PoolWater) && RuntimeMaterial)
				{
					TestTrue(TEXT("The calm pool tint is a deep, low-saturation teal"),
						RuntimeMaterial->K2_GetVectorParameterValue(TEXT("CalmPoolColor")).Equals(FLinearColor(0.0022f, 0.016f, 0.025f, 1.f), 0.001f));
					TestTrue(TEXT("The shallow swells have visible but bounded normal strength"),
						FMath::IsNearlyEqual(RuntimeMaterial->K2_GetScalarParameterValue(TEXT("CalmNormalGain")), 1.35f, 0.001f));
					TestTrue(TEXT("The calmer water softens broad highlights while retaining surface detail"),
						FMath::IsNearlyEqual(RuntimeMaterial->K2_GetScalarParameterValue(TEXT("CalmRoughness")), 0.34f, 0.001f));
				}
				if (RippleWater && RuntimeMaterial)
				{
					const FVector RippleCenter(123.f, -456.f, 78.f);
					AIslandPoolRippleEffect* MinnowImpact = World->SpawnActor<AIslandPoolRippleEffect>(RippleCenter, FRotator::ZeroRotator);
					TestNotNull(TEXT("A transient minnow surface disturbance can spawn beside the water"), MinnowImpact);
					if (MinnowImpact) MinnowImpact->ConfigureAsMinnowStartleImpact();
					TestTrue(TEXT("The water material receives the impulse in world-space XY"),
						RuntimeMaterial->K2_GetVectorParameterValue(TEXT("RippleCenter")).Equals(FLinearColor(RippleCenter.X, RippleCenter.Y, 0.f, 1.f), 0.1f));
					TestTrue(TEXT("The ring-front speed matches the interaction's radius and duration"),
						FMath::IsNearlyEqual(RuntimeMaterial->K2_GetScalarParameterValue(TEXT("RippleSpeedCmPerSecond")), 72.f / 1.15f, 0.01f));
					TestTrue(TEXT("The wave-train spacing follows the pool-scale startle radius"),
						FMath::IsNearlyEqual(RuntimeMaterial->K2_GetScalarParameterValue(TEXT("RippleSpacingCm")), 24.f, 0.01f));
					TestTrue(TEXT("The leading and trailing wave crests stay broad but separated"),
						FMath::IsNearlyEqual(RuntimeMaterial->K2_GetScalarParameterValue(TEXT("RippleWidthCm")), 15.84f, 0.01f));
					TestTrue(TEXT("The interaction sets a finite, restrained ripple amplitude"),
						FMath::IsNearlyEqual(RuntimeMaterial->K2_GetScalarParameterValue(TEXT("RippleAmplitude")), 0.62f, 0.001f));
					if (MinnowImpact) MinnowImpact->Destroy();
				}
				const FProcMeshSection* WaterSection = RuntimeWater->GetProcMeshSection(0);
				TestTrue(TEXT("The water surface follows the saved blockout component transform"),
					RuntimeWater->GetComponentLocation().Equals(Surface->GetComponentLocation(), 1.f));
				TestTrue(TEXT("The water surface contains a 64-segment inner pool and lowered three-ring shore shelf"),
					WaterSection && WaterSection->ProcVertexBuffer.Num() == 257 && WaterSection->ProcIndexBuffer.Num() == 1344);
				if (WaterSection && WaterSection->ProcVertexBuffer.Num() == 257)
				{
					const FVector& WaterEdge = WaterSection->ProcVertexBuffer[65].Position;
					const FVector& ShelfMid = WaterSection->ProcVertexBuffer[129].Position;
					const FVector& ShoreEdge = WaterSection->ProcVertexBuffer[193].Position;
					const FVector& WaterEdgeNormal = WaterSection->ProcVertexBuffer[65].Normal;
					const FVector& ShelfMidNormal = WaterSection->ProcVertexBuffer[129].Normal;
					const FVector& ShoreEdgeNormal = WaterSection->ProcVertexBuffer[193].Normal;
					const float HeightScale = FMath::Abs(Surface->GetComponentScale().Z);
					const float WaterEdgeRadius = FVector2D(WaterEdge.X, WaterEdge.Y).Size();
					const float ShelfMidRadius = FVector2D(ShelfMid.X, ShelfMid.Y).Size();
					const float ShoreEdgeRadius = FVector2D(ShoreEdge.X, ShoreEdge.Y).Size();
					TestTrue(TEXT("The shoreline shelf widens gradually beyond the water edge"),
						WaterEdgeRadius < ShelfMidRadius && ShelfMidRadius < ShoreEdgeRadius);
					TestTrue(TEXT("The inner shelf descends by eight world-space centimetres"),
						FMath::IsNearlyEqual((WaterEdge.Z - ShelfMid.Z) * HeightScale, 8.f, 0.1f));
					TestTrue(TEXT("The outer shoreline descends by thirty world-space centimetres"),
						FMath::IsNearlyEqual((WaterEdge.Z - ShoreEdge.Z) * HeightScale, 30.f, 0.1f));
					TestTrue(TEXT("Generated surface normals reveal the increasingly sloped pool edge"),
						WaterEdgeNormal.Z > ShelfMidNormal.Z && ShelfMidNormal.Z > ShoreEdgeNormal.Z && ShoreEdgeNormal.Z > 0.f);
				}
				TestTrue(TEXT("The pool's top faces use the winding Unreal renders from above"),
					WaterSection && WaterSection->ProcIndexBuffer.Num() >= 3 && WaterSection->ProcIndexBuffer[0] == 0 &&
					WaterSection->ProcIndexBuffer[1] == 2 && WaterSection->ProcIndexBuffer[2] == 1);
				TestTrue(TEXT("The hidden map sphere retains its authored material while transient water is active"), Surface->GetMaterial(0) == AuthoredBlockout);
				TestTrue(TEXT("The generated water surface does not replace the blockout's collision"),
					RuntimeWater->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				TestTrue(TEXT("The old sphere is hidden only while the procedural surface is active"), Surface->bHiddenInGame);
			}

			UStaticMesh* RockMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
			if (RockMesh && ShoreStoneProxies.Num() == 4)
			{
				TArray<TWeakObjectPtr<AStaticMeshActor>> ShoreStoneVisuals;
				TestEqual(TEXT("A complete set of four transient rough-rock visuals is created"),
					Tideglass->ShoreStonePresentationActors.Num(), 4);
				for (int32 Index = 0; Index < ShoreStoneProxies.Num(); ++Index)
				{
					AStaticMeshActor* Proxy = ShoreStoneProxies[Index];
					TestTrue(FString::Printf(TEXT("Shore-stone proxy %d is hidden in game while its presentation is active"), Index),
						Proxy->IsHidden());
					TestEqual(FString::Printf(TEXT("Shore-stone proxy %d keeps its original collision"), Index),
						Proxy->GetStaticMeshComponent()->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
					AStaticMeshActor* Visual = Tideglass->ShoreStonePresentationActors[Index].Get();
					ShoreStoneVisuals.Add(Visual);
					TestNotNull(FString::Printf(TEXT("Shore-stone visual %d exists"), Index), Visual);
					if (!Visual) continue;
					TestTrue(FString::Printf(TEXT("Shore-stone visual %d uses the rough-rock mesh"), Index),
						Visual->GetStaticMeshComponent()->GetStaticMesh() == RockMesh);
					TestEqual(FString::Printf(TEXT("Shore-stone visual %d adds no collision"), Index),
						Visual->GetStaticMeshComponent()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
					TestTrue(FString::Printf(TEXT("Shore-stone visual %d remains centered on its saved proxy"), Index),
						Visual->GetActorLocation().Equals(Proxy->GetStaticMeshComponent()->Bounds.Origin, 1.f));
				}
				Tideglass->RestoreShoreStonePresentation();
				for (int32 Index = 0; Index < ShoreStoneProxies.Num(); ++Index)
				{
					TestFalse(FString::Printf(TEXT("Shore-stone proxy %d is restored at teardown"), Index),
						ShoreStoneProxies[Index]->IsHidden());
					TestFalse(FString::Printf(TEXT("Transient shore-stone visual %d is destroyed at teardown"), Index),
						ShoreStoneVisuals.IsValidIndex(Index) && ShoreStoneVisuals[Index].IsValid());
				}
			}
			else
			{
				AddInfo(TEXT("Starter Content rock mesh is unavailable; reversible shore-stone presentation assertions were skipped."));
			}

			TestTrue(TEXT("Repeated runtime application is safe"), Tideglass->ApplyPoolMaterial(PreviewWater));
			Tideglass->RestorePoolMaterial();
			TestTrue(TEXT("Teardown restores the authored blockout material"), Surface->GetMaterial(0) == AuthoredBlockout);
			TestTrue(TEXT("Teardown restores the blockout component visibility"), Surface->IsVisible());
			TestFalse(TEXT("Teardown restores the blockout sphere visibility"), Surface->bHiddenInGame);
			TestNull(TEXT("Teardown destroys the transient procedural water surface"), Tideglass->RuntimeSurface.Get());
			TestFalse(TEXT("No Tideglass material remains applied after restore"), Tideglass->IsApplied());
			if (PoolWater)
			{
				Tideglass->MaterialOverride = nullptr;
				TestTrue(TEXT("The configured default material path resolves to the generated Lively asset"),
					LoadObject<UMaterialInterface>(nullptr, UIslandTideglassSubsystem::MaterialPath) == PoolWater);
				TestTrue(TEXT("The generated default material applies to the procedural surface"), Tideglass->ApplyPoolMaterial(PoolWater));
				UMaterialInstanceDynamic* DefaultRuntimeMaterial = Tideglass->RuntimeSurface
					? Cast<UMaterialInstanceDynamic>(Tideglass->RuntimeSurface->GetMaterial(0)) : nullptr;
				TestNotNull(TEXT("The default surface uses a transiently tuned material instance"), DefaultRuntimeMaterial);
				if (DefaultRuntimeMaterial)
				{
					TestTrue(TEXT("The transient instance remains based on the generated default asset"), DefaultRuntimeMaterial->Parent == PoolWater);
					TestTrue(TEXT("Repeated application preserves the tested readable tint"),
						DefaultRuntimeMaterial->K2_GetVectorParameterValue(TEXT("CalmPoolColor")).Equals(FLinearColor(0.0022f, 0.016f, 0.025f, 1.f), 0.001f));
				}
				Tideglass->RestorePoolMaterial();
			}
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
