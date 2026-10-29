#include "IslandListeningStonesChime.h"

#include "AgentMemoryComponent.h"
#include "AgentMemoryTypes.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "IslandWorldStateSubsystem.h"
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

void AIslandListeningStonesChime::BeginChime(float HorizontalWindSpeed, EIslandChimeCause InCause,
	const APawn* QuietResident)
{
	const float Speed = FMath::IsFinite(HorizontalWindSpeed) ? FMath::Clamp(HorizontalWindSpeed, 0.f, 300.f) : 0.f;
	BeginChimeWithPitch(Speed, CalculateWindPitchRatio(Speed), InCause, QuietResident);
}

void AIslandListeningStonesChime::BeginChime(const FVector& HorizontalWind, EIslandChimeCause InCause,
	const APawn* QuietResident)
{
	const float Speed = HorizontalWind.ContainsNaN() ? 0.f : FMath::Clamp(HorizontalWind.Size2D(), 0.f, 300.f);
	BeginChimeWithPitch(Speed, CalculateWindPitchRatio(Speed) *
		CalculateWindDirectionPitchRatio(HorizontalWind, GetActorForwardVector()), InCause, QuietResident);
}

void AIslandListeningStonesChime::BeginChimeWithPitch(float HorizontalWindSpeed, float PitchRatio, EIslandChimeCause InCause,
	const APawn* QuietResident)
{
	SampledWindSpeed = HorizontalWindSpeed;
	AppliedPitchRatio = PitchRatio;
	Cause = InCause;
	if (GetWorld())
	{
		ContextExpiresAt = GetWorld()->GetTimeSeconds() + ResidentContextLifetimeSeconds;
		SetLifeSpan(ResidentContextLifetimeSeconds);
	}
	RecordNearbyResidentObservations(QuietResident);
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
	case EIslandChimeCause::ResidentListening:
		return TEXT("I heard the Listening Stones answer a resident's quiet pause with a brief layered tone.");
	case EIslandChimeCause::Inspection:
		return TEXT("I heard a brief layered tone from the Listening Stones during a close inspection.");
	default:
		return TEXT("I heard a brief layered tone from the Listening Stones, and then it faded.");
	}
}

bool AIslandListeningStonesChime::ShouldRecordQuietPractice(bool bPlayerControlled, bool bHasMemory, bool bIsSubject,
	bool bWithinWitnessRange, bool bHasLineOfSight, bool bFacingSubject, bool bHasRecentSimilarMemory,
	bool bHasSubjectIdentity)
{
	return !bPlayerControlled && bHasMemory && !bIsSubject && bWithinWitnessRange && bHasLineOfSight &&
		bFacingSubject && !bHasRecentSimilarMemory && bHasSubjectIdentity;
}

FString AIslandListeningStonesChime::QuietPracticeObservationText(const FString& SubjectAgentId)
{
	if (SubjectAgentId.IsEmpty()) return FString();
	return FString::Printf(TEXT("I saw %s take a quiet moment at the Listening Stones. When someone listens here, I can leave them room."),
		*SubjectAgentId);
}

void AIslandListeningStonesChime::RecordNearbyResidentObservations(const APawn* QuietResident)
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
	RecordQuietPracticeWitnesses(QuietResident);
}

void AIslandListeningStonesChime::RecordQuietPracticeWitnesses(const APawn* QuietResident)
{
	if (Cause != EIslandChimeCause::ResidentListening || !IsValid(QuietResident)) return;
	UWorld* World = GetWorld();
	UAgentMemoryComponent* SubjectMemory = QuietResident->FindComponentByClass<UAgentMemoryComponent>();
	const FString SubjectAgentId = SubjectMemory ? SubjectMemory->GetResolvedAgentId() : FString();
	if (!World || SubjectAgentId.IsEmpty()) return;

	constexpr float MaximumWitnessDistance = 900.f;
	const FVector SubjectEye = QuietResident->GetActorLocation() + FVector(0.f, 0.f, 45.f);
	const FDateTime RecentMemoryCutoff = FDateTime::UtcNow() - FTimespan::FromHours(12.0);
	const int32 IslandDay = UIslandWorldStateSubsystem::CurrentIslandDay(World);
	const FString PracticeText = QuietPracticeObservationText(SubjectAgentId);
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Witness = *It;
		if (!IsValid(Witness)) continue;
		UAgentMemoryComponent* Memory = Witness->FindComponentByClass<UAgentMemoryComponent>();
		const bool bIsSubject = Witness == QuietResident;
		const FString WitnessAgentId = Memory ? Memory->GetResolvedAgentId() : FString();
		const bool bHasSubjectIdentity = !SubjectAgentId.IsEmpty() && SubjectAgentId != WitnessAgentId;
		const bool bWithinWitnessRange = FVector::DistSquared(Witness->GetActorLocation(), SubjectEye) <= FMath::Square(MaximumWitnessDistance);
		const FVector DirectionToSubject = (QuietResident->GetActorLocation() - Witness->GetActorLocation()).GetSafeNormal2D();
		const bool bFacingSubject = !DirectionToSubject.IsNearlyZero() &&
			FVector::DotProduct(Witness->GetActorForwardVector().GetSafeNormal2D(), DirectionToSubject) >= 0.5f;
		bool bHasLineOfSight = false;
		if (bWithinWitnessRange && bFacingSubject && !bIsSubject)
		{
			FCollisionQueryParams Query(SCENE_QUERY_STAT(ListeningStonePracticeWitness), true, Witness);
			Query.AddIgnoredActor(const_cast<APawn*>(QuietResident));
			Query.AddIgnoredActor(this);
			FHitResult Hit;
			bHasLineOfSight = !World->LineTraceSingleByChannel(Hit, Witness->GetPawnViewLocation(), SubjectEye,
				ECC_Visibility, Query);
		}
		const bool bHasRecentSimilarMemory = Memory && Memory->HasSimilarMemorySince(
			EAgentMemoryType::Observation, PracticeText, RecentMemoryCutoff, 0.75f);
		if (!ShouldRecordQuietPractice(Witness->IsPlayerControlled(), Memory != nullptr, bIsSubject,
			bWithinWitnessRange, bHasLineOfSight, bFacingSubject, bHasRecentSimilarMemory, bHasSubjectIdentity)) continue;

		const FString DayTag = FString::Printf(TEXT("day:%d"), IslandDay);
		FAgentMemoryRecord Record = Memory->MakeMemory(EAgentMemoryType::Observation, PracticeText, 0.72f,
			{ TEXT("place-practice"), TEXT("place:ListeningStones"), TEXT("practice:quiet-listening"),
				FString::Printf(TEXT("observer:%s"), *WitnessAgentId), FString::Printf(TEXT("actor:%s"), *SubjectAgentId),
				DayTag, TEXT("confidence:direct-visual") });
		Record.Location = GetActorLocation();
		Memory->AppendMemory(Record);
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
	case EIslandChimeCause::ResidentListening:
		CauseDescription = TEXT(" It answered a resident's quiet pause beside the stones.");
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
