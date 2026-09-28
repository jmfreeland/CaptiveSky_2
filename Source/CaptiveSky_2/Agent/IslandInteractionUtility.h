#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/** Shared, perception-checked responses for visitors and autonomous residents. */
namespace IslandInteractionUtility
{
	static constexpr float DefaultInteractionRange = 400.f;

	/** Resolve the stable interaction name for a visible landmark or nearby wild creature. */
	CAPTIVESKY_2_API FName GetTargetTag(const AActor* Target);

	/** Require a close, visible target in the same world. */
	CAPTIVESKY_2_API bool CanInteract(const AActor* Observer, const AActor* Target, float MaxRange = DefaultInteractionRange);

	/** Select the nearest supported visible target, using the same bounds and perception rule as visitor input. */
	CAPTIVESKY_2_API AActor* FindNearestVisibleTarget(const AActor* Observer, UWorld* World, float MaxRange = DefaultInteractionRange);

	/** Resident inspection perception: actor-to-actor visibility, retaining hidden landmark markers. */
	CAPTIVESKY_2_API bool CanInspect(const AActor* Observer, const AActor* Target, float MaxRange = DefaultInteractionRange);

	/** Perform a short response after the caller validates perception; this never changes saved world state. */
	CAPTIVESKY_2_API bool Perform(AActor* Observer, AActor* Target, FString& OutFact);
}
