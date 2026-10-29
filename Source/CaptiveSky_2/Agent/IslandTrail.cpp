#include "IslandTrail.h"
#include "AgentDataPaths.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandChronicle.h"
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
#include <cmath>

DEFINE_LOG_CATEGORY_STATIC(LogIslandTrail, Log, All);

namespace
{
	constexpr int32 TrailVersion = 2;
	constexpr int32 LegacyTrailVersion = 1;
	constexpr float PrintHalfStance = 10.f;

	float HashUnit(const FIntPoint& Cell, int32 Salt)
	{
		uint32 H = static_cast<uint32>(Cell.X) * 73856093u ^ static_cast<uint32>(Cell.Y) * 19349663u ^ static_cast<uint32>(Salt) * 83492791u;
		H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15;
		return static_cast<float>(H & 0xffffu) / 65535.f;
	}
}

FIntPoint FIslandTrailLedger::CellFor(const FVector& Position)
{
	return FIntPoint(FMath::FloorToInt(Position.X / CellSize), FMath::FloorToInt(Position.Y / CellSize));
}

FIntPoint FIslandTrailLedger::RegionForCell(const FIntPoint& CellKey)
{
	auto FloorDivide = [](int32 Value)
	{
		int32 Quotient = Value / RegionCellSpan;
		if (Value < 0 && Value % RegionCellSpan != 0) --Quotient;
		return Quotient;
	};
	return FIntPoint(FloorDivide(CellKey.X), FloorDivide(CellKey.Y));
}

FString FIslandTrailLedger::RegionId(const FIntPoint& RegionKey)
{
	return FString::Printf(TEXT("trail-region:%d:%d"), RegionKey.X, RegionKey.Y);
}

FIslandTrailRegionSummary FIslandTrailLedger::SummarizeRegionForCell(const FIntPoint& CellKey, int32 Today) const
{
	FIslandTrailRegionSummary Summary;
	Summary.RegionKey = RegionForCell(CellKey);
	Summary.StableId = RegionId(Summary.RegionKey);
	TArray<FIntPoint, TInlineAllocator<RegionCellSpan * RegionCellSpan>> ContributingCells;
	bool bHasUnknownAge = false;
	bool bHasFutureAge = false;

	const int64 StartX = static_cast<int64>(Summary.RegionKey.X) * RegionCellSpan;
	const int64 StartY = static_cast<int64>(Summary.RegionKey.Y) * RegionCellSpan;
	for (int32 DX = 0; DX < RegionCellSpan; ++DX)
	{
		for (int32 DY = 0; DY < RegionCellSpan; ++DY)
		{
			const FIntPoint Key(static_cast<int32>(StartX + DX), static_cast<int32>(StartY + DY));
			const FIslandTrailCell* Cell = Cells.Find(Key);
			if (!Cell || Cell->Steps < MinRegionCellSteps) continue;

			++Summary.WornCellCount;
			Summary.CurrentWearSteps += Cell->Steps;
			ContributingCells.Add(Key);
			if (Cell->LastUseDay < 0)
			{
				bHasUnknownAge = true;
			}
			else
			{
				Summary.LastUseDay = FMath::Max(Summary.LastUseDay, Cell->LastUseDay);
				bHasFutureAge |= Today < 0 || Cell->LastUseDay > Today;
			}
		}
	}

	if (Summary.WornCellCount == 0)
	{
		return Summary;
	}

	if (Today >= 0 && Summary.LastUseDay >= 0 && Summary.LastUseDay <= Today &&
		Today - Summary.LastUseDay < RegionRecoveryDays)
	{
		// A single dated recent crossing is enough to prove this region is active, even if older cells are undated.
		Summary.State = EIslandTrailRegionState::Active;
	}
	else if (!bHasUnknownAge && !bHasFutureAge && Summary.LastUseDay >= 0 && Today >= Summary.LastUseDay &&
		Today - Summary.LastUseDay >= RegionRecoveryDays)
	{
		Summary.State = EIslandTrailRegionState::Recovering;
	}
	else
	{
		Summary.State = EIslandTrailRegionState::Unknown;
	}

	const int32 SampleCount = FMath::Min(ContributingCells.Num(), MaxRegionSamples);
	Summary.SampleCells.Reserve(SampleCount);
	for (int32 I = 0; I < SampleCount; ++I)
	{
		const int32 Index = SampleCount == 1
			? 0
			: static_cast<int32>((static_cast<int64>(I) * (ContributingCells.Num() - 1)) / (SampleCount - 1));
		Summary.SampleCells.Add(ContributingCells[Index]);
	}
	return Summary;
}

TArray<FIslandTrailRegionSummary> FIslandTrailLedger::SummarizeWornRegions(int32 Today) const
{
	TSet<FIntPoint> UniqueRegionKeys;
	UniqueRegionKeys.Reserve(Cells.Num());
	for (const TPair<FIntPoint, FIslandTrailCell>& Pair : Cells)
	{
		if (Pair.Value.Steps >= MinRegionCellSteps)
		{
			UniqueRegionKeys.Add(RegionForCell(Pair.Key));
		}
	}

	TArray<FIntPoint> RegionKeys = UniqueRegionKeys.Array();
	RegionKeys.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X == B.X ? A.Y < B.Y : A.X < B.X;
	});

	TArray<FIslandTrailRegionSummary> Summaries;
	Summaries.Reserve(RegionKeys.Num());
	for (const FIntPoint& RegionKey : RegionKeys)
	{
		const FIntPoint FirstCell(RegionKey.X * RegionCellSpan, RegionKey.Y * RegionCellSpan);
		FIslandTrailRegionSummary Summary = SummarizeRegionForCell(FirstCell, Today);
		if (Summary.WornCellCount > 0)
		{
			Summaries.Add(MoveTemp(Summary));
		}
	}
	return Summaries;
}

bool FIslandTrailLedger::AddStep(const FVector& Position, const FVector& GroundNormal, int32 IslandDay)
{
	const double CellX = static_cast<double>(Position.X) / CellSize;
	const double CellY = static_cast<double>(Position.Y) / CellSize;
	if (IslandDay < -1 || !FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y) || !FMath::IsFinite(Position.Z) ||
		!FMath::IsFinite(GroundNormal.X) || !FMath::IsFinite(GroundNormal.Y) || !FMath::IsFinite(GroundNormal.Z) ||
		CellX < TNumericLimits<int32>::Min() || CellX > TNumericLimits<int32>::Max() ||
		CellY < TNumericLimits<int32>::Min() || CellY > TNumericLimits<int32>::Max())
	{
		return false;
	}

	const FIntPoint Key = CellFor(Position);
	FIslandTrailCell* Cell = Cells.Find(Key);
	if (!Cell)
	{
		if (Cells.Num() >= MaxCells) return false;
		Cell = &Cells.Add(Key);
	}
	++Cell->Steps;
	if (IslandDay >= 0) Cell->LastUseDay = FMath::Max(Cell->LastUseDay, IslandDay);
	Cell->Position = Position;
	Cell->NormalX = GroundNormal.X;
	Cell->NormalY = GroundNormal.Y;
	return true;
}

int32 FIslandTrailLedger::StepsAt(const FVector& Position) const
{
	const FIslandTrailCell* Cell = Cells.Find(CellFor(Position));
	return Cell ? Cell->Steps : 0;
}

void FIslandTrailLedger::Decay(int32 Days)
{
	for (int32 Day = 0; Day < Days && !Cells.IsEmpty(); ++Day)
	{
		for (auto It = Cells.CreateIterator(); It; ++It)
		{
			It.Value().Steps -= FMath::Max(1, It.Value().Steps / 25);
			if (It.Value().Steps <= 0) It.RemoveCurrent();
		}
	}
}

bool FIslandTrailLedger::WeatherTo(int32 Today)
{
	if (Today < 0) return false;
	if (LastDecayDay < 0)
	{
		LastDecayDay = Today;
		return true;
	}
	if (Today <= LastDecayDay) return false;
	Decay(FMath::Min(Today - LastDecayDay, 400));
	LastDecayDay = Today;
	return true;
}

FString FIslandTrailLedger::ToJson() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), TrailVersion);
	Root->SetNumberField(TEXT("decay_day"), LastDecayDay);
	TArray<TSharedPtr<FJsonValue>> Rows;
	for (const TPair<FIntPoint, FIslandTrailCell>& Pair : Cells)
	{
		const FIslandTrailCell& C = Pair.Value;
		const double Values[9] = {static_cast<double>(Pair.Key.X), static_cast<double>(Pair.Key.Y), static_cast<double>(C.Steps),
			static_cast<double>(C.LastUseDay), C.Position.X, C.Position.Y, C.Position.Z,
			static_cast<double>(C.NormalX), static_cast<double>(C.NormalY)};
		TArray<TSharedPtr<FJsonValue>> Row;
		for (double V : Values) Row.Add(MakeShared<FJsonValueNumber>(V));
		Rows.Add(MakeShared<FJsonValueArray>(Row));
	}
	Root->SetArrayField(TEXT("cells"), Rows);
	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool FIslandTrailLedger::FromJson(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) return false;
	const double MinInt = static_cast<double>(TNumericLimits<int32>::Min());
	const double MaxInt = static_cast<double>(TNumericLimits<int32>::Max());
	double Version = 0.0;
	if (!Root->TryGetNumberField(TEXT("version"), Version) || !std::isfinite(Version) ||
		(Version != static_cast<double>(LegacyTrailVersion) && Version != static_cast<double>(TrailVersion))) return false;
	const bool bHasLastUseDay = Version >= static_cast<double>(TrailVersion);
	const int32 ExpectedRowSize = bHasLastUseDay ? 9 : 8;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!Root->TryGetArrayField(TEXT("cells"), Rows) || Rows->Num() > MaxCells) return false;
	TMap<FIntPoint, FIslandTrailCell> Loaded;
	for (const TSharedPtr<FJsonValue>& RowValue : *Rows)
	{
		const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
		if (!RowValue.IsValid() || !RowValue->TryGetArray(Row) || Row->Num() != ExpectedRowSize) return false;
		double V[9] = {};
		for (int32 I = 0; I < ExpectedRowSize; ++I)
		{
			if (!(*Row)[I].IsValid() || !(*Row)[I]->TryGetNumber(V[I]) || !std::isfinite(V[I])) return false;
		}

		if (V[0] < MinInt || V[0] > MaxInt || V[1] < MinInt || V[1] > MaxInt ||
			V[2] < 0.0 || V[2] > MaxInt ||
			std::floor(V[0]) != V[0] || std::floor(V[1]) != V[1] || std::floor(V[2]) != V[2])
		{
			return false;
		}
		if (bHasLastUseDay && (V[3] < -1.0 || V[3] > MaxInt || std::floor(V[3]) != V[3])) return false;
		const int32 PositionIndex = bHasLastUseDay ? 4 : 3;
		const int32 NormalIndex = PositionIndex + 3;
		if (V[PositionIndex] / CellSize < MinInt || V[PositionIndex] / CellSize > MaxInt ||
			V[PositionIndex + 1] / CellSize < MinInt || V[PositionIndex + 1] / CellSize > MaxInt ||
			V[NormalIndex] < -1.0 || V[NormalIndex] > 1.0 || V[NormalIndex + 1] < -1.0 || V[NormalIndex + 1] > 1.0 ||
			FMath::Square(V[NormalIndex]) + FMath::Square(V[NormalIndex + 1]) > 1.0001)
		{
			return false;
		}

		const FIntPoint Key(static_cast<int32>(V[0]), static_cast<int32>(V[1]));
		FIslandTrailCell Cell;
		Cell.Steps = static_cast<int32>(V[2]);
		Cell.LastUseDay = bHasLastUseDay ? static_cast<int32>(V[3]) : -1;
		Cell.Position = FVector(V[PositionIndex], V[PositionIndex + 1], V[PositionIndex + 2]);
		Cell.NormalX = static_cast<float>(V[NormalIndex]);
		Cell.NormalY = static_cast<float>(V[NormalIndex + 1]);
		if (Cell.Steps == 0) continue;
		if (CellFor(Cell.Position) != Key || Loaded.Contains(Key)) return false;
		Loaded.Add(Key, Cell);
	}
	int32 Day = -1;
	if (Root->HasField(TEXT("decay_day")))
	{
		double DayValue = 0.0;
		if (!Root->TryGetNumberField(TEXT("decay_day"), DayValue) || !std::isfinite(DayValue) ||
			DayValue < -1.0 || DayValue > MaxInt || std::floor(DayValue) != DayValue)
		{
			return false;
		}
		Day = static_cast<int32>(DayValue);
	}
	Cells = MoveTemp(Loaded);
	LastDecayDay = Day;
	return true;
}

AIslandTrailMarks::AIslandTrailMarks()
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
		Layer->SetCastShadow(false);
		Layer->SetMobility(EComponentMobility::Movable);
		if (Mesh) Layer->SetStaticMesh(Mesh);
		if (Material) Layer->SetMaterial(0, Material);
		return Layer;
	};
	UMaterialInterface* Material = Basic.Succeeded() ? Basic.Object : nullptr;
	Prints = Make(TEXT("Prints"), Sphere.Succeeded() ? Sphere.Object : nullptr, Material);
	Wear = Make(TEXT("Wear"), Cylinder.Succeeded() ? Cylinder.Object : nullptr, Material);
	Tags.AddUnique(TEXT("IslandTrailMarks"));
}

void AIslandTrailMarks::EnsureMaterials()
{
	auto Bind = [this](UInstancedStaticMeshComponent* Layer, TObjectPtr<UMaterialInstanceDynamic>& Slot, const FLinearColor& Color)
	{
		if (Slot || !Layer || !Layer->GetMaterial(0)) return;
		Slot = UMaterialInstanceDynamic::Create(Layer->GetMaterial(0), this);
		if (Slot)
		{
			Slot->SetVectorParameterValue(TEXT("Color"), Color);
			Layer->SetMaterial(0, Slot);
		}
	};
	Bind(Prints, PrintMaterial, FLinearColor(0.035f, 0.04f, 0.045f));
	Bind(Wear, WearMaterial, UIslandTrailSubsystem::WearColor(0.f));
}

float UIslandTrailSubsystem::PrintLifetime(float Wetness)
{
	if (Wetness < PrintWetness) return 0.f;
	return FMath::Lerp(25.f, 140.f, FMath::SmoothStep(PrintWetness, 1.f, FMath::Min(Wetness, 1.f)));
}

float UIslandTrailSubsystem::PrintScale(float Age, float Life, float Wetness)
{
	if (Life <= 0.f || Age >= Life) return 0.f;
	const float Fade = 1.f - FMath::SmoothStep(Life * 0.7f, Life, FMath::Max(0.f, Age));
	const float Wet = FMath::SmoothStep(0.02f, 0.12f, Wetness);
	return Fade * Wet;
}

bool UIslandTrailSubsystem::ShouldRefreshPrintInstance(bool bActive, bool bRendered, float Scale)
{
	return bRendered || (bActive && Scale > 0.01f);
}

float UIslandTrailSubsystem::WearAmount(int32 Steps)
{
	return FMath::SmoothStep(static_cast<float>(WearStartSteps), static_cast<float>(WearFullSteps), static_cast<float>(Steps));
}

FLinearColor UIslandTrailSubsystem::WearColor(float Wetness)
{
	return FMath::Lerp(FLinearColor(0.2f, 0.15f, 0.09f), FLinearColor(0.055f, 0.04f, 0.028f), FMath::SmoothStep(0.1f, 0.8f, Wetness));
}

bool UIslandTrailSubsystem::ShouldChronicleTrailEmergence(const FIslandTrailRegionSummary& Before,
	const FIslandTrailRegionSummary& After, int32 EntriesToday, int32 IslandDay)
{
	if (IslandDay <= 0 || EntriesToday >= MaxTrailChronicleEntriesPerDay) return false;
	auto IsEstablishedActivePath = [](const FIslandTrailRegionSummary& Summary)
	{
		return Summary.State == EIslandTrailRegionState::Active &&
			Summary.WornCellCount >= TrailChronicleCellThreshold &&
			Summary.CurrentWearSteps >= TrailChronicleUseThreshold;
	};
	return !IsEstablishedActivePath(Before) && IsEstablishedActivePath(After);
}

FQuat UIslandTrailSubsystem::CalculateWearRotation(const FIntPoint& Key,
	const TMap<FIntPoint, FIslandTrailCell>& Cells, const FVector& GroundNormal, float& OutAspectRatio)
{
	double XX = 0.0;
	double YY = 0.0;
	double XY = 0.0;
	for (int32 DX = -1; DX <= 1; ++DX)
	{
		for (int32 DY = -1; DY <= 1; ++DY)
		{
			if (DX == 0 && DY == 0) continue;
			const FIslandTrailCell* Neighbor = Cells.Find(Key + FIntPoint(DX, DY));
			if (!Neighbor) continue;
			const double Weight = WearAmount(Neighbor->Steps);
			if (Weight <= 0.0) continue;

			const double X = static_cast<double>(DX);
			const double Y = static_cast<double>(DY);
			XX += Weight * X * X;
			YY += Weight * Y * Y;
			XY += Weight * X * Y;
		}
	}

	const double Trace = XX + YY;
	double Angle = static_cast<double>(HashUnit(Key, 1)) * 2.0 * PI;
	OutAspectRatio = 1.f;
	if (Trace > UE_DOUBLE_SMALL_NUMBER)
	{
		const double Anisotropy = FMath::Sqrt(FMath::Square(XX - YY) + 4.0 * FMath::Square(XY)) / Trace;
		if (Anisotropy > 0.2)
		{
			// The covariance's principal axis follows a line of adjacent, worn cells.
			Angle = 0.5 * FMath::Atan2(2.0 * XY, XX - YY);
			OutAspectRatio = 1.f + 0.45f * static_cast<float>(Anisotropy);
		}
	}
	const FVector Tangent(static_cast<float>(FMath::Cos(Angle)), static_cast<float>(FMath::Sin(Angle)), 0.f);
	const FVector Normal = GroundNormal.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
	return FRotationMatrix::MakeFromXZ(Tangent, Normal).ToQuat();
}

TStatId UIslandTrailSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandTrailSubsystem, STATGROUP_Tickables);
}

bool UIslandTrailSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UIslandTrailSubsystem::GetActivePrintCount() const
{
	int32 Count = 0;
	for (const FPrint& Print : PrintRing) Count += Print.bActive ? 1 : 0;
	return Count;
}

FString UIslandTrailSubsystem::DescribeTrail(int32 Steps, int32 NearbyPrints, float Wetness, int32 LastUseDay, int32 Today)
{
	FString Out;
	if (Steps >= WearFullSteps) Out += TEXT(" The ground underfoot is trodden bare along a track that many feet have worn.");
	else if (Steps >= WearStartSteps) Out += TEXT(" The grass here is worn thin along a track where feet often pass.");
	else if (Steps >= WearStartSteps / 2) Out += TEXT(" The grass is faintly flattened here, as if others have passed this way before.");
	if (Steps >= WearStartSteps / 2 && LastUseDay >= 0 && Today >= LastUseDay &&
		Today - LastUseDay >= RecoveryNoticeAfterDays)
		Out += TEXT(" The old track is softening as grass slowly creeps back over ground left unused.");
	if (NearbyPrints >= 3)
		Out += Wetness < 0.3f ? TEXT(" Dark footprints still mark the ground nearby, shrinking as it dries.") : TEXT(" Fresh footprints are pressed into the wet ground nearby.");
	return Out;
}

FString UIslandTrailSubsystem::DescribeUnderfoot(const FVector& Position, float Wetness) const
{
	int32 Steps = 0;
	int32 LastUseDay = -1;
	const FIntPoint Here = FIslandTrailLedger::CellFor(Position);
	for (int32 DX = -1; DX <= 1; ++DX)
		for (int32 DY = -1; DY <= 1; ++DY)
			if (const FIslandTrailCell* Cell = Ledger.Cells.Find(Here + FIntPoint(DX, DY)))
			{
				Steps = FMath::Max(Steps, Cell->Steps);
				if (Cell->Steps >= WearStartSteps / 2) LastUseDay = FMath::Max(LastUseDay, Cell->LastUseDay);
			}
	int32 Prints = 0;
	for (const FPrint& Print : PrintRing)
		if (Print.bActive && FVector::DistSquared2D(Print.Position, Position) < FMath::Square(600.f)) ++Prints;
	const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld());
	FString Description = DescribeTrail(Steps, Prints, Wetness, LastUseDay, Today);
	const FIslandTrailRegionSummary Region = Ledger.SummarizeRegionForCell(Here, Today);
	if (Region.WornCellCount >= 3)
	{
		Description += FString::Printf(TEXT(" Across this stretch, worn ground touches %d patches."), Region.WornCellCount);
		switch (Region.State)
		{
		case EIslandTrailRegionState::Active:
			Description += TEXT(" Footsteps have crossed here recently.");
			break;
		case EIslandTrailRegionState::Recovering:
			Description += TEXT(" The older worn patches are slowly softening as grass returns.");
			break;
		default:
			Description += TEXT(" The age of some worn patches is no longer known.");
			break;
		}
	}
	return Description;
}

FString UIslandTrailSubsystem::GetStorageFilePath() const
{
	if (!bAllowStorage) return FString();
	if (!StorageFileOverride.IsEmpty()) return StorageFileOverride;
	if (!GetWorld() || GetWorld()->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"))) return FString();
	const FString MapName = UWorld::RemovePIEPrefix(GetWorld()->GetMapName());
	return CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("WorldState") / (MapName + TEXT(".trails.json")));
}

void UIslandTrailSubsystem::Load()
{
	const FString Path = GetStorageFilePath();
	FString Contents;
	if (Path.IsEmpty() || !FFileHelper::LoadFileToString(Contents, *Path)) return;
	if (!Ledger.FromJson(Contents))
	{
		UE_LOG(LogIslandTrail, Warning, TEXT("Ignoring unreadable trail ledger %s and not overwriting it"), *Path);
		bAllowStorage = false;
	}
}

void UIslandTrailSubsystem::Save()
{
	bLedgerDirty = false;
	const FString Path = GetStorageFilePath();
	if (Path.IsEmpty()) return;
	const FString Temporary = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Ledger.ToJson(), *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
		!IFileManager::Get().Move(*Path, *Temporary, true, true))
		UE_LOG(LogIslandTrail, Error, TEXT("Failed to save trail ledger to %s"), *Path);
}

void UIslandTrailSubsystem::WeatherLedger()
{
	if (Ledger.WeatherTo(UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld()))) bLedgerDirty = true;
}

void UIslandTrailSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	PrintRing.SetNum(MaxPrints);
	Load();
	WeatherLedger();
	TrailChronicleDay = UIslandWorldStateSubsystem::CurrentIslandDay(&InWorld);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Marks = InWorld.SpawnActor<AIslandTrailMarks>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
}

void UIslandTrailSubsystem::Deinitialize()
{
	if (bLedgerDirty) Save();
	if (AIslandTrailMarks* Actor = Marks.Get()) Actor->Destroy();
	Marks.Reset();
	Super::Deinitialize();
}

void UIslandTrailSubsystem::Footfall(const APawn& Pawn, FWalker& Walker, double Now, float Wetness, int32 IslandDay)
{
	const ACharacter* Character = Cast<ACharacter>(&Pawn);
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !Movement->IsMovingOnGround() || !Movement->CurrentFloor.IsWalkableFloor())
	{
		Walker.bHasLast = false;
		return;
	}
	const FVector Velocity = Movement->Velocity;
	if (Velocity.Size2D() < MinWalkSpeed) return;
	const FHitResult& Floor = Movement->CurrentFloor.HitResult;
	if (!Cast<ALandscapeProxy>(Floor.GetActor()))
	{
		Walker.bHasLast = false;
		return;
	}

	const FVector Ground = Floor.ImpactPoint;
	const FVector Normal = Floor.ImpactNormal.GetSafeNormal();
	if (!Walker.bHasLast)
	{
		Walker.LastPrint = Ground;
		Walker.bHasLast = true;
		return;
	}
	if (FVector::Dist2D(Ground, Walker.LastPrint) < StrideLength) return;
	Walker.LastPrint = Ground;
	Walker.bRight = !Walker.bRight;

	if (IslandDay > 0 && TrailChronicleDay != IslandDay)
	{
		TrailChronicleDay = IslandDay;
		TrailChronicleEntriesToday = 0;
	}
	const FIntPoint CellKey = FIslandTrailLedger::CellFor(Ground);
	const FIslandTrailRegionSummary BeforeRegion = Ledger.SummarizeRegionForCell(CellKey, IslandDay);
	if (Ledger.AddStep(Ground, Normal, IslandDay))
	{
		bLedgerDirty = true;
		const FIslandTrailRegionSummary AfterRegion = Ledger.SummarizeRegionForCell(CellKey, IslandDay);
		if (ShouldChronicleTrailEmergence(BeforeRegion, AfterRegion, TrailChronicleEntriesToday, IslandDay))
		{
			++TrailChronicleEntriesToday;
			TMap<FString, FString> Extra;
			Extra.Add(TEXT("event"), TEXT("trail_emerged"));
			Extra.Add(TEXT("region_id"), AfterRegion.StableId);
			Extra.Add(TEXT("region_x"), FString::FromInt(AfterRegion.RegionKey.X));
			Extra.Add(TEXT("region_y"), FString::FromInt(AfterRegion.RegionKey.Y));
			Extra.Add(TEXT("worn_cells"), FString::FromInt(AfterRegion.WornCellCount));
			Extra.Add(TEXT("use_steps"), FString::Printf(TEXT("%lld"), AfterRegion.CurrentWearSteps));
			UIslandChronicleSubsystem::Record(GetWorld(), TEXT("trail"), FString(),
				TEXT("Repeated footfalls have begun to wear a familiar path into the ground."), Extra);
		}
	}

	const float Life = PrintLifetime(Wetness);
	if (Life <= 0.f || PrintRing.IsEmpty()) return;
	const FVector Forward = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
	const FVector Side(-Forward.Y, Forward.X, 0.f);
	FPrint& Print = PrintRing[NextPrint];
	NextPrint = (NextPrint + 1) % PrintRing.Num();
	Print.Position = Ground + Side * (Walker.bRight ? PrintHalfStance : -PrintHalfStance) + Normal * 0.4f;
	Print.Rotation = FRotationMatrix::MakeFromXZ(Forward, Normal).ToQuat();
	Print.Born = Now;
	Print.Life = Life;
	Print.bActive = true;
}

void UIslandTrailSubsystem::RefreshPrints(double Now, float Wetness)
{
	AIslandTrailMarks* Actor = Marks.Get();
	if (!Actor || !Actor->Prints) return;
	UInstancedStaticMeshComponent* Layer = Actor->Prints;
	while (Layer->GetInstanceCount() < PrintRing.Num()) Layer->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.0001f)));
	const FVector Footprint(0.28f, 0.12f, 0.03f);
	int32 LastRefreshIndex = INDEX_NONE;
	for (int32 I = 0; I < PrintRing.Num(); ++I)
	{
		FPrint& Print = PrintRing[I];
		float Scale = 0.f;
		if (Print.bActive)
		{
			Scale = PrintScale(static_cast<float>(Now - Print.Born), Print.Life, Wetness);
			if (Now - Print.Born >= Print.Life) Print.bActive = false;
		}
		if (ShouldRefreshPrintInstance(Print.bActive, Print.bRendered, Scale)) LastRefreshIndex = I;
	}
	for (int32 I = 0; I <= LastRefreshIndex; ++I)
	{
		FPrint& Print = PrintRing[I];
		const float Scale = Print.bActive
			? PrintScale(static_cast<float>(Now - Print.Born), Print.Life, Wetness)
			: 0.f;
		if (!ShouldRefreshPrintInstance(Print.bActive, Print.bRendered, Scale)) continue;
		const FTransform T = Scale > 0.01f
			? FTransform(Print.Rotation, Print.Position, Footprint * Scale)
			: FTransform(FQuat::Identity, Print.Position, FVector(0.0001f));
		Layer->UpdateInstanceTransform(I, T, true, I == LastRefreshIndex, true);
		Print.bRendered = Scale > 0.01f;
	}
}

void UIslandTrailSubsystem::RefreshWear(const FVector& Viewer, float Wetness)
{
	AIslandTrailMarks* Actor = Marks.Get();
	if (!Actor || !Actor->Wear) return;
	struct FCandidate
	{
		float DistSq;
		FIntPoint Key;
		FIslandTrailCell Cell;
	};
	TArray<FCandidate> Candidates;
	const float RangeSq = FMath::Square(WearDrawRadius);
	for (const TPair<FIntPoint, FIslandTrailCell>& Pair : Ledger.Cells)
	{
		if (WearAmount(Pair.Value.Steps) <= 0.f) continue;
		const float DistSq = static_cast<float>(FVector::DistSquared2D(Pair.Value.Position, Viewer));
		if (DistSq <= RangeSq) Candidates.Add({DistSq, Pair.Key, Pair.Value});
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.DistSq < B.DistSq; });

	UInstancedStaticMeshComponent* Layer = Actor->Wear;
	while (Layer->GetInstanceCount() < MaxWearDiscs) Layer->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(0.0001f)));
	for (int32 I = 0; I < MaxWearDiscs; ++I)
	{
		FTransform T(FQuat::Identity, Viewer, FVector(0.0001f));
		if (Candidates.IsValidIndex(I))
		{
			const FIntPoint Key = Candidates[I].Key;
			const FIslandTrailCell& Cell = Candidates[I].Cell;
			const float Amount = WearAmount(Cell.Steps);
			const float NormalZ = FMath::Sqrt(FMath::Max(0.01f, 1.f - Cell.NormalX * Cell.NormalX - Cell.NormalY * Cell.NormalY));
			const FVector Normal = FVector(Cell.NormalX, Cell.NormalY, NormalZ).GetSafeNormal();
			float AspectRatio = 1.f;
			const FQuat WearRotation = CalculateWearRotation(Key, Ledger.Cells, Normal, AspectRatio);
			const float Diameter = FMath::Lerp(80.f, 150.f, Amount) * (0.85f + 0.3f * HashUnit(Key, 2));
			const float Squash = 0.8f + 0.3f * HashUnit(Key, 3);
			T = FTransform(WearRotation, Cell.Position + Normal * 0.6f,
				FVector(Diameter / 100.f * AspectRatio, Diameter / 100.f / AspectRatio * Squash, 0.01f));
		}
		Layer->UpdateInstanceTransform(I, T, true, I == MaxWearDiscs - 1, true);
	}
	if (Actor->WearMaterial) Actor->WearMaterial->SetVectorParameterValue(TEXT("Color"), WearColor(Wetness));
}

void UIslandTrailSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	AIslandTrailMarks* Actor = Marks.Get();
	if (!World || !Actor) return;
	Actor->EnsureMaterials();
	const double Now = World->GetTimeSeconds();
	float Wetness = 0.f;
	if (const UIslandEnvironmentSubsystem* Environment = World->GetSubsystem<UIslandEnvironmentSubsystem>()) Wetness = Environment->GetWetness();

	if (Now >= NextRefresh)
	{
		NextRefresh = Now + 0.1;
		const int32 IslandDay = UIslandWorldStateSubsystem::CurrentIslandDay(World);
		for (TActorIterator<ACharacter> It(World); It; ++It)
			Footfall(**It, Walkers.FindOrAdd(*It), Now, Wetness, IslandDay);
		for (auto It = Walkers.CreateIterator(); It; ++It)
			if (!It.Key().IsValid()) It.RemoveCurrent();
		RefreshPrints(Now, Wetness);
	}

	const APlayerController* Player = World->GetFirstPlayerController();
	if (Player && Player->GetPawn() && Now >= NextWearRefresh)
	{
		NextWearRefresh = Now + 1.5;
		RefreshWear(Player->GetPawn()->GetActorLocation(), Wetness);
	}
	if (Now >= NextWeathering)
	{
		NextWeathering = Now + 30.0;
		WeatherLedger();
	}
	if (bLedgerDirty && Now >= NextSave)
	{
		NextSave = Now + 45.0;
		Save();
	}
}
