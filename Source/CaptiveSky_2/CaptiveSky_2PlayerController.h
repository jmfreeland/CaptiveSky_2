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
class UCaptiveSkyArrangementWidget;
class AAutonomousAgentCharacter;
class AIslandGuestBook;
class AIslandArrangement;
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
	void SubmitVisitorArrangement(const FString& Form, const FString& Title, const FString& Intent);
	void SubmitVisitorArrangementResponse(const FString& Intent);
	void CloseArrangement();
	
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

	UPROPERTY()
	TObjectPtr<UCaptiveSkyArrangementWidget> ArrangementWidget;

	TWeakObjectPtr<AIslandGuestBook> GuestBookTarget;
	TWeakObjectPtr<AIslandArrangement> ArrangementTarget;
	bool bGuestBookPanelOpen = false;
	bool bArrangementPanelOpen = false;

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float AmbientSpeechRadius = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float AmbientSpeechDurationSeconds = 6.f;

	FTimerHandle AmbientSpeechHideTimer;
	FTimerHandle InteractionHintRefreshTimer;

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float ConversationRadius = 500.f;

	/** World interactions require a visible responsive target this close. */
	UPROPERTY(EditAnywhere, Category = "Island Interaction", meta = (ClampMin = "100", ClampMax = "600"))
	float IslandInteractionRadius = 400.f;

	/** Real-time pause between visitor-triggered responses at the same target. */
	UPROPERTY(EditAnywhere, Category = "Island Interaction", meta = (ClampMin = "30", ClampMax = "900"))
	float IslandInteractionCooldownSeconds = 300.f;

	/** How often the local-only, non-interactive proximity hint refreshes. */
	UPROPERTY(EditAnywhere, Category = "Island Interaction", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float InteractionHintRefreshSeconds = 0.25f;

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
	void UpdateWorldInteractionHint();

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
	void ToggleConversation();
	void HandleEscape();
	virtual void SetGuestBookInputMode(bool bOpen);
	virtual void SetArrangementInputMode(bool bOpen);
	AAutonomousAgentCharacter* FindNearestConversationAgent() const;
	void InteractWithNearestWorldObject();
	void OpenGuestBook(AActor* Target);
	void OpenArrangement(AActor* Target);
	AActor* FindNearestWorldInteraction() const;
	void ShowWorldInteractionCaption(const FString& Caption);

	UFUNCTION()
	void HandleConversationDecision(const FAgentDecision& Decision);

	UFUNCTION()
	void HandleAmbientAgentSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech);

	void HideAmbientSpeech();
	FString DescribeWorldInteractionHint(AActor* Target) const;
	TMap<TWeakObjectPtr<AActor>, double> WorldInteractionCooldowns;

};
