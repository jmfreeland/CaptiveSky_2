// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Appends bounded, human-reviewable proposals for world assets and upgrades. */
namespace IslandAssetRequestQueue
{
	struct FReviewedOutcome
	{
		FString Id;
		FString RequestType;
		FString Description;
		FString Target;
		FString UpgradeKind;
		FString Status;
	};

	CAPTIVESKY_2_API bool AppendRequest(const FString& Requester, const FString& Description,
		FString& OutRequestId, FString& OutError, const FString& InboxPathOverride = FString(),
		const FString& RequestType = TEXT("new_object"), const FString& Target = FString(),
		const FString& UpgradeKind = FString());

	/** Reads completed human-review states for one requester without changing the inbox. */
	CAPTIVESKY_2_API bool ReadReviewedOutcomes(const FString& Requester, TArray<FReviewedOutcome>& OutOutcomes,
		FString& OutError, const FString& InboxPathOverride = FString());
}
