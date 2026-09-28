// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CaptiveSky_2PlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UCaptiveSkyConversationWidget;
class UCaptiveSkyAmbientSpeechWidget;
class UCaptiveSkyGuestBookWidget;
class AAutonomousAgentCharacter;
class AIslandGuestBook;
class AActor;
struct FAgentDecision;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ACaptiveSky_2PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SubmitConversation(const FString& Utterance);
	void CloseConversation();
	void SubmitGuestBookEntry(const FString& Line);
	void CloseGuestBook();
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY()
	TObjectPtr<UCaptiveSkyConversationWidget> ConversationWidget;

	UPROPERTY()
	TObjectPtr<UCaptiveSkyAmbientSpeechWidget> AmbientSpeechWidget;

	UPROPERTY()
	TObjectPtr<UCaptiveSkyGuestBookWidget> GuestBookWidget;

	TWeakObjectPtr<AIslandGuestBook> GuestBookTarget;
	bool bGuestBookPanelOpen = false;

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float AmbientSpeechRadius = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float AmbientSpeechDurationSeconds = 6.f;

	FTimerHandle AmbientSpeechHideTimer;

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float ConversationRadius = 500.f;

	/** World interactions require a visible responsive target this close. */
	UPROPERTY(EditAnywhere, Category = "Island Interaction", meta = (ClampMin = "100", ClampMax = "600"))
	float IslandInteractionRadius = 400.f;

	/** Real-time pause between visitor-triggered responses at the same target. */
	UPROPERTY(EditAnywhere, Category = "Island Interaction", meta = (ClampMin = "30", ClampMax = "900"))
	float IslandInteractionCooldownSeconds = 300.f;

	UPROPERTY()
	TObjectPtr<AAutonomousAgentCharacter> ConversationTarget;

	FString ConversationTargetDisplayName;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
	void ToggleConversation();
	void HandleEscape();
	virtual void SetGuestBookInputMode(bool bOpen);
	AAutonomousAgentCharacter* FindNearestConversationAgent() const;
	void InteractWithNearestWorldObject();
	void OpenGuestBook(AActor* Target);
	AActor* FindNearestWorldInteraction() const;
	void ShowWorldInteractionCaption(const FString& Caption);

	UFUNCTION()
	void HandleConversationDecision(const FAgentDecision& Decision);

	UFUNCTION()
	void HandleAmbientAgentSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech);

	void HideAmbientSpeech();
	TMap<TWeakObjectPtr<AActor>, double> WorldInteractionCooldowns;

};
