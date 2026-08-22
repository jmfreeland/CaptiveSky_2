// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentSocialSubsystem.h"

void UAgentSocialSubsystem::PublishSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech)
{
	if (Speaker && !Speech.IsEmpty())
	{
		OnAmbientSpeech.Broadcast(Speaker, Speech);
	}
}
