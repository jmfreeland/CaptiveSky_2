#pragma once

#include "CoreMinimal.h"

/** Shared paths for persistent resident and Island data. Defaults remain at the project root. */
namespace CaptiveSkyDataPaths
{
	/** Resolves a project-data-relative path, honoring optional -CaptiveSkyDataRoot=<path>. */
	CAPTIVESKY_2_API FString ResolveProjectDataPath(const FString& RelativePath);

	/** Pure resolver used by automation tests; a relative override is based on ProjectDirectory. */
	CAPTIVESKY_2_API FString ResolveProjectDataPathWithRoot(const FString& ProjectDirectory,
		const FString& DataRootOverride, const FString& RelativePath);
}
