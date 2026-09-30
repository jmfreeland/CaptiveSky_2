#include "AgentDataPaths.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

FString CaptiveSkyDataPaths::ResolveProjectDataPath(const FString& RelativePath)
{
	FString DataRootOverride;
	FParse::Value(FCommandLine::Get(), TEXT("CaptiveSkyDataRoot="), DataRootOverride);
	return ResolveProjectDataPathWithRoot(FPaths::ProjectDir(), DataRootOverride, RelativePath);
}

FString CaptiveSkyDataPaths::ResolveProjectDataPathWithRoot(const FString& ProjectDirectory,
	const FString& DataRootOverride, const FString& RelativePath)
{
	const FString DataRoot = DataRootOverride.IsEmpty()
		? ProjectDirectory
		: (FPaths::IsRelative(DataRootOverride) ? ProjectDirectory / DataRootOverride : DataRootOverride);
	return FPaths::ConvertRelativePathToFull(DataRoot / RelativePath);
}
