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
	if (const double* Until = CooldownUntilByAgentId.Find(OtherAgentId))
	{
		return FPlatformTime::Seconds() < *Until;
	}
	return false;
}

void UAgentSocialComponent::ApplyMutualCooldown(AAutonomousAgentCharacter* Other)
{
	if (!Other)
	{
		return;
	}
	const UAgentMemoryComponent* OtherMemory = Other->FindComponentByClass<UAgentMemoryComponent>();
	const UAgentMemoryComponent* OwnMemory = GetOwner()->FindComponentByClass<UAgentMemoryComponent>();
	const double Until = FPlatformTime::Seconds() + ConversationCooldownSeconds;
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
	if (const UAgentConsolidationComponent* Consolidation = GetOwner() ? GetOwner()->FindComponentByClass<UAgentConsolidationComponent>() : nullptr;
		Consolidation && !Consolidation->IsAwake())
	{
		return;
	}
	if (Utterance.Speech.IsEmpty() || Utterance.SenderAgentId.IsEmpty() ||
		Utterance.TurnIndex > MaximumConversationTurns || PendingUtterances.Num() >= MaximumPendingUtterances)
	{
		return;
	}
	AAutonomousAgentCharacter* Sender = FindAgent(Utterance.SenderAgentId);
	if (!IsWithinSpeakingRange(Sender))
	{
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
			ApplyMutualCooldown(Sender);
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
	if (!IsWithinSpeakingRange(Recipient) || !RecipientMemory || IsCoolingDownWith(RecipientMemory->GetResolvedAgentId()))
	{
		return;
	}
	DeliverSpeech(Recipient, Decision.Speech, FGuid::NewGuid().ToString(EGuidFormats::Digits), 1);
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
	const FString SpeakerName = Speaker->GetActorNameOrLabel();
	const FString RecipientName = Recipient->GetActorNameOrLabel();
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
	RecipientSocial->ReceiveUtterance(Utterance);
	if (TurnIndex >= MaximumConversationTurns)
	{
		ApplyMutualCooldown(Recipient);
	}
}
