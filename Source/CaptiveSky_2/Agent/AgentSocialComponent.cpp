// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentSocialComponent.h"
#include "AgentBrainComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentConversationTypes.h"
#include "AgentMemoryComponent.h"
#include "AgentRelationshipComponent.h"
#include "AgentSocialSubsystem.h"
#include "AutonomousAgentCharacter.h"
#include "IslandChronicle.h"
#include "IslandWorldStateSubsystem.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "Misc/Guid.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentSocial, Log, All);

static bool NormalizePersonalPlaceName(FString& Name)
{
	Name.TrimStartAndEndInline();
	if (Name.Len() < 3 || Name.Len() > 40) return false;
	bool bHasLetter = false;
	for (const TCHAR Character : Name)
	{
		if (FChar::IsAlpha(Character)) bHasLetter = true;
		else if (!FChar::IsDigit(Character) && Character != TEXT(' ') && Character != TEXT('\'') &&
			Character != TEXT('-') && Character != TEXT('.')) return false;
	}
	return bHasLetter;
}

static bool ContainsPersonalPlaceName(const FString& Speech, const FString& Name)
{
	if (Speech.IsEmpty() || Name.IsEmpty()) return false;
	auto IsJoinedNameCharacter = [](const FString& Text, int32 Position, bool bToLeft)
	{
		const TCHAR Character = Text[Position];
		if (FChar::IsAlnum(Character) || Character == TEXT('_')) return true;
		if (Character != TEXT('\'') && Character != 0x2019 && Character != TEXT('-') && Character != TEXT('.')) return false;
		const int32 Neighbor = bToLeft ? Position - 1 : Position + 1;
		return Neighbor >= 0 && Neighbor < Text.Len() && FChar::IsAlnum(Text[Neighbor]);
	};
	int32 SearchFrom = 0;
	while (SearchFrom < Speech.Len())
	{
		const int32 Found = Speech.Find(*Name, ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchFrom);
		if (Found == INDEX_NONE) return false;
		const int32 End = Found + Name.Len();
		const bool bLeftBoundary = Found == 0 || !IsJoinedNameCharacter(Speech, Found - 1, true);
		const bool bRightBoundary = End >= Speech.Len() || !IsJoinedNameCharacter(Speech, End, false);
		if (bLeftBoundary && bRightBoundary) return true;
		SearchFrom = Found + 1;
	}
	return false;
}

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
	UE_LOG(LogAgentSocial, Log, TEXT("[AgentSocial] automatic conversation opened: %s -> %s (id=%s)."),
		*OwnAgentId, *OtherAgentId, *ConversationId);
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
	const UAgentMemoryComponent* OtherMemory = Other
		? Other->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	const UAgentMemoryComponent* OwnMemory = GetOwner()
		? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
	UE_LOG(LogAgentSocial, Log, TEXT("[AgentSocial] automatic conversation closed: %s <-> %s (id=%s; cooldown=%s)."),
		OwnMemory ? *OwnMemory->GetResolvedAgentId() : TEXT("unknown"),
		OtherMemory ? *OtherMemory->GetResolvedAgentId() : TEXT("unknown"), *ConversationId,
		bApplyCooldown ? TEXT("yes") : TEXT("no"));

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

float UAgentSocialComponent::EffectiveConversationReplyPauseSeconds(float ConfiguredSeconds)
{
	return FMath::IsFinite(ConfiguredSeconds)
		? FMath::Max(MinimumConversationReplyPauseSeconds, ConfiguredSeconds)
		: MinimumConversationReplyPauseSeconds;
}

bool UAgentSocialComponent::IsConversationReplyPauseActive() const
{
	return FPlatformTime::Seconds() < ConversationReplyNotBeforeSeconds;
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

bool UAgentSocialComponent::ReceiveUtterance(const FAgentSocialUtterance& Utterance)
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
		return false;
	}

	const UAgentConsolidationComponent* Consolidation = GetOwner()->FindComponentByClass<UAgentConsolidationComponent>();
	if (Utterance.Speech.IsEmpty() || Utterance.SenderAgentId.IsEmpty() ||
		Utterance.TurnIndex <= 0 || IsConversationAtTurnLimit(Utterance.TurnIndex) ||
		PendingUtterances.Num() >= MaximumPendingUtterances || !IsWithinSpeakingRange(Sender) ||
		(Consolidation && !Consolidation->IsAwake()))
	{
		FinishAutomaticConversation(Sender, Utterance.ConversationId, true);
		return false;
	}
	if (!Utterance.TeachArrangementSiteId.IsEmpty())
	{
		if (UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>())
		{
			WorldState->RecordArrangementTeaching(FName(*Utterance.TeachArrangementSiteId),
				Utterance.SenderAgentId, OwnMemory->GetResolvedAgentId(),
				UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()));
		}
	}
	if (!Utterance.TeachCraftTraditionId.IsEmpty())
	{
		if (UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>())
		{
			WorldState->RecordCraftTeaching(FName(*Utterance.TeachCraftTraditionId),
				Utterance.SenderAgentId, OwnMemory->GetResolvedAgentId(),
				UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()));
		}
	}
	PendingUtterances.Add(Utterance);
	const float ReplyPauseSeconds = EffectiveConversationReplyPauseSeconds(ConversationReplyPauseSeconds);
	ConversationReplyNotBeforeSeconds = FPlatformTime::Seconds() + ReplyPauseSeconds;
	UE_LOG(LogAgentSocial, Log,
		TEXT("[AgentSocial] %s queued reply turn %d to %s in conversation %s; waiting %.1f real seconds."),
		*GetNameSafe(GetOwner()), Utterance.TurnIndex, *Utterance.SenderDisplayName,
		*Utterance.ConversationId, ReplyPauseSeconds);
	return true;
}

void UAgentSocialComponent::TryBeginPendingUtterance()
{
	UAgentBrainComponent* Brain = GetBrain();
	if (bHandlingUtterance || PendingUtterances.IsEmpty() || !Brain || Brain->bRequestInFlight ||
		IsConversationReplyPauseActive())
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
			DeliverSpeech(Sender, Decision.Speech, CompletedUtterance.ConversationId, CompletedUtterance.TurnIndex + 1,
				Decision.TeachArrangement, Decision.TeachCraft, Decision.PromiseObjectId,
				Decision.PromiseTargetPlaceId, Decision.PromiseDueInDays,
				Decision.PlaceNameTarget, Decision.PlaceNameLabel);
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
		DeliverSpeech(Recipient, Decision.Speech, ConversationId, 1, Decision.TeachArrangement,
			Decision.TeachCraft, Decision.PromiseObjectId, Decision.PromiseTargetPlaceId, Decision.PromiseDueInDays,
			Decision.PlaceNameTarget, Decision.PlaceNameLabel);
	}
}

void UAgentSocialComponent::DeliverSpeech(AAutonomousAgentCharacter* Recipient, const FString& Speech,
	const FString& ConversationId, int32 TurnIndex, const FString& TeachArrangementSiteId,
	const FString& TeachCraftTraditionId, const FString& PromiseObjectId,
	const FString& PromiseTargetPlaceId, int32 PromiseDueInDays,
	const FString& PlaceNameTarget, const FString& PlaceNameLabel)
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
	const bool bEndsConversation = IsConversationAtTurnLimit(TurnIndex);
	FAgentSocialUtterance Utterance;
	Utterance.ConversationId = ConversationId;
	Utterance.SenderAgentId = SpeakerId;
	Utterance.SenderDisplayName = SpeakerName;
	Utterance.Speech = Speech;
	Utterance.TeachArrangementSiteId = TeachArrangementSiteId;
	Utterance.TeachCraftTraditionId = TeachCraftTraditionId;
	Utterance.TurnIndex = TurnIndex;
	// Queue the listener's reply first. A rejected line must not become a remembered
	// promise/name, a relationship interaction, a Chronicle fact, or a public utterance.
	if (!bEndsConversation && !RecipientSocial->ReceiveUtterance(Utterance)) return;
	UE_LOG(LogAgentSocial, Log, TEXT("[AgentSocial] automatic conversation line %d delivered: %s -> %s (id=%s)."),
		TurnIndex, *SpeakerId, *RecipientId, *ConversationId);
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

	// A place name is a resident's attributed claim, not a canonical rename.
	// Record it only when the speaker knows the exact anchor and says the offered name.
	FString PersonalName = PlaceNameLabel;
	UAgentBrainComponent* SpeakerBrain = Speaker->FindComponentByClass<UAgentBrainComponent>();
	if (SpeakerBrain && SpeakerBrain->KnowsRememberedPlace(PlaceNameTarget) &&
		NormalizePersonalPlaceName(PersonalName) && ContainsPersonalPlaceName(Speech, PersonalName))
	{
		const FName PlaceId(*PlaceNameTarget);
		FString ConversationKey = ConversationId;
		ConversationKey.ReplaceInline(TEXT("-"), TEXT(""));
		const TArray<FString> ClaimTags = {
			TEXT("place-name"), TEXT("place-id:") + PlaceId.ToString(), TEXT("place-label:") + PersonalName,
			TEXT("claimed-by:") + SpeakerId,
			TEXT("conversation:") + ConversationKey, TEXT("turn:") + FString::FromInt(TurnIndex)
		};
		SpeakerMemory->AppendMemory(SpeakerMemory->MakeMemory(EAgentMemoryType::Conversation,
			FString::Printf(TEXT("I call the place %s \"%s\"; this is my personal name for it, not an official renaming."),
				*PlaceId.ToString(), *PersonalName), 0.78f, ClaimTags));
		RecipientMemory->AppendMemory(RecipientMemory->MakeMemory(EAgentMemoryType::Conversation,
			FString::Printf(TEXT("%s told me they call the place %s \"%s\"; this is their personal name, not an official renaming."),
				*SpeakerName, *PlaceId.ToString(), *PersonalName), 0.78f,
			{ TEXT("place-name"), TEXT("place-id:") + PlaceId.ToString(), TEXT("place-label:") + PersonalName,
			  TEXT("learned-from:") + SpeakerId, TEXT("conversation:") + ConversationKey,
			  TEXT("turn:") + FString::FromInt(TurnIndex) }));
		UIslandChronicleSubsystem::Record(GetWorld(), TEXT("place_name_claim"), SpeakerId, PersonalName,
			{ { TEXT("place_id"), PlaceId.ToString() }, { TEXT("with"), RecipientId },
			  { TEXT("conversation_id"), ConversationKey }, { TEXT("turn"), FString::FromInt(TurnIndex) } });
		UE_LOG(LogAgentSocial, Log, TEXT("[AgentSocial] %s shared personal place name '%s' for %s with %s (conversation %s, turn %d)."),
			*SpeakerId, *PersonalName, *PlaceId.ToString(), *RecipientId, *ConversationId, TurnIndex);
	}

	// Persist the one deliberately offered promise only after a nearby, mutually
	// reserved resident line has actually been delivered. This is intent provenance;
	// it does not create or move the promised object.
	if (SpeakerId == TEXT("Aster") && RecipientId == TEXT("Innkeeper") &&
		PromiseObjectId == TEXT("BlueJar") && PromiseTargetPlaceId == TEXT("InnCounter") && PromiseDueInDays == 1 &&
		Speech.Contains(TEXT("bring"), ESearchCase::IgnoreCase) &&
		Speech.Contains(TEXT("jar"), ESearchCase::IgnoreCase) &&
		Speech.Contains(TEXT("tomorrow"), ESearchCase::IgnoreCase))
	{
		if (UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>())
		{
			FString ConversationKey = ConversationId;
			ConversationKey.ReplaceInline(TEXT("-"), TEXT(""));
			if (ConversationKey.Len() > 32) ConversationKey.LeftInline(32);
			const FString PromiseIdText = FString::Printf(TEXT("Promise_%s_T%d"), *ConversationKey, TurnIndex);
			const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld());
			FIslandSocialCommitmentRecord Commitment;
			Commitment.Id = FName(*PromiseIdText);
			Commitment.ProposerAgentId = SpeakerId;
			Commitment.RecipientAgentId = RecipientId;
			Commitment.ObjectId = FName(*PromiseObjectId);
			Commitment.TargetPlaceId = FName(*PromiseTargetPlaceId);
			Commitment.SourceConversationId = ConversationId;
			Commitment.CreatedDay = Today;
			Commitment.DueDay = Today + PromiseDueInDays;
			if (WorldState->CreateBringObjectCommitment(Commitment))
				UE_LOG(LogAgentSocial, Log, TEXT("[AgentSocial] %s promised %s to bring %s to %s by Island day %d (conversation %s, turn %d)."),
					*SpeakerId, *RecipientId, *PromiseObjectId, *PromiseTargetPlaceId,
					Commitment.DueDay, *ConversationId, TurnIndex);
		}
	}

	if (bEndsConversation)
	{
		FinishAutomaticConversation(Recipient, ConversationId, true);
	}
}
