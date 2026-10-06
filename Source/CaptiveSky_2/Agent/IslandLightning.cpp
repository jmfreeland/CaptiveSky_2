#include "IslandLightning.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundWaveProcedural.h"
#include "UObject/ConstructorHelpers.h"

AIslandLightning::AIslandLightning()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(RootComponent);
	Flash->SetMobility(EComponentMobility::Movable);
	Flash->SetCastShadows(false);
	// A broad, soft wash rather than a physically falling-off point: it stands in for lit cloud.
	Flash->bUseInverseSquaredFalloff = false;
	Flash->LightFalloffExponent = 1.6f;
	Flash->SetAttenuationRadius(300000.f);
	Flash->SetLightColor(FLinearColor(0.78f, 0.84f, 1.f));
	Flash->SetIntensity(0.f);

	Bolt = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bolt"));
	Bolt->SetupAttachment(RootComponent);
	Bolt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bolt->SetCastShadow(false);
	Bolt->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Segment(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Segment.Succeeded()) Bolt->SetStaticMesh(Segment.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (Glow.Succeeded()) Bolt->SetMaterial(0, Glow.Object);

	Thunder = CreateDefaultSubobject<UAudioComponent>(TEXT("Thunder"));
	Thunder->SetupAttachment(RootComponent);
	Thunder->bAutoActivate = false;
	Thunder->bAllowSpatialization = false;

	PrimaryActorTick.bCanEverTick = true;
	Tags.AddUnique(TEXT("IslandTransientEffect"));
	Tags.AddUnique(TEXT("Lightning"));
}

float AIslandLightning::FlashAt(float T)
{
	// Leader, main return stroke, and a fainter restrike, each a quick rise and fall.
	auto Pulse = [T](float Start, float Width, float Peak) { return T < Start || T > Start + Width ? 0.f : Peak * FMath::Sin((T - Start) / Width * PI); };
	return FMath::Clamp(Pulse(0.f, 0.06f, 0.35f) + Pulse(0.09f, 0.12f, 1.f) + Pulse(0.28f, 0.14f, 0.55f), 0.f, 1.f);
}

bool AIslandLightning::HasBolt() const
{
	return Bolt->GetInstanceCount() > 0;
}

bool AIslandLightning::HasThunderReached(const FVector& ListenerLocation) const
{
	if (!bStrikeIssued) return false;
	const float ListenerDelay = FVector::Dist2D(StrikeGroundLocation, ListenerLocation) / SoundSpeed;
	return Elapsed >= ListenerDelay;
}

void AIslandLightning::Strike(const FVector& Ground, const FVector& Listener, int32 Seed)
{
	StrikeSeed = Seed;
	StrikeGroundLocation = Ground;
	bStrikeIssued = true;
	FRandomStream Random(Seed);
	const float Distance = FVector::Dist2D(Ground, Listener);
	// The flash lights the sky between the strike and the viewer, well above the ground.
	SetActorLocation(FMath::Lerp(Listener, Ground, 0.35f) + FVector(0.f, 0.f, 150000.f));
	Flash->SetIntensity(0.f);

	Bolt->ClearInstances();
	if (Distance <= BoltVisibleWithin)
	{
		// A jagged path from cloud base to the ground, built from thin cylinder segments.
		FVector Point = Ground + FVector(0.f, 0.f, 120000.f);
		constexpr int32 Segments = 12;
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const float Remaining = static_cast<float>(Segments - Index);
			FVector Next = Point + (Ground - Point) / Remaining + FVector(Random.FRandRange(-5000.f, 5000.f), Random.FRandRange(-5000.f, 5000.f), 0.f) * (Index + 1 < Segments ? 1.f : 0.f);
			const FVector Step = Next - Point;
			const FVector LocalCentre = GetActorTransform().InverseTransformPosition((Point + Next) * 0.5f);
			const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Step.GetSafeNormal());
			Bolt->AddInstance(FTransform(Rotation, LocalCentre, FVector(1.6f, 1.6f, Step.Size() / 100.f)), false);
			Point = Next;
		}
	}
	Bolt->SetVisibility(false);

	// Sound travels about a kilometre every three seconds; distant thunder is a quieter rumble.
	ThunderDelay = Distance / SoundSpeed;
	ThunderVolume = FMath::Clamp(1.f - Distance / 900000.f, 0.08f, 1.f) * 0.55f;
	Lifetime = ThunderDelay + 6.f;
	Elapsed = 0.f;
	bThundered = false;
}

void AIslandLightning::StartThunder()
{
	bThundered = true;
	constexpr int32 SampleRate = 24000;
	constexpr float Seconds = 5.f;
	ThunderWave = NewObject<USoundWaveProcedural>(this, TEXT("GeneratedThunder"));
	if (!ThunderWave || !Thunder) return;
	ThunderWave->SetSampleRate(SampleRate);
	ThunderWave->NumChannels = 1;
	ThunderWave->Duration = Seconds;
	ThunderWave->bLooping = false;
	ThunderWave->SoundGroup = SOUNDGROUP_Effects;
	FRandomStream Random(StrikeSeed ^ 0x5A17);
	const bool bClose = ThunderDelay < 3.f;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(static_cast<int32>(SampleRate * Seconds));
	float Brown = 0.f, Rumble = 0.f;
	for (int32 Index = 0; Index < Samples.Num(); ++Index)
	{
		const float T = Index / static_cast<float>(SampleRate);
		const float White = Random.FRandRange(-1.f, 1.f);
		Brown = FMath::Clamp(Brown * 0.996f + White * 0.06f, -1.f, 1.f);
		Rumble += (Brown - Rumble) * 0.02f;
		// A close strike cracks first; every strike then rolls and fades unevenly.
		const float Crack = bClose ? White * FMath::Exp(-T / 0.12f) * 0.8f : 0.f;
		const float Roll = Rumble * 3.f * FMath::Min(1.f, T / 0.08f) * FMath::Exp(-T / 1.6f) * (0.75f + 0.25f * FMath::Sin(T * 3.1f + Random.FRandRange(0.f, 0.2f)));
		Samples[Index] = static_cast<int16>(FMath::Clamp(Crack + Roll, -1.f, 1.f) * 32767.f);
	}
	ThunderWave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	Thunder->SetSound(ThunderWave);
	Thunder->VolumeMultiplier = ThunderVolume;
	Thunder->Play();
}

void AIslandLightning::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += FMath::Max(0.f, DeltaSeconds);
	const float Brightness = FlashAt(Elapsed);
	Flash->SetIntensity(Brightness * 60.f);
	Bolt->SetVisibility(Brightness > 0.25f && HasBolt());
	if (!bThundered && Elapsed >= ThunderDelay) StartThunder();
	if (Elapsed >= Lifetime) Destroy();
}
