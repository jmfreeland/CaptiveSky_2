#include "IslandListeningStonePresentation.h"

#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogListeningStonePresentation, Log, All);

namespace
{
	constexpr TCHAR ListeningStoneRockMeshPath[] = TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock");
	constexpr TCHAR EngineCubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr float ChimeDurationSeconds = 2.8f;
}

AListeningStonePresentation::AListeningStonePresentation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Stones = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StoneMonoliths"));
	Stones->SetupAttachment(RootComponent);
	Stones->SetMobility(EComponentMobility::Movable);
	Stones->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stones->SetCanEverAffectNavigation(false);
	Stones->SetGenerateOverlapEvents(false);
	Stones->SetCastShadow(true);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(
			*FString::Printf(TEXT("StoneResonanceLight_%d"), Index));
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensity(0.f);
		Light->SetAttenuationRadius(520.f);
		Light->SetSourceRadius(28.f);
		Light->SetLightColor(FLinearColor(0.48f, 0.76f, 1.f));
		Light->SetCastShadows(false);
		Light->SetVisibility(false);
		ResonanceLights.Add(Light);
	}
}

int32 AListeningStonePresentation::GetStoneCount() const
{
	return Stones ? Stones->GetInstanceCount() : 0;
}

bool AListeningStonePresentation::ShouldResonateForWind(float PreviousSpeed, float CurrentSpeed)
{
	return FMath::IsFinite(PreviousSpeed) && FMath::IsFinite(CurrentSpeed) &&
		CurrentSpeed >= 90.f && CurrentSpeed - PreviousSpeed >= 45.f;
}

bool AListeningStonePresentation::BuildStoneForms(UStaticMesh* RockMesh, const FTransform& MarkerTransform,
	const TArray<AStaticMeshActor*>& Proxies)
{
	if (!Stones || !RockMesh || Proxies.Num() != 3 || ResonanceLights.Num() != 3) return false;
	const FVector RockExtent = RockMesh->GetBounds().BoxExtent;
	if (RockExtent.IsNearlyZero()) return false;

	Stones->SetStaticMesh(RockMesh);
	Stones->ClearInstances();
	for (int32 Index = 0; Index < Proxies.Num(); ++Index)
	{
		const AStaticMeshActor* Proxy = Proxies[Index];
		const UStaticMeshComponent* ProxyMesh = Proxy ? Proxy->GetStaticMeshComponent() : nullptr;
		if (!ProxyMesh || !ProxyMesh->GetStaticMesh()) return false;

		const FBoxSphereBounds& Bounds = ProxyMesh->Bounds;
		const FVector Center = MarkerTransform.InverseTransformPosition(Bounds.Origin);
		const FVector WorldExtent = Bounds.BoxExtent;
		const FVector LocalExtent = MarkerTransform.InverseTransformVectorNoScale(WorldExtent).GetAbs();
		const FQuat LocalRotation = MarkerTransform.InverseTransformRotation(Proxy->GetActorQuat());
		const FVector Scale(LocalExtent.X / RockExtent.X, LocalExtent.Y / RockExtent.Y, LocalExtent.Z / RockExtent.Z);
		Stones->AddInstance(FTransform(LocalRotation, Center, Scale));
		ResonanceLights[Index]->SetRelativeLocation(Center);
	}
	return GetStoneCount() == 3;
}

void AListeningStonePresentation::BeginResonance(float WindSpeed)
{
	SampledWindSpeed = FMath::IsFinite(WindSpeed) ? FMath::Clamp(WindSpeed, 0.f, 300.f) : 0.f;
	ResonanceElapsed = 0.f;
	bIsResonating = true;
	LastAmbientChimeAt = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAmbientChimeAt;
	SetActorTickEnabled(true);
	for (UPointLightComponent* Light : ResonanceLights)
	{
		if (Light) Light->SetVisibility(true);
	}
}

void AListeningStonePresentation::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
}

void AListeningStonePresentation::CheckForNaturalGust(float DeltaSeconds)
{
	AmbientWindCheckAccumulator += FMath::Max(0.f, DeltaSeconds);
	if (AmbientWindCheckAccumulator < 1.f) return;
	AmbientWindCheckAccumulator = FMath::Fmod(AmbientWindCheckAccumulator, 1.f);
	if (!Weather.IsValid() || !GetWorld()) return;

	ObserveAmbientWind(Weather->GetLocalWind(GetActorLocation(), this).Size2D());
}

void AListeningStonePresentation::ObserveAmbientWind(float CurrentSpeed)
{
	CurrentSpeed = FMath::IsFinite(CurrentSpeed) ? FMath::Clamp(CurrentSpeed, 0.f, 300.f) : 0.f;
	const bool bGustOnset = bHasAmbientWindSample && ShouldResonateForWind(LastAmbientWindSpeed, CurrentSpeed);
	LastAmbientWindSpeed = CurrentSpeed;
	bHasAmbientWindSample = true;
	if (!bGustOnset || GetResonanceRemaining() > 0.f || GetWorld()->GetTimeSeconds() - LastAmbientChimeAt < 45.0)
		return;

	BeginNaturalResonance(CurrentSpeed);
}

void AListeningStonePresentation::BeginNaturalResonance(float WindSpeed)
{
	BeginResonance(WindSpeed);
	UWorld* World = GetWorld();
	if (!World) return;
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(GetActorLocation(), GetActorRotation(), Spawn))
		Chime->BeginChime(WindSpeed);
}

void AListeningStonePresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckForNaturalGust(DeltaSeconds);
	if (!bIsResonating) return;
	ResonanceElapsed += FMath::Max(0.f, DeltaSeconds);
	if (ResonanceElapsed >= ResonanceDuration)
	{
		for (UPointLightComponent* Light : ResonanceLights)
		{
			if (Light)
			{
				Light->SetIntensity(0.f);
				Light->SetVisibility(false);
			}
		}
		ResonanceElapsed = 0.f;
		bIsResonating = false;
		return;
	}

	const float Envelope = FMath::Exp(-0.82f * ResonanceElapsed) * FMath::Clamp(1.f - ResonanceElapsed / ResonanceDuration, 0.f, 1.f);
	const float WindLift = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 300.f), FVector2D(0.88f, 1.12f), SampledWindSpeed);
	for (int32 Index = 0; Index < ResonanceLights.Num(); ++Index)
	{
		UPointLightComponent* Light = ResonanceLights[Index];
		if (!Light) continue;
		const float Pulse = 0.68f + 0.32f * FMath::Sin((ResonanceElapsed * 2.8f + Index * 0.31f) * 2.f * PI);
		Light->SetIntensity(480.f * WindLift * Envelope * Pulse);
	}
}

bool UIslandListeningStonePresentationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandListeningStonePresentationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ApplyPresentation(&InWorld);
}

void UIslandListeningStonePresentationSubsystem::Deinitialize()
{
	RestorePresentation();
	Super::Deinitialize();
}

bool UIslandListeningStonePresentationSubsystem::FindStoneProxies(UWorld* World, AActor*& OutMarker,
	TArray<AStaticMeshActor*>& OutProxies)
{
	OutMarker = nullptr;
	OutProxies.Reset();
	if (!World) return false;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark")))
		{
			if (OutMarker) return false;
			OutMarker = *It;
		}
	}
	if (!OutMarker) return false;

	const FTransform MarkerTransform = OutMarker->GetActorTransform();
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* MeshComponent = It->GetStaticMeshComponent();
		const UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		if (!Mesh || Mesh->GetPathName() != EngineCubeMeshPath) continue;

		const FVector Local = MarkerTransform.InverseTransformPosition(It->GetActorLocation());
		const FVector Extent = MeshComponent->Bounds.BoxExtent;
		if (FMath::Abs(Local.X) > 500.f || FMath::Abs(Local.Y) > 500.f || FMath::Abs(Local.Z) > 350.f ||
			Extent.Z < 90.f || Extent.Z > 180.f || Extent.X > 100.f || Extent.Y > 100.f) continue;
		OutProxies.Add(*It);
	}

	if (OutProxies.Num() != 3)
	{
		UE_LOG(LogListeningStonePresentation, Warning, TEXT("Listening Stones presentation skipped: expected 3 nearby cube proxies, found %d."), OutProxies.Num());
		OutProxies.Reset();
		return false;
	}
	OutProxies.Sort([&MarkerTransform](const AStaticMeshActor& A, const AStaticMeshActor& B)
	{
		const FVector LocalA = MarkerTransform.InverseTransformPosition(A.GetActorLocation());
		const FVector LocalB = MarkerTransform.InverseTransformPosition(B.GetActorLocation());
		return LocalA.X == LocalB.X ? LocalA.Y < LocalB.Y : LocalA.X < LocalB.X;
	});
	return true;
}

void UIslandListeningStonePresentationSubsystem::NotifyChime(float WindSpeed)
{
	if (AListeningStonePresentation* Presentation = PresentationActor.Get()) Presentation->BeginResonance(WindSpeed);
}

void UIslandListeningStonePresentationSubsystem::ApplyPresentation(UWorld* World)
{
	if (!World || PresentationActor.IsValid()) return;
	AActor* Marker = nullptr;
	TArray<AStaticMeshActor*> Proxies;
	if (!FindStoneProxies(World, Marker, Proxies)) return;

	UStaticMesh* RockMesh = LoadObject<UStaticMesh>(nullptr, ListeningStoneRockMeshPath);
	if (!RockMesh)
	{
		UE_LOG(LogListeningStonePresentation, Warning, TEXT("Listening Stones rock mesh did not load: %s"), ListeningStoneRockMeshPath);
		return;
	}

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AListeningStonePresentation* Presentation = World->SpawnActor<AListeningStonePresentation>(Marker->GetActorLocation(), Marker->GetActorRotation(), Spawn);
	if (!Presentation || !Presentation->BuildStoneForms(RockMesh, Marker->GetActorTransform(), Proxies))
	{
		if (Presentation) Presentation->Destroy();
		UE_LOG(LogListeningStonePresentation, Warning, TEXT("Listening Stones could not build three fitted rock forms."));
		return;
	}

	HiddenProxies.Reset();
	PreviousProxyVisibility.Reset();
	for (AStaticMeshActor* Proxy : Proxies)
	{
		HiddenProxies.Add(Proxy);
		PreviousProxyVisibility.Add(Proxy->IsHidden());
		Proxy->SetActorHiddenInGame(true);
	}
	PresentationActor = Presentation;
	UE_LOG(LogListeningStonePresentation, Log, TEXT("Three cube visuals now present as transient collisionless rock monoliths; map proxies remain unchanged."));
}

void UIslandListeningStonePresentationSubsystem::RestorePresentation()
{
	for (int32 Index = 0; Index < HiddenProxies.Num(); ++Index)
	{
		if (AStaticMeshActor* Proxy = HiddenProxies[Index].Get())
			Proxy->SetActorHiddenInGame(PreviousProxyVisibility.IsValidIndex(Index) ? PreviousProxyVisibility[Index] : false);
	}
	HiddenProxies.Reset();
	PreviousProxyVisibility.Reset();
	if (AListeningStonePresentation* Presentation = PresentationActor.Get()) Presentation->Destroy();
	PresentationActor.Reset();
}
