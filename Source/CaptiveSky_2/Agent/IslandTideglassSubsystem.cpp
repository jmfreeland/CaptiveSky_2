#include "IslandTideglassSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandTideglass, Log, All);

const TCHAR* UIslandTideglassSubsystem::MaterialPath = TEXT("/Game/Materials/M_TideglassPool_GrazingReadable.M_TideglassPool_GrazingReadable");

namespace
{
	constexpr TCHAR TideglassRockMeshPath[] = TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock");
	constexpr TCHAR TideglassRockMaterialPath[] = TEXT("/Game/StarterContent/Props/Materials/M_Rock.M_Rock");
}

bool UIslandTideglassSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UStaticMeshComponent* UIslandTideglassSubsystem::FindPoolSurface(UWorld* World)
{
	if (!World) return nullptr;
	AActor* PoolMarker = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TideglassPool")))
		{
			PoolMarker = *It;
			break;
		}
	}
	if (!PoolMarker) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (FVector::Dist(It->GetActorLocation(), PoolMarker->GetActorLocation()) > 25.f) continue;
		TArray<UStaticMeshComponent*> MeshComponents;
		It->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (UStaticMeshComponent* Mesh : MeshComponents)
		{
			if (!Mesh || !Mesh->GetStaticMesh() || Mesh->GetStaticMesh()->GetName() != TEXT("Sphere")) continue;
			const FVector Scale = Mesh->GetComponentScale();
			if (Scale.X > 2.f && Scale.Y > 2.f && Scale.Z < 0.25f) return Mesh;
		}
	}
	return nullptr;
}

UProceduralMeshComponent* UIslandTideglassSubsystem::CreatePoolSurfaceMesh(UStaticMeshComponent* BlockoutSurface)
{
	if (!IsValid(BlockoutSurface) || !BlockoutSurface->GetStaticMesh() || !BlockoutSurface->GetOwner()) return nullptr;
	UWorld* World = BlockoutSurface->GetWorld();
	if (!World) return nullptr;
	const FBoxSphereBounds Bounds = BlockoutSurface->GetStaticMesh()->GetBounds();
	const float RadiusX = Bounds.BoxExtent.X;
	const float RadiusY = Bounds.BoxExtent.Y;
	if (RadiusX <= KINDA_SMALL_NUMBER || RadiusY <= KINDA_SMALL_NUMBER) return nullptr;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* WaterActor = World->SpawnActor<AActor>(AActor::StaticClass(), BlockoutSurface->GetComponentTransform(), SpawnParameters);
	if (!WaterActor) return nullptr;
	UProceduralMeshComponent* WaterSurface = NewObject<UProceduralMeshComponent>(WaterActor, NAME_None, RF_Transient);
	if (!WaterSurface)
	{
		WaterActor->Destroy();
		return nullptr;
	}
	WaterActor->SetRootComponent(WaterSurface);
	WaterActor->AddInstanceComponent(WaterSurface);
	WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurface->SetCastShadow(false);
	WaterSurface->RegisterComponent();
	WaterActor->SetActorTransform(BlockoutSurface->GetComponentTransform());

	constexpr int32 SegmentCount = 64;
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32> Triangles;
	Vertices.Reserve(1 + 2 * SegmentCount);
	Normals.Reserve(1 + 2 * SegmentCount);
	UVs.Reserve(1 + 2 * SegmentCount);
	Colors.Reserve(1 + 2 * SegmentCount);
	Tangents.Reserve(1 + 2 * SegmentCount);
	Triangles.Reserve(9 * SegmentCount);

	const float WaterHeight = Bounds.BoxExtent.Z;
	const float InnerHeight = WaterHeight - 12.f;
	const float EdgeHeight = WaterHeight - 22.f;
	auto AddVertex = [&](float X, float Y, float Z)
	{
		Vertices.Emplace(X, Y, Z);
		Normals.Emplace(FVector::UpVector);
		UVs.Emplace(0.5f + X / (2.f * RadiusX), 0.5f + Y / (2.f * RadiusY));
		Colors.Emplace(FLinearColor::White);
		Tangents.Emplace(FVector(1.f, 0.f, 0.f), false);
	};

	AddVertex(0.f, 0.f, WaterHeight);
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			const float Angle = 2.f * PI * static_cast<float>(Segment) / SegmentCount;
			const float Shape = 1.f + 0.060f * FMath::Sin(3.f * Angle + 0.45f)
				+ 0.038f * FMath::Sin(5.f * Angle - 1.1f) + 0.024f * FMath::Cos(7.f * Angle + 0.7f);
			const float RadialFraction = Ring == 0 ? 0.62f : Shape;
			const float Z = Ring == 0 ? InnerHeight : EdgeHeight;
			AddVertex(FMath::Cos(Angle) * RadiusX * RadialFraction,
				FMath::Sin(Angle) * RadiusY * RadialFraction, Z);
		}
	}
	for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
	{
		const int32 Next = (Segment + 1) % SegmentCount;
		const int32 Inner = 1 + Segment;
		const int32 InnerNext = 1 + Next;
		const int32 Outer = 1 + SegmentCount + Segment;
		const int32 OuterNext = 1 + SegmentCount + Next;
		Triangles.Add(0);
		Triangles.Add(InnerNext);
		Triangles.Add(Inner);
		Triangles.Add(Inner);
		Triangles.Add(OuterNext);
		Triangles.Add(Outer);
		Triangles.Add(Inner);
		Triangles.Add(InnerNext);
		Triangles.Add(OuterNext);
	}
	WaterSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
	return WaterSurface;
}

bool UIslandTideglassSubsystem::ApplyPoolMaterial(UMaterialInterface* Material)
{
	if (!Material) return false;
	UStaticMeshComponent* PoolSurface = FindPoolSurface(GetWorld());
	if (!PoolSurface) return false;
	UMaterialInstanceDynamic* RuntimeMaterial = UMaterialInstanceDynamic::Create(Material, this);
	if (!RuntimeMaterial) return false;
	TuneReadablePoolMaterial(RuntimeMaterial);
	if (AppliedTo.IsValid() && AppliedTo.Get() == RuntimeSurface.Get())
	{
		RuntimeSurface->SetMaterial(0, RuntimeMaterial);
		return true;
	}
	RestorePoolMaterial();
	RuntimeSurface = CreatePoolSurfaceMesh(PoolSurface);
	if (!RuntimeSurface) return false;
	BlockoutSurface = PoolSurface;
	bBlockoutWasVisible = PoolSurface->IsVisible();
	bBlockoutWasHiddenInGame = PoolSurface->bHiddenInGame;
	PoolSurface->SetVisibility(false);
	PoolSurface->SetHiddenInGame(true);
	RuntimeSurface->SetMaterial(0, RuntimeMaterial);
	AppliedTo = RuntimeSurface;
	return true;
}

void UIslandTideglassSubsystem::TuneReadablePoolMaterial(UMaterialInstanceDynamic* Material)
{
	if (!Material) return;

	// The source asset stays untouched. These transient overrides keep the shallow pool
	// legible as water at noon instead of a flat cyan patch, while retaining a restrained
	// sky-lit edge and stronger, still-small surface swells.
	Material->SetVectorParameterValue(TEXT("CalmPoolColor"), FLinearColor(0.0045f, 0.030f, 0.038f, 1.f));
	Material->SetVectorParameterValue(TEXT("StormPoolColor"), FLinearColor(0.004f, 0.024f, 0.040f, 1.f));
	Material->SetVectorParameterValue(TEXT("EdgeReflectionTint"), FLinearColor(0.022f, 0.070f, 0.083f, 1.f));
	Material->SetScalarParameterValue(TEXT("CalmRoughness"), 0.24f);
	Material->SetScalarParameterValue(TEXT("WeatherRoughness"), 0.38f);
	Material->SetScalarParameterValue(TEXT("Specular"), 0.42f);
	Material->SetScalarParameterValue(TEXT("CalmNormalGain"), 0.92f);
	Material->SetScalarParameterValue(TEXT("WeatherNormalGain"), 1.55f);
	Material->SetScalarParameterValue(TEXT("LongSwellStrength"), 0.50f);
	Material->SetScalarParameterValue(TEXT("ShortChopStrength"), 0.32f);
}

void UIslandTideglassSubsystem::RestorePoolMaterial()
{
	if (BlockoutSurface)
	{
		BlockoutSurface->SetVisibility(bBlockoutWasVisible);
		BlockoutSurface->SetHiddenInGame(bBlockoutWasHiddenInGame);
	}
	if (RuntimeSurface)
	{
		if (AActor* Owner = RuntimeSurface->GetOwner()) Owner->Destroy();
	}
	RuntimeSurface = nullptr;
	BlockoutSurface = nullptr;
	AppliedTo.Reset();
}

void UIslandTideglassSubsystem::ApplyShoreStonePresentation()
{
	UWorld* World = GetWorld();
	if (!World || !ShoreStonePresentationActors.IsEmpty()) return;

	AActor* PoolMarker = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
		if (It->ActorHasTag(TEXT("TideglassPool"))) { PoolMarker = *It; break; }
	if (!PoolMarker) return;

	TArray<AStaticMeshActor*> StoneProxies;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		const float DistanceToPool = FVector::Distance(It->GetActorLocation(), PoolMarker->GetActorLocation());
		if (DistanceToPool > 500.f) continue;
		UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		if (!Mesh || !Mesh->GetStaticMesh() || Mesh->GetStaticMesh()->GetPathName() != TEXT("/Engine/BasicShapes/Sphere.Sphere")) continue;
		// Editor labels are not stable UObject names in Game; identify these four authored proxies
		// by their small sphere bounds while excluding the much larger flattened pool surface.
		if (Mesh->Bounds.BoxExtent.GetMax() > 25.f) continue;
		StoneProxies.Add(*It);
	}

	// The saved four cardinal proxies remain authoritative for collision and navigation.
	// Only their play-session visuals are exchanged, and only when the complete set is found.
	if (StoneProxies.Num() != 4)
	{
		UE_LOG(LogIslandTideglass, Warning,
			TEXT("Tideglass shore-stone presentation skipped: matched %d of the expected 4 proxies near marker %s."),
			StoneProxies.Num(), *PoolMarker->GetName());
		return;
	}
	UStaticMesh* RockMesh = LoadObject<UStaticMesh>(nullptr, TideglassRockMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!RockMesh)
	{
		UE_LOG(LogIslandTideglass, Warning, TEXT("Tideglass shore-stone presentation skipped: could not load %s."), TideglassRockMeshPath);
		return;
	}
	UMaterialInterface* RockMaterial = LoadObject<UMaterialInterface>(nullptr, TideglassRockMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	const FVector RockExtent = RockMesh->GetBounds().BoxExtent;
	if (RockExtent.X <= UE_SMALL_NUMBER || RockExtent.Y <= UE_SMALL_NUMBER || RockExtent.Z <= UE_SMALL_NUMBER) return;

	HiddenShoreStoneProxies.Reset();
	PreviousShoreStoneHiddenStates.Reset();
	ShoreStonePresentationActors.Reset();
	for (AStaticMeshActor* Proxy : StoneProxies)
	{
		HiddenShoreStoneProxies.Add(Proxy);
		PreviousShoreStoneHiddenStates.Add(Proxy->IsHidden());
	}

	for (AStaticMeshActor* Proxy : StoneProxies)
	{
		UStaticMeshComponent* ProxyMesh = Proxy->GetStaticMeshComponent();
		const FVector TargetExtent = ProxyMesh->Bounds.BoxExtent;
		const FVector RockScale(TargetExtent.X / RockExtent.X, TargetExtent.Y / RockExtent.Y, TargetExtent.Z / RockExtent.Z);
		const FTransform VisualTransform(ProxyMesh->GetComponentRotation(), ProxyMesh->Bounds.Origin, RockScale);
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Presentation = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), VisualTransform, Spawn);
		if (!Presentation || !Presentation->GetStaticMeshComponent())
		{
			if (Presentation) Presentation->Destroy();
			RestoreShoreStonePresentation();
			UE_LOG(LogIslandTideglass, Warning, TEXT("Tideglass shore-stone presentation could not create all four transient rocks."));
			return;
		}

		UStaticMeshComponent* Visual = Presentation->GetStaticMeshComponent();
		Visual->SetStaticMesh(RockMesh);
		Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Visual->SetCanEverAffectNavigation(false);
		Visual->SetGenerateOverlapEvents(false);
		Visual->SetCastShadow(true);
		if (RockMaterial) Visual->SetMaterial(0, RockMaterial);
		ShoreStonePresentationActors.Add(Presentation);
		Proxy->SetActorHiddenInGame(true);
	}

	UE_LOG(LogIslandTideglass, Log, TEXT("Tideglass shore stones use four transient rough-rock visuals; original proxy collision and map data remain unchanged."));
}

void UIslandTideglassSubsystem::RestoreShoreStonePresentation()
{
	for (int32 Index = 0; Index < HiddenShoreStoneProxies.Num(); ++Index)
		if (AStaticMeshActor* Proxy = HiddenShoreStoneProxies[Index].Get())
			Proxy->SetActorHiddenInGame(PreviousShoreStoneHiddenStates.IsValidIndex(Index) ? PreviousShoreStoneHiddenStates[Index] : false);

	for (const TWeakObjectPtr<AStaticMeshActor>& Actor : ShoreStonePresentationActors)
		if (AStaticMeshActor* Presentation = Actor.Get()) Presentation->Destroy();

	HiddenShoreStoneProxies.Reset();
	PreviousShoreStoneHiddenStates.Reset();
	ShoreStonePresentationActors.Reset();
}

void UIslandTideglassSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bool bHasTideglassMarker = false;
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TideglassPool")))
		{
			bHasTideglassMarker = true;
			break;
		}
	}
	if (!bHasTideglassMarker) return;

	ApplyShoreStonePresentation();

	UMaterialInterface* Material = MaterialOverride
		? MaterialOverride.Get()
		: LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Material)
	{
		UE_LOG(LogIslandTideglass, Warning, TEXT("Tideglass runtime water is unavailable: could not load %s; the white blockout surface remains visible."), MaterialPath);
		return;
	}
	if (!ApplyPoolMaterial(Material))
	{
		UE_LOG(LogIslandTideglass, Warning, TEXT("Tideglass runtime water could not find a flattened sphere beside the TideglassPool marker."));
		return;
	}
	UE_LOG(LogIslandTideglass, Log, TEXT("Tideglass runtime water material applied to the shallow pool."));
}

void UIslandTideglassSubsystem::Deinitialize()
{
	RestorePoolMaterial();
	RestoreShoreStonePresentation();
	Super::Deinitialize();
}
