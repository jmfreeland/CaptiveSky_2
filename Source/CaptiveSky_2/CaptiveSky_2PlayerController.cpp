// Copyright Epic Games, Inc. All Rights Reserved.


#include "CaptiveSky_2PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSky_2.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "CaptiveSkyConversationWidget.h"
#include "CaptiveSkyAmbientSpeechWidget.h"
#include "Agent/AutonomousAgentCharacter.h"
#include "Agent/AgentMemoryComponent.h"
#include "Agent/AgentBrainComponent.h"
#include "Agent/AgentSocialSubsystem.h"
#include "Agent/IslandInteractionUtility.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

void ACaptiveSky_2PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCaptiveSky_2, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	if (IsLocalPlayerController())
	{
		ConversationWidget = CreateWidget<UCaptiveSkyConversationWidget>(this, UCaptiveSkyConversationWidget::StaticClass());
		if (ConversationWidget)
		{
			ConversationWidget->AddToPlayerScreen(20);
			ConversationWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		AmbientSpeechWidget = CreateWidget<UCaptiveSkyAmbientSpeechWidget>(this, UCaptiveSkyAmbientSpeechWidget::StaticClass());
		if (AmbientSpeechWidget)
		{
			AmbientSpeechWidget->AddToPlayerScreen(10);
			AmbientSpeechWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UAgentSocialSubsystem* SocialSubsystem = GetWorld()->GetSubsystem<UAgentSocialSubsystem>())
		{
			SocialSubsystem->OnAmbientSpeech.AddDynamic(this, &ACaptiveSky_2PlayerController::HandleAmbientAgentSpeech);
		}
	}
}

void ACaptiveSky_2PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		if (UAgentSocialSubsystem* SocialSubsystem = GetWorld()->GetSubsystem<UAgentSocialSubsystem>())
		{
			SocialSubsystem->OnAmbientSpeech.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleAmbientAgentSpeech);
		}
		GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ACaptiveSky_2PlayerController::HandleAmbientAgentSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech)
{
	if (!AmbientSpeechWidget || !Speaker || !GetPawn() ||
		FVector::DistSquared(Speaker->GetActorLocation(), GetPawn()->GetActorLocation()) > FMath::Square(AmbientSpeechRadius))
	{
		return;
	}
	AmbientSpeechWidget->ShowSpeech(Speaker->GetActorNameOrLabel(), Speech);
	GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	GetWorldTimerManager().SetTimer(AmbientSpeechHideTimer, this,
		&ACaptiveSky_2PlayerController::HideAmbientSpeech, AmbientSpeechDurationSeconds, false);
}

void ACaptiveSky_2PlayerController::HideAmbientSpeech()
{
	if (AmbientSpeechWidget)
	{
		AmbientSpeechWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ACaptiveSky_2PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACaptiveSky_2PlayerController::ToggleConversation);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ACaptiveSky_2PlayerController::InteractWithNearestWorldObject);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ACaptiveSky_2PlayerController::CloseConversation);

		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

AActor* ACaptiveSky_2PlayerController::FindNearestWorldInteraction() const
{
	const APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn || !GetWorld()) return nullptr;
	AActor* Nearest = nullptr;
	float BestDistanceSquared = FMath::Square(IslandInteractionRadius);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (IslandInteractionUtility::GetTargetTag(*It).IsNone() ||
			!IslandInteractionUtility::CanInteract(PlayerPawn, *It, IslandInteractionRadius)) continue;
		const float DistanceSquared = FVector::DistSquared(PlayerPawn->GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			Nearest = *It;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return Nearest;
}

void ACaptiveSky_2PlayerController::ShowWorldInteractionCaption(const FString& Caption)
{
	if (!AmbientSpeechWidget || !GetWorld()) return;
	AmbientSpeechWidget->ShowCaption(Caption);
	GetWorldTimerManager().ClearTimer(AmbientSpeechHideTimer);
	GetWorldTimerManager().SetTimer(AmbientSpeechHideTimer, this,
		&ACaptiveSky_2PlayerController::HideAmbientSpeech, AmbientSpeechDurationSeconds, false);
}

void ACaptiveSky_2PlayerController::InteractWithNearestWorldObject()
{
	if (!IsLocalPlayerController() || (ConversationWidget && ConversationWidget->IsVisible())) return;
	AActor* Target = FindNearestWorldInteraction();
	if (!Target)
	{
		ShowWorldInteractionCaption(TEXT("Nothing nearby catches your eye. Move closer to a landmark or wild creature and try E again."));
		return;
	}

	const FName TargetTag = IslandInteractionUtility::GetTargetTag(Target);
	const double Now = FPlatformTime::Seconds();
	for (auto CooldownIt = WorldInteractionCooldowns.CreateIterator(); CooldownIt; ++CooldownIt)
		if (!CooldownIt.Key().IsValid() || CooldownIt.Value() <= Now) CooldownIt.RemoveCurrent();
	const TWeakObjectPtr<AActor> TargetKey(Target);
	if (const double* CooldownUntil = WorldInteractionCooldowns.Find(TargetKey); CooldownUntil && *CooldownUntil > Now)
	{
		ShowWorldInteractionCaption(FString::Printf(TEXT("%s has already answered your attention; let the moment settle."), *TargetTag.ToString()));
		return;
	}

	FString Fact;
	if (!IslandInteractionUtility::Perform(GetPawn(), Target, Fact))
	{
		ShowWorldInteractionCaption(TEXT("You cannot reach or clearly see that from here."));
		return;
	}
	WorldInteractionCooldowns.Add(TargetKey, Now + FMath::Max(30.f, IslandInteractionCooldownSeconds));
	ShowWorldInteractionCaption(TargetTag.ToString() + TEXT(": ") + Fact);
}

AAutonomousAgentCharacter* ACaptiveSky_2PlayerController::FindNearestConversationAgent() const
{
	const APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn) return nullptr;
	AAutonomousAgentCharacter* Nearest = nullptr;
	float BestDistanceSquared = FMath::Square(ConversationRadius);
	for (TActorIterator<AAutonomousAgentCharacter> It(GetWorld()); It; ++It)
	{
		const float DistanceSquared = FVector::DistSquared(PlayerPawn->GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			Nearest = *It;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return Nearest;
}

void ACaptiveSky_2PlayerController::ToggleConversation()
{
	if (!ConversationWidget || ConversationWidget->IsVisible()) return;
	ConversationTarget = FindNearestConversationAgent();
	if (!ConversationTarget)
	{
		ConversationWidget->SetVisibility(ESlateVisibility::Visible);
		ConversationWidget->ShowMessage(TEXT("No one is close enough to hear you."), false);
		FTimerHandle HideHandle;
		GetWorldTimerManager().SetTimer(HideHandle, this, &ACaptiveSky_2PlayerController::CloseConversation, 2.f, false);
		return;
	}
	#if WITH_EDITOR
	ConversationTargetDisplayName = ConversationTarget->GetActorLabel();
	#else
	ConversationTargetDisplayName = ConversationTarget->Memory
		? ConversationTarget->Memory->GetResolvedAgentId()
		: ConversationTarget->GetName();
	#endif
	ConversationWidget->OpenFor(ConversationTargetDisplayName);
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ConversationWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ACaptiveSky_2PlayerController::SubmitConversation(const FString& Utterance)
{
	const FString Clean = Utterance.TrimStartAndEnd();
	if (Clean.IsEmpty() || !ConversationWidget || !ConversationTarget || !ConversationTarget->Brain || !GetPawn()) return;
	if (FVector::DistSquared(GetPawn()->GetActorLocation(), ConversationTarget->GetActorLocation()) > FMath::Square(ConversationRadius))
	{
		ConversationWidget->ShowMessage(TEXT("They are no longer close enough to hear you."), true);
		return;
	}
	if (ConversationTarget->Brain->bRequestInFlight)
	{
		ConversationWidget->ShowMessage(FString::Printf(TEXT("%s is still thinking."), *ConversationTargetDisplayName), true);
		return;
	}
	ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationTarget->Brain->OnDecisionReady.AddDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationWidget->ShowMessage(FString::Printf(TEXT("%s is thinking…"), *ConversationTargetDisplayName), false);
	ConversationTarget->Brain->RequestDecision(Clean);
}

void ACaptiveSky_2PlayerController::HandleConversationDecision(const FAgentDecision& Decision)
{
	if (ConversationTarget && ConversationTarget->Brain)
		ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	if (!ConversationWidget) return;
	if (!Decision.bValid)
	{
		ConversationWidget->ShowMessage(FString::Printf(TEXT("%s could not gather their thoughts."), *ConversationTargetDisplayName), true);
		return;
	}
	ConversationWidget->ShowMessage(Decision.Speech.IsEmpty()
		? FString::Printf(TEXT("%s considers what you said, but does not answer aloud."), *ConversationTargetDisplayName)
		: FString::Printf(TEXT("%s: %s"), *ConversationTargetDisplayName, *Decision.Speech), true);
}

void ACaptiveSky_2PlayerController::CloseConversation()
{
	if (ConversationTarget && ConversationTarget->Brain)
		ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationTarget = nullptr;
	ConversationTargetDisplayName.Reset();
	if (ConversationWidget) ConversationWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

bool ACaptiveSky_2PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
