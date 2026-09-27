#include "IslandWorldStateSubsystem.h"
#include "IslandNest.h"
#include "IslandDayNight.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
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
	// Nest and curio actors are transient and are torn down with the world itself.
	NestActors.Reset();
	CurioActors.Reset();
	Super::Deinitialize();
}

void UIslandWorldStateSubsystem::LoadAndSpawn()
{
	DestroyNestActors();
	for (const TPair<FName, TWeakObjectPtr<AIslandCurio>>& Pair : CurioActors)
		if (Pair.Value.IsValid()) Pair.Value->Destroy();
	CurioActors.Reset();
	for (const TPair<FName, TWeakObjectPtr<AIslandArrangement>>& Pair : ArrangementActors)
		if (Pair.Value.IsValid()) Pair.Value->Destroy();
	ArrangementActors.Reset();
	ArrangementSites.Reset();
	Nests.Reset();
	Curios.Reset();
	SavedHour.Reset();
	SavedDay.Reset();
	SavedWetness.Reset();
	bStorageUnreadable = false;
	const FString Path = GetStorageFilePath();
	if (Path.IsEmpty()) return;
	if (!FPaths::FileExists(Path)) { PlaceCurios(); PlaceArrangementSites(); return; }
	if (!ReadStateFile(Path))
	{
		// Leave the unreadable file untouched; a later save would otherwise erase whatever it holds.
		UE_LOG(LogIslandWorldState, Error, TEXT("Could not read world state %s; no lasting changes were loaded and none will be saved this session."), *Path);
		bStorageUnreadable = true;
		return;
	}
	for (const FIslandNestRecord& Record : Nests) RefreshNestActor(Record);
	for (const FIslandCurioRecord& Record : Curios) RefreshCurioActor(Record);
	for (const FIslandArrangementSite& Site : ArrangementSites) RefreshArrangementActor(Site);
	if (Curios.Num() == 0) PlaceCurios();
	if (ArrangementSites.Num() == 0) PlaceArrangementSites();
	UE_LOG(LogIslandWorldState, Log, TEXT("Loaded %d lasting nest(s) and %d curio(s) from %s"), Nests.Num(), Curios.Num(), *Path);
}

bool UIslandWorldStateSubsystem::ReadStateFile(const FString& Path)
{
	Nests.Reset();
	Curios.Reset();
	ArrangementSites.Reset();
	SavedHour.Reset();
	SavedDay.Reset();
	SavedWetness.Reset();
	FString Contents;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Contents, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Contents), Root) || !Root.IsValid())
		return false;
	const TSharedPtr<FJsonObject>* Clock = nullptr;
	double Hour = 0.0;
	if (Root->TryGetObjectField(TEXT("clock"), Clock) && (*Clock)->TryGetNumberField(TEXT("hour"), Hour) && Hour >= 0.0 && Hour < 24.0)
	{
		SavedHour = static_cast<float>(Hour);
		int32 Day = 1;
		if ((*Clock)->TryGetNumberField(TEXT("day"), Day) && Day >= 1) SavedDay = Day;
	}
	const TSharedPtr<FJsonObject>* Environment = nullptr;
	double Wetness = 0.0;
	if (Root->TryGetObjectField(TEXT("environment"), Environment) &&
		(*Environment)->TryGetNumberField(TEXT("wetness"), Wetness) && FMath::IsFinite(Wetness) && Wetness >= 0.0 && Wetness <= 1.0)
		SavedWetness = static_cast<float>(Wetness);
	const TArray<TSharedPtr<FJsonValue>>* CurioValues = nullptr;
	if (Root->TryGetArrayField(TEXT("curios"), CurioValues))
	{
		const UEnum* KindEnum = StaticEnum<EIslandCurioKind>();
		for (const TSharedPtr<FJsonValue>& Value : *CurioValues)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object)) continue;
			FIslandCurioRecord Record;
			FString Id, Kind;
			(*Object)->TryGetStringField(TEXT("id"), Id);
			(*Object)->TryGetStringField(TEXT("kind"), Kind);
			const int64 KindValue = KindEnum->GetValueByNameString(Kind);
			const TArray<TSharedPtr<FJsonValue>>* Where = nullptr;
			if (Id.IsEmpty() || KindValue == INDEX_NONE || !(*Object)->TryGetArrayField(TEXT("location"), Where) || Where->Num() != 3 || FindCurio(FName(*Id))) continue;
			Record.Id = FName(*Id);
			Record.Kind = static_cast<EIslandCurioKind>(KindValue);
			Record.Location = FVector((*Where)[0]->AsNumber(), (*Where)[1]->AsNumber(), (*Where)[2]->AsNumber());
			(*Object)->TryGetNumberField(TEXT("state"), Record.State);
			(*Object)->TryGetNumberField(TEXT("last_changed_day"), Record.LastChangedDay);
			(*Object)->TryGetStringArrayField(TEXT("contributors"), Record.Contributors);
			Record.State = FMath::Clamp(Record.State, 0, Record.Kind == EIslandCurioKind::Cairn ? AIslandCurio::CairnMaxStones : AIslandCurio::PodOpenState);
			Curios.Add(Record);
		}
	}
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
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* SiteValues = nullptr;
	if (Root->TryGetArrayField(TEXT("arrangement_sites"), SiteValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *SiteValues)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object)) continue;
			FIslandArrangementSite Site;
			FString Id, Form, Created;
			const TArray<TSharedPtr<FJsonValue>>* Where = nullptr;
			if (!(*Object)->TryGetStringField(TEXT("id"), Id) || Id.IsEmpty() || FindArrangementSite(FName(*Id)) ||
				!(*Object)->TryGetArrayField(TEXT("location"), Where) || Where->Num() != 3) continue;
			Site.Id = FName(*Id);
			Site.Location = FVector((*Where)[0]->AsNumber(), (*Where)[1]->AsNumber(), (*Where)[2]->AsNumber());
			const TSharedPtr<FJsonObject>* Work = nullptr;
			if ((*Object)->TryGetObjectField(TEXT("work"), Work) && (*Work)->TryGetStringField(TEXT("form"), Form) && ParseArrangementForm(Form, Site.Form))
			{
				Site.bHasWork = true;
				(*Work)->TryGetNumberField(TEXT("seed"), Site.Seed);
				(*Work)->TryGetStringField(TEXT("title"), Site.Title);
				(*Work)->TryGetStringField(TEXT("intent"), Site.Intent);
				(*Work)->TryGetStringField(TEXT("maker"), Site.MakerAgentId);
				(*Work)->TryGetNumberField(TEXT("day"), Site.Day);
				if ((*Work)->TryGetStringField(TEXT("created_utc"), Created)) FDateTime::ParseIso8601(*Created, Site.CreatedUtc);
				const TArray<TSharedPtr<FJsonValue>>* Responses = nullptr;
				if ((*Work)->TryGetArrayField(TEXT("responses"), Responses))
				{
					for (const TSharedPtr<FJsonValue>& ResponseValue : *Responses)
					{
						const TSharedPtr<FJsonObject>* ResponseObject = nullptr;
						if (Site.Responses.Num() >= AIslandArrangement::MaxResponses || !ResponseValue.IsValid() || !ResponseValue->TryGetObject(ResponseObject)) continue;
						FIslandArrangementResponse& Response = Site.Responses.AddDefaulted_GetRef();
						(*ResponseObject)->TryGetStringField(TEXT("agent"), Response.AgentId);
						(*ResponseObject)->TryGetNumberField(TEXT("day"), Response.Day);
						(*ResponseObject)->TryGetStringField(TEXT("intent"), Response.Intent);
					}
				}
			}
			ArrangementSites.Add(Site);
		}
	}
	return true;
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
	TArray<TSharedPtr<FJsonValue>> CurioValues;
	for (const FIslandCurioRecord& Record : Curios)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("id"), Record.Id.ToString());
		Object->SetStringField(TEXT("kind"), StaticEnum<EIslandCurioKind>()->GetNameStringByValue(static_cast<int64>(Record.Kind)));
		Object->SetArrayField(TEXT("location"), {
			MakeShared<FJsonValueNumber>(Record.Location.X), MakeShared<FJsonValueNumber>(Record.Location.Y), MakeShared<FJsonValueNumber>(Record.Location.Z) });
		Object->SetNumberField(TEXT("state"), Record.State);
		Object->SetNumberField(TEXT("last_changed_day"), Record.LastChangedDay);
		TArray<TSharedPtr<FJsonValue>> Contributors;
		for (const FString& Contributor : Record.Contributors) Contributors.Add(MakeShared<FJsonValueString>(Contributor));
		Object->SetArrayField(TEXT("contributors"), Contributors);
		CurioValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("curios"), CurioValues);
	TArray<TSharedPtr<FJsonValue>> SiteValues;
	for (const FIslandArrangementSite& Site : ArrangementSites)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("id"), Site.Id.ToString());
		Object->SetArrayField(TEXT("location"), {
			MakeShared<FJsonValueNumber>(Site.Location.X), MakeShared<FJsonValueNumber>(Site.Location.Y), MakeShared<FJsonValueNumber>(Site.Location.Z) });
		if (Site.bHasWork)
		{
			const TSharedRef<FJsonObject> Work = MakeShared<FJsonObject>();
			Work->SetStringField(TEXT("form"), FormName(Site.Form));
			Work->SetNumberField(TEXT("seed"), Site.Seed);
			Work->SetStringField(TEXT("title"), Site.Title);
			Work->SetStringField(TEXT("intent"), Site.Intent);
			Work->SetStringField(TEXT("maker"), Site.MakerAgentId);
			Work->SetNumberField(TEXT("day"), Site.Day);
			Work->SetStringField(TEXT("created_utc"), Site.CreatedUtc.ToIso8601());
			TArray<TSharedPtr<FJsonValue>> Responses;
			for (const FIslandArrangementResponse& Response : Site.Responses)
			{
				const TSharedRef<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
				ResponseObject->SetStringField(TEXT("agent"), Response.AgentId);
				ResponseObject->SetNumberField(TEXT("day"), Response.Day);
				ResponseObject->SetStringField(TEXT("intent"), Response.Intent);
				Responses.Add(MakeShared<FJsonValueObject>(ResponseObject));
			}
			Work->SetArrayField(TEXT("responses"), Responses);
			Object->SetObjectField(TEXT("work"), Work);
		}
		SiteValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("arrangement_sites"), SiteValues);
	if (SavedHour.IsSet())
	{
		const TSharedRef<FJsonObject> Clock = MakeShared<FJsonObject>();
		Clock->SetNumberField(TEXT("hour"), SavedHour.GetValue());
		Clock->SetNumberField(TEXT("day"), SavedDay.Get(1));
		Clock->SetStringField(TEXT("saved_utc"), FDateTime::UtcNow().ToIso8601());
		Root->SetObjectField(TEXT("clock"), Clock);
	}
	if (SavedWetness.IsSet())
	{
		const TSharedRef<FJsonObject> Environment = MakeShared<FJsonObject>();
		Environment->SetNumberField(TEXT("wetness"), SavedWetness.GetValue());
		Root->SetObjectField(TEXT("environment"), Environment);
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

bool UIslandWorldStateSubsystem::SaveClock(float Hour, int32 Day)
{
	const TOptional<float> PreviousHour = SavedHour;
	const TOptional<int32> PreviousDay = SavedDay;
	SavedHour = FMath::Clamp(Hour, 0.f, 23.999f);
	SavedDay = FMath::Max(1, Day);
	// A new Island day ages every arrangement a little.
	if (SavedDay.GetValue() != ShownArrangementDay)
		for (const FIslandArrangementSite& Site : ArrangementSites) RefreshArrangementActor(Site);
	if (Save()) return true;
	SavedHour = PreviousHour;
	SavedDay = PreviousDay;
	return false;
}

bool UIslandWorldStateSubsystem::SaveWetness(float Wetness)
{
	if (!FMath::IsFinite(Wetness)) return false;
	const TOptional<float> PreviousWetness = SavedWetness;
	SavedWetness = FMath::Clamp(Wetness, 0.f, 1.f);
	if (Save()) return true;
	SavedWetness = PreviousWetness;
	return false;
}

int32 UIslandWorldStateSubsystem::CurrentIslandDay(const UWorld* World)
{
	if (World)
		for (TActorIterator<AIslandDayNight> It(World); It; ++It) return It->DayNumber;
	return 1;
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

const FIslandCurioRecord* UIslandWorldStateSubsystem::FindCurio(FName Id) const
{
	return Curios.FindByPredicate([Id](const FIslandCurioRecord& Record) { return Record.Id == Id; });
}

namespace
{
	/** Finds open, walkable-looking ground near Desired, trying a few nearby points. */
	bool FindCurioGround(UWorld* World, const FVector& Desired, FRandomStream& Random, float Jitter, FVector& OutGround)
	{
		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		for (int32 Attempt = 0; Attempt < 10; ++Attempt)
		{
			const FVector2D Offset = Attempt == 0 ? FVector2D::ZeroVector : FVector2D(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * Jitter;
			const FVector Probe = Desired + FVector(Offset.X, Offset.Y, 0.f);
			FHitResult Hit;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandCurioGround), false);
			if (!World->LineTraceSingleByChannel(Hit, Probe + FVector(0.f, 0.f, 3000.f), Probe - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Query)) continue;
			// Reject roofs, trunks, rocks, and residents: curios sit on gentle open ground.
			if (Hit.ImpactNormal.Z < 0.75f || Cast<APawn>(Hit.GetActor()) || Hit.ImpactPoint.Z > Desired.Z + 600.f) continue;
			FNavLocation Walkable;
			// Grounded residents must be able to walk up to it when the level has navigation.
			if (Navigation && Navigation->GetDefaultNavDataInstance() &&
				(!Navigation->ProjectPointToNavigation(Hit.ImpactPoint, Walkable, FVector(80.f, 80.f, 150.f)) || FVector::Dist2D(Walkable.Location, Hit.ImpactPoint) > 80.f))
				continue;
			OutGround = Hit.ImpactPoint;
			return true;
		}
		return false;
	}
}

bool UIslandWorldStateSubsystem::BuildCurioLayout(UWorld* World, TArray<FIslandCurioRecord>& OutLayout)
{
	OutLayout.Reset();
	if (!World) return false;
	const AActor* ListeningStones = nullptr;
	const AActor* WindArch = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("IslandLandmark"))) continue;
		if (It->ActorHasTag(TEXT("ListeningStones"))) ListeningStones = *It;
		if (It->ActorHasTag(TEXT("WindArch"))) WindArch = *It;
	}
	// Only levels with the Island's landmarks get curios.
	if (!ListeningStones) return false;

	const FVector Anchor = ListeningStones->GetActorLocation();
	FRandomStream Random(7331);
	for (int32 Heading = 0; Heading < 8; ++Heading)
	{
		// A gently curving trail leading away from the stones, ending at the pod.
		const float Base = Random.FRandRange(0.f, 2.f * PI);
		TArray<FIslandCurioRecord> Layout;
		bool bPlaced = true;
		for (int32 Index = 0; Index <= 6 && bPlaced; ++Index)
		{
			const float Distance = 700.f + Index * 500.f + (Index == 6 ? 150.f : 0.f);
			const float Angle = Base + 0.18f * FMath::Sin(Index * 0.9f);
			FVector Ground;
			bPlaced = FindCurioGround(World, Anchor + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance, Random, 120.f, Ground);
			FIslandCurioRecord& Record = Layout.AddDefaulted_GetRef();
			Record.Id = Index < 6 ? FName(*FString::Printf(TEXT("PaleStone_%d"), Index + 1)) : FName(TEXT("Seedpod"));
			Record.Kind = Index < 6 ? EIslandCurioKind::PaleStone : EIslandCurioKind::SeedPod;
			Record.Location = Ground;
		}
		// The cairn stands apart, on the far side of the WindArch (or the stones if there is no arch).
		const FVector CairnAnchor = WindArch ? WindArch->GetActorLocation() : Anchor;
		FVector CairnGround;
		if (!bPlaced || !FindCurioGround(World, CairnAnchor + FVector(FMath::Cos(Base + PI), FMath::Sin(Base + PI), 0.f) * 900.f, Random, 250.f, CairnGround)) continue;
		FIslandCurioRecord& Cairn = Layout.AddDefaulted_GetRef();
		Cairn.Id = TEXT("Cairn");
		Cairn.Kind = EIslandCurioKind::Cairn;
		Cairn.Location = CairnGround;
		Cairn.State = 3; // Someone began it long ago; who is not recorded anywhere.
		OutLayout = Layout;
		return true;
	}
	UE_LOG(LogIslandWorldState, Warning, TEXT("Could not find open walkable ground for the Island curios; none were placed."));
	return false;
}

bool UIslandWorldStateSubsystem::PlaceCurios()
{
	if (bStorageUnreadable || GetStorageFilePath().IsEmpty() || Curios.Num() > 0 || !BuildCurioLayout(GetWorld(), Curios)) return false;
	if (!Save())
	{
		Curios.Reset();
		return false;
	}
	for (const FIslandCurioRecord& Record : Curios) RefreshCurioActor(Record);
	UE_LOG(LogIslandWorldState, Log, TEXT("Placed %d curios near the ListeningStones."), Curios.Num());
	return true;
}

FString UIslandWorldStateSubsystem::ExamineCurio(FName Id, int32 Today, const FString& ContributorAgentId)
{
	FIslandCurioRecord* Record = Curios.FindByPredicate([Id](const FIslandCurioRecord& Existing) { return Existing.Id == Id; });
	if (!Record) return TEXT("There is nothing here to examine.");
	const FIslandCurioRecord Before = *Record;
	FString Fact;
	switch (Record->Kind)
	{
	case EIslandCurioKind::PaleStone:
		return TEXT("A small, smooth pale stone, set deliberately into the ground. It is cool and unmarked; nothing lies beneath it. You leave it as it was.");
	case EIslandCurioKind::SeedPod:
		if (Record->State >= AIslandCurio::PodOpenState)
			return TEXT("The pod stands open. The small seed inside glows faintly and steadily; it does not respond to touch, and what it is remains unknown.");
		if (Record->LastChangedDay == Today)
			return TEXT("The pod feels faintly warm, but nothing about it has changed since earlier today.");
		++Record->State;
		Record->LastChangedDay = Today;
		Fact = Record->State == 1
			? TEXT("As you examine the closed pod, two of its husk-leaves slowly peel back, as though it had been waiting for a visitor. Inside is only darkness for now.")
			: Record->State == 2
			? TEXT("Two more husk-leaves curl open. A pale light shows through the gap; it was not there on the last day anyone came.")
			: TEXT("The last husk-leaves fold back. A small seed rests inside, glowing faintly and steadily. It stays where it is.");
		Fact += TEXT(" This change remains after this session.");
		break;
	case EIslandCurioKind::Cairn:
		if (Record->State >= AIslandCurio::CairnMaxStones)
			return FString::Printf(TEXT("The cairn stands %d stones high; the top is too narrow to hold another."), Record->State);
		if (Record->LastChangedDay == Today)
			return FString::Printf(TEXT("The cairn stands %d stones high. Its top stone was set there today and still sits a little unsteadily; another would topple it."), Record->State);
		++Record->State;
		Record->LastChangedDay = Today;
		if (!ContributorAgentId.IsEmpty() && !Record->Contributors.Contains(ContributorAgentId)) Record->Contributors.Add(ContributorAgentId);
		Fact = FString::Printf(TEXT("You find a flat stone nearby and set it on the small cairn; it now stands %d stones high. Someone began it before you came. The shared record will let you recognize this contribution later, but does not identify who set the other stones."), Record->State);
		break;
	}
	if (!Save())
	{
		*Record = Before;
		return TEXT("You examined it, but the change could not be kept, so nothing lasting happened.");
	}
	RefreshCurioActor(*Record);
	return Fact;
}

bool UIslandWorldStateSubsystem::ForgetCurios()
{
	const TArray<FIslandCurioRecord> Previous = Curios;
	Curios.Reset();
	if (!Save())
	{
		Curios = Previous;
		return false;
	}
	for (const TPair<FName, TWeakObjectPtr<AIslandCurio>>& Pair : CurioActors)
		if (Pair.Value.IsValid()) Pair.Value->Destroy();
	CurioActors.Reset();
	return true;
}

void UIslandWorldStateSubsystem::RefreshCurioActor(const FIslandCurioRecord& Record)
{
	UWorld* World = GetWorld();
	if (!World) return;
	TWeakObjectPtr<AIslandCurio>& Actor = CurioActors.FindOrAdd(Record.Id);
	if (!Actor.IsValid())
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<AIslandCurio>(Record.Location + FVector(0.f, 0.f, AIslandCurio::GroundClearance), FRotator::ZeroRotator, Spawn);
		if (!Actor.IsValid()) return;
		// First tag is the unique move_to/interact target, matching the landmark convention.
		Actor->Tags.Insert(Record.Id, 0);
	}
	Actor->ShowRecord(Record);
}

static FAutoConsoleCommandWithWorld GIslandForgetCuriosCommand(
	TEXT("Island.ForgetCurios"),
	TEXT("Forgets every Island curio (stones, pod, cairn) and its state; fresh ones are placed the next time play begins."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
		UE_LOG(LogIslandWorldState, Log, TEXT("Island.ForgetCurios: %s"), State && State->ForgetCurios() ? TEXT("forgotten") : TEXT("nothing changed (needs a running play world with writable state)"));
	}));

const FIslandArrangementSite* UIslandWorldStateSubsystem::FindArrangementSite(FName Id) const
{
	return ArrangementSites.FindByPredicate([Id](const FIslandArrangementSite& Site) { return Site.Id == Id; });
}

bool UIslandWorldStateSubsystem::ParseArrangementForm(const FString& Text, EIslandArrangementForm& OutForm)
{
	const FString Form = Text.TrimStartAndEnd().ToLower();
	if (Form == TEXT("ring")) { OutForm = EIslandArrangementForm::Ring; return true; }
	if (Form == TEXT("line")) { OutForm = EIslandArrangementForm::Line; return true; }
	if (Form == TEXT("spiral")) { OutForm = EIslandArrangementForm::Spiral; return true; }
	if (Form == TEXT("pair")) { OutForm = EIslandArrangementForm::Pair; return true; }
	return false;
}

FString UIslandWorldStateSubsystem::FormName(EIslandArrangementForm Form)
{
	switch (Form)
	{
	case EIslandArrangementForm::Line: return TEXT("line");
	case EIslandArrangementForm::Spiral: return TEXT("spiral");
	case EIslandArrangementForm::Pair: return TEXT("pair");
	case EIslandArrangementForm::Ring:
	default: return TEXT("ring");
	}
}

namespace
{
	/** Single-line, printable, bounded text for model-written titles and intents. */
	FString CleanArrangementText(const FString& Text, int32 MaxLength)
	{
		FString Clean;
		for (const TCHAR Character : Text)
			Clean.AppendChar(FChar::IsPrint(Character) && Character != TEXT('"') ? Character : TEXT(' '));
		while (Clean.ReplaceInline(TEXT("  "), TEXT(" ")) > 0) {}
		Clean = Clean.Left(MaxLength).TrimStartAndEnd();
		// Sentences built around this text add their own full stop.
		while (Clean.EndsWith(TEXT("."))) Clean.LeftChopInline(1);
		return Clean.TrimEnd();
	}

	bool FindOpenGround(UWorld* World, const FVector& Desired, FVector& OutGround)
	{
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandArrangementGround), false);
		if (!World->LineTraceSingleByChannel(Hit, Desired + FVector(0.f, 0.f, 3000.f), Desired - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Query)) return false;
		if (Hit.ImpactNormal.Z < 0.9f || Cast<APawn>(Hit.GetActor()) || Hit.ImpactPoint.Z > Desired.Z + 600.f) return false;
		// Level, clear of anything a stone circle would sit on or under: probe a 1.3 m disc for obstacles.
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const float Angle = 2.f * PI * Index / 8.f;
			const FVector Edge = Hit.ImpactPoint + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 130.f;
			FHitResult EdgeHit;
			if (!World->LineTraceSingleByChannel(EdgeHit, Edge + FVector(0.f, 0.f, 250.f), Edge - FVector(0.f, 0.f, 120.f), ECC_Visibility, Query)) return false;
			if (FMath::Abs(EdgeHit.ImpactPoint.Z - Hit.ImpactPoint.Z) > 30.f) return false;
		}
		if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World); Navigation && Navigation->GetDefaultNavDataInstance())
		{
			FNavLocation Walkable;
			if (!Navigation->ProjectPointToNavigation(Hit.ImpactPoint, Walkable, FVector(80.f, 80.f, 150.f)) || FVector::Dist2D(Walkable.Location, Hit.ImpactPoint) > 80.f) return false;
		}
		OutGround = Hit.ImpactPoint;
		return true;
	}
}

bool UIslandWorldStateSubsystem::BuildArrangementSiteLayout(UWorld* World, const TArray<FVector>& Avoid, TArray<FIslandArrangementSite>& OutSites)
{
	OutSites.Reset();
	if (!World) return false;
	const AActor* ListeningStones = nullptr;
	for (TActorIterator<AActor> It(World); It && !ListeningStones; ++It)
		if (It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("ListeningStones"))) ListeningStones = *It;
	if (!ListeningStones) return false;

	// A loose terrace of arranging grounds within a short walk of the stones.
	constexpr int32 SiteCount = 4;
	const FVector Anchor = ListeningStones->GetActorLocation();
	FRandomStream Random(4219);
	TArray<FVector> Taken = Avoid;
	for (int32 Attempt = 0; Attempt < 160 && OutSites.Num() < SiteCount; ++Attempt)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		const float Distance = Random.FRandRange(450.f, 1200.f);
		FVector Ground;
		if (!FindOpenGround(World, Anchor + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance, Ground)) continue;
		if (Taken.ContainsByPredicate([&Ground](const FVector& Other) { return FVector::Dist2D(Other, Ground) < 400.f; })) continue;
		Taken.Add(Ground);
		FIslandArrangementSite& Site = OutSites.AddDefaulted_GetRef();
		Site.Id = FName(*FString::Printf(TEXT("ArrangingGround_%d"), OutSites.Num()));
		Site.Location = Ground;
	}
	if (OutSites.Num() == SiteCount) return true;
	OutSites.Reset();
	return false;
}

bool UIslandWorldStateSubsystem::PlaceArrangementSites()
{
	if (bStorageUnreadable || GetStorageFilePath().IsEmpty() || ArrangementSites.Num() > 0) return false;
	TArray<FVector> Avoid;
	for (const FIslandCurioRecord& Curio : Curios) Avoid.Add(Curio.Location);
	if (!BuildArrangementSiteLayout(GetWorld(), Avoid, ArrangementSites))
	{
		// Levels without the ListeningStones simply have no arranging grounds; only warn when they should.
		bool bHasListeningStones = false;
		for (TActorIterator<AActor> It(GetWorld()); It && !bHasListeningStones; ++It)
			bHasListeningStones = It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("ListeningStones"));
		if (bHasListeningStones) UE_LOG(LogIslandWorldState, Warning, TEXT("Could not find level open ground for arranging sites near the ListeningStones."));
		return false;
	}
	if (!Save())
	{
		ArrangementSites.Reset();
		return false;
	}
	UE_LOG(LogIslandWorldState, Log, TEXT("Placed %d arranging sites near the ListeningStones."), ArrangementSites.Num());
	return true;
}

FString UIslandWorldStateSubsystem::ArrangeStones(FName SiteId, const FString& Form, const FString& Title, const FString& Intent,
	const FString& AgentId, int32 Today, bool& bOutChanged)
{
	bOutChanged = false;
	FIslandArrangementSite* Site = ArrangementSites.FindByPredicate([SiteId](const FIslandArrangementSite& Existing) { return Existing.Id == SiteId; });
	if (!Site) return TEXT("There is no arranging ground by that name here. Nothing changed.");
	for (const FIslandArrangementSite& Other : ArrangementSites)
	{
		const bool bMadeToday = Other.bHasWork && Other.MakerAgentId == AgentId && Other.Day == Today;
		const bool bAnsweredToday = Other.Responses.ContainsByPredicate([&AgentId, Today](const FIslandArrangementResponse& Response) { return Response.AgentId == AgentId && Response.Day == Today; });
		if (bMadeToday || bAnsweredToday) return TEXT("You have already arranged stones today; another arrangement will have to wait for a new Island day. Nothing changed.");
	}
	const FString CleanIntent = CleanArrangementText(Intent, 200);
	const FIslandArrangementSite Before = *Site;
	FString Fact;
	if (!Site->bHasWork)
	{
		EIslandArrangementForm Chosen;
		if (!ParseArrangementForm(Form, Chosen)) return TEXT("Choose one form for the stones: ring, line, spiral, or pair. Nothing changed.");
		const FString CleanTitle = CleanArrangementText(Title, 60);
		Site->bHasWork = true;
		Site->Form = Chosen;
		Site->Seed = static_cast<int32>(HashCombine(GetTypeHash(SiteId), HashCombine(GetTypeHash(AgentId), GetTypeHash(Today))));
		Site->Title = CleanTitle.IsEmpty() ? TEXT("Untitled") : CleanTitle;
		Site->Intent = CleanIntent;
		Site->MakerAgentId = AgentId;
		Site->Day = Today;
		Site->CreatedUtc = FDateTime::UtcNow();
		Site->Responses.Reset();
		Fact = FString::Printf(TEXT("You gathered stones from around the ListeningStones and arranged %d of them into a %s at %s. You call it \"%s\"%s. It stays in the world after this session. Others who come here will see its shape and age, but not your title or intent unless you tell them."),
			AIslandArrangement::StoneCountFor(Chosen), *FormName(Chosen), *SiteId.ToString(), *Site->Title,
			CleanIntent.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(", meaning: %s"), *CleanIntent));
	}
	else
	{
		if (Site->MakerAgentId == AgentId) return FString::Printf(TEXT("This is your own arrangement, \"%s\"; you leave it as it is. To make something new, choose empty arranging ground. Nothing changed."), *Site->Title);
		if (Site->Responses.ContainsByPredicate([&AgentId](const FIslandArrangementResponse& Response) { return Response.AgentId == AgentId; }))
			return TEXT("You have already added your response beside this arrangement. Nothing changed.");
		if (Site->Responses.Num() >= AIslandArrangement::MaxResponses)
			return TEXT("The ground around this arrangement has no room for more stones. Nothing changed.");
		FIslandArrangementResponse& Response = Site->Responses.AddDefaulted_GetRef();
		Response.AgentId = AgentId;
		Response.Day = Today;
		Response.Intent = CleanIntent;
		Fact = FString::Printf(TEXT("Beside the %s someone else arranged here, you set %d small stones in an arc as your response%s. It stays in the world after this session. You still do not know who made the original or what they meant."),
			*FormName(Site->Form), AIslandArrangement::StonesPerResponse,
			CleanIntent.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(", meaning: %s"), *CleanIntent));
	}
	if (!Save())
	{
		*Site = Before;
		return TEXT("You began to arrange the stones, but the change could not be kept, so nothing lasting happened.");
	}
	bOutChanged = true;
	RefreshArrangementActor(*Site);
	return Fact;
}

bool UIslandWorldStateSubsystem::ForgetArrangements()
{
	const TArray<FIslandArrangementSite> Previous = ArrangementSites;
	ArrangementSites.Reset();
	if (!Save())
	{
		ArrangementSites = Previous;
		return false;
	}
	for (const TPair<FName, TWeakObjectPtr<AIslandArrangement>>& Pair : ArrangementActors)
		if (Pair.Value.IsValid()) Pair.Value->Destroy();
	ArrangementActors.Reset();
	return true;
}

int32 UIslandWorldStateSubsystem::DisplayDay() const
{
	if (const UWorld* World = GetWorld())
		for (TActorIterator<AIslandDayNight> It(World); It; ++It)
			if (It->HasActorBegunPlay()) return It->DayNumber;
	return SavedDay.Get(1);
}

void UIslandWorldStateSubsystem::RefreshArrangementActor(const FIslandArrangementSite& Site)
{
	UWorld* World = GetWorld();
	if (!World) return;
	TWeakObjectPtr<AIslandArrangement>& Actor = ArrangementActors.FindOrAdd(Site.Id);
	if (!Actor.IsValid())
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<AIslandArrangement>(Site.Location, FRotator::ZeroRotator, Spawn);
		if (!Actor.IsValid()) return;
		Actor->Tags.Insert(Site.Id, 0);
	}
	ShownArrangementDay = DisplayDay();
	Actor->ShowSite(Site, ShownArrangementDay);
}

static FAutoConsoleCommandWithWorld GIslandForgetArrangementsCommand(
	TEXT("Island.ForgetArrangements"),
	TEXT("Forgets every arranging site and resident stone arrangement; fresh empty sites are placed the next time play begins."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		UIslandWorldStateSubsystem* State = World ? World->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
		UE_LOG(LogIslandWorldState, Log, TEXT("Island.ForgetArrangements: %s"), State && State->ForgetArrangements() ? TEXT("forgotten") : TEXT("nothing changed (needs a running play world with writable state)"));
	}));
