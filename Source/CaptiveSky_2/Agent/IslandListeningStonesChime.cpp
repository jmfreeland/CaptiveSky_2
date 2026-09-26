#include "IslandListeningStonesChime.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"

AIslandListeningStonesChime::AIslandListeningStonesChime()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	Tags.AddUnique(TEXT("TransientLandmarkResponse"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("StoneResonance"));
	SetRootComponent(AudioComponent);
	AudioComponent->bAutoActivate = false;
	AudioComponent->bAllowSpatialization = true;
	AudioComponent->bOverrideAttenuation = true;
	AudioComponent->AttenuationOverrides.bAttenuate = true;
	AudioComponent->AttenuationOverrides.bSpatialize = true;
	AudioComponent->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
	AudioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(350.f, 0.f, 0.f);
	AudioComponent->AttenuationOverrides.FalloffDistance = 750.f;
	AudioComponent->VolumeMultiplier = 0.28f;
}

void AIslandListeningStonesChime::BeginChime()
{
	BuildChimeWave();
	if (ChimeWave && AudioComponent)
	{
		AudioComponent->SetSound(ChimeWave);
		AudioComponent->Play();
	}
}

void AIslandListeningStonesChime::BuildChimeWave()
{
	if (ChimeWave) return;
	constexpr int32 SampleRate = 24000;
	constexpr int32 SampleCount = static_cast<int32>(SampleRate * 2.8f);
	ChimeWave = NewObject<USoundWaveProcedural>(this, TEXT("GeneratedStoneResonance"));
	ChimeWave->SetSampleRate(SampleRate);
	ChimeWave->NumChannels = 1;
	ChimeWave->Duration = DurationSeconds;
	ChimeWave->bLooping = false;
	ChimeWave->SoundGroup = SOUNDGROUP_Effects;
	TArray<int16> Samples;
	Samples.SetNumUninitialized(SampleCount);
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Time = static_cast<float>(Index) / SampleRate;
		const float Attack = FMath::Clamp(Time / 0.12f, 0.f, 1.f);
		const float Envelope = Attack * FMath::Exp(-1.65f * Time);
		const float Fundamental = FMath::Sin(2.f * PI * 220.f * Time);
		const float Fifth = FMath::Sin(2.f * PI * 329.63f * Time + 0.18f);
		const float Octave = FMath::Sin(2.f * PI * 440.7f * Time + 0.35f);
		const float Signal = Envelope * (0.040f * Fundamental + 0.024f * Fifth + 0.010f * Octave);
		Samples[Index] = static_cast<int16>(FMath::Clamp(Signal, -1.f, 1.f) * 32767.f);
	}
	ChimeWave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
}

void AIslandListeningStonesChime::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedSeconds += FMath::Max(0.f, DeltaSeconds);
	if (ElapsedSeconds >= DurationSeconds)
	{
		if (AudioComponent) AudioComponent->Stop();
		Destroy();
	}
}
