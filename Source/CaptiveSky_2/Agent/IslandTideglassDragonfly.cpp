#include "IslandTideglassDragonfly.h"

#include "IslandCommonFrog.h"
#include "IslandListeningStonesChime.h"
#include "IslandPoolRippleEffect.h"
#include "IslandRainBasin.h"
#include "IslandWeather.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Logging/LogMacros.h"
#include "ProceduralMeshComponent.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	DEFINE_LOG_CATEGORY_STATIC(LogIslandTideglassDragonfly, Log, All);

	TAutoConsoleVariable<int32> CVarDragonflyFlowerTelemetry(
		TEXT("CaptiveSky.Island.DragonflyFlowerTelemetry"),
		0,
		TEXT("When non-zero, logs the start, completion, and interruption of dragonfly flower visits."));

	constexpr float FlowerInterestDurationSeconds = 8.0f;
	constexpr float FlowerInterestFadeStartSeconds = 6.8f;
	// Stay close to a bloom instead of orbiting a full metre above its instance pivot.
	constexpr float FlowerHoverHeight = 70.f;
	constexpr float BasinInterestDurationSeconds = 6.5f;
	constexpr float BasinInterestFadeStartSeconds = 5.4f;

	void BuildWingSurface(UProceduralMeshComponent* Wing, float Side, float Sweep, float Length, float MaximumHalfWidth,
		const FLinearColor& WingColor)
	{
		if (!Wing) return;

		constexpr int32 LengthSegments = 10;
		constexpr int32 WidthSegments = 4;
		constexpr int32 SurfaceVertexCount = (LengthSegments + 1) * (WidthSegments + 1);
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;
		Vertices.Reserve(SurfaceVertexCount * 2);
		Normals.Reserve(SurfaceVertexCount * 2);
		UVs.Reserve(SurfaceVertexCount * 2);
		Colors.Reserve(SurfaceVertexCount * 2);
		Tangents.Reserve(SurfaceVertexCount * 2);
		Triangles.Reserve(LengthSegments * WidthSegments * 12);

		for (int32 Surface = 0; Surface < 2; ++Surface)
		{
			for (int32 LengthIndex = 0; LengthIndex <= LengthSegments; ++LengthIndex)
			{
				const float Along = static_cast<float>(LengthIndex) / LengthSegments;
				const float Arch = FMath::Max(0.f, FMath::Sin(Along * PI));
				const float CenterX = Sweep * Arch + 1.5f * Along;
				const float HalfWidth = MaximumHalfWidth * FMath::Pow(Arch, 0.72f);
				for (int32 WidthIndex = 0; WidthIndex <= WidthSegments; ++WidthIndex)
				{
					const float Across = -1.f + 2.f * WidthIndex / WidthSegments;
					Vertices.Emplace(CenterX + Across * HalfWidth, Side * Along * Length,
						Surface == 0 ? 0.35f * Arch : -0.35f * Arch);
					Normals.Emplace(0.f, 0.f, Surface == 0 ? 1.f : -1.f);
					UVs.Emplace(Along, (Across + 1.f) * 0.5f);
					Colors.Emplace(WingColor);
					Tangents.Emplace(FVector(1.f, 0.f, 0.f), false);
				}
			}

			const int32 SurfaceOffset = Surface * SurfaceVertexCount;
			for (int32 LengthIndex = 0; LengthIndex < LengthSegments; ++LengthIndex)
			{
				for (int32 WidthIndex = 0; WidthIndex < WidthSegments; ++WidthIndex)
				{
					const int32 A = SurfaceOffset + LengthIndex * (WidthSegments + 1) + WidthIndex;
					const int32 B = A + WidthSegments + 1;
					const int32 C = B + 1;
					const int32 D = A + 1;
					const bool bFaceUp = (Side > 0.f) == (Surface == 0);
					if (bFaceUp)
					{
						Triangles.Add(A);
						Triangles.Add(C);
						Triangles.Add(B);
						Triangles.Add(A);
						Triangles.Add(D);
						Triangles.Add(C);
					}
					else
					{
						Triangles.Add(A);
						Triangles.Add(B);
						Triangles.Add(C);
						Triangles.Add(A);
						Triangles.Add(C);
						Triangles.Add(D);
					}
				}
			}
		}

		Wing->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

		// The membrane alone reads like a blank leaf. Add a light, two-sided vein
		// lattice as a second translucent section, keeping the silhouette and
		// collision-free single-component setup unchanged.
		TArray<FVector> VeinVertices;
		TArray<int32> VeinTriangles;
		TArray<FVector> VeinNormals;
		TArray<FVector2D> VeinUVs;
		TArray<FLinearColor> VeinColors;
		TArray<FProcMeshTangent> VeinTangents;
		const FLinearColor VeinColor(WingColor.R * 0.34f, WingColor.G * 0.34f, WingColor.B * 0.34f, 0.88f);
		const auto WingPoint = [Side, Sweep, Length, MaximumHalfWidth](float Along, float Across)
		{
			const float Arch = FMath::Max(0.f, FMath::Sin(Along * PI));
			const float CenterX = Sweep * Arch + 1.5f * Along;
			const float HalfWidth = MaximumHalfWidth * FMath::Pow(Arch, 0.72f);
			return FVector(CenterX + Across * HalfWidth, Side * Along * Length, 0.35f * Arch);
		};
		const auto AppendVeinSegment = [&VeinVertices, &VeinTriangles, &VeinNormals, &VeinUVs,
			&VeinColors, &VeinTangents, &VeinColor](const FVector& Start, const FVector& End, float Width)
		{
			const FVector2D Direction(End.X - Start.X, End.Y - Start.Y);
			const FVector2D SafeDirection = Direction.GetSafeNormal();
			if (SafeDirection.IsNearlyZero()) return;
			const FVector2D Perpendicular(-SafeDirection.Y, SafeDirection.X);
			for (int32 Surface = 0; Surface < 2; ++Surface)
			{
				const float StartHeight = Surface == 0 ? Start.Z + 0.06f : -Start.Z - 0.06f;
				const float EndHeight = Surface == 0 ? End.Z + 0.06f : -End.Z - 0.06f;
				const int32 Base = VeinVertices.Num();
				VeinVertices.Emplace(Start.X - Perpendicular.X * Width, Start.Y - Perpendicular.Y * Width, StartHeight);
				VeinVertices.Emplace(Start.X + Perpendicular.X * Width, Start.Y + Perpendicular.Y * Width, StartHeight);
				VeinVertices.Emplace(End.X + Perpendicular.X * Width, End.Y + Perpendicular.Y * Width, EndHeight);
				VeinVertices.Emplace(End.X - Perpendicular.X * Width, End.Y - Perpendicular.Y * Width, EndHeight);
				const bool bFaceUp = Surface == 0;
				if (bFaceUp)
				{
					VeinTriangles.Add(Base);
					VeinTriangles.Add(Base + 2);
					VeinTriangles.Add(Base + 1);
					VeinTriangles.Add(Base);
					VeinTriangles.Add(Base + 3);
					VeinTriangles.Add(Base + 2);
				}
				else
				{
					VeinTriangles.Add(Base);
					VeinTriangles.Add(Base + 1);
					VeinTriangles.Add(Base + 2);
					VeinTriangles.Add(Base);
					VeinTriangles.Add(Base + 2);
					VeinTriangles.Add(Base + 3);
				}
				const FVector Normal(0.f, 0.f, bFaceUp ? 1.f : -1.f);
				for (int32 Vertex = 0; Vertex < 4; ++Vertex)
				{
					VeinNormals.Add(Normal);
					VeinUVs.Emplace(Vertex == 0 || Vertex == 3 ? 0.f : 1.f, Vertex < 2 ? 0.f : 1.f);
					VeinColors.Add(VeinColor);
					VeinTangents.Emplace(FVector(1.f, 0.f, 0.f), false);
				}
			}
		};

		constexpr float VeinHalfWidth = 0.095f;
		constexpr int32 VeinLengthSegments = 14;
		const float LongitudinalAcross[] = { 0.68f, 0.12f, -0.48f };
		for (const float Across : LongitudinalAcross)
		{
			for (int32 Segment = 0; Segment < VeinLengthSegments; ++Segment)
			{
				const float StartAlong = 0.06f + 0.88f * Segment / VeinLengthSegments;
				const float EndAlong = 0.06f + 0.88f * (Segment + 1) / VeinLengthSegments;
				const float StartAcross = Across + 0.10f * StartAlong;
				const float EndAcross = Across + 0.10f * EndAlong;
				AppendVeinSegment(WingPoint(StartAlong, StartAcross), WingPoint(EndAlong, EndAcross), VeinHalfWidth);
			}
		}
		for (int32 CrossVein = 0; CrossVein < 9; ++CrossVein)
		{
			const float Along = 0.16f + 0.075f * CrossVein;
			AppendVeinSegment(WingPoint(Along, -0.68f), WingPoint(Along, 0.76f), VeinHalfWidth * 0.72f);
		}
		Wing->CreateMeshSection_LinearColor(1, VeinVertices, VeinTriangles, VeinNormals,
			VeinUVs, VeinColors, VeinTangents, false);
	}

	const FLinearColor DragonflyBodyColors[] = {
		FLinearColor(0.13f, 0.26f, 0.12f), // moss green
		FLinearColor(0.10f, 0.24f, 0.28f), // blue-green
		FLinearColor(0.34f, 0.19f, 0.08f)  // copper
	};
	const FLinearColor DragonflyWingColors[] = {
		FLinearColor(0.18f, 0.30f, 0.20f, 0.44f),
		FLinearColor(0.18f, 0.31f, 0.36f, 0.44f),
		FLinearColor(0.36f, 0.24f, 0.12f, 0.44f)
	};
}

AIslandTideglassDragonfly::AIslandTideglassDragonfly()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.06f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("TideglassDragonfly"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentWingMaterial(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	BaseMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;
	WingBaseMaterial = TranslucentWingMaterial.Succeeded() ? TranslucentWingMaterial.Object : BaseMaterial;
	UStaticMesh* SphereMesh = Sphere.Succeeded() ? Sphere.Object : nullptr;
	auto MakeBodyPart = [this, SphereMesh](const TCHAR* Name, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(FName(Name));
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(SphereMesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	};
	Head = MakeBodyPart(TEXT("Head"), FVector(8.f, 0.f, 0.f), FVector(0.055f, 0.052f, 0.052f));
	Thorax = MakeBodyPart(TEXT("Thorax"), FVector(0.f, 0.f, 0.f), FVector(0.07f, 0.055f, 0.055f));
	Abdomen = MakeBodyPart(TEXT("Abdomen"), FVector(-16.f, 0.f, -0.2f), FVector(0.31f, 0.013f, 0.014f));
	const FVector SegmentLocations[] = {
		FVector(-3.0f, 0.0f, 0.0f), FVector(-7.3f, 0.2f, -0.1f), FVector(-11.6f, 0.35f, -0.2f),
		FVector(-15.9f, 0.35f, -0.35f), FVector(-20.2f, 0.15f, -0.55f), FVector(-24.5f, -0.25f, -0.85f),
		FVector(-28.7f, -0.7f, -1.15f)
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SegmentLocations); ++Index)
	{
		const FVector SegmentScale = Index == UE_ARRAY_COUNT(SegmentLocations) - 1
			? FVector(0.035f, 0.018f, 0.018f)
			: FVector(0.050f, 0.023f - 0.0005f * Index, 0.024f - 0.00055f * Index);
		const FString SegmentName = FString::Printf(TEXT("AbdomenSegment_%02d"), Index);
		AbdomenSegments.Add(MakeBodyPart(*SegmentName, SegmentLocations[Index], SegmentScale));
	}
	Eyes.Add(MakeBodyPart(TEXT("Eye_Left"), FVector(9.3f, -3.0f, 1.4f), FVector(0.032f, 0.032f, 0.032f)));
	Eyes.Add(MakeBodyPart(TEXT("Eye_Right"), FVector(9.3f, 3.0f, 1.4f), FVector(0.032f, 0.032f, 0.032f)));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const bool bLeft = Index % 2 == 0;
		UProceduralMeshComponent* Wing = CreateDefaultSubobject<UProceduralMeshComponent>(
			FName(*FString::Printf(TEXT("Wing_%d"), Index)));
		Wing->SetupAttachment(RootComponent);
		Wing->SetRelativeLocation(FVector(Index < 2 ? 2.f : -4.f, bLeft ? -5.f : 5.f, 1.7f));
		Wing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wing->SetCanEverAffectNavigation(false);
		Wing->SetCastShadow(false);
		Wing->SetRelativeRotation(FRotator(0.f, bLeft ? -12.f : 12.f, 0.f));
		Wings.Add(Wing);
	}
}

void AIslandTideglassDragonfly::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureAppearance();
}

void AIslandTideglassDragonfly::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	Phase = FMath::FRandRange(0.f, 2.f * PI);
	MotionRate = FMath::FRandRange(0.8f, 1.2f);
	WingPhase = FMath::FRandRange(0.f, 2.f * PI);
	FrogCheckRemaining = FMath::FRandRange(0.25f, 0.8f);
	FlowerCheckRemaining = FMath::FRandRange(0.8f, 2.2f);
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	ConfigureAppearance();
}

void AIslandTideglassDragonfly::SetColorVariant(int32 Variant)
{
	ColorVariant = ((Variant % UE_ARRAY_COUNT(DragonflyBodyColors)) + UE_ARRAY_COUNT(DragonflyBodyColors)) % UE_ARRAY_COUNT(DragonflyBodyColors);
	for (UProceduralMeshComponent* Wing : Wings)
		if (Wing) Wing->ClearAllMeshSections();
	ConfigureAppearance();
}

void AIslandTideglassDragonfly::ConfigureAppearance()
{
	if (!BaseMaterial) return;
	if (!BodyMaterial) BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), DragonflyBodyColors[ColorVariant]);
		if (Head) Head->SetMaterial(0, BodyMaterial);
		if (Thorax) Thorax->SetMaterial(0, BodyMaterial);
		if (Abdomen) Abdomen->SetMaterial(0, BodyMaterial);
	}
	AbdomenSegmentMaterials.SetNum(AbdomenSegments.Num());
	for (int32 Index = 0; Index < AbdomenSegments.Num(); ++Index)
	{
		if (!AbdomenSegmentMaterials[Index])
			AbdomenSegmentMaterials[Index] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		UMaterialInstanceDynamic* SegmentMaterial = AbdomenSegmentMaterials[Index];
		if (!SegmentMaterial || !AbdomenSegments[Index]) continue;
		const float Tone = Index == AbdomenSegments.Num() - 1
			? 0.52f : (Index % 2 == 0 ? 0.62f : 0.90f);
		SegmentMaterial->SetVectorParameterValue(TEXT("Color"), DragonflyBodyColors[ColorVariant] * Tone);
		AbdomenSegments[Index]->SetMaterial(0, SegmentMaterial);
	}
	if (!EyeMaterial) EyeMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (EyeMaterial)
	{
		EyeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.012f, 0.024f, 0.018f));
		for (UStaticMeshComponent* Eye : Eyes)
			if (Eye) Eye->SetMaterial(0, EyeMaterial);
	}
	WingMaterials.SetNum(Wings.Num());
	WingVeinMaterials.SetNum(Wings.Num());
	for (int32 Index = 0; Index < Wings.Num(); ++Index)
	{
		if (Wings[Index] && Wings[Index]->GetNumSections() == 0)
		{
			const bool bForewing = Index < 2;
			BuildWingSurface(Wings[Index], Index % 2 == 0 ? -1.f : 1.f,
				bForewing ? 1.7f : -1.7f,
				bForewing ? 33.f : 39.f,
				bForewing ? 3.f : 4.f,
				DragonflyWingColors[ColorVariant]);
		}
		if (!WingMaterials[Index]) WingMaterials[Index] = UMaterialInstanceDynamic::Create(WingBaseMaterial, this);
		if (!WingMaterials[Index]) continue;
		WingMaterials[Index]->SetVectorParameterValue(TEXT("Color"), DragonflyWingColors[ColorVariant]);
		Wings[Index]->SetMaterial(0, WingMaterials[Index]);
		if (!WingVeinMaterials[Index]) WingVeinMaterials[Index] = UMaterialInstanceDynamic::Create(WingBaseMaterial, this);
		if (WingVeinMaterials[Index])
		{
			const FLinearColor MembraneColor = DragonflyWingColors[ColorVariant];
			WingVeinMaterials[Index]->SetVectorParameterValue(TEXT("Color"), FLinearColor(
				MembraneColor.R * 0.34f, MembraneColor.G * 0.34f, MembraneColor.B * 0.34f, 0.88f));
			Wings[Index]->SetMaterial(1, WingVeinMaterials[Index]);
		}
	}
}

FVector AIslandTideglassDragonfly::WindDisplacement(const FVector& LocalWind)
{
	return LocalWind.GetClampedToMaxSize(300.f) * 0.18f;
}

float AIslandTideglassDragonfly::RainMovementScale(float RainIntensity)
{
	const float RainActivity = FMath::SmoothStep(0.35f, 0.85f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	return FMath::Lerp(1.f, 0.3f, RainActivity);
}

float AIslandTideglassDragonfly::WindMovementScale(float WindSpeedCmPerSecond)
{
	if (!FMath::IsFinite(WindSpeedCmPerSecond)) return 1.f;
	const float WindActivity = FMath::SmoothStep(25.f, 180.f, FMath::Clamp(WindSpeedCmPerSecond, 0.f, 300.f));
	// Dragonflies keep flying through a gust, but their own patrol and wingbeat
	// soften as wind rises; local wind drift remains visible independently.
	return FMath::Lerp(1.f, 0.55f, WindActivity);
}

bool AIslandTideglassDragonfly::ShouldScatterFromHoppingFrog(bool bFrogHopping, float DistanceCm, float VerticalOffsetCm)
{
	constexpr float StartleRadiusCm = 230.f;
	constexpr float MaximumVerticalSeparationCm = 250.f;
	return bFrogHopping && FMath::IsFinite(DistanceCm) && DistanceCm >= 0.f && DistanceCm <= StartleRadiusCm &&
		FMath::IsFinite(VerticalOffsetCm) && FMath::Abs(VerticalOffsetCm) <= MaximumVerticalSeparationCm;
}

void AIslandTideglassDragonfly::RespondToQuietObservation(const FVector& ObserverLocation)
{
	if (FlowerInterestRemaining > 0.f)
		LogFlowerInterestTelemetry(TEXT("interrupted: quiet observation"));
	ScatterDirection = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (ScatterDirection.IsNearlyZero()) ScatterDirection = GetActorRightVector();
	ScatterRemaining = 1.6f;
	FlowerInterestRemaining = 0.f;
	BasinInterestRemaining = 0.f;
	ListeningStonesInterestRemaining = 0.f;
}

void AIslandTideglassDragonfly::CheckForLowRavenFlyby()
{
	if (!GetWorld() || ScatterRemaining > 0.f || RavenFlybyCooldownRemaining > 0.f) return;

	constexpr float FlybyRadius = 425.f;
	constexpr float MinimumHeight = 120.f;
	constexpr float MaximumHeight = 480.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		if (It->LocomotionState != ERavenLocomotionState::Flying) continue;
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		if (Offset.Z < MinimumHeight || Offset.Z > MaximumHeight || Offset.SizeSquared2D() > FMath::Square(FlybyRadius))
			continue;

		// A close, low wing shadow sends the dragonfly briefly sideways; it resumes its
		// independent pool-side patrol without pursuing, attacking, or persisting a change.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTideglassDragonfly::CheckForQuietPlayerObservation()
{
	if (!GetWorld()) return;

	constexpr float NoticeRadiusCm = 220.f;
	constexpr float RearmRadiusCm = 480.f;
	constexpr float MaximumVerticalSeparationCm = 220.f;
	constexpr float QuietMovementSpeedCmPerSecond = 110.f;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		APawn* Player = It->GetPawn();
		if (!IsValid(Player)) continue;

		const FVector Offset = Player->GetActorLocation() - GetActorLocation();
		const float DistanceSquared = Offset.SizeSquared2D();
		if (PlayerObservationLatch.Get() == Player && DistanceSquared > FMath::Square(RearmRadiusCm))
			PlayerObservationLatch.Reset();

		if (DistanceSquared > FMath::Square(NoticeRadiusCm) || FMath::Abs(Offset.Z) > MaximumVerticalSeparationCm ||
			Player->GetVelocity().SizeSquared2D() > FMath::Square(QuietMovementSpeedCmPerSecond))
			continue;

		// Only a close, unhurried visitor gets a brief flutter. Keep one response
		// per approach so standing nearby does not continually interrupt local life.
		if (PlayerObservationLatch.Get() == Player) continue;
		PlayerObservationLatch = Player;
		if (PlayerObservationCooldownRemaining > 0.f || ScatterRemaining > 0.f) return;

		RespondToQuietObservation(Player->GetActorLocation());
		PlayerObservationCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTideglassDragonfly::CheckForNearbyHoppingFrog()
{
	if (!GetWorld() || FrogScatterCooldownRemaining > 0.f || ScatterRemaining > 0.f ||
		FlowerInterestRemaining > 0.f || BasinInterestRemaining > 0.f || RippleInterestRemaining > 0.f)
		return;

	constexpr float RearmRadiusCm = 360.f;
	if (FrogPresenceLatch.IsValid())
	{
		const FVector Offset = FrogPresenceLatch->GetActorLocation() - GetActorLocation();
		if (!FrogPresenceLatch->IsHopping() || Offset.Size2D() > RearmRadiusCm || FMath::Abs(Offset.Z) > 300.f)
			FrogPresenceLatch.Reset();
	}

	for (TActorIterator<AIslandCommonFrog> It(GetWorld()); It; ++It)
	{
		AIslandCommonFrog* Frog = *It;
		if (!IsValid(Frog) || FrogPresenceLatch.Get() == Frog) continue;
		const FVector Offset = Frog->GetActorLocation() - GetActorLocation();
		if (!ShouldScatterFromHoppingFrog(Frog->IsHopping(), Offset.Size2D(), Offset.Z)) continue;

		// A near hop sends the dragonfly a short distance away. It never attacks,
		// chases, writes world state, or interrupts a more deliberate local cue.
		RespondToQuietObservation(Frog->GetActorLocation());
		FrogPresenceLatch = Frog;
		FrogScatterCooldownRemaining = 5.f;
		return;
	}
}

bool AIslandTideglassDragonfly::RespondToSurfaceRipple(const FVector& RippleLocation)
{
	if (RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f ||
		FVector::DistSquared2D(GetActorLocation(), RippleLocation) > FMath::Square(250.f) ||
		FMath::Abs(GetActorLocation().Z - RippleLocation.Z) > 300.f)
		return false;

	// Give each color morph its own small station around the disturbance so the
	// whole daytime group does not converge on one point. This remains a brief
	// hover above the water, not a landing or a persistent change.
	constexpr float HoverRadius = 42.f;
	const float HoverAngle = 2.f * PI * static_cast<float>(ColorVariant) / 3.f;
	RippleInterestLocation = RippleLocation + FVector(
		FMath::Cos(HoverAngle) * HoverRadius,
		FMath::Sin(HoverAngle) * HoverRadius,
		110.f);
	RippleInterestRemaining = 1.8f;
	RippleInterestCooldownRemaining = 7.f;
	ListeningStonesInterestRemaining = 0.f;
	if (FlowerInterestRemaining > 0.f)
		LogFlowerInterestTelemetry(TEXT("interrupted: water ripple"));
	FlowerInterestRemaining = 0.f;
	BasinInterestRemaining = 0.f;
	return true;
}

void AIslandTideglassDragonfly::CheckForNearbyMeadowFlower()
{
	if (!Weather.IsValid() || !GetWorld()) return;

	const float RainIntensity = Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds());
	if (FlowerInterestRemaining > 0.f)
	{
		if (RainIntensity > 0.45f)
		{
			LogFlowerInterestTelemetry(TEXT("interrupted: heavy rain"));
			FlowerInterestRemaining = 0.f;
		}
		return;
	}

	if (FlowerInterestCooldownRemaining > 0.f || BasinInterestRemaining > 0.f || BasinInterestCooldownRemaining > 0.f || ScatterRemaining > 0.f ||
		RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f || ListeningStonesInterestRemaining > 0.f || RainIntensity > 0.45f)
		return;

	FVector FlowerLocation;
	if (Weather->FindNearestMeadowFlower(GetActorLocation(), 900.f, FlowerLocation))
		RespondToMeadowFlower(FlowerLocation);
}

bool AIslandTideglassDragonfly::RespondToMeadowFlower(const FVector& FlowerLocation)
{
	if (FlowerLocation.ContainsNaN() || FlowerInterestRemaining > 0.f || FlowerInterestCooldownRemaining > 0.f ||
		BasinInterestRemaining > 0.f || BasinInterestCooldownRemaining > 0.f || ScatterRemaining > 0.f ||
		RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f || ListeningStonesInterestRemaining > 0.f ||
		FVector::DistSquared(GetActorLocation(), FlowerLocation) > FMath::Square(900.f))
		return false;

	// This is a brief hover above an existing bloom, not a landing or plant interaction.
	// Color morphs take slightly different stations so the group does not stack.
	constexpr float HoverRadius = 32.f;
	const float HoverAngle = 2.f * PI * static_cast<float>(ColorVariant) / 3.f;
	FlowerInterestLocation = FlowerLocation + FVector(
		FMath::Cos(HoverAngle) * HoverRadius,
		FMath::Sin(HoverAngle) * HoverRadius,
		FlowerHoverHeight);
	FlowerInterestRemaining = FlowerInterestDurationSeconds;
	FlowerInterestCooldownRemaining = 16.f;
	LogFlowerInterestTelemetry(TEXT("started"));
	return true;
}

void AIslandTideglassDragonfly::LogFlowerInterestTelemetry(const TCHAR* Event) const
{
	if (CVarDragonflyFlowerTelemetry.GetValueOnGameThread() == 0 || !GetWorld()) return;
	const float Elapsed = FlowerInterestDurationSeconds - FlowerInterestRemaining;
	UE_LOG(LogIslandTideglassDragonfly, Display,
		TEXT("FlowerVisit %s dragonfly=%s variant=%d gameTime=%.2f elapsed=%.2f flower=%s actor=%s"),
		Event, *GetName(), ColorVariant, GetWorld()->GetTimeSeconds(), Elapsed,
		*FlowerInterestLocation.ToCompactString(), *GetActorLocation().ToCompactString());
}

float AIslandTideglassDragonfly::GetFlowerInterestAlpha() const
{
	if (FlowerInterestRemaining <= 0.f) return 0.f;
	const float Elapsed = FlowerInterestDurationSeconds - FlowerInterestRemaining;
	return FMath::SmoothStep(0.f, 0.65f, Elapsed) *
		(1.f - FMath::SmoothStep(FlowerInterestFadeStartSeconds, FlowerInterestDurationSeconds, Elapsed));
}

void AIslandTideglassDragonfly::CheckForNearbyRainBasin()
{
	if (!Weather.IsValid() || !GetWorld()) return;

	const float RainIntensity = Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds());
	if (BasinInterestRemaining > 0.f)
	{
		if (RainIntensity > 0.45f)
			BasinInterestRemaining = 0.f;
		return;
	}
	if (BasinInterestCooldownRemaining > 0.f || FlowerInterestRemaining > 0.f ||
		FlowerInterestCooldownRemaining > 0.f || ScatterRemaining > 0.f || ListeningStonesInterestRemaining > 0.f ||
		RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f || RainIntensity > 0.45f)
		return;

	const UIslandRainBasinSubsystem* Basin = GetWorld()->GetSubsystem<UIslandRainBasinSubsystem>();
	if (!Basin) return;
	const FIslandBasinState& State = Basin->GetState();
	if (!State.bPlaced || State.Water < FIslandBasinState::FloatLevel) return;
	if (FVector::DistSquared2D(GetActorLocation(), State.Location) > FMath::Square(1200.f) ||
		FMath::Abs(GetActorLocation().Z - State.Location.Z) > 450.f)
		return;

	RespondToRainBasin(State.Location, State.Water);
}

bool AIslandTideglassDragonfly::RespondToRainBasin(const FVector& BasinLocation, float WaterLevel)
{
	if (BasinLocation.ContainsNaN() || WaterLevel < FIslandBasinState::FloatLevel ||
		BasinInterestRemaining > 0.f || BasinInterestCooldownRemaining > 0.f ||
		FlowerInterestRemaining > 0.f || FlowerInterestCooldownRemaining > 0.f || ScatterRemaining > 0.f ||
		RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f || ListeningStonesInterestRemaining > 0.f ||
		FVector::DistSquared2D(GetActorLocation(), BasinLocation) > FMath::Square(1200.f) ||
		FMath::Abs(GetActorLocation().Z - BasinLocation.Z) > 450.f)
		return false;
	if (Weather.IsValid() && GetWorld() && Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds()) > 0.45f)
		return false;

	// Circle just outside the stone rim, above the actual water surface. This is
	// a temporary look, not a landing or a change to the basin or its leaves.
	constexpr float StationRadius = AIslandRainBasin::RimRadius + 12.f;
	constexpr float HoverHeight = 78.f;
	const float HoverAngle = 2.f * PI * static_cast<float>(ColorVariant) / 3.f;
	const float SurfaceHeight = AIslandRainBasin::FloorThickness + AIslandRainBasin::WaterDepth(WaterLevel);
	BasinInterestLocation = BasinLocation + FVector(
		FMath::Cos(HoverAngle) * StationRadius,
		FMath::Sin(HoverAngle) * StationRadius,
		SurfaceHeight + HoverHeight);
	BasinInterestRemaining = BasinInterestDurationSeconds;
	BasinInterestCooldownRemaining = 22.f;
	return true;
}

float AIslandTideglassDragonfly::GetBasinInterestAlpha() const
{
	if (BasinInterestRemaining <= 0.f) return 0.f;
	const float Elapsed = BasinInterestDurationSeconds - BasinInterestRemaining;
	return FMath::SmoothStep(0.f, 0.65f, Elapsed) *
		(1.f - FMath::SmoothStep(BasinInterestFadeStartSeconds, BasinInterestDurationSeconds, Elapsed));
}

void AIslandTideglassDragonfly::CheckForNearbyListeningStonesChime()
{
	if (!GetWorld()) return;
	if (!LastHeardListeningStonesChime.IsValid() ||
		!LastHeardListeningStonesChime->IsAudibleAt(GetActorLocation()))
	{
		LastHeardListeningStonesChime.Reset();
	}
	if (ListeningStonesInterestRemaining > 0.f || ListeningStonesInterestCooldownRemaining > 0.f ||
		ScatterRemaining > 0.f || FlowerInterestRemaining > 0.f || BasinInterestRemaining > 0.f ||
		RippleInterestRemaining > 0.f || (Weather.IsValid() && Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds()) > 0.45f))
		return;

	for (TActorIterator<AIslandListeningStonesChime> It(GetWorld()); It; ++It)
	{
		if (RespondToListeningStonesChime(*It)) return;
	}
}

bool AIslandTideglassDragonfly::RespondToListeningStonesChime(AIslandListeningStonesChime* Chime)
{
	if (!IsValid(Chime) || Chime->IsActorBeingDestroyed() || !Chime->IsAudibleAt(GetActorLocation()) ||
		LastHeardListeningStonesChime.Get() == Chime || ListeningStonesInterestRemaining > 0.f ||
		ListeningStonesInterestCooldownRemaining > 0.f || ScatterRemaining > 0.f ||
		FlowerInterestRemaining > 0.f || BasinInterestRemaining > 0.f || RippleInterestRemaining > 0.f)
		return false;

	// Different color morphs take separate, quiet stations just above the stones.
	// The dragonfly resumes its patrol when the short interest fades; no landmark
	// state, memory, or persistent world data is changed.
	const float StationAngle = 2.f * PI * static_cast<float>(FMath::Clamp(ColorVariant, 0, 2)) / 3.f;
	constexpr float StationRadiusCm = 58.f;
	ListeningStonesInterestLocation = Chime->GetActorLocation() + FVector(
		FMath::Cos(StationAngle) * StationRadiusCm,
		FMath::Sin(StationAngle) * StationRadiusCm,
		125.f);
	LastHeardListeningStonesChime = Chime;
	ListeningStonesInterestRemaining = ListeningStonesInterestDurationSeconds;
	ListeningStonesInterestCooldownRemaining = 18.f;
	return true;
}

float AIslandTideglassDragonfly::GetListeningStonesInterestAlpha() const
{
	if (ListeningStonesInterestRemaining <= 0.f) return 0.f;
	const float Elapsed = ListeningStonesInterestDurationSeconds - ListeningStonesInterestRemaining;
	return FMath::SmoothStep(0.f, 0.55f, Elapsed) *
		(1.f - FMath::SmoothStep(1.8f, ListeningStonesInterestDurationSeconds, Elapsed));
}

void AIslandTideglassDragonfly::CheckForNearbySurfaceCue()
{
	if (!GetWorld() || RippleInterestRemaining > 0.f || RippleInterestCooldownRemaining > 0.f) return;

	for (TActorIterator<AIslandPoolRippleEffect> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("WindImpact")) && !It->ActorHasTag(TEXT("RainImpact")) &&
			!It->ActorHasTag(TEXT("MinnowImpact")) && !It->ActorHasTag(TEXT("PlayerSwimImpact"))) continue;
		if (RespondToSurfaceRipple(It->GetActorLocation())) return;
	}
}

float AIslandTideglassDragonfly::GetRippleInterestAlpha() const
{
	if (RippleInterestRemaining <= 0.f) return 0.f;
	const float Elapsed = 1.8f - RippleInterestRemaining;
	return FMath::SmoothStep(0.f, 0.45f, Elapsed) * (1.f - FMath::SmoothStep(1.2f, 1.8f, Elapsed));
}

FVector AIslandTideglassDragonfly::ResolveFlightPath(const FVector& Start, const FVector& Desired) const
{
	UWorld* World = GetWorld();
	if (!World || Start.Equals(Desired)) return Desired;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(5.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TideglassDragonflyFlight), false, this);
	FHitResult Hit;
	if (!World->SweepSingleByChannel(Hit, Start, Desired, FQuat::Identity, ECC_WorldStatic, Shape, Query)) return Desired;
	if (Hit.bStartPenetrating) return Start;
	const FVector Contact = Hit.Location + Hit.Normal * 2.f;
	const FVector Remaining = Desired - Contact;
	const FVector Slide = Remaining - Hit.Normal * FVector::DotProduct(Remaining, Hit.Normal);
	if (Slide.IsNearlyZero()) return Contact;
	FHitResult SlideHit;
	if (World->SweepSingleByChannel(SlideHit, Contact, Contact + Slide, FQuat::Identity, ECC_WorldStatic, Shape, Query))
		return SlideHit.bStartPenetrating ? Contact : SlideHit.Location;
	return Contact + Slide;
}

void AIslandTideglassDragonfly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()) return;
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	const float Time = GetWorld()->GetTimeSeconds();
	ScatterRemaining = FMath::Max(0.f, ScatterRemaining - SafeDelta);
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - SafeDelta);
	PlayerObservationCooldownRemaining = FMath::Max(0.f, PlayerObservationCooldownRemaining - SafeDelta);
	FrogScatterCooldownRemaining = FMath::Max(0.f, FrogScatterCooldownRemaining - SafeDelta);
	RippleInterestRemaining = FMath::Max(0.f, RippleInterestRemaining - SafeDelta);
	RippleInterestCooldownRemaining = FMath::Max(0.f, RippleInterestCooldownRemaining - SafeDelta);
	if (FlowerInterestRemaining > 0.f)
	{
		FlowerInterestRemaining = FMath::Max(0.f, FlowerInterestRemaining - SafeDelta);
		if (FlowerInterestRemaining <= 0.f)
			LogFlowerInterestTelemetry(TEXT("completed"));
	}
	FlowerInterestCooldownRemaining = FMath::Max(0.f, FlowerInterestCooldownRemaining - SafeDelta);
	BasinInterestRemaining = FMath::Max(0.f, BasinInterestRemaining - SafeDelta);
	BasinInterestCooldownRemaining = FMath::Max(0.f, BasinInterestCooldownRemaining - SafeDelta);
	ListeningStonesInterestRemaining = FMath::Max(0.f, ListeningStonesInterestRemaining - SafeDelta);
	ListeningStonesInterestCooldownRemaining = FMath::Max(0.f, ListeningStonesInterestCooldownRemaining - SafeDelta);
	RavenCheckRemaining -= SafeDelta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.35f;
		CheckForLowRavenFlyby();
	}
	PlayerCheckRemaining -= SafeDelta;
	if (PlayerCheckRemaining <= 0.f)
	{
		PlayerCheckRemaining = 0.45f;
		CheckForQuietPlayerObservation();
	}
	FrogCheckRemaining -= SafeDelta;
	if (FrogCheckRemaining <= 0.f)
	{
		FrogCheckRemaining = 0.45f;
		CheckForNearbyHoppingFrog();
	}
	RippleCheckRemaining -= SafeDelta;
	if (RippleCheckRemaining <= 0.f)
	{
		RippleCheckRemaining = 0.35f;
		CheckForNearbySurfaceCue();
	}
	FlowerCheckRemaining -= SafeDelta;
	if (FlowerCheckRemaining <= 0.f)
	{
		FlowerCheckRemaining = 1.2f;
		CheckForNearbyMeadowFlower();
	}
	BasinCheckRemaining -= SafeDelta;
	if (BasinCheckRemaining <= 0.f)
	{
		BasinCheckRemaining = 1.6f;
		CheckForNearbyRainBasin();
	}
	ListeningStonesCheckRemaining -= SafeDelta;
	if (ListeningStonesCheckRemaining <= 0.f)
	{
		ListeningStonesCheckRemaining = 1.1f;
		CheckForNearbyListeningStonesChime();
	}
	const float Rain = Weather.IsValid() ? Weather->SampleRainIntensity(Time) : 0.f;
	const FVector LocalWind = Weather.IsValid() ? Weather->GetLocalWind(GetActorLocation(), this) : FVector::ZeroVector;
	const float MoveScale = RainMovementScale(Rain) * WindMovementScale(LocalWind.Size());
	const float MotionTime = Time * MotionRate;
	const float ScatterAlpha = FMath::SmoothStep(0.f, 1.2f, ScatterRemaining);
	const FVector Offset(
		(190.f * FMath::Sin(MotionTime * 0.38f + Phase) + ScatterDirection.X * 270.f * ScatterAlpha) * MoveScale,
		(150.f * FMath::Sin(MotionTime * 0.29f + Phase * 1.7f) + ScatterDirection.Y * 270.f * ScatterAlpha) * MoveScale,
		110.f + 55.f * FMath::Sin(MotionTime * 0.9f + Phase * 1.2f) + (ScatterAlpha * 35.f));
	const FVector PatrolLocation = HomeLocation + Offset + WindDisplacement(LocalWind);
	const FVector RippleInterestLocationWithOrbit = RippleInterestLocation + FVector(0.f, 0.f, 12.f) + Offset * 0.12f;
	const FVector FlowerInterestLocationWithOrbit = FlowerInterestLocation + Offset * 0.08f;
	const float FlowerInterestAlpha = ScatterRemaining > 0.f ? 0.f : GetFlowerInterestAlpha();
	const float BasinInterestAlpha = ScatterRemaining > 0.f ? 0.f : GetBasinInterestAlpha();
	const FVector Desired = FMath::Lerp(
		FMath::Lerp(
			FMath::Lerp(PatrolLocation, FlowerInterestLocationWithOrbit, FlowerInterestAlpha),
			BasinInterestLocation + Offset * 0.08f, BasinInterestAlpha),
		RippleInterestLocationWithOrbit, GetRippleInterestAlpha());
	const FVector DesiredWithListeningStones = FMath::Lerp(
		Desired,
		ListeningStonesInterestLocation + Offset * 0.08f,
		ScatterRemaining > 0.f ? 0.f : GetListeningStonesInterestAlpha());
	const FVector Previous = GetActorLocation();
	const FVector Safe = ResolveFlightPath(Previous, DesiredWithListeningStones);
	SetActorLocation(Safe, false);
	const FVector Facing = (Safe - Previous).GetSafeNormal2D();
	if (!Facing.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Facing.Rotation(), SafeDelta, 2.5f));
	const float Beat = FMath::Sin(Time * 34.f * MoveScale + WingPhase) * 26.f;
	for (int32 Index = 0; Index < Wings.Num(); ++Index)
	{
		const float Side = (Index % 2 == 0) ? -1.f : 1.f;
		if (Wings[Index]) Wings[Index]->SetRelativeRotation(FRotator(Beat * (Index < 2 ? 0.5f : -0.5f), Side * (12.f + Index * 2.f), Side * Beat));
	}
}
