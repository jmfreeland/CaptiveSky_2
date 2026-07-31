// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AutonomousAgentCharacter.generated.h"

class UAgentMemoryComponent;
class UAgentBrainComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/**
 * Base class for LLM-driven autonomous characters.
 *
 * Deliberately has no mesh/skeleton/animation-blueprint references -- those
 * are set per-Blueprint subclass (e.g. BP_Agent_Placeholder now,
 * BP_Agent_Crow later), matching this project's existing convention of
 * keeping content references out of C++ (see CaptiveSky_2Character). This
 * is what makes the body swappable without touching this class.
 */
UCLASS(Abstract)
class CAPTIVESKY_2_API AAutonomousAgentCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAutonomousAgentCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAgentMemoryComponent> Memory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAgentBrainComponent> Brain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> EyeCapture;

	UPROPERTY(BlueprintReadOnly, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextureRenderTarget2D> EyeRenderTarget;

	// Socket to attach the eye capture to, if the assigned mesh has one. Falls back to a fixed offset otherwise.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FName EyeSocketName = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	int32 EyeCaptureResolution = 512;

	/** Renders the current first-person view and returns it as base64-encoded PNG. Empty string on failure. */
	UFUNCTION(BlueprintCallable, Category = "Agent")
	FString CaptureFirstPersonSnapshot() const;

protected:
	virtual void BeginPlay() override;
};
