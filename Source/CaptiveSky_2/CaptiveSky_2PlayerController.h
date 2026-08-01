// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CaptiveSky_2PlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UCaptiveSkyConversationWidget;
class AAutonomousAgentCharacter;
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

	UPROPERTY(EditAnywhere, Category = "Conversation")
	float ConversationRadius = 500.f;

	UPROPERTY()
	TObjectPtr<AAutonomousAgentCharacter> ConversationTarget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
	void ToggleConversation();
	AAutonomousAgentCharacter* FindNearestConversationAgent() const;

	UFUNCTION()
	void HandleConversationDecision(const FAgentDecision& Decision);

};
