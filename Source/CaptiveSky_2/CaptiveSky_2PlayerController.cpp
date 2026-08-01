// Copyright Epic Games, Inc. All Rights Reserved.


#include "CaptiveSky_2PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CaptiveSky_2.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "CaptiveSkyConversationWidget.h"
#include "Agent/AutonomousAgentCharacter.h"
#include "Agent/AgentBrainComponent.h"
#include "EngineUtils.h"
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
	}
}

void ACaptiveSky_2PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACaptiveSky_2PlayerController::ToggleConversation);
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
	ConversationWidget->OpenFor(ConversationTarget->GetActorLabel());
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
		ConversationWidget->ShowMessage(TEXT("Aster is still thinking."), true);
		return;
	}
	ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationTarget->Brain->OnDecisionReady.AddDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationWidget->ShowMessage(TEXT("Aster is thinking…"), false);
	ConversationTarget->Brain->RequestDecision(Clean);
}

void ACaptiveSky_2PlayerController::HandleConversationDecision(const FAgentDecision& Decision)
{
	if (ConversationTarget && ConversationTarget->Brain)
		ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	if (!ConversationWidget) return;
	if (!Decision.bValid)
	{
		ConversationWidget->ShowMessage(TEXT("Aster could not gather his thoughts."), true);
		return;
	}
	ConversationWidget->ShowMessage(Decision.Speech.IsEmpty()
		? TEXT("Aster considers what you said, but does not answer aloud.")
		: FString::Printf(TEXT("Aster: %s"), *Decision.Speech), true);
}

void ACaptiveSky_2PlayerController::CloseConversation()
{
	if (ConversationTarget && ConversationTarget->Brain)
		ConversationTarget->Brain->OnDecisionReady.RemoveDynamic(this, &ACaptiveSky_2PlayerController::HandleConversationDecision);
	ConversationTarget = nullptr;
	if (ConversationWidget) ConversationWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

bool ACaptiveSky_2PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
