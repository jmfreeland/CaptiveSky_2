#include "IslandListeningStonesChime.h"

#include "AgentMemoryComponent.h"
#include "AgentMemoryTypes.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
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
	AudioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(AttenuationInnerRadius, 0.f, 0.f);
	AudioComponent->AttenuationOverrides.FalloffDistance = AttenuationFalloffDistance;
	AudioComponent->VolumeMultiplier = 0.28f;
}

void AIslandListeningStonesChime::BeginChime(float HorizontalWindSpeed, EIslandChimeCause InCause)
{
	const float Speed = FMath::IsFinite(HorizontalWindSpeed) ? FMath::Clamp(HorizontalWindSpeed, 0.f, 300.f) : 0.f;
	BeginChimeWithPitch(Speed, CalculateWindPitchRatio(Speed), InCause);
}

void AIslandListeningStonesChime::BeginChime(const FVector& HorizontalWind, EIslandChimeCause InCause)
{
	const float Speed = HorizontalWind.ContainsNaN() ? 0.f : FMath::Clamp(HorizontalWind.Size2D(), 0.f, 300.f);
	BeginChimeWithPitch(Speed, CalculateWindPitchRatio(Speed) *
		CalculateWindDirectionPitchRatio(HorizontalWind, GetActorForwardVector()), InCause);
}

void AIslandListeningStonesChime::BeginChimeWithPitch(float HorizontalWindSpeed, float PitchRatio, EIslandChimeCause InCause)
{
	SampledWindSpeed = HorizontalWindSpeed;
	AppliedPitchRatio = PitchRatio;
	Cause = InCause;
	if (GetWorld())
	{
		ContextExpiresAt = GetWorld()->GetTimeSeconds() + ResidentContextLifetimeSeconds;
		SetLifeSpan(ResidentContextLifetimeSeconds);
	}
	RecordNearbyResidentObservations();
	BuildChimeWave(AppliedPitchRatio);
	if (ChimeWave && AudioComponent)
	{
		AudioComponent->SetSound(ChimeWave);
		AudioComponent->Play();
	}
}

bool AIslandListeningStonesChime::ShouldRecordResidentObservation(bool bPlayerControlled, bool bHasMemory,
	bool bAudible, bool bHasRecentSimilarMemory)
{
	return !bPlayerControlled && bHasMemory && bAudible && !bHasRecentSimilarMemory;
}

FString AIslandListeningStonesChime::ResidentObservationText(EIslandChimeCause InCause)
{
	switch (InCause)
	{
	case EIslandChimeCause::NaturalGust:
		return TEXT("I heard a rising gust awaken the Listening Stones; a layered tone carried on the wind and then faded.");
	case EIslandChimeCause::VisitorApproach:
		return TEXT("I heard the Listening Stones answer a quiet approach with a brief layered tone.");
	case EIslandChimeCause::ResidentApproach:
		return TEXT("I heard the Listening Stones answer a quiet approach with a brief layered tone.");
	case EIslandChimeCause::Inspection:
		return TEXT("I heard a brief layered tone from the Listening Stones during a close inspection.");
	default:
		return TEXT("I heard a brief layered tone from the Listening Stones, and then it faded.");
	}
}

void AIslandListeningStonesChime::RecordNearbyResidentObservations()
{
	UWorld* World = GetWorld();
	if (!World) return;
	const FString Observation = ResidentObservationText(Cause);
	const FDateTime RecentMemoryCutoff = FDateTime::UtcNow() - FTimespan::FromHours(12.0);
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Resident = *It;
		if (!IsValid(Resident)) continue;
		UAgentMemoryComponent* Memory = Resident->FindComponentByClass<UAgentMemoryComponent>();
		const bool bAudible = IsAudibleAt(Resident->GetActorLocation());
		const bool bHasRecentSimilarMemory = Memory && Memory->HasSimilarMemorySince(
			EAgentMemoryType::Observation, Observation, RecentMemoryCutoff, 0.75f);
		if (!ShouldRecordResidentObservation(Resident->IsPlayerControlled(), Memory != nullptr, bAudible,
			bHasRecentSimilarMemory)) continue;

		Memory->AppendMemory(Memory->MakeMemory(EAgentMemoryType::Observation, Observation, 0.45f,
			{ TEXT("listening_stones"), TEXT("place:ListeningStones"), TEXT("heard"), TEXT("landmark") }));
	}
}

FString AIslandListeningStonesChime::DescribeForListener(const FVector& ListenerLocation) const
{
	const float Distance = FVector::Distance(GetActorLocation(), ListenerLocation);
	const UWorld* World = GetWorld();
	if (!World || ContextExpiresAt <= World->GetTimeSeconds() || Distance > AudibleRadius) return FString();
	FString CauseDescription;
	switch (Cause)
	{
	case EIslandChimeCause::NaturalGust:
		CauseDescription = TEXT(" A rising natural gust stirred the stones.");
		break;
	case EIslandChimeCause::VisitorApproach:
		CauseDescription = TEXT(" It sounded as someone approached the stones quietly.");
		break;
	case EIslandChimeCause::ResidentApproach:
		CauseDescription = TEXT(" It answered a resident's quiet approach to the stones.");
		break;
	case EIslandChimeCause::Inspection:
		CauseDescription = TEXT(" It sounded during someone's close inspection of the stones.");
		break;
	default:
		break;
	}
	if (ElapsedSeconds < DurationSeconds)
	{
		return FString::Printf(TEXT("A soft, layered tone is fading from the ListeningStones, about %.0f metres away.%s It will pass on its own; it is not a puzzle or promised discovery."),
			Distance / 100.f, *CauseDescription);
	}
	return FString::Printf(TEXT("A brief, layered tone sounded from the ListeningStones within the last few minutes.%s It has faded; it was not a puzzle or promised discovery."),
		*CauseDescription);
}

bool AIslandListeningStonesChime::IsAudibleAt(const FVector& ListenerLocation) const
{
	return ElapsedSeconds < DurationSeconds &&
		FVector::DistSquared(GetActorLocation(), ListenerLocation) <= FMath::Square(AudibleRadius);
}

float AIslandListeningStonesChime::CalculateWindPitchRatio(float HorizontalWindSpeed)
{
	const float Speed = FMath::IsFinite(HorizontalWindSpeed) ? FMath::Clamp(HorizontalWindSpeed, 0.f, 300.f) : 0.f;
	const float Semitones = (Speed / 300.f) * 1.5f;
	return FMath::Pow(2.f, Semitones / 12.f);
}

float AIslandListeningStonesChime::CalculateWindDirectionPitchRatio(const FVector& HorizontalWind, const FVector& LandmarkForward)
{
	if (HorizontalWind.ContainsNaN() || LandmarkForward.ContainsNaN() || HorizontalWind.SizeSquared2D() <= KINDA_SMALL_NUMBER)
		return 1.f;
	const FVector WindDirection = HorizontalWind.GetSafeNormal2D();
	const FVector ForwardDirection = LandmarkForward.GetSafeNormal2D();
	if (WindDirection.IsNearlyZero() || ForwardDirection.IsNearlyZero()) return 1.f;
	const float Alignment = FMath::Clamp(FVector::DotProduct(WindDirection, ForwardDirection), -1.f, 1.f);
	const float Semitones = Alignment * 0.35f;
	return FMath::Pow(2.f, Semitones / 12.f);
}

void AIslandListeningStonesChime::BuildChimeWave(float PitchRatio)
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
		const float Fundamental = FMath::Sin(2.f * PI * 220.f * PitchRatio * Time);
		const float Fifth = FMath::Sin(2.f * PI * 329.63f * PitchRatio * Time + 0.18f);
		const float Octave = FMath::Sin(2.f * PI * 440.7f * PitchRatio * Time + 0.35f);
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
		SetActorTickEnabled(false);
	}
}
