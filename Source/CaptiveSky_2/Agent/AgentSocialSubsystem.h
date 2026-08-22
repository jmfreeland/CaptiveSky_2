// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AgentSocialSubsystem.generated.h"

class AAutonomousAgentCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmbientAgentSpeech,
	AAutonomousAgentCharacter*, Speaker, const FString&, Speech);

/** World-scoped presentation bus for speech that was actually spoken aloud nearby. */
UCLASS()
class CAPTIVESKY_2_API UAgentSocialSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Agent Social")
	FOnAmbientAgentSpeech OnAmbientSpeech;

	void PublishSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech);
};
