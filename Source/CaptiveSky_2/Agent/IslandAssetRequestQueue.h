// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Appends bounded, human-reviewable requests for the local ComfyBlender asset pipeline. */
namespace IslandAssetRequestQueue
{
	CAPTIVESKY_2_API bool AppendRequest(const FString& Requester, const FString& Description,
		FString& OutRequestId, FString& OutError, const FString& InboxPathOverride = FString());
}
