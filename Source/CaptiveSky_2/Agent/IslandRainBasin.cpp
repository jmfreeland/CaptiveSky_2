#include "IslandRainBasin.h"
#include "AgentDataPaths.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IslandChronicle.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandWorldStateSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "NavigationSystem.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogIslandRainBasin, Log, All);

namespace
{
	constexpr int32 BasinVersion = 1;
	constexpr float MaxWaterDepth = 9.f;
	constexpr float FillSecondsAtFullRain = 240.f;
	constexpr float NightDryRate = 0.0001f;
	constexpr float SunDryRate = 0.0004f;
	constexpr int32 RimStones = 12;
	constexpr float LeafWaterRadius = 30.f;
}

float FIslandBasinState::Advance(float Water, float Rain, float SunHeight, float Seconds)
{
	if (!FMath::IsFinite(Water) || !FMath::IsFinite(Rain) || !FMath::IsFinite(SunHeight) || !FMath::IsFinite(Seconds) || Seconds <= 0.f)
		return FMath::Clamp(FMath::IsFinite(Water) ? Water : 0.f, 0.f, 1.f);
	Water = FMath::Clamp(Water, 0.f, 1.f);
	const float Falling = FMath::Clamp(Rain, 0.f, 1.f);
	// A passing sprinkle wets the stone but does not gather in the hollow.
	if (Falling > 0.05f) Water += Falling * Seconds / FillSecondsAtFullRain;
	Water -= (NightDryRate + SunDryRate * FMath::Clamp(SunHeight, 0.f, 1.f)) * Seconds;
	return FMath::Clamp(Water, 0.f, 1.f);
}

int32 FIslandBasinState::LeavesFrom(const FString& AgentId) const
{
	int32 Count = 0;
	for (const FIslandBasinLeaf& Leaf : Leaves) Count += Leaf.AgentId == AgentId ? 1 : 0;
	return Count;
}

EIslandBasinFloat FIslandBasinState::FloatLeaf(const FString& AgentId, int32 Today, int32 Seed)
{
	if (Water < FloatLevel) return EIslandBasinFloat::TooDry;
	for (const FIslandBasinLeaf& Leaf : Leaves)
		if (Leaf.AgentId == AgentId && Leaf.Day == Today) return EIslandBasinFloat::AlreadyToday;
	const bool bCrowded = Leaves.Num() >= MaxLeaves;
	if (bCrowded) Leaves.RemoveAt(0);
	FIslandBasinLeaf& Added = Leaves.AddDefaulted_GetRef();
	Added.AgentId = AgentId;
	Added.Day = Today;
	Added.Seed = Seed;
	return bCrowded ? EIslandBasinFloat::FloatedReplacingOldest : EIslandBasinFloat::Floated;
}

FString FIslandBasinState::ToJson() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), BasinVersion);
	Root->SetBoolField(TEXT("placed"), bPlaced);
	Root->SetArrayField(TEXT("location"), { MakeShared<FJsonValueNumber>(Location.X), MakeShared<FJsonValueNumber>(Location.Y), MakeShared<FJsonValueNumber>(Location.Z) });
	Root->SetNumberField(TEXT("water"), Water);
	TArray<TSharedPtr<FJsonValue>> LeafValues;
	for (const FIslandBasinLeaf& Leaf : Leaves)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("agent"), Leaf.AgentId);
		Object->SetNumberField(TEXT("day"), Leaf.Day);
		Object->SetNumberField(TEXT("seed"), Leaf.Seed);
		LeafValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("leaves"), LeafValues);
	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool FIslandBasinState::FromJson(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) return false;
	double Version = 0;
	if (!Root->TryGetNumberField(TEXT("version"), Version) || static_cast<int32>(Version) != BasinVersion) return false;
	FIslandBasinState Loaded;
	Root->TryGetBoolField(TEXT("placed"), Loaded.bPlaced);
	const TArray<TSharedPtr<FJsonValue>>* Where = nullptr;
	if (Root->TryGetArrayField(TEXT("location"), Where) && Where->Num() == 3)
		Loaded.Location = FVector((*Where)[0]->AsNumber(), (*Where)[1]->AsNumber(), (*Where)[2]->AsNumber());
	else
		Loaded.bPlaced = false;
	double WaterValue = 0;
	Root->TryGetNumberField(TEXT("water"), WaterValue);
	Loaded.Water = FMath::IsFinite(WaterValue) ? FMath::Clamp(static_cast<float>(WaterValue), 0.f, 1.f) : 0.f;
	const TArray<TSharedPtr<FJsonValue>>* LeafValues = nullptr;
	if (Root->TryGetArrayField(TEXT("leaves"), LeafValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *LeafValues)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object) continue;
			FIslandBasinLeaf Leaf;
			double Day = 0, Seed = 0;
			if (!(*Object)->TryGetStringField(TEXT("agent"), Leaf.AgentId) || !(*Object)->TryGetNumberField(TEXT("day"), Day)) continue;
			(*Object)->TryGetNumberField(TEXT("seed"), Seed);
			Leaf.Day = static_cast<int32>(Day);
			Leaf.Seed = static_cast<int32>(Seed);
			if (Loaded.Leaves.Num() < MaxLeaves) Loaded.Leaves.Add(Leaf);
		}
	}
	*this = Loaded;
	return true;
}

AIslandRainBasin::AIslandRainBasin()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* Material = Basic.Succeeded() ? Basic.Object : nullptr;
	auto Prepare = [this, Material](UStaticMeshComponent* Part, UStaticMesh* Mesh)
	{
		Part->SetupAttachment(RootComponent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetMobility(EComponentMobility::Movable);
		if (Mesh) Part->SetStaticMesh(Mesh);
		if (Material) Part->SetMaterial(0, Material);
	};
	Rim = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rim"));
	Prepare(Rim, Sphere.Succeeded() ? Sphere.Object : nullptr);
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Prepare(Floor, Cylinder.Succeeded() ? Cylinder.Object : nullptr);
	Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
	Prepare(Surface, Cylinder.Succeeded() ? Cylinder.Object : nullptr);
	Surface->SetCastShadow(false);
	Surface->SetVisibility(false);
	for (const TCHAR* Name : { TEXT("LeavesFresh"), TEXT("LeavesAging"), TEXT("LeavesOld") })
	{
		UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Prepare(Layer, Sphere.Succeeded() ? Sphere.Object : nullptr);
		Layer->SetCastShadow(false);
		LeafLayers.Add(Layer);
	}
	// The first tag after IslandLandmark is the move_to/interact target name.
	Tags.AddUnique(TEXT("IslandLandmark"));
	Tags.AddUnique(TEXT("RainBasin"));
}

float AIslandRainBasin::WaterDepth(float Water)
{
	return MaxWaterDepth * FMath::Clamp(Water, 0.f, 1.f);
}

FLinearColor AIslandRainBasin::LeafColor(int32 AgeDays)
{
	const float Age = FMath::Clamp(AgeDays / 5.f, 0.f, 1.f);
	return FMath::Lerp(FLinearColor(0.10f, 0.30f, 0.06f), FLinearColor(0.30f, 0.17f, 0.06f), Age);
}

void AIslandRainBasin::Show(const FIslandBasinState& State, int32 Today)
{
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* Surface = Part->CreateAndSetMaterialInstanceDynamic(0))
			Surface->SetVectorParameterValue(TEXT("Color"), Color);
	};
	const float Ground = -OriginLift;
	if (!bBuilt)
	{
		bBuilt = true;
		Tint(Rim, FLinearColor(0.30f, 0.29f, 0.27f));
		Tint(Floor, FLinearColor(0.22f, 0.21f, 0.20f));
		Tint(Surface, FLinearColor(0.03f, 0.09f, 0.12f));
		Tint(LeafLayers[0], LeafColor(0));
		Tint(LeafLayers[1], LeafColor(3));
		Tint(LeafLayers[2], LeafColor(6));
		Floor->SetRelativeLocation(FVector(0.f, 0.f, Ground + FloorThickness * 0.5f));
		Floor->SetRelativeScale3D(FVector(RimRadius * 2.f / 100.f, RimRadius * 2.f / 100.f, FloorThickness / 100.f));
		Rim->ClearInstances();
		for (int32 Index = 0; Index < RimStones; ++Index)
		{
			const float Yaw = 360.f * Index / RimStones;
			const FRotator Facing(0.f, Yaw + 90.f, 0.f);
			const FVector Out = FRotator(0.f, Yaw, 0.f).RotateVector(FVector(RimRadius, 0.f, 0.f));
			const float Wobble = 0.9f + 0.2f * FMath::Frac(Index * 0.618f);
			Rim->AddInstance(FTransform(Facing, FVector(Out.X, Out.Y, Ground + RimHeight * 0.5f),
				FVector(0.27f * Wobble, 0.20f, RimHeight / 100.f * Wobble)), false);
		}
	}

	const bool bWater = State.Water > 0.02f;
	const float SurfaceZ = Ground + FloorThickness + WaterDepth(State.Water);
	Surface->SetVisibility(bWater);
	if (bWater)
	{
		Surface->SetRelativeLocation(FVector(0.f, 0.f, SurfaceZ - 0.5f));
		Surface->SetRelativeScale3D(FVector((RimRadius - 5.f) * 2.f / 100.f, (RimRadius - 5.f) * 2.f / 100.f, 0.01f));
	}

	for (UInstancedStaticMeshComponent* Layer : LeafLayers) Layer->ClearInstances();
	for (const FIslandBasinLeaf& Leaf : State.Leaves)
	{
		FRandomStream Random(Leaf.Seed);
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Radius = LeafWaterRadius * FMath::Sqrt(Random.FRand());
		const int32 Age = FMath::Max(0, Today - Leaf.Day);
		// On dry stone a leaf lies on the floor; afloat it rides the surface.
		const float Z = (bWater ? SurfaceZ : Ground + FloorThickness) + 0.8f;
		const FRotator Spin(Random.FRandRange(-4.f, 4.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-4.f, 4.f));
		const FVector Scale(Random.FRandRange(0.10f, 0.14f), Random.FRandRange(0.055f, 0.08f), 0.012f);
		LeafLayers[Age <= 1 ? 0 : Age <= 3 ? 1 : 2]->AddInstance(FTransform(Spin, FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z), Scale), false);
	}
}

FString UIslandRainBasinSubsystem::DescribeWater(float Water, int32 LeafCount)
{
	FString Out;
	if (Water < 0.02f) Out = TEXT("The basin's hollow is dry stone; rain would fill it.");
	else if (Water < FIslandBasinState::FloatLevel) Out = TEXT("The basin holds only a thin film of rainwater, too little to float anything.");
	else if (Water < 0.6f) Out = TEXT("The basin holds a shallow pool of rainwater.");
	else if (Water < 0.9f) Out = TEXT("The basin is nearly full of rainwater.");
	else Out = TEXT("The basin is full to the rim with rainwater.");
	if (LeafCount > 0)
	{
		const TCHAR* Noun = LeafCount == 1 ? TEXT("leaf") : TEXT("leaves");
		Out += Water >= 0.02f
			? FString::Printf(TEXT(" %d %s %s on the water."), LeafCount, Noun, LeafCount == 1 ? TEXT("drifts") : TEXT("drift"))
			: FString::Printf(TEXT(" %d dried %s %s on the stone."), LeafCount, Noun, LeafCount == 1 ? TEXT("lies") : TEXT("lie"));
	}
	return Out;
}

FString UIslandRainBasinSubsystem::DescribeNearby(const FVector& Position) const
{
	if (!State.bPlaced) return FString();
	const float Distance = FVector::Dist(State.Location, Position);
	if (Distance > NoticeRadius) return FString();
	return FString::Printf(TEXT(" A shallow stone rain basin stands %.0f metres away (move_to/interact target: RainBasin). %s Interact sets a leaf afloat when the water is deep enough, once per resident per day; the leaves stay."),
		Distance / 100.f, *DescribeWater(State.Water, State.Leaves.Num()));
}

void UIslandRainBasinSubsystem::ForceWater(float Water)
{
	State.Water = FMath::Clamp(Water, 0.f, 1.f);
	bDirty = true;
	Refresh();
}

bool UIslandRainBasinSubsystem::Examine(const FString& AgentId, int32 Today, FString& OutFact)
{
	if (!State.bPlaced)
	{
		OutFact = TEXT("There is no basin here. Nothing changed.");
		return false;
	}
	const int32 Seed = FMath::Abs(static_cast<int32>(GetTypeHash(AgentId))) + Today * 7919 + State.Leaves.Num() * 104729;
	const FIslandBasinState Before = State;
	switch (State.FloatLeaf(AgentId, Today, Seed))
	{
	case EIslandBasinFloat::TooDry:
		OutFact = DescribeWater(State.Water, State.Leaves.Num()) + TEXT(" A leaf set here would only lie on the stone. Nothing changed.");
		return false;
	case EIslandBasinFloat::AlreadyToday:
		OutFact = FString::Printf(TEXT("%s You set a leaf afloat here earlier today; it still turns slowly. Nothing changed."), *DescribeWater(State.Water, State.Leaves.Num()));
		return false;
	default: break;
	}
	const bool bReplaced = State.Leaves.Num() == FIslandBasinState::MaxLeaves && Before.Leaves.Num() == FIslandBasinState::MaxLeaves;
	const int32 Mine = State.LeavesFrom(AgentId);
	const FString Ownership = State.Leaves.Num() == 1 ? FString(TEXT("It is yours.")) : Mine == 1 ? FString(TEXT("1 of them is yours.")) : FString::Printf(TEXT("%d of them are yours."), Mine);
	OutFact = FString::Printf(TEXT("You pick a fallen leaf from the ground nearby and set it on the water; it turns slowly and settles. %s%s %s This remains after this session, until the leaves are crowded out by newer ones."),
		*DescribeWater(State.Water, State.Leaves.Num()), bReplaced ? TEXT(" The oldest leaf drifted over the rim to make room.") : TEXT(""), *Ownership);
	bDirty = true;
	Save();
	Refresh();
	UIslandChronicleSubsystem::Record(GetWorld(), TEXT("basin"), AgentId,
		FString::Printf(TEXT("set a leaf afloat on the rain basin; %d leaves now drift there."), State.Leaves.Num()),
		{ { TEXT("leaves"), FString::FromInt(State.Leaves.Num()) } });
	return true;
}

TStatId UIslandRainBasinSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandRainBasinSubsystem, STATGROUP_Tickables);
}

bool UIslandRainBasinSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FString UIslandRainBasinSubsystem::GetStorageFilePath() const
{
	if (!bAllowStorage) return FString();
	if (!StorageFileOverride.IsEmpty()) return StorageFileOverride;
	if (!GetWorld() || GetWorld()->GetOutermost()->GetName().StartsWith(TEXT("/Temp/"))) return FString();
	const FString MapName = UWorld::RemovePIEPrefix(GetWorld()->GetMapName());
	return CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("WorldState") / (MapName + TEXT(".basin.json")));
}

void UIslandRainBasinSubsystem::Load()
{
	const FString Path = GetStorageFilePath();
	FString Contents;
	if (Path.IsEmpty() || !FFileHelper::LoadFileToString(Contents, *Path)) return;
	if (!State.FromJson(Contents))
	{
		UE_LOG(LogIslandRainBasin, Warning, TEXT("Ignoring unreadable basin file %s and not overwriting it"), *Path);
		bAllowStorage = false;
	}
}

void UIslandRainBasinSubsystem::Save()
{
	bDirty = false;
	SinceSave = 0.f;
	const FString Path = GetStorageFilePath();
	if (Path.IsEmpty()) return;
	const FString Temporary = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(State.ToJson(), *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
		!IFileManager::Get().Move(*Path, *Temporary, true, true))
		UE_LOG(LogIslandRainBasin, Error, TEXT("Failed to save basin to %s"), *Path);
}

bool UIslandRainBasinSubsystem::PlaceBasin()
{
	UWorld* World = GetWorld();
	if (!World) return false;
	const AActor* ListeningStones = nullptr;
	for (TActorIterator<AActor> It(World); It && !ListeningStones; ++It)
		if (It->ActorHasTag(TEXT("IslandLandmark")) && It->ActorHasTag(TEXT("ListeningStones"))) ListeningStones = *It;
	// Only levels with the Island's landmarks get a basin.
	if (!ListeningStones) return false;

	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	FRandomStream Random(4242);
	const FVector Anchor = ListeningStones->GetActorLocation();
	for (int32 Attempt = 0; Attempt < 40; ++Attempt)
	{
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Distance = Random.FRandRange(900.f, 1500.f);
		const FVector Probe = Anchor + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandBasinGround), false);
		if (!World->LineTraceSingleByChannel(Hit, Probe + FVector(0.f, 0.f, 3000.f), Probe - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Query)) continue;
		// A basin wants flat, open ground that is not a roof, trunk, rock or resident.
		if (Hit.ImpactNormal.Z < 0.92f || Cast<APawn>(Hit.GetActor()) || Hit.ImpactPoint.Z > Anchor.Z + 600.f) continue;
		// Keep clear of props, walls and trunks: the rim is about 50 cm wide and must not clip anything.
		FCollisionQueryParams Clearance(SCENE_QUERY_STAT(IslandBasinClearance), false);
		Clearance.AddIgnoredActor(Hit.GetActor());
		if (World->OverlapAnyTestByChannel(Hit.ImpactPoint + FVector(0.f, 0.f, 95.f), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(70.f), Clearance)) continue;
		FNavLocation Walkable;
		if (Navigation && Navigation->GetDefaultNavDataInstance() &&
			(!Navigation->ProjectPointToNavigation(Hit.ImpactPoint, Walkable, FVector(80.f, 80.f, 150.f)) || FVector::Dist2D(Walkable.Location, Hit.ImpactPoint) > 80.f))
			continue;
		State.bPlaced = true;
		State.Location = Hit.ImpactPoint;
		UE_LOG(LogIslandRainBasin, Log, TEXT("Placed the rain basin at %s"), *State.Location.ToCompactString());
		return true;
	}
	UE_LOG(LogIslandRainBasin, Warning, TEXT("Could not find flat open ground for the rain basin near the ListeningStones; none was placed."));
	return false;
}

void UIslandRainBasinSubsystem::Refresh()
{
	UWorld* World = GetWorld();
	if (!World || !State.bPlaced) return;
	if (!Actor.IsValid())
	{
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Actor = World->SpawnActor<AIslandRainBasin>(State.Location + FVector(0.f, 0.f, AIslandRainBasin::OriginLift), FRotator::ZeroRotator, Spawn);
	}
	if (AIslandRainBasin* Basin = Actor.Get())
	{
		Basin->Show(State, UIslandWorldStateSubsystem::CurrentIslandDay(World));
		ShownWater = State.Water;
	}
}

void UIslandRainBasinSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Load();
	if (!State.bPlaced && PlaceBasin()) bDirty = true;
	Refresh();
	if (bDirty) Save();
}

void UIslandRainBasinSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	const UIslandEnvironmentSubsystem* Environment = World ? World->GetSubsystem<UIslandEnvironmentSubsystem>() : nullptr;
	if (!State.bPlaced || !Environment) return;
	const float Before = State.Water;
	State.Water = FIslandBasinState::Advance(State.Water, Environment->GetRainIntensity(), Environment->GetSunHeight(), DeltaTime);
	if (!FMath::IsNearlyEqual(State.Water, Before)) bDirty = true;
	SinceSave += DeltaTime;
	SinceShow += DeltaTime;
	if (SinceShow >= 1.f)
	{
		SinceShow = 0.f;
		if (FMath::Abs(State.Water - ShownWater) > 0.01f) Refresh();
	}
	if (bDirty && SinceSave >= 30.f) Save();
}

void UIslandRainBasinSubsystem::Deinitialize()
{
	if (bDirty) Save();
	if (AIslandRainBasin* Basin = Actor.Get()) Basin->Destroy();
	Actor.Reset();
	Super::Deinitialize();
}

static FAutoConsoleCommandWithWorldAndArgs GIslandBasinFillCommand(
	TEXT("Island.BasinFill"),
	TEXT("Island.BasinFill <0..1>: set the rain basin's water level (debug)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIslandRainBasinSubsystem* Basin = World ? World->GetSubsystem<UIslandRainBasinSubsystem>() : nullptr;
		if (Basin && Args.Num() > 0) Basin->ForceWater(FCString::Atof(*Args[0]));
	}));

static FAutoConsoleCommandWithWorldAndArgs GIslandBasinLeafCommand(
	TEXT("Island.BasinLeaf"),
	TEXT("Island.BasinLeaf [agent] [day]: set a leaf afloat as a debug visitor, optionally as of another Island day."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UIslandRainBasinSubsystem* Basin = World ? World->GetSubsystem<UIslandRainBasinSubsystem>() : nullptr;
		if (!Basin) return;
		FString Fact;
		const int32 Today = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : UIslandWorldStateSubsystem::CurrentIslandDay(World);
		Basin->Examine(Args.Num() > 0 ? Args[0] : TEXT("debug_visitor"), Today, Fact);
		UE_LOG(LogIslandRainBasin, Log, TEXT("Island.BasinLeaf: %s"), *Fact);
	}));
