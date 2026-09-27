#include "IslandWorldStateSubsystem.h"
#include "IslandNest.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWorldState, Log, All);

bool UIslandWorldStateSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Never touch persistent state from editor preview worlds.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FString UIslandWorldStateSubsystem::GetStorageFilePath() const
{
	if (!StorageFileOverride.IsEmpty()) return StorageFileOverride;
	// Worlds created in code (test fixtures) have no saved map, so they get no lasting state.
	if (!GetWorld() || GetWorld()->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"))) return FString();
	const FString MapName = UWorld::RemovePIEPrefix(GetWorld()->GetMapName());
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("WorldState") / (MapName + TEXT(".json")));
}

const FIslandNestRecord* UIslandWorldStateSubsystem::FindNest(FName SiteTag) const
{
	return Nests.FindByPredicate([SiteTag](const FIslandNestRecord& Record) { return Record.SiteTag == SiteTag; });
}

void UIslandWorldStateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	LoadAndSpawn();
}

void UIslandWorldStateSubsystem::Deinitialize()
{
	// Nest actors are transient and are torn down with the world itself.
	NestActors.Reset();
	Super::Deinitialize();
}

void UIslandWorldStateSubsystem::LoadAndSpawn()
{
	DestroyNestActors();
	Nests.Reset();
	SavedHour.Reset();
	bStorageUnreadable = false;
	FString Contents;
	const FString Path = GetStorageFilePath();
	if (Path.IsEmpty() || !FPaths::FileExists(Path)) return;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Contents, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Contents), Root) || !Root.IsValid())
	{
		// Leave the unreadable file untouched; a later save would otherwise erase whatever it holds.
		UE_LOG(LogIslandWorldState, Error, TEXT("Could not read world state %s; no lasting changes were loaded and none will be saved this session."), *Path);
		bStorageUnreadable = true;
		return;
	}
	const TSharedPtr<FJsonObject>* Clock = nullptr;
	double Hour = 0.0;
	if (Root->TryGetObjectField(TEXT("clock"), Clock) && (*Clock)->TryGetNumberField(TEXT("hour"), Hour) && Hour >= 0.0 && Hour < 24.0)
		SavedHour = static_cast<float>(Hour);
	const TArray<TSharedPtr<FJsonValue>>* NestValues = nullptr;
	if (Root->TryGetArrayField(TEXT("nests"), NestValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *NestValues)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object)) continue;
			FIslandNestRecord Record;
			FString Site, Created, Updated;
			(*Object)->TryGetStringField(TEXT("site"), Site);
			Record.SiteTag = FName(*Site);
			(*Object)->TryGetNumberField(TEXT("layers"), Record.Layers);
			Record.Layers = FMath::Clamp(Record.Layers, 0, MaxNestLayers);
			const TArray<TSharedPtr<FJsonValue>>* Where = nullptr;
			if ((*Object)->TryGetArrayField(TEXT("location"), Where) && Where->Num() == 3)
				Record.Location = FVector((*Where)[0]->AsNumber(), (*Where)[1]->AsNumber(), (*Where)[2]->AsNumber());
			(*Object)->TryGetStringArrayField(TEXT("builders"), Record.Builders);
			if ((*Object)->TryGetStringField(TEXT("created_utc"), Created)) FDateTime::ParseIso8601(*Created, Record.CreatedUtc);
			if ((*Object)->TryGetStringField(TEXT("updated_utc"), Updated)) FDateTime::ParseIso8601(*Updated, Record.UpdatedUtc);
			if (Record.SiteTag.IsNone() || Record.Layers <= 0 || FindNest(Record.SiteTag)) continue;
			Nests.Add(Record);
			RefreshNestActor(Record);
		}
	}
	UE_LOG(LogIslandWorldState, Log, TEXT("Loaded %d lasting nest(s) from %s"), Nests.Num(), *Path);
}

bool UIslandWorldStateSubsystem::Save() const
{
	if (bStorageUnreadable || GetStorageFilePath().IsEmpty()) return false;
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), 1);
	TArray<TSharedPtr<FJsonValue>> NestValues;
	for (const FIslandNestRecord& Record : Nests)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("site"), Record.SiteTag.ToString());
		Object->SetNumberField(TEXT("layers"), Record.Layers);
		Object->SetArrayField(TEXT("location"), {
			MakeShared<FJsonValueNumber>(Record.Location.X), MakeShared<FJsonValueNumber>(Record.Location.Y), MakeShared<FJsonValueNumber>(Record.Location.Z) });
		TArray<TSharedPtr<FJsonValue>> Builders;
		for (const FString& Builder : Record.Builders) Builders.Add(MakeShared<FJsonValueString>(Builder));
		Object->SetArrayField(TEXT("builders"), Builders);
		Object->SetStringField(TEXT("created_utc"), Record.CreatedUtc.ToIso8601());
		Object->SetStringField(TEXT("updated_utc"), Record.UpdatedUtc.ToIso8601());
		NestValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("nests"), NestValues);
	if (SavedHour.IsSet())
	{
		const TSharedRef<FJsonObject> Clock = MakeShared<FJsonObject>();
		Clock->SetNumberField(TEXT("hour"), SavedHour.GetValue());
		Clock->SetStringField(TEXT("saved_utc"), FDateTime::UtcNow().ToIso8601());
		Root->SetObjectField(TEXT("clock"), Clock);
	}

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	if (!FJsonSerializer::Serialize(Root, Writer)) return false;
	// Write beside the target, then replace it, so an interrupted save cannot truncate existing state.
	const FString Path = GetStorageFilePath();
	const FString Temporary = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Json, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
		!IFileManager::Get().Move(*Path, *Temporary, true, true))
	{
		UE_LOG(LogIslandWorldState, Error, TEXT("Failed to save world state to %s"), *Path);
		return false;
	}
	return true;
}

int32 UIslandWorldStateSubsystem::AddNestLayer(FName SiteTag, const FVector& SupportLocation, const FString& BuilderAgentId)
{
	if (SiteTag.IsNone()) return 0;
	const TArray<FIslandNestRecord> Previous = Nests;
	FIslandNestRecord* Record = Nests.FindByPredicate([SiteTag](const FIslandNestRecord& Existing) { return Existing.SiteTag == SiteTag; });
	if (!Record)
	{
		Record = &Nests.AddDefaulted_GetRef();
		Record->SiteTag = SiteTag;
		Record->Location = SupportLocation;
		Record->CreatedUtc = FDateTime::UtcNow();
	}
	if (Record->Layers >= MaxNestLayers) return 0;
	++Record->Layers;
	Record->UpdatedUtc = FDateTime::UtcNow();
	if (!BuilderAgentId.IsEmpty()) Record->Builders.AddUnique(BuilderAgentId);
	const FIslandNestRecord Updated = *Record;
	if (!Save())
	{
		// Only report and show changes that will actually outlive this session.
		Nests = Previous;
		return 0;
	}
	RefreshNestActor(Updated);
	return Updated.Layers;
}

bool UIslandWorldStateSubsystem::SaveHour(float Hour)
{
	const TOptional<float> Previous = SavedHour;
	SavedHour = FMath::Clamp(Hour, 0.f, 23.999f);
	if (Save()) return true;
	SavedHour = Previous;
	return false;
}

bool UIslandWorldStateSubsystem::RemoveNest(FName SiteTag)
{
	const TArray<FIslandNestRecord> Previous = Nests;
	if (Nests.RemoveAll([SiteTag](const FIslandNestRecord& Record) { return Record.SiteTag == SiteTag; }) == 0) return false;
	if (!Save())
	{
		Nests = Previous;
		return false;
	}
	if (TWeakObjectPtr<AIslandNest>* Actor = NestActors.Find(SiteTag); Actor && Actor->IsValid()) (*Actor)->Destroy();
	NestActors.Remove(SiteTag);
	return true;
}

void UIslandWorldStateSubsystem::RefreshNestActor(const FIslandNestRecord& Record)
{
	UWorld* World = GetWorld();
	if (!World) return;
	TWeakObjectPtr<AIslandNest>& Actor = NestActors.FindOrAdd(Record.SiteTag);
	if (!Actor.IsValid())
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<AIslandNest>(Record.Location, FRotator::ZeroRotator, Spawn);
		if (!Actor.IsValid()) return;
		Actor->Tags.AddUnique(FName(*(TEXT("Nest_") + Record.SiteTag.ToString())));
	}
	Actor->SetWoven(Record.SiteTag, Record.Layers);
}

void UIslandWorldStateSubsystem::DestroyNestActors()
{
	for (const TPair<FName, TWeakObjectPtr<AIslandNest>>& Pair : NestActors)
		if (Pair.Value.IsValid()) Pair.Value->Destroy();
	NestActors.Reset();
}

static FAutoConsoleCommandWithWorldAndArgs GIslandRemoveNestCommand(
	TEXT("Island.RemoveNest"),
	TEXT("Removes a resident-woven nest and its persistent record. Usage: Island.RemoveNest <SiteTag>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
		if (!State || Args.Num() != 1)
		{
			UE_LOG(LogIslandWorldState, Warning, TEXT("Island.RemoveNest needs a running play world and one SiteTag."));
			return;
		}
		UE_LOG(LogIslandWorldState, Log, TEXT("Island.RemoveNest %s: %s"), *Args[0], State->RemoveNest(FName(*Args[0])) ? TEXT("removed") : TEXT("no nest found"));
	}));
