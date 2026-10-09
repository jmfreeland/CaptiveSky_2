#include "IslandWrack.h"
#include "AgentDataPaths.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IslandChronicle.h"
#include "IslandOceanSubsystem.h"
#include "IslandWorldStateSubsystem.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandWrack, Log, All);

namespace
{
	constexpr int32 WrackVersion = 1;
	/** Ground this far above the sea counts as dry sand; below it is water. */
	constexpr float TideLineHeight = 20.f;
	constexpr float MarchStep = 300.f;
	constexpr float MaxMarch = 120000.f;

	FString KindName(EIslandWrackKind Kind)
	{
		switch (Kind)
		{
		case EIslandWrackKind::Kelp: return TEXT("kelp");
		case EIslandWrackKind::Shells: return TEXT("shells");
		case EIslandWrackKind::Float: return TEXT("float");
		default: return TEXT("driftwood");
		}
	}

	EIslandWrackKind KindFromName(const FString& Name)
	{
		for (int32 I = 0; I < static_cast<int32>(EIslandWrackKind::Count); ++I)
			if (KindName(static_cast<EIslandWrackKind>(I)) == Name) return static_cast<EIslandWrackKind>(I);
		return EIslandWrackKind::Count;
	}

	/** Landscape height at a point, or nothing where there is no ground (open sea beyond the landscape). */
	bool HeightAt(const TArray<const ALandscapeProxy*>& Landscapes, const FVector2D& XY, float& OutZ)
	{
		for (const ALandscapeProxy* Landscape : Landscapes)
		{
			const TOptional<float> Height = Landscape->GetHeightAtLocation(FVector(XY.X, XY.Y, 0.f));
			if (Height.IsSet())
			{
				OutZ = Height.GetValue();
				return true;
			}
		}
		return false;
	}

	/** Orientation that lays an item on the ground under it: up follows the slope, forward follows Yaw. */
	FQuat GroundRotation(UWorld* World, const FVector& Position, float Yaw, FVector& OutNormal)
	{
		TArray<const ALandscapeProxy*> Landscapes;
		for (TActorIterator<ALandscapeProxy> It(World); It; ++It) Landscapes.Add(*It);
		OutNormal = FVector::UpVector;
		constexpr float Reach = 150.f;
		float XPlus, XMinus, YPlus, YMinus;
		if (HeightAt(Landscapes, FVector2D(Position.X + Reach, Position.Y), XPlus) && HeightAt(Landscapes, FVector2D(Position.X - Reach, Position.Y), XMinus) &&
			HeightAt(Landscapes, FVector2D(Position.X, Position.Y + Reach), YPlus) && HeightAt(Landscapes, FVector2D(Position.X, Position.Y - Reach), YMinus))
			OutNormal = FVector(-(XPlus - XMinus) / (2.f * Reach), -(YPlus - YMinus) / (2.f * Reach), 1.f).GetSafeNormal();
		const FVector Forward(FMath::Cos(FMath::DegreesToRadians(Yaw)), FMath::Sin(FMath::DegreesToRadians(Yaw)), 0.f);
		return FRotationMatrix::MakeFromZX(OutNormal, Forward).ToQuat();
	}

	/** Walk from Start along Direction until dry land gives way to water; return the tide line. */
	bool MarchToTideLine(const TArray<const ALandscapeProxy*>& Landscapes, const FBox& Bounds, const FVector2D& Start, const FVector2D& Direction,
		float SeaZ, float Step, float MaxDistance, FVector2D& OutEdge)
	{
		float Z = 0.f;
		if (!HeightAt(Landscapes, Start, Z) || Z < SeaZ + TideLineHeight) return false;
		FVector2D Last = Start;
		for (float Distance = Step; Distance <= MaxDistance; Distance += Step)
		{
			const FVector2D Here = Start + Direction * Distance;
			if (Here.X < Bounds.Min.X || Here.X > Bounds.Max.X || Here.Y < Bounds.Min.Y || Here.Y > Bounds.Max.Y) return false;
			if (HeightAt(Landscapes, Here, Z) && Z >= SeaZ + TideLineHeight)
			{
				Last = Here;
				continue;
			}
			FVector2D Low = Here;
			FVector2D High = Last;
			for (int32 Iteration = 0; Iteration < 9; ++Iteration)
			{
				const FVector2D Mid = (Low + High) * 0.5f;
				float MidZ = 0.f;
				if (HeightAt(Landscapes, Mid, MidZ) && MidZ >= SeaZ + TideLineHeight) High = Mid;
				else Low = Mid;
			}
			OutEdge = High;
			return true;
		}
		return false;
	}
}

int32 FIslandWrackLedger::LifespanDays(EIslandWrackKind Kind)
{
	switch (Kind)
	{
	case EIslandWrackKind::Kelp: return 5;
	case EIslandWrackKind::Shells: return 9;
	case EIslandWrackKind::Float: return 12;
	default: return 14;
	}
}

const FIslandWrackItem& FIslandWrackLedger::Add(EIslandWrackKind Kind, const FVector& Position, float Yaw, int32 Seed, int32 Day)
{
	while (Items.Num() >= MaxItems) Items.RemoveAt(0);
	FIslandWrackItem& Item = Items.AddDefaulted_GetRef();
	Item.Id = NextId++;
	Item.Kind = Kind;
	Item.Position = Position;
	Item.Yaw = Yaw;
	Item.Seed = Seed;
	Item.Day = Day;
	return Item;
}

bool FIslandWrackLedger::Weather(int32 Today)
{
	return Items.RemoveAll([Today](const FIslandWrackItem& Item) { return Today - Item.Day > LifespanDays(Item.Kind); }) > 0;
}

const FIslandWrackItem* FIslandWrackLedger::Find(int32 Id) const
{
	return Items.FindByPredicate([Id](const FIslandWrackItem& Item) { return Item.Id == Id; });
}

EIslandWrackTurn FIslandWrackLedger::Turn(int32 Id, const FString& AgentId, FIslandWrackItem& OutItem)
{
	FIslandWrackItem* Item = Items.FindByPredicate([Id](const FIslandWrackItem& Candidate) { return Candidate.Id == Id; });
	if (!Item) return EIslandWrackTurn::Missing;
	const bool bAlready = Item->bTurned;
	if (!bAlready)
	{
		Item->bTurned = true;
		Item->TurnedBy = AgentId;
		Item->Find = UIslandWrackSubsystem::FindFor(Item->Kind, Item->Seed);
	}
	OutItem = *Item;
	return bAlready ? EIslandWrackTurn::AlreadyTurned : EIslandWrackTurn::Turned;
}

bool FIslandWrackLedger::GatherDriftwoodTwigs(int32 Id, FIslandWrackItem& OutItem)
{
	const int32 Index = Items.IndexOfByPredicate([Id](const FIslandWrackItem& Candidate)
		{ return Candidate.Id == Id; });
	if (!Items.IsValidIndex(Index)) return false;
	const FIslandWrackItem& Candidate = Items[Index];
	// A resident's discovery stays available to everyone; only untouched driftwood can be foraged.
	if (Candidate.Kind != EIslandWrackKind::Driftwood || Candidate.bTurned) return false;
	OutItem = Candidate;
	Items.RemoveAt(Index);
	return true;
}

FString FIslandWrackLedger::ToJson() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), WrackVersion);
	Root->SetNumberField(TEXT("next_id"), NextId);
	TArray<TSharedPtr<FJsonValue>> Rows;
	for (const FIslandWrackItem& Item : Items)
	{
		const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetNumberField(TEXT("id"), Item.Id);
		Row->SetStringField(TEXT("kind"), KindName(Item.Kind));
		Row->SetNumberField(TEXT("x"), Item.Position.X);
		Row->SetNumberField(TEXT("y"), Item.Position.Y);
		Row->SetNumberField(TEXT("z"), Item.Position.Z);
		Row->SetNumberField(TEXT("yaw"), Item.Yaw);
		Row->SetNumberField(TEXT("seed"), Item.Seed);
		Row->SetNumberField(TEXT("day"), Item.Day);
		Row->SetBoolField(TEXT("turned"), Item.bTurned);
		Row->SetStringField(TEXT("find"), Item.Find);
		Row->SetStringField(TEXT("by"), Item.TurnedBy);
		Rows.Add(MakeShared<FJsonValueObject>(Row));
	}
	Root->SetArrayField(TEXT("items"), Rows);
	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool FIslandWrackLedger::FromJson(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) return false;
	int32 Version = 0;
	if (!Root->TryGetNumberField(TEXT("version"), Version) || Version != WrackVersion) return false;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!Root->TryGetArrayField(TEXT("items"), Rows)) return false;
	TArray<FIslandWrackItem> Loaded;
	int32 HighestId = 0;
	for (const TSharedPtr<FJsonValue>& RowValue : *Rows)
	{
		const TSharedPtr<FJsonObject>* Row = nullptr;
		if (!RowValue.IsValid() || !RowValue->TryGetObject(Row) || !Row->IsValid()) return false;
		FIslandWrackItem Item;
		FString KindText;
		double X = 0, Y = 0, Z = 0, Yaw = 0;
		if (!(*Row)->TryGetNumberField(TEXT("id"), Item.Id) || !(*Row)->TryGetStringField(TEXT("kind"), KindText) ||
			!(*Row)->TryGetNumberField(TEXT("x"), X) || !(*Row)->TryGetNumberField(TEXT("y"), Y) || !(*Row)->TryGetNumberField(TEXT("z"), Z) ||
			!(*Row)->TryGetNumberField(TEXT("day"), Item.Day))
			return false;
		Item.Kind = KindFromName(KindText);
		if (Item.Kind == EIslandWrackKind::Count) return false;
		(*Row)->TryGetNumberField(TEXT("yaw"), Yaw);
		(*Row)->TryGetNumberField(TEXT("seed"), Item.Seed);
		(*Row)->TryGetBoolField(TEXT("turned"), Item.bTurned);
		(*Row)->TryGetStringField(TEXT("find"), Item.Find);
		(*Row)->TryGetStringField(TEXT("by"), Item.TurnedBy);
		Item.Position = FVector(X, Y, Z);
		Item.Yaw = static_cast<float>(Yaw);
		HighestId = FMath::Max(HighestId, Item.Id);
		if (Loaded.Num() < MaxItems) Loaded.Add(Item);
	}
	Items = MoveTemp(Loaded);
	int32 Next = 0;
	Root->TryGetNumberField(TEXT("next_id"), Next);
	NextId = FMath::Max(Next, HighestId + 1);
	return true;
}

AIslandWrack::AIslandWrack()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto Make = [this](const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material)
	{
		UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Layer->SetupAttachment(RootComponent);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetCanEverAffectNavigation(false);
		Layer->SetMobility(EComponentMobility::Movable);
		if (Mesh) Layer->SetStaticMesh(Mesh);
		if (Material) Layer->SetMaterial(0, Material);
		return Layer;
	};
	UMaterialInterface* Material = Basic.Succeeded() ? Basic.Object : nullptr;
	Long = Make(TEXT("Long"), Cylinder.Succeeded() ? Cylinder.Object : nullptr, Material);
	Blobs = Make(TEXT("Blobs"), Sphere.Succeeded() ? Sphere.Object : nullptr, Material);
}

bool AIslandWrack::UsesLongPieces(EIslandWrackKind Kind)
{
	return Kind == EIslandWrackKind::Driftwood;
}

TArray<FIslandWrackPiece> AIslandWrack::Layout(EIslandWrackKind Kind, int32 Seed)
{
	FRandomStream Random(Seed);
	TArray<FIslandWrackPiece> Out;
	const float Ground = -OriginLift;
	auto InDisc = [&Random](float Radius)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		const float Distance = Radius * FMath::Sqrt(Random.FRand());
		return FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);
	};
	switch (Kind)
	{
	case EIslandWrackKind::Driftwood:
	{
		const int32 Logs = 1 + Random.RandRange(0, 2);
		for (int32 I = 0; I < Logs; ++I)
		{
			const float Length = Random.FRandRange(150.f, 320.f) * (I == 0 ? 1.f : 0.7f);
			const float Thick = Random.FRandRange(9.f, 22.f);
			const float Angle = Random.FRandRange(-0.4f, 0.4f);
			const FVector Direction = FVector(FMath::Cos(Angle), FMath::Sin(Angle), Random.FRandRange(-0.04f, 0.12f)).GetSafeNormal();
			FVector Offset = FVector::ZeroVector;
			if (I > 0) Offset = FVector(Random.FRandRange(-100.f, 100.f), (Random.FRand() < 0.5f ? -1.f : 1.f) * Random.FRandRange(45.f, 120.f), 0.f);
			Offset.Z = Ground + Thick * 0.35f;
			const FQuat Rotation = FRotationMatrix::MakeFromZX(Direction, FVector::UpVector).ToQuat();
			Out.Add({FTransform(Rotation, Offset, FVector(Thick / 100.f, Thick / 100.f, Length / 100.f))});
		}
		break;
	}
	case EIslandWrackKind::Kelp:
	{
		const int32 Fronds = 4 + Random.RandRange(0, 3);
		for (int32 I = 0; I < Fronds; ++I)
		{
			FVector Offset = InDisc(60.f);
			Offset.Z = Ground + 3.f;
			const FRotator Yaw(0.f, Random.FRandRange(0.f, 360.f), 0.f);
			Out.Add({FTransform(Yaw, Offset, FVector(Random.FRandRange(0.35f, 0.85f), Random.FRandRange(0.12f, 0.3f), 0.04f))});
		}
		break;
	}
	case EIslandWrackKind::Shells:
	{
		const int32 Count = 6 + Random.RandRange(0, 4);
		for (int32 I = 0; I < Count; ++I)
		{
			FVector Offset = InDisc(60.f);
			const float Size = Random.FRandRange(0.04f, 0.075f);
			Offset.Z = Ground + Size * 25.f;
			const FRotator Spin(Random.FRandRange(-20.f, 20.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-20.f, 20.f));
			Out.Add({FTransform(Spin, Offset, FVector(Size, Size * 0.8f, Size * 0.55f))});
		}
		break;
	}
	default:
	{
		const FRotator Spin(0.f, Random.FRandRange(0.f, 360.f), 0.f);
		Out.Add({FTransform(Spin, FVector(0.f, 0.f, Ground + 10.f), FVector(0.26f))});
		break;
	}
	}
	return Out;
}

FLinearColor AIslandWrack::ItemColor(EIslandWrackKind Kind, int32 AgeDays, bool bTurned)
{
	const float Age = FMath::Clamp(static_cast<float>(AgeDays) / static_cast<float>(FIslandWrackLedger::LifespanDays(Kind)), 0.f, 1.f);
	FLinearColor Color;
	switch (Kind)
	{
	case EIslandWrackKind::Kelp:
		Color = FMath::Lerp(FLinearColor(0.05f, 0.1f, 0.03f), FLinearColor(0.09f, 0.065f, 0.04f), FMath::SmoothStep(0.f, 0.6f, Age));
		break;
	case EIslandWrackKind::Shells:
		Color = FMath::Lerp(FLinearColor(0.6f, 0.5f, 0.42f), FLinearColor(0.72f, 0.67f, 0.6f), FMath::SmoothStep(0.f, 0.7f, Age));
		break;
	case EIslandWrackKind::Float:
		Color = FMath::Lerp(FLinearColor(0.06f, 0.3f, 0.26f), FLinearColor(0.1f, 0.34f, 0.3f), Age);
		break;
	default:
		Color = FMath::Lerp(FLinearColor(0.09f, 0.06f, 0.04f), FLinearColor(0.42f, 0.37f, 0.3f), FMath::SmoothStep(0.f, 0.5f, Age));
		break;
	}
	if (bTurned) Color = FMath::Lerp(Color, FLinearColor(0.02f, 0.018f, 0.015f), 0.5f);
	Color.A = 1.f;
	return Color;
}

void AIslandWrack::Show(int32 InItemId, EIslandWrackKind Kind, int32 Seed, int32 AgeDays, bool bTurned)
{
	ItemId = InItemId;
	Tags.Reset();
	Tags.Add(UIslandWrackSubsystem::TargetTagFor(InItemId));
	Tags.Add(TEXT("IslandLandmark"));
	Tags.Add(TEXT("StormWrack"));

	UInstancedStaticMeshComponent* Layer = UsesLongPieces(Kind) ? Long.Get() : Blobs.Get();
	TObjectPtr<UMaterialInstanceDynamic>& Slot = UsesLongPieces(Kind) ? LongMaterial : BlobMaterial;
	if (!Layer) return;
	if (BuiltKind != Kind || BuiltSeed != Seed)
	{
		Long->ClearInstances();
		Blobs->ClearInstances();
		for (const FIslandWrackPiece& Piece : Layout(Kind, Seed)) Layer->AddInstance(Piece.Transform, false);
		BuiltKind = Kind;
		BuiltSeed = Seed;
	}
	if (!Slot && Layer->GetMaterial(0))
	{
		Slot = UMaterialInstanceDynamic::Create(Layer->GetMaterial(0), this);
		if (Slot) Layer->SetMaterial(0, Slot);
	}
	if (Slot)
	{
		Slot->SetVectorParameterValue(TEXT("Color"), ItemColor(Kind, AgeDays, bTurned));
		Slot->SetScalarParameterValue(TEXT("Roughness"), Kind == EIslandWrackKind::Float ? 0.15f : 0.95f);
	}
}

int32 AIslandWrack::GetPieceCount() const
{
	return (Long ? Long->GetInstanceCount() : 0) + (Blobs ? Blobs->GetInstanceCount() : 0);
}

EIslandWrackKind UIslandWrackSubsystem::PickKind(int32 Seed)
{
	const int32 Roll = FMath::Abs(Seed) % 10;
	if (Roll < 4) return EIslandWrackKind::Driftwood;
	if (Roll < 7) return EIslandWrackKind::Kelp;
	if (Roll < 9) return EIslandWrackKind::Shells;
	return EIslandWrackKind::Float;
}

FString UIslandWrackSubsystem::FindFor(EIslandWrackKind Kind, int32 Seed)
{
	static const TCHAR* Wood[] = {
		TEXT("a small crab sheltering in a hollow of the wood, which scuttles off sideways"),
		TEXT("a fine line of tiny holes bored along the grain, like stitches, where shipworm have been at it"),
		TEXT("a rusted nail still driven through one end - this was part of something built, once, far from here"),
		TEXT("a notch cut too regularly to be the sea's work, worn smooth by the water"),
	};
	static const TCHAR* Kelp[] = {
		TEXT("a hop of sandhoppers that scatter in every direction"),
		TEXT("a snarl of fishing line wound through the fronds"),
		TEXT("a small translucent jellyfish stranded in the folds, already losing its shape"),
		TEXT("a string of pale egg cases clinging to a stalk"),
	};
	static const TCHAR* Shells[] = {
		TEXT("a hermit crab, withdrawn deep into a whorled shell, waiting for you to leave"),
		TEXT("one shell worn through with a neat hole, the kind you could thread on a cord"),
		TEXT("a shell that is violet inside, brighter than the sand around it"),
		TEXT("a tiny spiral shell packed with fine wet sand, as though it had been carried a long way"),
	};
	static const TCHAR* Float[] = {
		TEXT("a faint number etched into the glass, and a smear of weed - it has crossed a great deal of water to get here"),
		TEXT("a scrap of knotted net still tied around it, brittle with salt"),
		TEXT("a hairline crack along one side, with a trace of seawater inside"),
		TEXT("a tiny bubble in the glass, a flaw from whoever made it, far away"),
	};
	const int32 Index = FMath::Abs(Seed / 7) % 4;
	switch (Kind)
	{
	case EIslandWrackKind::Kelp: return Kelp[Index];
	case EIslandWrackKind::Shells: return Shells[Index];
	case EIslandWrackKind::Float: return Float[Index];
	default: return Wood[Index];
	}
}

FString UIslandWrackSubsystem::EchoFromChronicle(const TArray<FString>& ChronicleLines, int32 Seed, int32 Today)
{
	struct FEcho { FString Agent; FString Text; int32 Day; };
	TArray<FEcho> Candidates;
	for (const FString& Line : ChronicleLines)
	{
		TSharedPtr<FJsonObject> Entry;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Line), Entry) || !Entry.IsValid()) continue;
		FString Type, Agent, Text;
		double Day = 0.0;
		if (!Entry->TryGetStringField(TEXT("type"), Type) || Type != TEXT("decision")) continue;
		if (!Entry->TryGetStringField(TEXT("agent"), Agent) || Agent.IsEmpty()) continue;
		if (!Entry->TryGetStringField(TEXT("text"), Text)) continue;
		Entry->TryGetNumberField(TEXT("day"), Day);
		if (static_cast<int32>(Day) >= Today) continue;
		Text.TrimStartAndEndInline();
		if (Text.Len() < 30 || Text.Len() > 220) continue;
		bool bClean = true;
		for (const TCHAR Character : Text) if (Character == 0xFFFD || Character > 0x7E) { bClean = false; break; }
		if (!bClean) continue;
		Candidates.Add({Agent, Text, static_cast<int32>(Day)});
	}
	if (Candidates.IsEmpty()) return FString();
	const FEcho& Pick = Candidates[FMath::Abs(Seed / 11) % Candidates.Num()];
	return FString::Printf(TEXT("a rolled slip of paper sealed inside the glass, written by %s on Island day %d: \"%s\" - a thought from before, carried round the sea and back"),
		*Pick.Agent.Left(24), Pick.Day, *Pick.Text);
}

FString UIslandWrackSubsystem::DescribeItem(EIslandWrackKind Kind, int32 AgeDays, bool bTurned)
{
	FString Out;
	switch (Kind)
	{
	case EIslandWrackKind::Kelp:
		Out = AgeDays <= 1 ? TEXT("a glossy heap of kelp, still wet and smelling of the sea")
			: AgeDays <= 3 ? TEXT("kelp drying stiff on the sand") : TEXT("brittle blackened kelp, almost gone");
		break;
	case EIslandWrackKind::Shells:
		Out = AgeDays <= 3 ? TEXT("a scatter of fresh shells, still wet") : TEXT("a scatter of bleached shells half sunk in the sand");
		break;
	case EIslandWrackKind::Float:
		Out = AgeDays <= 3 ? TEXT("a round glass float, green-blue, rolled up out of the surf") : TEXT("a green-blue glass float settled in the sand");
		break;
	default:
		Out = AgeDays <= 1 ? TEXT("a length of driftwood, dark and dripping where the storm left it")
			: AgeDays <= 6 ? TEXT("driftwood drying to grey on the sand") : TEXT("silvered, bleached driftwood");
		break;
	}
	if (bTurned) Out += TEXT(" (turned over; the sand beneath is pressed flat)");
	return Out;
}

FName UIslandWrackSubsystem::TargetTagFor(int32 ItemId)
{
	return FName(*FString::Printf(TEXT("Wrack_%d"), ItemId));
}

int32 UIslandWrackSubsystem::ItemIdFromTag(const FName& Tag)
{
	const FString Text = Tag.ToString();
	if (!Text.StartsWith(TEXT("Wrack_")) || !Text.Mid(6).IsNumeric()) return 0;
	return FCString::Atoi(*Text.Mid(6));
}

TStatId UIslandWrackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandWrackSubsystem, STATGROUP_Tickables);
}

bool UIslandWrackSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FString UIslandWrackSubsystem::GetStorageFilePath() const
{
	if (!bAllowStorage) return FString();
	if (!StorageFileOverride.IsEmpty()) return StorageFileOverride;
	if (!GetWorld() || GetWorld()->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"))) return FString();
	const FString MapName = UWorld::RemovePIEPrefix(GetWorld()->GetMapName());
	return CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("WorldState") / (MapName + TEXT(".wrack.json")));
}

void UIslandWrackSubsystem::Load()
{
	const FString Path = GetStorageFilePath();
	FString Contents;
	if (Path.IsEmpty() || !FFileHelper::LoadFileToString(Contents, *Path)) return;
	if (!Ledger.FromJson(Contents))
	{
		UE_LOG(LogIslandWrack, Warning, TEXT("Ignoring unreadable wrack ledger %s and not overwriting it"), *Path);
		bAllowStorage = false;
	}
}

bool UIslandWrackSubsystem::Save()
{
	const FString Path = GetStorageFilePath();
	if (Path.IsEmpty()) return false;
	if (IFileManager::Get().DirectoryExists(*Path))
	{
		UE_LOG(LogIslandWrack, Error, TEXT("Failed to save wrack ledger to %s: destination is a directory."), *Path);
		return false;
	}
	const FString Temporary = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Ledger.ToJson(), *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
		!IFileManager::Get().Move(*Path, *Temporary, true, true))
	{
		UE_LOG(LogIslandWrack, Error, TEXT("Failed to save wrack ledger to %s"), *Path);
		return false;
	}
	bDirty = false;
	return true;
}

void UIslandWrackSubsystem::SyncActors(int32 Today)
{
	UWorld* World = GetWorld();
	if (!World) return;
	for (auto It = Actors.CreateIterator(); It; ++It)
	{
		if (!Ledger.Find(It.Key()) || !It.Value().IsValid())
		{
			if (AIslandWrack* Stale = It.Value().Get()) Stale->Destroy();
			It.RemoveCurrent();
		}
	}
	for (const FIslandWrackItem& Item : Ledger.Items)
	{
		TWeakObjectPtr<AIslandWrack>& Slot = Actors.FindOrAdd(Item.Id);
		if (!Slot.IsValid())
		{
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			FVector Normal;
				const FQuat Rotation = GroundRotation(World, Item.Position, Item.Yaw, Normal);
				Slot = World->SpawnActor<AIslandWrack>(Item.Position + Normal * AIslandWrack::OriginLift, Rotation.Rotator(), Spawn);
		}
		if (AIslandWrack* Actor = Slot.Get()) Actor->Show(Item.Id, Item.Kind, Item.Seed, FMath::Max(0, Today - Item.Day), Item.bTurned);
	}
	SyncedDay = Today;
}

void UIslandWrackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Load();
	const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(&InWorld);
	if (Today >= 0 && Ledger.Weather(Today)) bDirty = true;
	SyncActors(Today);
	if (bDirty) Save();
}

void UIslandWrackSubsystem::Tick(float DeltaTime)
{
	SinceCheck += DeltaTime;
	if (SinceCheck < 20.f || !GetWorld()) return;
	SinceCheck = 0.f;
	const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld());
	if (Today < 0 || Today == SyncedDay) return;
	if (Ledger.Weather(Today)) bDirty = true;
	SyncActors(Today);
	if (bDirty) Save();
}

void UIslandWrackSubsystem::Deinitialize()
{
	if (bDirty) Save();
	for (const TPair<int32, TWeakObjectPtr<AIslandWrack>>& Pair : Actors)
		if (AIslandWrack* Actor = Pair.Value.Get()) Actor->Destroy();
	Actors.Reset();
	Super::Deinitialize();
}

int32 UIslandWrackSubsystem::DepositAfterStorm(int32 Today, int32 Count)
{
	UWorld* World = GetWorld();
	AStaticMeshActor* Ocean = World ? UIslandOceanSubsystem::FindOceanPlane(World) : nullptr;
	if (!Ocean || Today < 0)
	{
		UE_LOG(LogIslandWrack, Warning, TEXT("Storm wrack placement skipped: ocean plane %s, Island day %d."), Ocean ? TEXT("found") : TEXT("missing"), Today);
		return 0;
	}
	const float SeaZ = Ocean->GetActorLocation().Z;

	TArray<const ALandscapeProxy*> Landscapes;
	FBox Bounds(ForceInit);
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		Landscapes.Add(*It);
		Bounds += It->GetComponentsBoundingBox();
	}
	if (Landscapes.IsEmpty() || !Bounds.IsValid)
	{
		UE_LOG(LogIslandWrack, Warning, TEXT("Storm wrack placement skipped: no landscape bounds were available."));
		return 0;
	}

	FRandomStream Random(Today * 7919 + Ledger.NextId * 104729);
	if (Count <= 0) Count = 3 + (Today + Ledger.NextId) % 4;

	// Find a stretch of gentle beach: pick high ground, walk to the water's edge, step back to the tide line.
	FVector2D Edge = FVector2D::ZeroVector;
	FVector2D Seaward = FVector2D(1.f, 0.f);
	bool bFound = false;
	for (int32 Attempt = 0; Attempt < 600 && !bFound; ++Attempt)
	{
		const FVector2D Start(Random.FRandRange(Bounds.Min.X, Bounds.Max.X), Random.FRandRange(Bounds.Min.Y, Bounds.Max.Y));
		float Z = 0.f;
		if (!HeightAt(Landscapes, Start, Z) || Z < SeaZ + 300.f) continue;
		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
		FVector2D Candidate;
		if (!MarchToTideLine(Landscapes, Bounds, Start, Direction, SeaZ, MarchStep, MaxMarch, Candidate)) continue;
		float Inland = 0.f;
		if (!HeightAt(Landscapes, Candidate - Direction * 800.f, Inland) || Inland > SeaZ + 450.f) continue;
		Edge = Candidate;
		Seaward = Direction;
		bFound = true;
	}
	if (!bFound)
	{
		UE_LOG(LogIslandWrack, Warning, TEXT("Storm wrack placement found no suitable beach across %d landscape(s)."), Landscapes.Num());
		return 0;
	}

	const FVector2D Along(-Seaward.Y, Seaward.X);
	const float AlongYaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
	TArray<FVector> Placed;
	TArray<FString> Descriptions;
	for (int32 Attempt = 0; Attempt < Count * 14 && Placed.Num() < Count; ++Attempt)
	{
		const float Shift = Placed.IsEmpty() ? 0.f : Random.FRandRange(-1400.f, 1400.f);
		const FVector2D Tilt = (Seaward + Along * Random.FRandRange(-0.25f, 0.25f)).GetSafeNormal();
		const FVector2D Start = Edge + Along * Shift - Seaward * 2500.f;
		FVector2D Line;
		if (!MarchToTideLine(Landscapes, Bounds, Start, Tilt, SeaZ, 100.f, 7000.f, Line)) continue;
		const FVector2D Spot = Line - Tilt * Random.FRandRange(120.f, 450.f);
		float Z = 0.f;
		if (!HeightAt(Landscapes, Spot, Z) || Z < SeaZ + TideLineHeight) continue;
		const FVector Position(Spot.X, Spot.Y, Z);
		bool bCrowded = false;
		for (const FIslandWrackItem& Existing : Ledger.Items) bCrowded |= FVector::DistSquared2D(Existing.Position, Position) < FMath::Square(250.f);
		for (const FVector& Other : Placed) bCrowded |= FVector::DistSquared2D(Other, Position) < FMath::Square(250.f);
		if (bCrowded) continue;
		const int32 Seed = Random.RandRange(0, 1 << 20);
		const FIslandWrackItem& Item = Ledger.Add(PickKind(Seed), Position, AlongYaw + Random.FRandRange(-25.f, 25.f), Seed, Today);
		Placed.Add(Position);
		Descriptions.Add(DescribeItem(Item.Kind, 0, false));
		UE_LOG(LogIslandWrack, Log, TEXT("Wrack %d (%s) left at %s"), Item.Id, *KindName(Item.Kind), *Position.ToCompactString());
	}
	if (Placed.IsEmpty())
	{
		UE_LOG(LogIslandWrack, Warning, TEXT("Storm wrack placement found a beach but could not place any items."));
		return 0;
	}

	SyncActors(Today);
	Save();
	UIslandChronicleSubsystem::Record(World, TEXT("wrack"), FString(),
		FString::Printf(TEXT("The storm left wrack along the shore: %s."), *FString::Join(Descriptions, TEXT("; "))));
	return Placed.Num();
}

bool UIslandWrackSubsystem::Examine(int32 ItemId, int32 Today, const FString& AgentId, FString& OutFact)
{
	FIslandWrackItem Item;
	const EIslandWrackTurn Result = Ledger.Turn(ItemId, AgentId, Item);
	if (Result == EIslandWrackTurn::Missing)
	{
		OutFact = TEXT("The tide has already taken this wrack back; there is nothing left to look at. Nothing changed.");
		return false;
	}
	if (Result == EIslandWrackTurn::Turned && Item.Kind == EIslandWrackKind::Float && Item.Seed % 2 == 0)
	{
		FString Contents;
		TArray<FString> Lines;
		const UIslandChronicleSubsystem* Chronicle = GetWorld() ? GetWorld()->GetSubsystem<UIslandChronicleSubsystem>() : nullptr;
		const FString ChroniclePath = Chronicle ? Chronicle->GetChroniclePath() : FString();
		if (bAllowStorage && !ChroniclePath.IsEmpty() && FFileHelper::LoadFileToString(Contents, *ChroniclePath)) Contents.ParseIntoArrayLines(Lines);
		if (Lines.Num() > 3000) Lines.RemoveAt(0, Lines.Num() - 3000);
		const FString Echo = EchoFromChronicle(Lines, Item.Seed, Today);
		if (!Echo.IsEmpty())
		{
			Item.Find = Echo;
			for (FIslandWrackItem& Stored : Ledger.Items) if (Stored.Id == ItemId) Stored.Find = Echo;
		}
	}
	const int32 Age = FMath::Max(0, Today - Item.Day);
	const int32 Remaining = FMath::Max(1, FIslandWrackLedger::LifespanDays(Item.Kind) - Age);
	if (Result == EIslandWrackTurn::AlreadyTurned)
	{
		OutFact = FString::Printf(TEXT("You looked at %s. %s already lifted it and looked underneath; there was %s. Nothing more to find. The sea will take it back in about %d Island days; the next storm leaves more on this shore."),
			*DescribeItem(Item.Kind, Age, false), Item.TurnedBy.IsEmpty() ? TEXT("Someone") : *Item.TurnedBy.Left(24), *Item.Find, Remaining);
		return true;
	}
	bDirty = true;
	if (TWeakObjectPtr<AIslandWrack>* Slot = Actors.Find(ItemId))
		if (AIslandWrack* Actor = Slot->Get()) Actor->Show(Item.Id, Item.Kind, Item.Seed, Age, true);
	Save();
	OutFact = FString::Printf(TEXT("You crouched by %s and turned it over. Underneath: %s. You left it turned over, and the shore remembers it until the sea takes it back in about %d Island days; the next storm leaves more wrack along this shore."),
		*DescribeItem(Item.Kind, Age, false), *Item.Find, Remaining);
	UIslandChronicleSubsystem::Record(GetWorld(), TEXT("wrack_turned"), AgentId,
		FString::Printf(TEXT("Turned over %s on the shore and found %s."), *DescribeItem(Item.Kind, Age, false), *Item.Find));
	return true;
}

bool UIslandWrackSubsystem::GatherDriftwoodTwigs(int32 ItemId, const FString& AgentId, FString& OutFact)
{
	if (!bAllowStorage || GetStorageFilePath().IsEmpty())
	{
		OutFact = TEXT("This shore cannot safely remember a gathered bundle right now; the driftwood was left untouched.");
		return false;
	}

	const FIslandWrackLedger Previous = Ledger;
	FIslandWrackItem Gathered;
	if (!Ledger.GatherDriftwoodTwigs(ItemId, Gathered))
	{
		OutFact = TEXT("There is no fresh, untouched driftwood bundle within reach; nothing was gathered.");
		return false;
	}
	bDirty = true;
	if (!Save())
	{
		Ledger = Previous;
		OutFact = TEXT("The shore could not save the change, so the driftwood was left where it was.");
		return false;
	}
	SyncActors(UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()));
	OutFact = TEXT("You pulled a small bundle of loose branchlets from the storm-fallen driftwood. The larger piece is gone from the shore, and you carry the twigs in your beak; nothing else was taken.");
	UIslandChronicleSubsystem::Record(GetWorld(), TEXT("wrack_forage"), AgentId,
		TEXT("Gathered a small bundle of loose branchlets from storm-fallen driftwood for a nest."));
	return true;
}

FString UIslandWrackSubsystem::DescribeNearby(const FVector& Position, int32 Today) const
{
	TArray<TPair<float, const FIslandWrackItem*>> Near;
	for (const FIslandWrackItem& Item : Ledger.Items)
	{
		const float Distance = FVector::Dist(Item.Position, Position);
		if (Distance <= NoticeRadius) Near.Emplace(Distance, &Item);
	}
	if (Near.IsEmpty()) return FString();
	Near.Sort([](const TPair<float, const FIslandWrackItem*>& A, const TPair<float, const FIslandWrackItem*>& B) { return A.Key < B.Key; });
	FString Out;
	for (int32 I = 0; I < FMath::Min(2, Near.Num()); ++I)
	{
		const FIslandWrackItem& Item = *Near[I].Value;
		Out += FString::Printf(TEXT(" Storm wrack lies on the shore %.0f metres away (move_to/interact target: %s): %s."),
			Near[I].Key / 100.f, *TargetTagFor(Item.Id).ToString(), *DescribeItem(Item.Kind, FMath::Max(0, Today - Item.Day), Item.bTurned));
	}
	if (Near.Num() > 2) Out += FString::Printf(TEXT(" %d more pieces lie further along."), Near.Num() - 2);
	Out += TEXT(" Interact lifts a piece and looks underneath; it stays turned over until the sea reclaims it.");
	return Out;
}

FString UIslandWrackSubsystem::DescribeShoreForRaven(const TArray<FIslandWrackItem>& Items, const FVector& Position, int32 Today)
{
	TArray<TPair<float, const FIslandWrackItem*>> Distant;
	for (const FIslandWrackItem& Item : Items)
	{
		const int32 Age = Today - Item.Day;
		if (Item.bTurned || Age < 0 || Age > FIslandWrackLedger::LifespanDays(Item.Kind)) continue;
		const float Distance = FVector::Dist2D(Item.Position, Position);
		if (Distance <= NoticeRadius || Distance > RavenShoreAwarenessRadius) continue;
		Distant.Emplace(Distance, &Item);
	}
	if (Distant.IsEmpty()) return FString();
	Distant.Sort([](const TPair<float, const FIslandWrackItem*>& A, const TPair<float, const FIslandWrackItem*>& B)
		{ return A.Key < B.Key; });

	const FIslandWrackItem& Nearest = *Distant[0].Value;
	FString Out = FString::Printf(
		TEXT("The last storm left unturned wrack along the shore, about %.0f metres away (move_to target: %s). If curiosity takes you there, fly to it and inspect only once close; it is an invitation, not an obligation."),
		Distant[0].Key / 100.f, *TargetTagFor(Nearest.Id).ToString());
	if (Distant.Num() > 1)
		Out += FString::Printf(TEXT(" Another unturned piece lies elsewhere along the shore (move_to target: %s)."), *TargetTagFor(Distant[1].Value->Id).ToString());
	if (Distant.Num() > 2)
		Out += FString::Printf(TEXT(" %d more fresh pieces lie along the shore."), Distant.Num() - 2);
	for (const TPair<float, const FIslandWrackItem*>& Entry : Distant)
	{
		if (Entry.Value->Kind != EIslandWrackKind::Driftwood) continue;
		Out += FString::Printf(TEXT(" Untouched storm-fallen driftwood also lies at %s; if the Raven lands and moves close, it may gather a small bundle of loose branchlets for its nest (build target: GatherTwigs). This is optional, and gathering it does not oblige it to weave a nest."),
			*TargetTagFor(Entry.Value->Id).ToString());
		break;
	}
	return Out;
}

static FAutoConsoleCommandWithWorldAndArgs GIslandWrackStormCommand(
	TEXT("Island.WrackStorm"),
	TEXT("Developer: leave fresh storm wrack on the shore now (saved like a real storm's). Usage: Island.WrackStorm [count]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIslandWrackSubsystem* Wrack = World ? World->GetSubsystem<UIslandWrackSubsystem>() : nullptr;
		if (!Wrack) return;
		const int32 Placed = Wrack->DepositAfterStorm(UIslandWorldStateSubsystem::CurrentIslandDay(World), Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0);
		UE_LOG(LogIslandWrack, Log, TEXT("Island.WrackStorm placed %d items (%d on the shore)"), Placed, Wrack->GetLedger().Items.Num());
	}));

static FAutoConsoleCommandWithWorldAndArgs GIslandWrackTurnCommand(
	TEXT("Island.WrackTurn"),
	TEXT("Developer: turn over a piece of wrack by id as a debug visitor. Usage: Island.WrackTurn <id>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIslandWrackSubsystem* Wrack = World ? World->GetSubsystem<UIslandWrackSubsystem>() : nullptr;
		if (!Wrack || Args.IsEmpty()) return;
		FString Fact;
		Wrack->Examine(FCString::Atoi(*Args[0]), UIslandWorldStateSubsystem::CurrentIslandDay(World), TEXT("debug_visitor"), Fact);
		UE_LOG(LogIslandWrack, Log, TEXT("Island.WrackTurn: %s"), *Fact);
	}));
