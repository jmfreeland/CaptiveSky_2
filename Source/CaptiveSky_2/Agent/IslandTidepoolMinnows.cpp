#include "IslandTidepoolMinnows.h"

#include "EngineUtils.h"
#include "IslandDayNight.h"
#include "IslandPoolRippleEffect.h"
#include "IslandWeather.h"
#include "IslandTideglassSubsystem.h"
#include "RavenAgentAIController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void BuildDoubleSidedTriangles(UProceduralMeshComponent* Mesh, const TArray<FVector>& SourceVertices, const TArray<int32>& SourceTriangles)
{
	if (!Mesh) return;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(SourceTriangles.Num() * 2);
	Triangles.Reserve(SourceTriangles.Num() * 2);
	Normals.Reserve(SourceTriangles.Num() * 2);
	UVs.Reserve(SourceTriangles.Num() * 2);
	Colors.Reserve(SourceTriangles.Num() * 2);
	Tangents.Reserve(SourceTriangles.Num() * 2);

	for (int32 Triangle = 0; Triangle < SourceTriangles.Num(); Triangle += 3)
	{
		const FVector A = SourceVertices[SourceTriangles[Triangle]];
		const FVector B = SourceVertices[SourceTriangles[Triangle + 1]];
		const FVector C = SourceVertices[SourceTriangles[Triangle + 2]];
		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const int32 FirstVertex = Vertices.Num();
			const FVector FaceVertices[3] = { A, Side == 0 ? B : C, Side == 0 ? C : B };
			const FVector FaceNormal = Side == 0 ? Normal : -Normal;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				Vertices.Add(FaceVertices[Corner]);
				Triangles.Add(FirstVertex + Corner);
				Normals.Add(FaceNormal);
				UVs.Add(FVector2D(Corner == 1 ? 1.f : 0.f, Corner == 2 ? 1.f : 0.f));
				Colors.Add(FLinearColor::White);
				Tangents.Add(FProcMeshTangent((B - A).GetSafeNormal(), false));
			}
		}
	}

	Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void BuildForkedCaudalFin(UProceduralMeshComponent* Tail)
{
	if (!Tail) return;

	// Coordinates are centimeters in the fish's local frame. The fork stands
	// vertically behind the tapered body rather than lying in the water plane.
	const TArray<FVector> Vertices = {
		FVector(0.f, 0.f, 0.f),
		FVector(-1.4f, 0.f, 0.f),
		FVector(-3.1f, 0.f, 3.2f),
		FVector(-3.1f, 0.f, -3.2f)
	};
	const TArray<int32> Triangles = { 0, 2, 1, 0, 1, 3 };
	BuildDoubleSidedTriangles(Tail, Vertices, Triangles);
}

void BuildMinnowBody(UProceduralMeshComponent* Body)
{
	if (!Body) return;

	struct FBodyRing
	{
		float X;
		float RadiusY;
		float RadiusZ;
	};
	static constexpr int32 SegmentCount = 12;
	const TArray<FBodyRing> Rings = {
		{ -7.0f, 0.55f, 0.55f },
		{ -5.8f, 1.05f, 1.15f },
		{ -4.0f, 1.55f, 1.60f },
		{ -1.0f, 1.95f, 1.95f },
		{  3.0f, 1.80f, 1.85f },
		{  6.2f, 1.25f, 1.40f },
		{  8.0f, 0.52f, 0.65f }
	};
	const FVector TailTip(-8.0f, 0.f, 0.f);
	const FVector Snout(8.8f, 0.f, 0.f);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	const int32 RingVertexCount = Rings.Num() * SegmentCount;
	Vertices.Reserve(RingVertexCount + 2);
	Normals.Reserve(RingVertexCount + 2);
	UVs.Reserve(RingVertexCount + 2);
	Colors.Reserve(RingVertexCount + 2);
	Tangents.Reserve(RingVertexCount + 2);
	Triangles.Reserve((Rings.Num() - 1) * SegmentCount * 6 + SegmentCount * 6);

	Vertices.Add(TailTip);
	Normals.Add(FVector(-1.f, 0.f, 0.f));
	UVs.Add(FVector2D(0.f, 0.5f));
	Colors.Add(FLinearColor::White);
	Tangents.Add(FProcMeshTangent(FVector(0.f, 1.f, 0.f), false));

	for (int32 RingIndex = 0; RingIndex < Rings.Num(); ++RingIndex)
	{
		const FBodyRing& Ring = Rings[RingIndex];
		const int32 PreviousIndex = FMath::Max(0, RingIndex - 1);
		const int32 NextIndex = FMath::Min(Rings.Num() - 1, RingIndex + 1);
		const float DeltaX = FMath::Max(0.01f, Rings[NextIndex].X - Rings[PreviousIndex].X);
		const float RadiusYSlope = (Rings[NextIndex].RadiusY - Rings[PreviousIndex].RadiusY) / DeltaX;
		const float RadiusZSlope = (Rings[NextIndex].RadiusZ - Rings[PreviousIndex].RadiusZ) / DeltaX;
		for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			const float Angle = 2.f * PI * Segment / SegmentCount;
			const float CosAngle = FMath::Cos(Angle);
			const float SinAngle = FMath::Sin(Angle);
			Vertices.Add(FVector(Ring.X, CosAngle * Ring.RadiusY, SinAngle * Ring.RadiusZ));
			Normals.Add(FVector(
				-0.5f * (RadiusYSlope / Ring.RadiusY + RadiusZSlope / Ring.RadiusZ),
				CosAngle / Ring.RadiusY,
				SinAngle / Ring.RadiusZ).GetSafeNormal());
			UVs.Add(FVector2D(static_cast<float>(RingIndex + 1) / (Rings.Num() + 1), static_cast<float>(Segment) / SegmentCount));
			Colors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
		}
	}

	const int32 SnoutIndex = Vertices.Num();
	Vertices.Add(Snout);
	Normals.Add(FVector::ForwardVector);
	UVs.Add(FVector2D(1.f, 0.5f));
	Colors.Add(FLinearColor::White);
	Tangents.Add(FProcMeshTangent(FVector(0.f, 1.f, 0.f), false));

	const int32 FirstRingIndex = 1;
	for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
	{
		const int32 Current = FirstRingIndex + Segment;
		const int32 Next = FirstRingIndex + (Segment + 1) % SegmentCount;
		Triangles.Append({ 0, Next, Current });
	}
	for (int32 RingIndex = 0; RingIndex < Rings.Num() - 1; ++RingIndex)
	{
		const int32 CurrentRing = FirstRingIndex + RingIndex * SegmentCount;
		const int32 NextRing = CurrentRing + SegmentCount;
		for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			const int32 A = CurrentRing + Segment;
			const int32 B = CurrentRing + (Segment + 1) % SegmentCount;
			const int32 C = NextRing + (Segment + 1) % SegmentCount;
			const int32 D = NextRing + Segment;
			Triangles.Append({ A, B, C, A, C, D });
		}
	}
	const int32 LastRing = FirstRingIndex + (Rings.Num() - 1) * SegmentCount;
	for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
	{
		const int32 Current = LastRing + Segment;
		const int32 Next = LastRing + (Segment + 1) % SegmentCount;
		Triangles.Append({ Current, Next, SnoutIndex });
	}

	Body->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void BuildBodyFins(UProceduralMeshComponent* Fins)
{
	if (!Fins) return;

	// The dorsal fin rises from the upper skin while paired pectorals fan out
	// from the body flanks. All coordinates are local centimeters.
	const TArray<FVector> Vertices = {
		FVector(-5.0f, 0.f, 1.45f), FVector(-2.0f, 0.f, 1.90f), FVector(-3.3f, 0.f, 4.25f),
		FVector(-0.7f, 1.8f, 0.15f), FVector(-2.8f, 2.0f, 0.10f), FVector(-1.3f, 4.2f, 0.55f),
		FVector(-0.7f, -1.8f, 0.15f), FVector(-1.3f, -4.2f, 0.55f), FVector(-2.8f, -2.0f, 0.10f)
	};
	const TArray<int32> Triangles = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	BuildDoubleSidedTriangles(Fins, Vertices, Triangles);
}

void BuildDorsalMark(UProceduralMeshComponent* Mark)
{
	if (!Mark) return;

	// A narrow, muted dorsal flash sits just above the body surface, where it
	// remains readable from the elevated pool-edge viewpoints.
	static constexpr int32 StationCount = 7;
	static constexpr float Width = 0.34f;
	static constexpr float SurfaceOffset = 0.06f;
	static constexpr float XPositions[StationCount] = { -4.8f, -3.2f, -1.6f, 0.f, 1.6f, 3.2f, 4.8f };
	static constexpr float RingX[StationCount] = { -7.0f, -5.8f, -4.0f, -1.0f, 3.0f, 6.2f, 8.0f };
	static constexpr float RingRadiusY[StationCount] = { 0.55f, 1.05f, 1.55f, 1.95f, 1.80f, 1.25f, 0.52f };
	static constexpr float RingRadiusZ[StationCount] = { 0.55f, 1.15f, 1.60f, 1.95f, 1.85f, 1.40f, 0.65f };
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(StationCount * 2);
	Normals.Reserve(StationCount * 2);
	UVs.Reserve(StationCount * 2);
	Colors.Reserve(StationCount * 2);
	Tangents.Reserve(StationCount * 2);
	Triangles.Reserve((StationCount - 1) * 6);

	for (int32 Index = 0; Index < StationCount; ++Index)
	{
		const float X = XPositions[Index];
		int32 Ring = 0;
		while (Ring < StationCount - 2 && X > RingX[Ring + 1]) ++Ring;
		const float Alpha = FMath::Clamp((X - RingX[Ring]) / (RingX[Ring + 1] - RingX[Ring]), 0.f, 1.f);
		const float RadiusY = FMath::Lerp(RingRadiusY[Ring], RingRadiusY[Ring + 1], Alpha);
		const float RadiusZ = FMath::Lerp(RingRadiusZ[Ring], RingRadiusZ[Ring + 1], Alpha);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Y = Side == 0 ? -Width : Width;
			const float Z = RadiusZ * FMath::Sqrt(FMath::Max(0.f, 1.f - FMath::Square(Y / RadiusY))) + SurfaceOffset;
			Vertices.Add(FVector(X, Y, Z));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(static_cast<float>(Index) / (StationCount - 1), static_cast<float>(Side)));
			Colors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
		}
	}

	for (int32 Index = 0; Index < StationCount - 1; ++Index)
	{
		const int32 A = Index * 2;
		const int32 B = A + 2;
		Triangles.Append({ A, B, B + 1, A, B + 1, A + 1 });
	}
	Mark->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	auto GetBodyRadiiAt = [](float X)
	{
		int32 Ring = 0;
		while (Ring < StationCount - 2 && X > RingX[Ring + 1]) ++Ring;
		const float Alpha = FMath::Clamp((X - RingX[Ring]) / (RingX[Ring + 1] - RingX[Ring]), 0.f, 1.f);
		return FVector2D(FMath::Lerp(RingRadiusY[Ring], RingRadiusY[Ring + 1], Alpha),
			FMath::Lerp(RingRadiusZ[Ring], RingRadiusZ[Ring + 1], Alpha));
	};
	auto SurfaceYAt = [&GetBodyRadiiAt](float X, float Z, float Side, float Offset)
	{
		const FVector2D Radii = GetBodyRadiiAt(X);
		const float SideY = Radii.X * FMath::Sqrt(FMath::Max(0.f, 1.f - FMath::Square(Z / Radii.Y))) + Offset;
		return Side * SideY;
	};
	auto BuildEyesSection = [&](int32 SectionIndex, float Radius, float SurfaceOffset, float CenterXOffset, float CenterZOffset)
	{
		TArray<FVector> EyeVertices;
		TArray<int32> EyeTriangles;
		TArray<FVector> EyeNormals;
		TArray<FVector2D> EyeUVs;
		TArray<FLinearColor> EyeColors;
		TArray<FProcMeshTangent> EyeTangents;
		EyeVertices.Reserve(18);
		EyeTriangles.Reserve(48);
		EyeNormals.Reserve(18);
		EyeUVs.Reserve(18);
		EyeColors.Reserve(18);
		EyeTangents.Reserve(18);
		for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
		{
			const float Side = SideIndex == 0 ? -1.f : 1.f;
			const float CenterX = 4.45f + CenterXOffset;
			const float CenterZ = 0.66f + CenterZOffset;
			const int32 FirstVertex = EyeVertices.Num();
			EyeVertices.Add(FVector(CenterX, SurfaceYAt(CenterX, CenterZ, Side, SurfaceOffset), CenterZ));
			EyeNormals.Add(FVector(0.f, Side, 0.f));
			EyeUVs.Add(FVector2D(0.5f, 0.5f));
			EyeColors.Add(FLinearColor::White);
			EyeTangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
			for (int32 Segment = 0; Segment < 8; ++Segment)
			{
				const float Angle = 2.f * PI * Segment / 8.f;
				const float X = CenterX + FMath::Cos(Angle) * Radius;
				const float Z = CenterZ + FMath::Sin(Angle) * Radius;
				EyeVertices.Add(FVector(X, SurfaceYAt(X, Z, Side, SurfaceOffset), Z));
				EyeNormals.Add(FVector(0.f, Side, 0.f));
				EyeUVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
				EyeColors.Add(FLinearColor::White);
				EyeTangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
			}
			for (int32 Segment = 0; Segment < 8; ++Segment)
			{
				const int32 Current = FirstVertex + 1 + Segment;
				const int32 Next = FirstVertex + 1 + (Segment + 1) % 8;
				if (Side > 0.f) EyeTriangles.Append({ FirstVertex, Next, Current });
				else EyeTriangles.Append({ FirstVertex, Current, Next });
			}
		}
		Mark->CreateMeshSection_LinearColor(SectionIndex, EyeVertices, EyeTriangles, EyeNormals, EyeUVs, EyeColors, EyeTangents, false);
	};
	BuildEyesSection(1, 0.46f, 0.11f, 0.f, 0.f);
	BuildEyesSection(2, 0.19f, 0.20f, 0.08f, 0.04f);
}
}

AIslandTidepoolMinnows::AIslandTidepoolMinnows()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.06f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("MinnowSchool"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseShapeMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;

	Fish.Reserve(FishCount);
	Tails.Reserve(FishCount);
	BodyFins.Reserve(FishCount);
	DorsalMarks.Reserve(FishCount);
	for (int32 Index = 0; Index < FishCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("Minnow_%d"), Index));
		UProceduralMeshComponent* Minnow = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Minnow->SetupAttachment(RootComponent);
		BuildMinnowBody(Minnow);
		Minnow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Minnow->SetCastShadow(false);
		Minnow->SetCanEverAffectNavigation(false);
		Fish.Add(Minnow);

		const FName TailName(*FString::Printf(TEXT("MinnowTail_%d"), Index));
		UProceduralMeshComponent* Tail = CreateDefaultSubobject<UProceduralMeshComponent>(TailName);
		Tail->SetupAttachment(Minnow);
		Tail->SetRelativeLocation(FVector(-7.8f, 0.f, 0.f));
		BuildForkedCaudalFin(Tail);
		Tail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Tail->SetCastShadow(false);
		Tail->SetCanEverAffectNavigation(false);
		Tail->SetGenerateOverlapEvents(false);
		Tails.Add(Tail);

		const FName FinsName(*FString::Printf(TEXT("MinnowFins_%d"), Index));
		UProceduralMeshComponent* Fins = CreateDefaultSubobject<UProceduralMeshComponent>(FinsName);
		Fins->SetupAttachment(Minnow);
		BuildBodyFins(Fins);
		Fins->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Fins->SetCastShadow(false);
		Fins->SetCanEverAffectNavigation(false);
		Fins->SetGenerateOverlapEvents(false);
		BodyFins.Add(Fins);

		const FName MarkName(*FString::Printf(TEXT("MinnowDorsalMark_%d"), Index));
		UProceduralMeshComponent* Mark = CreateDefaultSubobject<UProceduralMeshComponent>(MarkName);
		Mark->SetupAttachment(Minnow);
		BuildDorsalMark(Mark);
		Mark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mark->SetCastShadow(false);
		Mark->SetCanEverAffectNavigation(false);
		Mark->SetGenerateOverlapEvents(false);
		DorsalMarks.Add(Mark);
	}
}

void AIslandTidepoolMinnows::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		IslandClock = *It;
		break;
	}
	CachePoolSwimmingBounds();
	ConfigureAppearance();
	UpdateSchool(0.f);
}

void AIslandTidepoolMinnows::CachePoolSwimmingBounds()
{
	bHasPoolSwimmingBounds = false;
	PoolSwimmingRadii = FVector2D::ZeroVector;
	if (!GetWorld()) return;

	UStaticMeshComponent* PoolSurface = UIslandTideglassSubsystem::FindPoolSurface(GetWorld());
	if (!PoolSurface) return;
	const FBoxSphereBounds& Bounds = PoolSurface->Bounds;
	const float BodyClearanceCm = 10.f;
	PoolSwimmingRadii = FVector2D(
		FMath::Max(0.f, Bounds.BoxExtent.X * PoolSwimmingFootprintFraction - BodyClearanceCm),
		FMath::Max(0.f, Bounds.BoxExtent.Y * PoolSwimmingFootprintFraction - BodyClearanceCm));
	if (PoolSwimmingRadii.X <= KINDA_SMALL_NUMBER || PoolSwimmingRadii.Y <= KINDA_SMALL_NUMBER)
	{
		PoolSwimmingRadii = FVector2D::ZeroVector;
		return;
	}

	PoolSurfaceBoundsOrigin = Bounds.Origin;
	bHasPoolSwimmingBounds = true;
}

FVector AIslandTidepoolMinnows::ClampToPoolSwimmingBounds(const FVector& DesiredRelativeLocation) const
{
	if (!bHasPoolSwimmingBounds) return DesiredRelativeLocation;

	FVector ClampedWorldLocation = GetActorTransform().TransformPosition(DesiredRelativeLocation);
	FVector2D PoolOffset(ClampedWorldLocation.X - PoolSurfaceBoundsOrigin.X,
		ClampedWorldLocation.Y - PoolSurfaceBoundsOrigin.Y);
	const FVector2D NormalizedOffset(PoolOffset.X / PoolSwimmingRadii.X, PoolOffset.Y / PoolSwimmingRadii.Y);
	const float NormalizedDistance = NormalizedOffset.Size();
	if (NormalizedDistance > 1.f)
	{
		PoolOffset.X = NormalizedOffset.X / NormalizedDistance * PoolSwimmingRadii.X;
		PoolOffset.Y = NormalizedOffset.Y / NormalizedDistance * PoolSwimmingRadii.Y;
		ClampedWorldLocation.X = PoolSurfaceBoundsOrigin.X + PoolOffset.X;
		ClampedWorldLocation.Y = PoolSurfaceBoundsOrigin.Y + PoolOffset.Y;
	}
	return GetActorTransform().InverseTransformPosition(ClampedWorldLocation);
}

void AIslandTidepoolMinnows::ConfigureAppearance()
{
	if (BaseShapeMaterial)
	{
		static const FLinearColor SchoolPalette[FishCount] =
		{
			FLinearColor(0.18f, 0.34f, 0.40f),
			FLinearColor(0.22f, 0.36f, 0.34f),
			FLinearColor(0.28f, 0.32f, 0.19f),
			FLinearColor(0.38f, 0.36f, 0.24f),
			FLinearColor(0.17f, 0.28f, 0.39f)
		};
		for (int32 Index = 0; Index < Fish.Num(); ++Index)
		{
			UMaterialInstanceDynamic* FishMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
			if (!FishMaterial) continue;
			FishMaterial->SetVectorParameterValue(TEXT("Color"), SchoolPalette[Index % FishCount]);
			FishMaterial->SetVectorParameterValue(TEXT("BaseColor"), SchoolPalette[Index % FishCount]);
			if (Fish[Index]) Fish[Index]->SetMaterial(0, FishMaterial);

			const FLinearColor FinColor = SchoolPalette[Index % FishCount] * 0.55f;
			UMaterialInstanceDynamic* FinMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
			if (!FinMaterial) continue;
			FinMaterial->SetVectorParameterValue(TEXT("Color"), FinColor);
			FinMaterial->SetVectorParameterValue(TEXT("BaseColor"), FinColor);
			if (Tails.IsValidIndex(Index) && Tails[Index]) Tails[Index]->SetMaterial(0, FinMaterial);
			if (BodyFins.IsValidIndex(Index) && BodyFins[Index]) BodyFins[Index]->SetMaterial(0, FinMaterial);

			const FLinearColor DorsalMarkColor = FMath::Lerp(SchoolPalette[Index % FishCount], FLinearColor(0.70f, 0.62f, 0.38f), 0.65f);
			UMaterialInstanceDynamic* DorsalMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
			if (!DorsalMaterial) continue;
			DorsalMaterial->SetVectorParameterValue(TEXT("Color"), DorsalMarkColor);
			DorsalMaterial->SetVectorParameterValue(TEXT("BaseColor"), DorsalMarkColor);
			if (DorsalMarks.IsValidIndex(Index) && DorsalMarks[Index]) DorsalMarks[Index]->SetMaterial(0, DorsalMaterial);

			UMaterialInstanceDynamic* EyeMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
			if (!EyeMaterial) continue;
			const FLinearColor IrisColor(0.88f, 0.63f, 0.25f);
			EyeMaterial->SetVectorParameterValue(TEXT("Color"), IrisColor);
			EyeMaterial->SetVectorParameterValue(TEXT("BaseColor"), IrisColor);
			if (DorsalMarks.IsValidIndex(Index) && DorsalMarks[Index]) DorsalMarks[Index]->SetMaterial(1, EyeMaterial);

			UMaterialInstanceDynamic* PupilMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
			if (!PupilMaterial) continue;
			const FLinearColor PupilColor(0.035f, 0.028f, 0.018f);
			PupilMaterial->SetVectorParameterValue(TEXT("Color"), PupilColor);
			PupilMaterial->SetVectorParameterValue(TEXT("BaseColor"), PupilColor);
			if (DorsalMarks.IsValidIndex(Index) && DorsalMarks[Index]) DorsalMarks[Index]->SetMaterial(2, PupilMaterial);
		}
	}
}

void AIslandTidepoolMinnows::RespondToQuietObservation(const FVector& ObserverLocation)
{
	ScatterDirection = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (ScatterDirection.IsNearlyZero()) ScatterDirection = GetActorForwardVector();
	ScatterRemaining = 2.4f;
	CreateScatterSurfaceCue(ObserverLocation);
}

void AIslandTidepoolMinnows::CreateScatterSurfaceCue(const FVector& ObserverLocation)
{
	if (!GetWorld() || Fish.IsEmpty() || ScatterSurfaceCueCooldownRemaining > 0.f) return;

	UProceduralMeshComponent* NearestFish = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (UProceduralMeshComponent* FishBody : Fish)
	{
		if (!FishBody) continue;
		const float DistanceSquared = FVector::DistSquared2D(FishBody->GetComponentLocation(), ObserverLocation);
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestFish = FishBody;
		}
	}
	if (!NearestFish) return;

	// The closest fish to a quiet approach breaks the surface as the rest of the
	// school fans away. Keep the cue short, tide-locked, and visually secondary.
	const FVector FishLocation = NearestFish->GetComponentLocation();
	const FVector RippleLocation(FishLocation.X, FishLocation.Y, GetActorLocation().Z + GetTideOffsetCm() - 24.f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(
		RippleLocation, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsMinnowStartleImpact();
		ScatterSurfaceCueCooldownRemaining = ScatterSurfaceCueCooldownSeconds;
	}
}

bool AIslandTidepoolMinnows::RespondToSurfaceRipple()
{
	if (SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f) return false;
	SurfacePulseRemaining = 1.2f;
	SurfacePulseCooldownRemaining = 3.f;
	return true;
}

FString AIslandTidepoolMinnows::DescribeRecentSurfaceBreak(const FVector& ObserverLocation) const
{
	if (SurfaceBreakContextRemaining <= 0.f ||
		FVector::DistSquared2D(ObserverLocation, GetActorLocation()) > FMath::Square(1100.f) ||
		FMath::Abs(ObserverLocation.Z - GetActorLocation().Z) > 500.f)
	{
		return FString();
	}

	return TEXT("A minnow made a brief surface break in the Tideglass shallows within the last few minutes. The ripple has faded; the school remains wild and nothing lasting changed.");
}

void AIslandTidepoolMinnows::CheckForNaturalSurfaceRipple()
{
	if (!GetWorld() || SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f) return;

	constexpr float RippleResponseRadius = 250.f;
	for (TActorIterator<AIslandPoolRippleEffect> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("WindImpact")) && !It->ActorHasTag(TEXT("RainImpact"))) continue;
		if (FVector::DistSquared2D(GetActorLocation(), It->GetActorLocation()) > FMath::Square(RippleResponseRadius)) continue;
		RespondToSurfaceRipple();
		return;
	}
}

void AIslandTidepoolMinnows::TryCreateSurfaceBreak(float RainIntensity)
{
	if (!GetWorld() || Fish.IsEmpty() || ScatterRemaining > 0.f ||
		SurfacePulseRemaining > 0.f || SurfacePulseCooldownRemaining > 0.f || RainIntensity >= 0.35f)
	{
		SurfaceBreakRemaining = 4.f;
		return;
	}

	UProceduralMeshComponent* FishThatBrokeSurface = Fish[SurfaceBreakFishIndex % Fish.Num()];
	if (!FishThatBrokeSurface)
	{
		SurfaceBreakRemaining = 4.f;
		return;
	}

	// Fish skim about 1.5 cm above the moving waterline; the ripple lights sit
	// 24 cm above their root, so place the actor below the same tidal surface.
	const FVector FishLocation = FishThatBrokeSurface->GetComponentLocation();
	const FVector RippleLocation(FishLocation.X, FishLocation.Y, GetActorLocation().Z + GetTideOffsetCm() - 24.f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AIslandPoolRippleEffect* Ripple = GetWorld()->SpawnActor<AIslandPoolRippleEffect>(
		RippleLocation, FRotator::ZeroRotator, SpawnParameters))
	{
		Ripple->ConfigureAsMinnowImpact();
		SurfaceBreakFishIndex = (SurfaceBreakFishIndex + 1) % Fish.Num();
		SurfaceBreakRemaining = 19.f;
		SurfaceBreakContextRemaining = SurfaceBreakContextLifetime;
		SurfacePulseCooldownRemaining = FMath::Max(SurfacePulseCooldownRemaining, 3.f);
		return;
	}

	SurfaceBreakRemaining = 4.f;
}

float AIslandTidepoolMinnows::RainMovementScale(float RainIntensity)
{
	const float RainActivity = FMath::SmoothStep(0.35f, 0.85f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	return FMath::Lerp(1.f, 0.68f, RainActivity);
}

float AIslandTidepoolMinnows::GetTideOffsetCm() const
{
	return IslandClock.IsValid()
		? UIslandTideglassSubsystem::TideOffsetCm(IslandClock->CurrentHour, IslandClock->DayNumber)
		: 0.f;
}

float AIslandTidepoolMinnows::GetScatterAlpha() const
{
	if (ScatterRemaining <= 0.f) return 0.f;
	if (ScatterRemaining > 1.7f)
		return FMath::SmoothStep(0.f, 0.7f, 2.4f - ScatterRemaining);
	return FMath::SmoothStep(0.f, 1.7f, ScatterRemaining);
}

float AIslandTidepoolMinnows::GetSurfacePulseAlpha() const
{
	if (SurfacePulseRemaining <= 0.f) return 0.f;
	const float Elapsed = 1.2f - SurfacePulseRemaining;
	return FMath::SmoothStep(0.f, 0.18f, Elapsed) * (1.f - FMath::SmoothStep(0.45f, 1.2f, Elapsed));
}

void AIslandTidepoolMinnows::CheckForNearbyRavenDisturbance()
{
	if (!GetWorld()) return;

	constexpr float FlybyRadius = 550.f;
	constexpr float MinimumHeight = 150.f;
	constexpr float MaximumHeight = 700.f;
	constexpr float SettledRavenRadius = 450.f;
	constexpr float SettledRavenHeight = 350.f;
	constexpr float PresenceRearmRadius = 800.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		const float DistanceSquared = Offset.SizeSquared2D();
		const bool bSettledRaven = It->LocomotionState == ERavenLocomotionState::Grounded ||
			It->LocomotionState == ERavenLocomotionState::Hopping || It->LocomotionState == ERavenLocomotionState::Perched;
		if (RavenPresenceLatch.Get() == *It && DistanceSquared > FMath::Square(PresenceRearmRadius))
			RavenPresenceLatch.Reset();

		const bool bCloseSettledPresence = bSettledRaven && FMath::Abs(Offset.Z) <= SettledRavenHeight &&
			DistanceSquared <= FMath::Square(SettledRavenRadius);
		const bool bLowFlyby = It->LocomotionState == ERavenLocomotionState::Flying &&
			Offset.Z >= MinimumHeight && Offset.Z <= MaximumHeight && DistanceSquared <= FMath::Square(FlybyRadius);
		if (bCloseSettledPresence)
		{
			// The school scatters once for an approach. Remaining nearby, including
			// changing between perched and flying, does not repeatedly reset its path.
			const bool bAlreadyNoticedThisApproach = RavenPresenceLatch.Get() == *It;
			if (!bAlreadyNoticedThisApproach) RavenPresenceLatch = *It;
			if (bAlreadyNoticedThisApproach || RavenFlybyCooldownRemaining > 0.f || ScatterRemaining > 0.f) continue;
		}
		else if (!bLowFlyby || RavenFlybyCooldownRemaining > 0.f || ScatterRemaining > 0.f) continue;

		// A close, low pass or settled approach briefly breaks the school's pattern;
		// this is not a hunt, capture, model call, or lasting change.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTidepoolMinnows::UpdateSchool(float RainIntensity)
{
	const float TuckScale = RainMovementScale(RainIntensity);
	const float TideOffset = GetTideOffsetCm();
	const float ScatterAlpha = GetScatterAlpha();
	const float CircleScale = 1.f + 0.65f * GetSurfacePulseAlpha();
	const FVector Side(-ScatterDirection.Y, ScatterDirection.X, 0.f);
	for (int32 Index = 0; Index < Fish.Num(); ++Index)
	{
		UProceduralMeshComponent* Minnow = Fish[Index];
		if (!Minnow) continue;
		const float Angle = Phase + ElapsedSeconds * 0.62f + Index * 2.f * PI / FishCount;
		const FVector IdleOffset(
			FMath::Cos(Angle) * 135.f * TuckScale * CircleScale,
			FMath::Sin(Angle) * 82.f * TuckScale * CircleScale,
			TideOffset + 0.8f + FMath::Sin(Angle * 1.7f) * 0.4f);
		const float FanOffset = (Index - (FishCount - 1) * 0.5f) * 22.f;
		const FVector ScatterOffset = ScatterDirection * 210.f + Side * FanOffset;
		Minnow->SetRelativeLocation(ClampToPoolSwimmingBounds(IdleOffset + ScatterOffset * ScatterAlpha));

		FVector Facing(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		Facing += ScatterDirection * (ScatterAlpha * 1.2f);
		if (!Facing.IsNearlyZero())
			Minnow->SetRelativeRotation(FRotator(0.f, Facing.Rotation().Yaw, 0.f));

		if (Tails.IsValidIndex(Index) && Tails[Index])
		{
			const float TailBeat = FMath::Sin(ElapsedSeconds * 8.f + Index * 1.35f);
			Tails[Index]->SetRelativeRotation(FRotator(0.f, TailBeat * 16.f * TuckScale, TailBeat * 3.f));
		}
	}
}

void AIslandTidepoolMinnows::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ElapsedSeconds += SafeDelta;
	ScatterRemaining = FMath::Max(0.f, ScatterRemaining - SafeDelta);
	SurfacePulseRemaining = FMath::Max(0.f, SurfacePulseRemaining - SafeDelta);
	SurfacePulseCooldownRemaining = FMath::Max(0.f, SurfacePulseCooldownRemaining - SafeDelta);
	ScatterSurfaceCueCooldownRemaining = FMath::Max(0.f, ScatterSurfaceCueCooldownRemaining - SafeDelta);
	RippleCheckRemaining -= SafeDelta;
	if (RippleCheckRemaining <= 0.f)
	{
		RippleCheckRemaining = 0.35f;
		CheckForNaturalSurfaceRipple();
	}
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - SafeDelta);
	RavenCheckRemaining -= SafeDelta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = RavenCheckIntervalSeconds;
		CheckForNearbyRavenDisturbance();
	}
	const float Rain = Weather.IsValid() && GetWorld()
		? Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds())
		: 0.f;
	SurfaceBreakRemaining -= SafeDelta;
	SurfaceBreakContextRemaining = FMath::Max(0.f, SurfaceBreakContextRemaining - SafeDelta);
	if (SurfaceBreakRemaining <= 0.f)
	{
		TryCreateSurfaceBreak(Rain);
	}
	UpdateSchool(Rain);
}
