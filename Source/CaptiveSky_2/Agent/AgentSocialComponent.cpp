// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentSocialComponent.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentConversationTypes.h"
#include "AgentMemoryComponent.h"
#include "AgentRelationshipComponent.h"
#include "AgentSocialSubsystem.h"
#include "AutonomousAgentCharacter.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Misc/Guid.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentSocial, Log, All);

UAgentSocialComponent::UAgentSocialComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
}

void UAgentSocialComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UAgentBrainComponent* Brain = GetBrain())
	{
		Brain->OnDecisionReady.AddDynamic(this, &UAgentSocialComponent::HandleDecisionReady);
	}
}

void UAgentSocialComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAgentBrainComponent* Brain = GetBrain())
	{
		Brain->OnDecisionReady.RemoveDynamic(this, &UAgentSocialComponent::HandleDecisionReady);
	}
	if (!ActiveAutomaticConversationId.IsEmpty())
	{
		FinishAutomaticConversation(FindAgent(ActiveAutomaticConversationPartnerId),
			ActiveAutomaticConversationId, true);
	}
	Super::EndPlay(EndPlayReason);
}

void UAgentSocialComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TryBeginPendingUtterance();
}

UAgentBrainComponent* UAgentSocialComponent::GetBrain() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UAgentBrainComponent>() : nullptr;
}

AAutonomousAgentCharacter* UAgentSocialComponent::GetAgentOwner() const
{
	return Cast<AAutonomousAgentCharacter>(GetOwner());
}

AAutonomousAgentCharacter* UAgentSocialComponent::FindAgent(const FString& IdOrName) const
{
	if (IdOrName.IsEmpty() || !GetWorld())
	{
		return nullptr;
	}
	for (TActorIterator<AAutonomousAgentCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == GetOwner())
		{
			continue;
		}
		const UAgentMemoryComponent* Memory = It->FindComponentByClass<UAgentMemoryComponent>();
		if ((Memory && Memory->GetResolvedAgentId().Equals(IdOrName, ESearchCase::IgnoreCase)) ||
			It->GetAgentDisplayName().Equals(IdOrName, ESearchCase::IgnoreCase) ||
			It->GetActorNameOrLabel().Equals(IdOrName, ESearchCase::IgnoreCase) ||
			It->GetName().Equals(IdOrName, ESearchCase::IgnoreCase) || It->ActorHasTag(FName(*IdOrName)))
		{
			return *It;
		}
	}
	return nullptr;
}

bool UAgentSocialComponent::IsWithinSpeakingRange(const AAutonomousAgentCharacter* Other) const
{
	return Other && GetOwner() && FVector::DistSquared(Other->GetActorLocation(), GetOwner()->GetActorLocation()) <=
		FMath::Square(SpeakingRadius);
}

bool UAgentSocialComponent::IsCoolingDownWith(const FString& OtherAgentId) const
{
	return GetConversationCooldownRemainingWith(OtherAgentId) > 0.f;
}

bool UAgentSocialComponent::TryReserveAutomaticConversation(UAgentSocialComponent* OtherSocial,
	const FString& OwnAgentId, const FString& OtherAgentId, const FString& ConversationId)
{
	if (!OtherSocial || OwnAgentId.IsEmpty() || OtherAgentId.IsEmpty() || ConversationId.IsEmpty() ||
		!ActiveAutomaticConversationId.IsEmpty() || !OtherSocial->ActiveAutomaticConversationId.IsEmpty() ||
		bHandlingUtterance || OtherSocial->bHandlingUtterance || !PendingUtterances.IsEmpty() ||
		!OtherSocial->PendingUtterances.IsEmpty() || IsCoolingDownWith(OtherAgentId) ||
		OtherSocial->IsCoolingDownWith(OwnAgentId))
	{
		return false;
	}

	ActiveAutomaticConversationId = ConversationId;
	ActiveAutomaticConversationPartnerId = OtherAgentId;
	OtherSocial->ActiveAutomaticConversationId = ConversationId;
	OtherSocial->ActiveAutomaticConversationPartnerId = OwnAgentId;
	return true;
}

bool UAgentSocialComponent::IsReservedFor(const FString& OtherAgentId, const FString& ConversationId) const
{
	return !ConversationId.IsEmpty() && ActiveAutomaticConversationId == ConversationId &&
		ActiveAutomaticConversationPartnerId == OtherAgentId;
}

void UAgentSocialComponent::ReleaseAutomaticConversation(UAgentSocialComponent* OtherSocial,
	const FString& ConversationId)
{
	if (ConversationId.IsEmpty() || ActiveAutomaticConversationId != ConversationId)
	{
		return;
	}

	ActiveAutomaticConversationId.Reset();
	ActiveAutomaticConversationPartnerId.Reset();
	if (OtherSocial && OtherSocial->ActiveAutomaticConversationId == ConversationId)
	{
		OtherSocial->ActiveAutomaticConversationId.Reset();
		OtherSocial->ActiveAutomaticConversationPartnerId.Reset();
	}

	PendingUtterances.RemoveAll([&ConversationId](const FAgentSocialUtterance& Utterance)
	{
		return Utterance.ConversationId == ConversationId;
	});
}

bool UAgentSocialComponent::IsConversationAtTurnLimit(int32 TurnIndex) const
{
	return TurnIndex >= FMath::Max(1, MaximumConversationTurns);
}

void UAgentSocialComponent::FinishAutomaticConversation(AAutonomousAgentCharacter* Other,
	const FString& ConversationId, bool bApplyCooldown)
{
	if (ConversationId.IsEmpty() || ActiveAutomaticConversationId != ConversationId)
	{
		return;
	}

	if (bApplyCooldown)
	{
		ApplyMutualCooldown(Other);
	}

	ReleaseAutomaticConversation(Other ? Other->FindComponentByClass<UAgentSocialComponent>() : nullptr,
		ConversationId);
}

float UAgentSocialComponent::GetConversationCooldownRemainingWith(const FString& OtherAgentId) const
{
	if (const double* Until = CooldownUntilByAgentId.Find(OtherAgentId))
	{
		return static_cast<float>(FMath::Max(0.0, *Until - FPlatformTime::Seconds()));
	}
	return 0.f;
}

float UAgentSocialComponent::EffectiveConversationCooldownSeconds(float ConfiguredSeconds)
{
	return FMath::IsFinite(ConfiguredSeconds)
		? FMath::Max(MinimumConversationCooldownSeconds, ConfiguredSeconds)
		: MinimumConversationCooldownSeconds;
}

void UAgentSocialComponent::ApplyMutualCooldown(AAutonomousAgentCharacter* Other)
{
	if (!Other)
	{
		return;
	}
	const UAgentMemoryComponent* OtherMemory = Other->FindComponentByClass<UAgentMemoryComponent>();
	const UAgentMemoryComponent* OwnMemory = GetOwner()->FindComponentByClass<UAgentMemoryComponent>();
	const double Until = FPlatformTime::Seconds() + EffectiveConversationCooldownSeconds(ConversationCooldownSeconds);
	if (OtherMemory)
	{
		CooldownUntilByAgentId.Add(OtherMemory->GetResolvedAgentId(), Until);
	}
	if (UAgentSocialComponent* OtherSocial = Other->FindComponentByClass<UAgentSocialComponent>(); OwnMemory && OtherSocial)
	{
		OtherSocial->CooldownUntilByAgentId.Add(OwnMemory->GetResolvedAgentId(), Until);
	}
}

void UAgentSocialComponent::ReceiveUtterance(const FAgentSocialUtterance& Utterance)
{
	AAutonomousAgentCharacter* Sender = FindAgent(Utterance.SenderAgentId);
	const UAgentMemoryComponent* OwnMemory = GetOwner()
		? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	UAgentSocialComponent* SenderSocial = Sender
		? Sender->FindComponentByClass<UAgentSocialComponent>() : nullptr;
	if (!Sender || !OwnMemory || !SenderSocial ||
		!IsReservedFor(Utterance.SenderAgentId, Utterance.ConversationId) ||
		!SenderSocial->IsReservedFor(OwnMemory->GetResolvedAgentId(), Utterance.ConversationId))
	{
		return;
	}

	const UAgentConsolidationComponent* Consolidation = GetOwner()->FindComponentByClass<UAgentConsolidationComponent>();
	if (Utterance.Speech.IsEmpty() || Utterance.SenderAgentId.IsEmpty() ||
		Utterance.TurnIndex <= 0 || IsConversationAtTurnLimit(Utterance.TurnIndex) ||
		PendingUtterances.Num() >= MaximumPendingUtterances || !IsWithinSpeakingRange(Sender) ||
		(Consolidation && !Consolidation->IsAwake()))
	{
		FinishAutomaticConversation(Sender, Utterance.ConversationId, true);
		return;
	}
	PendingUtterances.Add(Utterance);
}

void UAgentSocialComponent::TryBeginPendingUtterance()
{
	UAgentBrainComponent* Brain = GetBrain();
	if (bHandlingUtterance || PendingUtterances.IsEmpty() || !Brain || Brain->bRequestInFlight)
	{
		return;
	}
	ActiveUtterance = PendingUtterances[0];
	PendingUtterances.RemoveAt(0);
	AAutonomousAgentCharacter* Sender = FindAgent(ActiveUtterance.SenderAgentId);
	const UAgentMemoryComponent* OwnMemory = GetOwner()
		? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	UAgentSocialComponent* SenderSocial = Sender
		? Sender->FindComponentByClass<UAgentSocialComponent>() : nullptr;
	if (!OwnMemory || !SenderSocial ||
		!IsReservedFor(ActiveUtterance.SenderAgentId, ActiveUtterance.ConversationId) ||
		!SenderSocial->IsReservedFor(OwnMemory->GetResolvedAgentId(), ActiveUtterance.ConversationId) ||
		ActiveUtterance.TurnIndex <= 0 || IsConversationAtTurnLimit(ActiveUtterance.TurnIndex))
	{
		FinishAutomaticConversation(Sender, ActiveUtterance.ConversationId, true);
		ActiveUtterance = FAgentSocialUtterance();
		return;
	}
	bHandlingUtterance = true;

	FAgentConversationContext Context;
	Context.Text = ActiveUtterance.Speech;
	Context.Source = TEXT("in_world");
	Context.ConversationId = ActiveUtterance.ConversationId;
	Context.MessageId = FString::Printf(TEXT("%s:%d"), *ActiveUtterance.ConversationId, ActiveUtterance.TurnIndex);
	Context.ParticipantId = ActiveUtterance.SenderAgentId;
	Context.ParticipantName = ActiveUtterance.SenderDisplayName;
	Context.bAgentToAgent = true;
	Context.TurnIndex = ActiveUtterance.TurnIndex;
	Brain->RequestContextualDecision(Context);
}

void UAgentSocialComponent::HandleDecisionReady(const FAgentDecision& Decision)
{
	UAgentBrainComponent* Brain = GetBrain();
	if (bHandlingUtterance)
	{
		const FAgentSocialUtterance CompletedUtterance = ActiveUtterance;
		bHandlingUtterance = false;
		ActiveUtterance = FAgentSocialUtterance();
		AAutonomousAgentCharacter* Sender = FindAgent(CompletedUtterance.SenderAgentId);
		if (Decision.bValid && Decision.ActionType == EAgentActionType::Speak && !Decision.Speech.IsEmpty() &&
			CompletedUtterance.TurnIndex < MaximumConversationTurns && IsWithinSpeakingRange(Sender))
		{
			DeliverSpeech(Sender, Decision.Speech, CompletedUtterance.ConversationId, CompletedUtterance.TurnIndex + 1);
		}
		else
		{
			FinishAutomaticConversation(Sender, CompletedUtterance.ConversationId, true);
		}
		return;
	}

	// Only autonomous think cycles may initiate agent conversation. Player and external replies
	// have non-empty context and must never be accidentally forwarded to another being.
	if (Brain && Brain->LastConversationContext.Text.IsEmpty())
	{
		TryBeginSpontaneousConversation(Decision);
	}
}

void UAgentSocialComponent::TryBeginSpontaneousConversation(const FAgentDecision& Decision)
{
	if (const UAgentConsolidationComponent* Consolidation = GetOwner() ? GetOwner()->FindComponentByClass<UAgentConsolidationComponent>() : nullptr;
		Consolidation && !Consolidation->IsAwake())
	{
		return;
	}
	if (!Decision.bValid || Decision.ActionType != EAgentActionType::Speak ||
		Decision.Speech.IsEmpty() || Decision.ActionTarget.IsEmpty())
	{
		return;
	}
	AAutonomousAgentCharacter* Recipient = FindAgent(Decision.ActionTarget);
	const UAgentMemoryComponent* RecipientMemory = Recipient ? Recipient->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	UAgentMemoryComponent* OwnMemory = GetOwner()
		? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	UAgentSocialComponent* RecipientSocial = Recipient
		? Recipient->FindComponentByClass<UAgentSocialComponent>() : nullptr;
	const UAgentConsolidationComponent* RecipientConsolidation = Recipient
		? Recipient->FindComponentByClass<UAgentConsolidationComponent>() : nullptr;
	if (!IsWithinSpeakingRange(Recipient) || !RecipientMemory || !OwnMemory || !RecipientSocial ||
		(RecipientConsolidation && !RecipientConsolidation->IsAwake()))
	{
		return;
	}
	const FString ConversationId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	if (TryReserveAutomaticConversation(RecipientSocial, OwnMemory->GetResolvedAgentId(),
		RecipientMemory->GetResolvedAgentId(), ConversationId))
	{
		DeliverSpeech(Recipient, Decision.Speech, ConversationId, 1);
	}
}

void UAgentSocialComponent::DeliverSpeech(AAutonomousAgentCharacter* Recipient, const FString& Speech,
	const FString& ConversationId, int32 TurnIndex)
{
	AAutonomousAgentCharacter* Speaker = GetAgentOwner();
	if (!Speaker || !Recipient || Speech.IsEmpty() || !IsWithinSpeakingRange(Recipient))
	{
		return;
	}
	UAgentMemoryComponent* SpeakerMemory = Speaker->FindComponentByClass<UAgentMemoryComponent>();
	UAgentMemoryComponent* RecipientMemory = Recipient->FindComponentByClass<UAgentMemoryComponent>();
	UAgentSocialComponent* RecipientSocial = Recipient->FindComponentByClass<UAgentSocialComponent>();
	if (!SpeakerMemory || !RecipientMemory || !RecipientSocial)
	{
		return;
	}

	const FString SpeakerId = SpeakerMemory->GetResolvedAgentId();
	const FString RecipientId = RecipientMemory->GetResolvedAgentId();
	if (!IsReservedFor(RecipientId, ConversationId) ||
		!RecipientSocial->IsReservedFor(SpeakerId, ConversationId))
	{
		return;
	}
	const FString SpeakerName = Speaker->GetAgentDisplayName();
	const FString RecipientName = Recipient->GetAgentDisplayName();
	if (UAgentRelationshipComponent* Relationships = Speaker->FindComponentByClass<UAgentRelationshipComponent>())
	{
		Relationships->RecordInteraction(RecipientId, RecipientName,
			FString::Printf(TEXT("I said nearby: \"%s\""), *Speech), ConversationId);
	}
	if (UAgentRelationshipComponent* Relationships = Recipient->FindComponentByClass<UAgentRelationshipComponent>())
	{
		Relationships->RecordInteraction(SpeakerId, SpeakerName,
			FString::Printf(TEXT("They said nearby: \"%s\""), *Speech), ConversationId);
	}
	if (UAgentSocialSubsystem* SocialSubsystem = GetWorld()->GetSubsystem<UAgentSocialSubsystem>())
	{
		SocialSubsystem->PublishSpeech(Speaker, Speech);
	}

	FAgentSocialUtterance Utterance;
	Utterance.ConversationId = ConversationId;
	Utterance.SenderAgentId = SpeakerId;
	Utterance.SenderDisplayName = SpeakerName;
	Utterance.Speech = Speech;
	Utterance.TurnIndex = TurnIndex;
	if (IsConversationAtTurnLimit(TurnIndex))
	{
		FinishAutomaticConversation(Recipient, ConversationId, true);
		return;
	}
	RecipientSocial->ReceiveUtterance(Utterance);
}
