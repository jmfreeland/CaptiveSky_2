#include "RavenAgentAIController.h"
#include "AutonomousAgentCharacter.h"
#include "AgentRestPresentationComponent.h"
#include "IslandArrangement.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "IslandListeningStonesChime.h"
#include "IslandWeather.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandInteractionUtility.h"
#include "AgentMemoryComponent.h"
#include "HAL/PlatformTime.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogRavenAgentAI, Log, All);

namespace
{
	struct FRavenProcMeshData
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;
	};

	static bool IsRavenVisualPawn(const APawn* Pawn)
	{
		if (!Pawn) return false;
		if (Pawn->GetClass()->GetName().Contains(TEXT("Raven"), ESearchCase::IgnoreCase)) return true;
		const AAutonomousAgentCharacter* Agent = Cast<AAutonomousAgentCharacter>(Pawn);
		return Agent && Agent->DisplayName.Contains(TEXT("Raven"), ESearchCase::IgnoreCase);
	}

	static void AppendDoubleSidedTriangle(FRavenProcMeshData& Data, const FVector& A, const FVector& B, const FVector& C)
	{
		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		const int32 Base = Data.Vertices.Num();
		for (const FVector& Vertex : { A, B, C })
		{
			Data.Vertices.Add(Vertex);
			Data.Normals.Add(Normal);
			Data.UVs.Add(FVector2D::ZeroVector);
			Data.Colors.Add(FLinearColor::White);
		}
		Data.Triangles.Append({ Base, Base + 1, Base + 2, Base + 2, Base + 1, Base });
	}

	static void AppendEllipsoid(FRavenProcMeshData& Data, const FVector& Center, const FVector& Radii,
		int32 Segments = 12, int32 Rings = 8)
	{
		const int32 Base = Data.Vertices.Num();
		for (int32 Ring = 0; Ring <= Rings; ++Ring)
		{
			const float Latitude = -0.5f * PI + PI * static_cast<float>(Ring) / static_cast<float>(Rings);
			const float CosLatitude = FMath::Cos(Latitude);
			const float SinLatitude = FMath::Sin(Latitude);
			for (int32 Segment = 0; Segment <= Segments; ++Segment)
			{
				const float Longitude = 2.f * PI * static_cast<float>(Segment) / static_cast<float>(Segments);
				const FVector Unit(CosLatitude * FMath::Cos(Longitude), CosLatitude * FMath::Sin(Longitude), SinLatitude);
				Data.Vertices.Add(Center + Unit * Radii);
				Data.Normals.Add(FVector(Unit.X / FMath::Max(Radii.X, 0.01f), Unit.Y / FMath::Max(Radii.Y, 0.01f),
					Unit.Z / FMath::Max(Radii.Z, 0.01f)).GetSafeNormal());
				Data.UVs.Add(FVector2D(static_cast<float>(Segment) / Segments, static_cast<float>(Ring) / Rings));
				Data.Colors.Add(FLinearColor::White);
			}
		}
		const int32 Row = Segments + 1;
		for (int32 Ring = 0; Ring < Rings; ++Ring)
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				const int32 A = Base + Ring * Row + Segment;
				const int32 B = A + 1;
				const int32 C = A + Row;
				const int32 D = C + 1;
				Data.Triangles.Append({ A, B, D, A, D, C, D, B, A, C, D, A });
			}
	}

	static void AppendRavenWing(FRavenProcMeshData& Data, float Side)
	{
		const FVector Outline[] = {
			FVector(0.f, 0.f, 0.f), FVector(27.f, 22.f, 1.f), FVector(38.f, 53.f, 0.f),
			FVector(31.f, 88.f, -1.f), FVector(3.f, 112.f, -3.f), FVector(-30.f, 96.f, -3.f),
			FVector(-57.f, 67.f, -1.f), FVector(-50.f, 42.f, 0.f), FVector(-27.f, 17.f, 1.f)
		};
		FVector Center = FVector::ZeroVector;
		for (const FVector& Point : Outline) Center += FVector(Point.X, Point.Y * Side, Point.Z + 1.5f);
		Center /= UE_ARRAY_COUNT(Outline);
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Outline); ++Index)
		{
			FVector A = FVector(Outline[Index].X, Outline[Index].Y * Side, Outline[Index].Z + 1.5f);
			FVector B = FVector(Outline[(Index + 1) % UE_ARRAY_COUNT(Outline)].X,
				Outline[(Index + 1) % UE_ARRAY_COUNT(Outline)].Y * Side,
				Outline[(Index + 1) % UE_ARRAY_COUNT(Outline)].Z + 1.5f);
			AppendDoubleSidedTriangle(Data, Center, A, B);
			AppendDoubleSidedTriangle(Data, Center - FVector(0.f, 0.f, 3.f), B - FVector(0.f, 0.f, 3.f), A - FVector(0.f, 0.f, 3.f));
		}
		// Slender primaries break the outer edge into feather tips instead of one flat slab.
		for (int32 Feather = 0; Feather < 6; ++Feather)
		{
			const float T = static_cast<float>(Feather) / 5.f;
			const FVector Root(-20.f - T * 18.f, Side * (38.f + T * 10.f), 1.f);
			const FVector Tip(-44.f + T * 16.f, Side * (106.f - T * 17.f), -2.f);
			AppendDoubleSidedTriangle(Data, Root, Tip, Root + FVector(7.f, Side * 5.f, 0.f));
		}
	}

	static UMaterialInstanceDynamic* MakeRavenMaterial(UObject* Outer, const FLinearColor& Color)
	{
		static UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
		if (!BasicMaterial)
			BasicMaterial = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UMaterialInstanceDynamic* Material = BasicMaterial ? UMaterialInstanceDynamic::Create(BasicMaterial, Outer) : nullptr;
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		}
		return Material;
	}

	static UProceduralMeshComponent* AddRavenMesh(ACharacter* Raven, USceneComponent* Parent, FName Name,
		FRavenProcMeshData&& Data, UMaterialInterface* Material)
	{
		if (!Raven || !Parent || Data.Vertices.IsEmpty() || Data.Triangles.IsEmpty()) return nullptr;
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Raven, Name, RF_Transient);
		if (!Mesh) return nullptr;
		Raven->AddInstanceComponent(Mesh);
		Mesh->SetupAttachment(Parent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->bUseAsyncCooking = false;
		Mesh->CreateMeshSection_LinearColor(0, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, Data.Colors, Data.Tangents, false);
		if (Material) Mesh->SetMaterial(0, Material);
		Mesh->RegisterComponent();
		return Mesh;
	}

	static void EnsureProceduralRavenAppearance(APawn* Pawn)
	{
		ACharacter* Raven = Cast<ACharacter>(Pawn);
		if (!IsRavenVisualPawn(Pawn) || !Raven || !Raven->GetMesh() ||
			Raven->FindComponentByClass<UProceduralMeshComponent>()) return;

		USkeletalMeshComponent* PlaceholderBody = Raven->GetMesh();
		PlaceholderBody->SetVisibility(false, false);
		PlaceholderBody->SetHiddenInGame(true, false);
		TArray<UStaticMeshComponent*> PlaceholderParts;
		Raven->GetComponents<UStaticMeshComponent>(PlaceholderParts);
		for (UStaticMeshComponent* Part : PlaceholderParts)
			if (Part)
			{
				// This pawn uses the runtime bird silhouette below; hide every old static-mesh
				// placeholder part, not just components whose names happen to identify a wing.
				Part->SetVisibility(false, false);
				Part->SetHiddenInGame(true, false);
			}

		// Prefer the free, rigged Crow when it is present in this checkout. The procedural
		// raven below remains a runtime fallback for projects without the optional pack.
		USkeletalMesh* CrowAsset = LoadObject<USkeletalMesh>(nullptr,
			TEXT("/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow_CaptiveSky.SK_Crow_CaptiveSky"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!CrowAsset)
			CrowAsset = LoadObject<USkeletalMesh>(nullptr,
				TEXT("/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow.SK_Crow"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (CrowAsset)
		{
			USceneComponent* VisualRoot = NewObject<USceneComponent>(Raven, TEXT("RavenRiggedVisualRoot"), RF_Transient);
			Raven->AddInstanceComponent(VisualRoot);
			VisualRoot->SetupAttachment(PlaceholderBody);
			VisualRoot->SetRelativeLocation(-PlaceholderBody->GetRelativeLocation());
			VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
			VisualRoot->SetRelativeScale3D(FVector::OneVector);
			VisualRoot->RegisterComponent();

			USkeletalMeshComponent* CrowBody = NewObject<USkeletalMeshComponent>(Raven, TEXT("RavenRiggedCrowBody"), RF_Transient);
			Raven->AddInstanceComponent(CrowBody);
			CrowBody->SetupAttachment(VisualRoot);
			CrowBody->SetSkeletalMesh(CrowAsset);
			CrowBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			CrowBody->SetCanEverAffectNavigation(false);
			CrowBody->SetGenerateOverlapEvents(false);
			CrowBody->SetCastShadow(true);
			CrowBody->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			const FBoxSphereBounds Bounds = CrowAsset->GetBounds();
			const float HorizontalSpan = 2.f * FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
			const float Scale = HorizontalSpan > 1.f ? FMath::Clamp(112.f / HorizontalSpan, 0.25f, 3.f) : 1.f;
			CrowBody->SetRelativeScale3D(FVector(Scale));
			CrowBody->SetRelativeLocation(FVector(0.f, 0.f, -Bounds.Origin.Z * Scale));
			CrowBody->RegisterComponent();

			// Carrying is a real action state; keep its small visual attached to the head
			// bone when available, but never let it affect movement or navigation.
			FName CarryBone = NAME_None;
			FName HeadBone = NAME_None;
			const FReferenceSkeleton& RefSkeleton = CrowAsset->GetRefSkeleton();
			for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
			{
				const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
				const FString LowerBoneName = BoneName.ToString().ToLower();
				if (HeadBone.IsNone() && LowerBoneName.Contains(TEXT("head"))) HeadBone = BoneName;
				if (LowerBoneName.Contains(TEXT("beak")) || LowerBoneName.Contains(TEXT("bill")))
				{
					CarryBone = BoneName;
					break;
				}
			}
			if (CarryBone.IsNone()) CarryBone = HeadBone;
			UInstancedStaticMeshComponent* Twigs = NewObject<UInstancedStaticMeshComponent>(Raven, TEXT("RavenCarriedTwigs"), RF_Transient);
			Raven->AddInstanceComponent(Twigs);
			Twigs->SetupAttachment(CrowBody, CarryBone);
			Twigs->SetRelativeLocation(CarryBone == HeadBone ? FVector(1.5f, 0.f, -1.f) : FVector(0.5f, 0.f, 0.f));
			if (UStaticMesh* TwigMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
			{
				Twigs->SetStaticMesh(TwigMesh);
				Twigs->SetMaterial(0, MakeRavenMaterial(Raven, FLinearColor(0.16f, 0.075f, 0.025f, 1.f)));
				const FTransform TwigTransforms[] = {
					FTransform(FRotator(82.f, 0.f, -7.f), FVector(17.f, -2.5f, -4.f), FVector(0.025f, 0.025f, 0.18f)),
					FTransform(FRotator(88.f, 5.f, 8.f), FVector(17.f, 0.f, -5.f), FVector(0.025f, 0.025f, 0.18f)),
					FTransform(FRotator(76.f, -4.f, 12.f), FVector(16.f, 2.5f, -3.f), FVector(0.025f, 0.025f, 0.18f))
				};
				for (const FTransform& TwigTransform : TwigTransforms) Twigs->AddInstance(TwigTransform, false);
			}
			Twigs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Twigs->SetCanEverAffectNavigation(false);
			Twigs->SetGenerateOverlapEvents(false);
			Twigs->SetVisibility(false, false);
			Twigs->SetHiddenInGame(true, false);
			Twigs->RegisterComponent();
			return;
		}

		USceneComponent* VisualRoot = NewObject<USceneComponent>(Raven, TEXT("RavenProceduralVisualRoot"), RF_Transient);
		Raven->AddInstanceComponent(VisualRoot);
		VisualRoot->SetupAttachment(PlaceholderBody);
		VisualRoot->SetRelativeLocation(-PlaceholderBody->GetRelativeLocation());
		VisualRoot->SetRelativeRotation(FRotator::ZeroRotator);
		VisualRoot->SetRelativeScale3D(FVector::OneVector);
		VisualRoot->RegisterComponent();

		FRavenProcMeshData Body;
		AppendEllipsoid(Body, FVector(-1.f, 0.f, -2.f), FVector(41.f, 22.f, 23.f));
		AppendEllipsoid(Body, FVector(17.f, 0.f, -1.f), FVector(27.f, 19.f, 23.f));
		AppendEllipsoid(Body, FVector(-39.f, 0.f, -7.f), FVector(21.f, 13.f, 12.f));
		AppendEllipsoid(Body, FVector(2.f, -10.f, -29.f), FVector(4.f, 4.f, 13.f));
		AppendEllipsoid(Body, FVector(2.f, 10.f, -29.f), FVector(4.f, 4.f, 13.f));
		AppendEllipsoid(Body, FVector(7.f, -13.f, -38.f), FVector(11.f, 3.f, 3.f));
		AppendEllipsoid(Body, FVector(7.f, 13.f, -38.f), FVector(11.f, 3.f, 3.f));
		UMaterialInstanceDynamic* BodyMaterial = MakeRavenMaterial(Raven, FLinearColor(0.012f, 0.018f, 0.028f, 1.f));
		AddRavenMesh(Raven, VisualRoot, TEXT("RavenBodyMesh"), MoveTemp(Body), BodyMaterial);

		USceneComponent* HeadPivot = NewObject<USceneComponent>(Raven, TEXT("RavenHeadPivot"), RF_Transient);
		Raven->AddInstanceComponent(HeadPivot);
		HeadPivot->SetupAttachment(VisualRoot);
		HeadPivot->SetRelativeLocation(FVector(34.f, 0.f, 20.f));
		HeadPivot->RegisterComponent();
		FRavenProcMeshData Head;
		AppendEllipsoid(Head, FVector::ZeroVector, FVector(17.f, 15.f, 17.f));
		AddRavenMesh(Raven, HeadPivot, TEXT("RavenHeadMesh"), MoveTemp(Head), BodyMaterial);

		FRavenProcMeshData Beak;
		const FVector BeakBaseA(12.f, -7.f, 3.f), BeakBaseB(12.f, 7.f, 3.f), BeakBaseC(12.f, 6.f, -6.f), BeakBaseD(12.f, -6.f, -6.f);
		const FVector BeakTip(38.f, 0.f, -9.f);
		AppendDoubleSidedTriangle(Beak, BeakBaseA, BeakBaseB, BeakTip);
		AppendDoubleSidedTriangle(Beak, BeakBaseB, BeakBaseC, BeakTip);
		AppendDoubleSidedTriangle(Beak, BeakBaseC, BeakBaseD, BeakTip);
		AppendDoubleSidedTriangle(Beak, BeakBaseD, BeakBaseA, BeakTip);
		AppendDoubleSidedTriangle(Beak, BeakBaseA, BeakBaseD, BeakBaseC);
		AppendDoubleSidedTriangle(Beak, BeakBaseA, BeakBaseC, BeakBaseB);
		AddRavenMesh(Raven, HeadPivot, TEXT("RavenBeakMesh"), MoveTemp(Beak), MakeRavenMaterial(Raven, FLinearColor(0.055f, 0.065f, 0.08f, 1.f)));

		FRavenProcMeshData Eyes;
		AppendEllipsoid(Eyes, FVector(6.f, -13.4f, 2.f), FVector(4.f, 2.1f, 4.f), 10, 6);
		AppendEllipsoid(Eyes, FVector(6.f, 13.4f, 2.f), FVector(4.f, 2.1f, 4.f), 10, 6);
		AddRavenMesh(Raven, HeadPivot, TEXT("RavenEyeMesh"), MoveTemp(Eyes), MakeRavenMaterial(Raven, FLinearColor(0.5f, 0.31f, 0.08f, 1.f)));

		FRavenProcMeshData Pupils;
		AppendEllipsoid(Pupils, FVector(7.5f, -15.1f, 2.5f), FVector(2.f, 1.f, 2.2f), 8, 5);
		AppendEllipsoid(Pupils, FVector(7.5f, 15.1f, 2.5f), FVector(2.f, 1.f, 2.2f), 8, 5);
		AddRavenMesh(Raven, HeadPivot, TEXT("RavenPupilMesh"), MoveTemp(Pupils), MakeRavenMaterial(Raven, FLinearColor(0.003f, 0.004f, 0.006f, 1.f)));

		// A small warm-brown bundle sits at the bill while the existing forage state says
		// twigs are carried. It is presentation-only and never affects collision or nav.
		UStaticMesh* TwigMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (TwigMesh)
		{
			UInstancedStaticMeshComponent* Twigs = NewObject<UInstancedStaticMeshComponent>(Raven, TEXT("RavenCarriedTwigs"), RF_Transient);
			Raven->AddInstanceComponent(Twigs);
			Twigs->SetupAttachment(HeadPivot);
			Twigs->SetStaticMesh(TwigMesh);
			Twigs->SetMaterial(0, MakeRavenMaterial(Raven, FLinearColor(0.16f, 0.075f, 0.025f, 1.f)));
			Twigs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Twigs->SetCanEverAffectNavigation(false);
			Twigs->SetGenerateOverlapEvents(false);
			const FTransform TwigTransforms[] = {
				FTransform(
					FQuat::FindBetweenNormals(FVector::UpVector, FVector(1.f, -0.10f, 0.13f).GetSafeNormal()),
					FVector(50.f, -2.5f, -8.f), FVector(0.034f, 0.034f, 0.24f)),
				FTransform(
					FQuat::FindBetweenNormals(FVector::UpVector, FVector(1.f, 0.08f, -0.12f).GetSafeNormal()),
					FVector(49.f, 0.f, -10.f), FVector(0.034f, 0.034f, 0.24f)),
				FTransform(
					FQuat::FindBetweenNormals(FVector::UpVector, FVector(1.f, 0.15f, 0.04f).GetSafeNormal()),
					FVector(48.f, 2.5f, -7.f), FVector(0.034f, 0.034f, 0.24f))
			};
			for (const FTransform& TwigTransform : TwigTransforms) Twigs->AddInstance(TwigTransform, false);
			Twigs->SetVisibility(false, false);
			Twigs->SetHiddenInGame(true, false);
			Twigs->RegisterComponent();
		}

		for (const float Side : { -1.f, 1.f })
		{
			USceneComponent* WingPivot = NewObject<USceneComponent>(Raven,
				Side < 0.f ? TEXT("RavenLeftWingPivot") : TEXT("RavenRightWingPivot"), RF_Transient);
			Raven->AddInstanceComponent(WingPivot);
			WingPivot->SetupAttachment(VisualRoot);
			WingPivot->SetRelativeLocation(FVector(1.f, Side * 15.f, 7.f));
			WingPivot->SetRelativeRotation(FRotator(0.f, Side * 104.f, 0.f));
			WingPivot->RegisterComponent();
			FRavenProcMeshData Wing;
			AppendRavenWing(Wing, Side);
			AddRavenMesh(Raven, WingPivot, Side < 0.f ? TEXT("RavenLeftWingFeathers") : TEXT("RavenRightWingFeathers"),
				MoveTemp(Wing), MakeRavenMaterial(Raven, FLinearColor(0.018f, 0.029f, 0.048f, 1.f)));
		}
	}
}

ARavenAgentAIController::ARavenAgentAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

bool ARavenAgentAIController::IsActionInProgress() const
{
	return bHasMovementTarget || LocomotionState == ERavenLocomotionState::Hopping;
}
bool ARavenAgentAIController::CanRest() const
{
	return !bHasMovementTarget && (LocomotionState == ERavenLocomotionState::Perched || (LocomotionState == ERavenLocomotionState::Grounded && Super::CanRest()));
}

void ARavenAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EnsureProceduralRavenAppearance(InPawn);
	CacheCrowAnimations(InPawn);
	CacheWingComponents(InPawn);
	UE_LOG(LogRavenAgentAI, Display, TEXT("Raven appearance on %s: mesh=%s material=%s visible=%s"),
		InPawn ? *InPawn->GetName() : TEXT("<none>"), *GetPathNameSafe(RiggedCrowBody.IsValid() ? RiggedCrowBody->GetSkeletalMeshAsset() : nullptr),
		*GetPathNameSafe(RiggedCrowBody.IsValid() ? RiggedCrowBody->GetMaterial(0) : nullptr),
		RiggedCrowBody.IsValid() && RiggedCrowBody->IsVisible() && !RiggedCrowBody->bHiddenInGame ? TEXT("yes") : TEXT("no"));
	if (AAutonomousAgentCharacter* Agent = Cast<AAutonomousAgentCharacter>(InPawn))
		if (Agent->RestPresentation) Agent->RestPresentation->SetRestPosture(EAgentRestPosture::PerchedBird);
	HomeAltitude = InPawn ? InPawn->GetActorLocation().Z + TakeoffHeight : 0.f;
	SetGrounded();
}

void ARavenAgentAIController::CacheCrowAnimations(APawn* Raven)
{
	RiggedCrowBody.Reset();
	CrowIdleAnimation = nullptr;
	CrowHopAnimation = nullptr;
	CrowTakeoffAnimation = nullptr;
	CrowFlyAnimation = nullptr;
	CrowLandingAnimation = nullptr;
	CurrentCrowAnimation = nullptr;
	if (!Raven) return;
	TArray<USkeletalMeshComponent*> Meshes;
	Raven->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
		if (Mesh && Mesh->GetFName() == TEXT("RavenRiggedCrowBody")) { RiggedCrowBody = Mesh; break; }
	if (!RiggedCrowBody.IsValid()) return;
	CrowIdleAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimalVarietyPack/Crow/Animations/ANIM_Crow_IdleLookAround.ANIM_Crow_IdleLookAround"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	CrowHopAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimalVarietyPack/Crow/Animations/ANIM_Crow_Hop.ANIM_Crow_Hop"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	CrowTakeoffAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimalVarietyPack/Crow/Animations/ANIM_Crow_TakeOff.ANIM_Crow_TakeOff"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	CrowFlyAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimalVarietyPack/Crow/Animations/ANIM_Crow_Fly.ANIM_Crow_Fly"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	CrowLandingAnimation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/AnimalVarietyPack/Crow/Animations/ANIM_Crow_Landing.ANIM_Crow_Landing"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UpdateCrowAnimation();
}

void ARavenAgentAIController::UpdateCrowAnimation()
{
	USkeletalMeshComponent* Crow = RiggedCrowBody.Get();
	if (!Crow) return;
	UAnimSequence* Desired = CrowIdleAnimation;
	bool bLoop = true;
	switch (LocomotionState)
	{
	case ERavenLocomotionState::Hopping:
		if (CrowHopAnimation) { Desired = CrowHopAnimation; bLoop = false; }
		break;
	case ERavenLocomotionState::TakingOff:
		if (CrowTakeoffAnimation) { Desired = CrowTakeoffAnimation; bLoop = false; }
		break;
	case ERavenLocomotionState::Flying:
		if (CrowFlyAnimation) Desired = CrowFlyAnimation;
		break;
	case ERavenLocomotionState::Landing:
		if (CrowLandingAnimation) { Desired = CrowLandingAnimation; bLoop = false; }
		break;
	default:
		break;
	}
	if (Desired && CurrentCrowAnimation != Desired)
	{
		CurrentCrowAnimation = Desired;
		Crow->PlayAnimation(Desired, bLoop);
	}
}

void ARavenAgentAIController::CacheWingComponents(APawn* Raven)
{
	LeftWing.Reset();
	RightWing.Reset();
	LeftWingRestRotation = FRotator::ZeroRotator;
	RightWingRestRotation = FRotator::ZeroRotator;
	LeftWingFlightRotation = FRotator::ZeroRotator;
	RightWingFlightRotation = FRotator::ZeroRotator;
	RavenHeadPivot.Reset();
	CarriedTwigVisual.Reset();
	RavenHeadRestRotation = FRotator::ZeroRotator;
	HeadScanTime = 0.f;
	WingDeployment = 0.f;
	WingAnimationTime = 0.f;
	if (!Raven) return;

	TArray<USceneComponent*> Components;
	Raven->GetComponents<USceneComponent>(Components);
	for (USceneComponent* Component : Components)
	{
		if (!Component) continue;
		const FString ComponentName = Component->GetName();
		if (ComponentName == TEXT("RavenHeadPivot"))
		{
			RavenHeadPivot = Component;
			RavenHeadRestRotation = Component->GetRelativeRotation();
		}
		else if (ComponentName == TEXT("RavenCarriedTwigs"))
		{
			CarriedTwigVisual = Cast<UInstancedStaticMeshComponent>(Component);
		}
		else if (ComponentName == TEXT("RavenLeftWingPivot"))
		{
			LeftWing = Component;
			LeftWingRestRotation = Component->GetRelativeRotation();
			LeftWingFlightRotation = LeftWingRestRotation;
			LeftWingFlightRotation.Yaw = 0.f;
		}
		else if (ComponentName == TEXT("RavenRightWingPivot"))
		{
			RightWing = Component;
			RightWingRestRotation = Component->GetRelativeRotation();
			RightWingFlightRotation = RightWingRestRotation;
			RightWingFlightRotation.Yaw = 0.f;
		}
	}
	for (USceneComponent* Component : Components)
	{
		if (!Component) continue;
		const FString ComponentName = Component->GetName();
		if (!LeftWing.IsValid() && ComponentName.Contains(TEXT("LeftWing"), ESearchCase::IgnoreCase))
		{
			LeftWing = Component;
			LeftWingRestRotation = Component->GetRelativeRotation();
			LeftWingFlightRotation = LeftWingRestRotation;
		}
		else if (!RightWing.IsValid() && ComponentName.Contains(TEXT("RightWing"), ESearchCase::IgnoreCase))
		{
			RightWing = Component;
			RightWingRestRotation = Component->GetRelativeRotation();
			RightWingFlightRotation = RightWingRestRotation;
		}
	}
}

void ARavenAgentAIController::UpdateWingAnimation(float DeltaSeconds)
{
	USceneComponent* Left = Cast<USceneComponent>(LeftWing.Get());
	USceneComponent* Right = Cast<USceneComponent>(RightWing.Get());
	if (!Left || !Right)
	{
		if (Left) Left->SetRelativeRotation(LeftWingRestRotation);
		if (Right) Right->SetRelativeRotation(RightWingRestRotation);
		return;
	}

	float TargetDeployment = 0.f;
	float AmplitudeDegrees = 0.f;
	float FrequencyHz = 0.f;
	switch (LocomotionState)
	{
	case ERavenLocomotionState::Hopping:
		TargetDeployment = 0.65f;
		AmplitudeDegrees = 16.f;
		FrequencyHz = 2.4f;
		break;
	case ERavenLocomotionState::TakingOff:
		TargetDeployment = 1.f;
		AmplitudeDegrees = 46.f;
		FrequencyHz = 4.2f;
		break;
	case ERavenLocomotionState::Flying:
		TargetDeployment = 1.f;
		AmplitudeDegrees = 34.f;
		FrequencyHz = 3.8f;
		break;
	case ERavenLocomotionState::Landing:
		TargetDeployment = 1.f;
		AmplitudeDegrees = 20.f;
		FrequencyHz = 2.6f;
		break;
	default:
		break;
	}

	if (IsResting()) TargetDeployment = 0.f;
	WingDeployment = FMath::FInterpConstantTo(WingDeployment, TargetDeployment, FMath::Max(0.f, DeltaSeconds), 10.f);
	const bool bIsFlapping = FrequencyHz > 0.f && !IsResting();
	if (bIsFlapping)
	{
		WingAnimationTime += FMath::Max(0.f, DeltaSeconds);
	}
	else WingAnimationTime = 0.f;

	const float FlapDegrees = bIsFlapping
		? FMath::Sin(WingAnimationTime * 2.f * PI * FrequencyHz) * AmplitudeDegrees
		: 0.f;
	const FRotator LeftBase = FMath::Lerp(LeftWingRestRotation, LeftWingFlightRotation, WingDeployment);
	const FRotator RightBase = FMath::Lerp(RightWingRestRotation, RightWingFlightRotation, WingDeployment);
	// The procedural feathers extend in opposite local Y directions. Deployment
	// first unfolds both wings; mirrored roll then drives their up/down flight stroke.
	Left->SetRelativeRotation(LeftBase + FRotator(0.f, 0.f, FlapDegrees));
	Right->SetRelativeRotation(RightBase + FRotator(0.f, 0.f, -FlapDegrees));
}

void ARavenAgentAIController::UpdateHeadAnimation(float DeltaSeconds)
{
	USceneComponent* Head = Cast<USceneComponent>(RavenHeadPivot.Get());
	if (!Head) return;
	const bool bCanScan = (LocomotionState == ERavenLocomotionState::Grounded || LocomotionState == ERavenLocomotionState::Perched) && !IsResting();
	if (!bCanScan)
	{
		Head->SetRelativeRotation(RavenHeadRestRotation);
		return;
	}

	HeadScanTime = FMath::Fmod(HeadScanTime + FMath::Max(0.f, DeltaSeconds), 100.f);
	const float YawDegrees = 7.f * FMath::Sin(HeadScanTime * 2.f * PI * 0.13f);
	const float PitchDegrees = 2.5f * FMath::Sin(HeadScanTime * 2.f * PI * 0.09f + 1.2f);
	const FRotator IdlePose = RavenHeadRestRotation + FRotator(PitchDegrees, YawDegrees, 0.f);
	if (ListeningStoneAttentionRemaining > 0.f)
	{
		if (const APawn* Raven = GetPawn())
		{
			const FVector LocalDirection = Raven->GetActorTransform().InverseTransformVectorNoScale(
				ListeningChimeLocation - Raven->GetActorLocation()).GetSafeNormal();
			FRotator FocusOffset = LocalDirection.Rotation();
			FocusOffset.Pitch = FMath::Clamp(FocusOffset.Pitch, -12.f, 12.f);
			FocusOffset.Yaw = FMath::ClampAngle(FocusOffset.Yaw, -22.f, 22.f);
			const float Elapsed = ListeningStoneAttentionDuration - ListeningStoneAttentionRemaining;
			const float AttentionAlpha = FMath::SmoothStep(0.f, 0.3f, Elapsed) *
				(1.f - FMath::SmoothStep(2.0f, ListeningStoneAttentionDuration, Elapsed));
			const FRotator FocusPose = IdlePose + FocusOffset * AttentionAlpha;
			Head->SetRelativeRotation(FMath::RInterpTo(Head->GetRelativeRotation(), FocusPose,
				FMath::Max(0.f, DeltaSeconds), 7.f));
			return;
		}
	}
	Head->SetRelativeRotation(FMath::RInterpTo(Head->GetRelativeRotation(), IdlePose,
		FMath::Max(0.f, DeltaSeconds), 7.f));
}

void ARavenAgentAIController::CheckForNearbyListeningStoneChime()
{
	const APawn* Raven = GetPawn();
	if (!GetWorld() || !Raven || IsResting() ||
		(LocomotionState != ERavenLocomotionState::Grounded && LocomotionState != ERavenLocomotionState::Perched))
		return;

	for (TActorIterator<AIslandListeningStonesChime> It(GetWorld()); It; ++It)
	{
		AIslandListeningStonesChime* Chime = *It;
		if (!IsValid(Chime) || LastNoticedListeningChime.Get() == Chime ||
			Chime->DescribeForListener(Raven->GetActorLocation()).IsEmpty())
			continue;

		LastNoticedListeningChime = Chime;
		ListeningChimeLocation = Chime->GetActorLocation();
		ListeningStoneAttentionRemaining = ListeningStoneAttentionDuration;
		return;
	}
}

void ARavenAgentAIController::UpdateCarriedTwigVisual()
{
	if (UInstancedStaticMeshComponent* Twigs = CarriedTwigVisual.Get())
	{
		Twigs->SetVisibility(bCarryingTwigs, true);
		Twigs->SetHiddenInGame(!bCarryingTwigs, true);
	}
}

void ARavenAgentAIController::SetFlyingMovement(bool bFlying) const
{
	if (const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn()))
	{
		UCharacterMovementComponent* Movement = RavenCharacter->GetCharacterMovement();
		Movement->StopMovementImmediately();
		Movement->GravityScale = bFlying ? 0.f : 1.f;
		Movement->SetMovementMode(bFlying ? MOVE_Flying : MOVE_Walking);
		FRotator Facing = RavenCharacter->GetActorRotation();
		Facing.Pitch = Facing.Roll = 0.f;
		GetPawn()->SetActorRotation(Facing);
	}
}

FVector ARavenAgentAIController::SelectWanderCruiseTarget(const FVector& Origin, const TArray<FVector>& RandomCandidates,
	const TArray<FVector>& VisibleLandmarks, int32 RandomFallbackIndex, bool bApplyCuriosityBias)
{
	if (RandomCandidates.IsEmpty()) return Origin;
	const int32 SafeFallbackIndex = FMath::Clamp(RandomFallbackIndex, 0, RandomCandidates.Num() - 1);
	if (!bApplyCuriosityBias || VisibleLandmarks.IsEmpty()) return RandomCandidates[SafeFallbackIndex];

	int32 BestCandidateIndex = SafeFallbackIndex;
	float BestScore = AAutonomousAgentAIController::WanderLandmarkProgressScore(
		RandomCandidates[BestCandidateIndex], Origin, VisibleLandmarks);
	for (int32 CandidateIndex = 0; CandidateIndex < RandomCandidates.Num(); ++CandidateIndex)
	{
		const float Score = AAutonomousAgentAIController::WanderLandmarkProgressScore(
			RandomCandidates[CandidateIndex], Origin, VisibleLandmarks);
		if (Score > BestScore)
		{
			BestCandidateIndex = CandidateIndex;
			BestScore = Score;
		}
	}
	return RandomCandidates[BestCandidateIndex];
}

FVector ARavenAgentAIController::MakeCruiseTarget(bool bForceCuriosityForProbe) const
{
	const FVector Origin = GetPawn()->GetActorLocation();
	const float CruiseAltitude = FMath::Clamp(HomeAltitude + FMath::FRandRange(-VerticalRange, VerticalRange),
		HomeAltitude - 100.f, HomeAltitude + VerticalRange);
	const auto RandomCruisePoint = [this, &Origin, CruiseAltitude]()
	{
		const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
		return FVector(Origin.X + Offset.X, Origin.Y + Offset.Y, CruiseAltitude);
	};

	TArray<FVector> VisibleLandmarks;
	TArray<FName> VisibleLandmarkNames;
	if (UWorld* World = GetWorld())
	{
		VisibleLandmarks.Reserve(6);
		VisibleLandmarkNames.Reserve(6);
		const double CuriosityNow = FPlatformTime::Seconds();
		for (TActorIterator<AActor> It(World); It && VisibleLandmarks.Num() < 6; ++It)
		{
			if (!CanFollowWanderCuriosityToward(*It, CuriosityNow) ||
				FVector::DistSquared(Origin, It->GetActorLocation()) > FMath::Square(5000.f)) continue;

			FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(RavenWanderLandmarkVisibility), false, GetPawn());
			VisibilityParams.AddIgnoredActor(*It);
			FHitResult VisibilityHit;
			if (World->LineTraceSingleByChannel(VisibilityHit, Origin, It->GetActorLocation(), ECC_Visibility, VisibilityParams)) continue;
			VisibleLandmarks.Add(It->GetActorLocation());
			FName LandmarkName = It->GetFName();
			for (const FName& Tag : It->Tags)
				if (Tag != FName(TEXT("IslandLandmark"))) { LandmarkName = Tag; break; }
			VisibleLandmarkNames.Add(LandmarkName);
		}
	}

	const FVector RandomFallback = RandomCruisePoint();
	if (VisibleLandmarks.IsEmpty())
	{
		if (bForceCuriosityForProbe)
			UE_LOG(LogRavenAgentAI, Warning, TEXT("Forced curiosity probe found no visible IslandLandmark within 5000 cm of %s."), *Origin.ToCompactString());
		return RandomFallback;
	}
	if (!bForceCuriosityForProbe && FMath::FRand() >= WanderLandmarkCuriosityChance) return RandomFallback;

	TArray<FVector> Candidates;
	Candidates.Reserve(12);
	Candidates.Add(RandomFallback);
	for (int32 CandidateIndex = 1; CandidateIndex < 12; ++CandidateIndex) Candidates.Add(RandomCruisePoint());
	const FVector CuriousTarget = SelectWanderCruiseTarget(Origin, Candidates, VisibleLandmarks, 0, true);
	float BestProgress = 0.f;
	FName BestLandmark = NAME_None;
	for (int32 LandmarkIndex = 0; LandmarkIndex < VisibleLandmarks.Num(); ++LandmarkIndex)
	{
		const float Progress = FVector::Dist2D(Origin, VisibleLandmarks[LandmarkIndex]) -
			FVector::Dist2D(CuriousTarget, VisibleLandmarks[LandmarkIndex]);
		if (Progress > BestProgress)
		{
			BestProgress = Progress;
			BestLandmark = VisibleLandmarkNames[LandmarkIndex];
		}
	}
	if (BestProgress > 0.f)
	{
		UE_LOG(LogRavenAgentAI, Log, TEXT("Raven chose a curious flight-wander target %s, gaining %.0f cm toward visible landmark %s (%d visible nearby)."),
			*CuriousTarget.ToCompactString(), BestProgress, *BestLandmark.ToString(), VisibleLandmarks.Num());
		return CuriousTarget;
	}
	if (bForceCuriosityForProbe)
		UE_LOG(LogRavenAgentAI, Warning, TEXT("Forced curiosity probe found %d visible landmarks, but none of twelve cruise candidates made progress from %s."),
			VisibleLandmarks.Num(), *Origin.ToCompactString());
	return RandomFallback;
}

void ARavenAgentAIController::BeginTakeoff(const FVector& Destination)
{
	APawn* Raven = GetPawn();
	if (!Raven) return;
	CruiseTarget = Destination;
	bApproachingPerch = false;
	FlightWaypoints.Reset();
	bHasTakeoffEscapeTarget = false;
	const FVector Origin = Raven->GetActorLocation();
	MovementTarget = Origin + FVector(0.f, 0.f, TakeoffHeight);
	const ACharacter* RavenCharacter = Cast<ACharacter>(Raven);
	const float Radius = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 30.f;
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenTakeoffExit), false, Raven);
	FHitResult Hit;
	const bool bVerticalExitBlocked = GetWorld() &&
		GetWorld()->SweepSingleByChannel(Hit, Origin, MovementTarget, FQuat::Identity, ECC_WorldStatic, Body, Query);
	if (bVerticalExitBlocked)
	{
		// A perch under a branch or roof may have no vertical exit. Search for a
		// collision-clear lateral move beneath the obstruction, then climb at its edge.
		const float SearchStep = FMath::Max(90.f, Radius + 50.f);
		FVector2D PreferredDirection(Destination.X - Origin.X, Destination.Y - Origin.Y);
		PreferredDirection = PreferredDirection.GetSafeNormal();
		if (PreferredDirection.IsNearlyZero()) PreferredDirection = FVector2D(1.f, 0.f);
		const float PreferredAngle = FMath::Atan2(PreferredDirection.Y, PreferredDirection.X);
		static constexpr int32 AngleOffsetsDegrees[] = { 0, 45, -45, 90, -90, 135, -135, 180 };
		for (float Distance = SearchStep; Distance <= 700.f && !bHasTakeoffEscapeTarget; Distance += SearchStep)
		{
			for (const int32 OffsetDegrees : AngleOffsetsDegrees)
			{
				const float Angle = PreferredAngle + FMath::DegreesToRadians(static_cast<float>(OffsetDegrees));
				const FVector Side = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;
				const FVector SideUp = Side + FVector(0.f, 0.f, TakeoffHeight);
				if (GetWorld()->SweepSingleByChannel(Hit, Origin, Side, FQuat::Identity, ECC_WorldStatic, Body, Query)) continue;
				if (GetWorld()->SweepSingleByChannel(Hit, Side, SideUp, FQuat::Identity, ECC_WorldStatic, Body, Query)) continue;
				MovementTarget = Side;
				TakeoffEscapeTarget = SideUp;
				bHasTakeoffEscapeTarget = true;
				break;
			}
		}
	}
	bHasMovementTarget = true;
	bTargetIsPerch = false;
	LocomotionState = ERavenLocomotionState::TakingOff;
	SetFlyingMovement(true);
}

bool ARavenAgentAIController::TraceGround(const FVector& DesiredLocation, FVector& OutGroundLocation) const
{
	FHitResult Hit;
	const FVector Start(DesiredLocation.X, DesiredLocation.Y, DesiredLocation.Z + 1000.f);
	const FVector End(DesiredLocation.X, DesiredLocation.Y, DesiredLocation.Z - 5000.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenGround), false, GetPawn());
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query) || Hit.ImpactNormal.Z < 0.7f) return false;
	float HalfHeight = 45.f;
	if (const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn()))
		HalfHeight = RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	OutGroundLocation = Hit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
	return true;
}

bool ARavenAgentAIController::BeginLanding(const FVector& DesiredLocation)
{
	if (!TraceGround(DesiredLocation, MovementTarget)) return false;
	bHasMovementTarget = true;
	bTargetIsPerch = false;
	LocomotionState = ERavenLocomotionState::Landing;
	bApproachingPerch = false;
	SetFlyingMovement(true);
	return true;
}

void ARavenAgentAIController::BeginGroundLandingAt(FName SiteTag)
{
	if (LocomotionState == ERavenLocomotionState::Grounded)
	{
		ReportAction(TEXT("Already grounded; you may gather fallen twigs here if you choose."));
		return;
	}
	if (bHasMovementTarget || LocomotionState == ERavenLocomotionState::TakingOff || LocomotionState == ERavenLocomotionState::Landing || LocomotionState == ERavenLocomotionState::Hopping)
	{
		ReportAction(TEXT("Finish the current movement before choosing a landing site."));
		return;
	}
	const UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
	if (!WorldState || !SiteTag.ToString().StartsWith(TEXT("ArrangingGround_")))
	{
		ReportAction(TEXT("Landing needs an exact listed ArrangingGround target; no lasting change occurred."));
		return;
	}
	const FIslandArrangementSite* Site = nullptr;
	int32 VisibleSites = 0;
	for (const FIslandArrangementSite& Candidate : WorldState->GetArrangementSites())
	{
		if (VisibleSites >= 3) break;
		const FVector View = Candidate.Location + FVector(0.f, 0.f, 30.f);
		if (FVector::DistSquared(GetPawn()->GetActorLocation(), View) > FMath::Square(GroundLandingVisibilityRange)) continue;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenLandingSiteVisibility), false, GetPawn());
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetPawn()->GetActorLocation(), View, ECC_Visibility, Query)) continue;
		++VisibleSites;
		if (Candidate.Id == SiteTag) { Site = &Candidate; break; }
	}
	if (!Site)
	{
		ReportAction(TEXT("That exact open-ground target is not currently visible. Choose one of the listed sites."));
		return;
	}

	ArrangementLandingTarget = Site->Location;
	bLandingAtArrangementSite = true;
	const FVector FlightTarget = ArrangementLandingTarget + FVector(0.f, 0.f, 180.f);
	if (LocomotionState == ERavenLocomotionState::Perched)
	{
		BeginTakeoff(FlightTarget);
	}
	else
	{
		FlightWaypoints.Reset();
		CruiseTarget = FlightTarget;
		MovementTarget = PlanFlightLeg(GetPawn()->GetActorLocation(), FlightTarget);
		bHasMovementTarget = true;
		bApproachingPerch = false;
		bTargetIsPerch = false;
		LocomotionState = ERavenLocomotionState::Flying;
		SetFlyingMovement(true);
	}
	ReportAction(TEXT("Flight to the listed open-ground site started; the descent will be confirmed at arrival."));
}

void ARavenAgentAIController::BeginHop()
{
	APawn* Raven = GetPawn();
	HopStart = Raven->GetActorLocation();
	const FVector2D Offset = FMath::RandPointInCircle(HopDistance);
	FVector Desired = HopStart + FVector(Offset.X, Offset.Y, 0.f);
	if (!TraceGround(Desired, HopEnd)) HopEnd = Desired;
	HopElapsed = 0.f;
	LocomotionState = ERavenLocomotionState::Hopping;
	SetFlyingMovement(true);
}

bool ARavenAgentAIController::BeginPerch()
{
	TArray<AActor*> CandidatePerches;
	TArray<FVector> CandidateLocations;
	TArray<float> CandidateWindSpeeds;
	TArray<int32> CandidateOverheadCoverProbeCounts;
	AIslandWeather* Weather = nullptr;
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("RavenPerch")) || !HasSuitablePerchSupport(*It)) continue;
		CandidatePerches.Add(*It);
		CandidateLocations.Add(It->GetActorLocation());
		CandidateWindSpeeds.Add(Weather ? Weather->GetLocalWind(It->GetActorLocation(), GetPawn()).Size() : 0.f);
		CandidateOverheadCoverProbeCounts.Add(CountOverheadCoverProbes(*It));
	}
	const float CurrentWindSpeed = Weather ? Weather->GetLocalWind(GetPawn()->GetActorLocation(), GetPawn()).Size() : 0.f;
	const float RainIntensity = Weather ? Weather->SampleRainIntensity(GetWorld()->GetTimeSeconds()) : 0.f;
	const int32 PreferredIndex = SelectWeatherAwarePerch(GetPawn()->GetActorLocation(), CurrentWindSpeed, RainIntensity,
		CandidateLocations, CandidateWindSpeeds, CandidateOverheadCoverProbeCounts);
	AActor* BestPerch = CandidatePerches.IsValidIndex(PreferredIndex) ? CandidatePerches[PreferredIndex] : nullptr;
	return BeginPerchAt(BestPerch);
}

int32 ARavenAgentAIController::SelectWindAwarePerch(const FVector& Origin, float CurrentWindSpeed,
	const TArray<FVector>& PerchLocations, const TArray<float>& PerchWindSpeeds)
{
	if (PerchLocations.IsEmpty() || PerchLocations.Num() != PerchWindSpeeds.Num()) return INDEX_NONE;

	int32 NearestIndex = INDEX_NONE;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < PerchLocations.Num(); ++Index)
	{
		const float DistanceSquared = FVector::DistSquared(Origin, PerchLocations[Index]);
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestIndex = Index;
		}
	}

	// In light air, preserve the ordinary nearest-roost choice. In stronger wind,
	// trade travel distance against the wind measured at each physically supported site.
	static constexpr float StrongWindThreshold = 85.f;
	static constexpr float TravelCostPerCentimeter = 0.008f;
	static constexpr float MinimumUsefulShelterGain = 15.f;
	if (CurrentWindSpeed < StrongWindThreshold) return NearestIndex;

	int32 CalmestIndex = NearestIndex;
	float NearestScore = PerchWindSpeeds[NearestIndex] + FMath::Sqrt(NearestDistanceSquared) * TravelCostPerCentimeter;
	float CalmestScore = NearestScore;
	for (int32 Index = 0; Index < PerchLocations.Num(); ++Index)
	{
		const float Distance = FVector::Distance(Origin, PerchLocations[Index]);
		const float Score = PerchWindSpeeds[Index] + Distance * TravelCostPerCentimeter;
		if (Score < CalmestScore)
		{
			CalmestScore = Score;
			CalmestIndex = Index;
		}
	}
	return CalmestScore + MinimumUsefulShelterGain < NearestScore ? CalmestIndex : NearestIndex;
}

int32 ARavenAgentAIController::SelectWeatherAwarePerch(const FVector& Origin, float CurrentWindSpeed, float RainIntensity,
	const TArray<FVector>& PerchLocations, const TArray<float>& PerchWindSpeeds,
	const TArray<int32>& OverheadCoverProbeCounts)
{
	if (PerchLocations.IsEmpty() || PerchLocations.Num() != PerchWindSpeeds.Num()) return INDEX_NONE;
	const int32 WindPreferredIndex = SelectWindAwarePerch(Origin, CurrentWindSpeed, PerchLocations, PerchWindSpeeds);
	if (WindPreferredIndex == INDEX_NONE || PerchLocations.Num() != OverheadCoverProbeCounts.Num() ||
		RainIntensity < 0.55f)
		return WindPreferredIndex;

	// A short overhead line-of-sight sample is only a local cover clue. In a strong shower,
	// use it only when the alternative has at least two more hits (40% more sampled points),
	// then let the existing wind/distance rule choose among similarly covered sites.
	const int32 MinimumUsefulCoverGain = 2;
	int32 BestCoverCount = OverheadCoverProbeCounts[WindPreferredIndex];
	for (const int32 Count : OverheadCoverProbeCounts) BestCoverCount = FMath::Max(BestCoverCount, Count);
	if (BestCoverCount - OverheadCoverProbeCounts[WindPreferredIndex] < MinimumUsefulCoverGain)
		return WindPreferredIndex;

	TArray<FVector> BetterCoveredLocations;
	TArray<float> BetterCoveredWindSpeeds;
	TArray<int32> OriginalIndices;
	for (int32 Index = 0; Index < PerchLocations.Num(); ++Index)
	{
		if (OverheadCoverProbeCounts[Index] < BestCoverCount - 1) continue;
		BetterCoveredLocations.Add(PerchLocations[Index]);
		BetterCoveredWindSpeeds.Add(PerchWindSpeeds[Index]);
		OriginalIndices.Add(Index);
	}
	const int32 CoveredChoice = SelectWindAwarePerch(Origin, CurrentWindSpeed,
		BetterCoveredLocations, BetterCoveredWindSpeeds);
	return OriginalIndices.IsValidIndex(CoveredChoice) ? OriginalIndices[CoveredChoice] : WindPreferredIndex;
}

bool ARavenAgentAIController::HasSuitablePerchSupport(const AActor* Site, FHitResult* OutSupport) const
{
	if (!Site || !GetWorld() || !GetPawn()) return false;
	const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn());
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenRoostAssessment), false, GetPawn());
	Query.AddIgnoredActor(Site);
	FHitResult Support;
	const FVector SiteLocation = Site->GetActorLocation();
	const bool bSupportHit = GetWorld()->LineTraceSingleByChannel(Support, SiteLocation,
		SiteLocation - FVector(0.f, 0.f, HalfHeight + 12.f), ECC_Visibility, Query);
	if (OutSupport) *OutSupport = Support;
	return bSupportHit && Support.ImpactNormal.Z >= 0.5f;
}

bool ARavenAgentAIController::RequestPerch(FName PerchTag)
{
	if (!GetPawn() || PerchTag.IsNone()) return false;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->ActorHasTag(PerchTag) && It->ActorHasTag(TEXT("RavenPerch"))) return BeginPerchAt(*It);
	return false;
}

FString ARavenAgentAIController::AssessRoostSite(const AActor* Site) const
{
	if (!Site || !GetWorld() || !GetPawn()) return TEXT("Roost conditions cannot be assessed without a visible site and embodied raven.");
	FHitResult Support;
	const bool bHasSuitableSupport = HasSuitablePerchSupport(Site, &Support);

	const int32 OverheadBlockCount = CountOverheadCoverProbes(Site);

	return FString::Printf(TEXT("Read-only site check: %s. %d of 5 short vertical visibility probes above the raven's head found solid overhead geometry or crossed a conservative mature-spruce upper-crown envelope; this is only a local rain-cover clue, not proof of waterproof shelter. The canopy envelope is approximate and does not establish branch strength, nest suitability, ownership, or a home. A physical perch approach must still confirm upward-facing support at arrival."),
		bHasSuitableSupport ? TEXT("an upward-facing support surface is currently beneath the marker") : TEXT("suitable upward-facing support was not confirmed beneath the marker"),
		OverheadBlockCount);
}

int32 ARavenAgentAIController::CountOverheadCoverProbes(const AActor* Site) const
{
	if (!Site || !GetWorld() || !GetPawn()) return 0;
	const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn());
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	const float CapsuleRadius = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 30.f;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenRoostOverheadCover), false, GetPawn());
	Query.AddIgnoredActor(Site);
	const float ProbeSpread = FMath::Min(CapsuleRadius * 0.65f, 30.f);
	const FVector HeadHeight = Site->GetActorLocation() + FVector(0.f, 0.f, HalfHeight + 5.f);
	const FVector ProbeOffsets[] = {
		FVector::ZeroVector,
		FVector(ProbeSpread, 0.f, 0.f), FVector(-ProbeSpread, 0.f, 0.f),
		FVector(0.f, ProbeSpread, 0.f), FVector(0.f, -ProbeSpread, 0.f)
	};
	int32 OverheadBlockCount = 0;
	TArray<FVector> UncoveredStarts;
	TArray<FVector> UncoveredEnds;
	for (const FVector& Offset : ProbeOffsets)
	{
		const FVector ProbeStart = HeadHeight + Offset;
		const FVector ProbeEnd = ProbeStart + FVector(0.f, 0.f, 300.f);
		FHitResult Overhead;
		if (GetWorld()->LineTraceSingleByChannel(Overhead, ProbeStart, ProbeEnd, ECC_Visibility, Query))
			++OverheadBlockCount;
		else
		{
			UncoveredStarts.Add(ProbeStart);
			UncoveredEnds.Add(ProbeEnd);
		}
	}
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		OverheadBlockCount += It->CountSpruceCrownCoverProbes(UncoveredStarts, UncoveredEnds);
		break;
	}
	return OverheadBlockCount;
}

AActor* ARavenAgentAIController::FindPerchedNestSite() const
{
	if (LocomotionState != ERavenLocomotionState::Perched || !GetPawn()) return nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (It->ActorHasTag(TEXT("RavenNestSite")) && It->Tags.Num() > 0 &&
			FVector::DistSquared(It->GetActorLocation(), GetPawn()->GetActorLocation()) < FMath::Square(30.f))
			return *It;
	return nullptr;
}

AIslandArrangement* ARavenAgentAIController::FindForageableTwigPatch() const
{
	const APawn* Body = GetPawn();
	if (!Body || LocomotionState != ERavenLocomotionState::Grounded || !GetWorld()) return nullptr;
	AIslandArrangement* BestPatch = nullptr;
	float BestDistanceSquared = FMath::Square(250.f);
	for (TActorIterator<AIslandArrangement> It(GetWorld()); It; ++It)
	{
		if (!It->GetSiteId().ToString().StartsWith(TEXT("ArrangingGround_")) || !It->HasForageableTwigs()) continue;
		const FVector Delta = It->GetActorLocation() - Body->GetActorLocation();
		if (FMath::Abs(Delta.Z) > 250.f || Delta.SizeSquared2D() >= BestDistanceSquared) continue;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenTwigForageVisibility), false, Body);
		Query.AddIgnoredActor(*It);
		FHitResult VisibilityHit;
		const FVector EyePoint = Body->GetActorLocation() + FVector(0.f, 0.f, 20.f);
		const FVector PilePoint = It->GetActorLocation() + FVector(0.f, 0.f, 15.f);
		if (GetWorld()->LineTraceSingleByChannel(VisibilityHit, EyePoint, PilePoint, ECC_Visibility, Query)) continue;
		BestDistanceSquared = Delta.SizeSquared2D();
		BestPatch = *It;
	}
	return BestPatch;
}

FString ARavenAgentAIController::DescribeBuildOptions() const
{
	if (!GetPawn() || !GetWorld()) return FString();
	if (LocomotionState == ERavenLocomotionState::Grounded && !bCarryingTwigs)
		return FindForageableTwigPatch()
			? TEXT("A small visible pile of fallen twigs lies beside you; you may gather its bundle into your beak (build target: GatherTwigs). This site yields at most one bundle per Island day and can renew when the day advances; carrying twigs does not oblige you to build anything.")
			: TEXT("There are no fallen twigs within reach here. Fly to a listed open-ground ArrangingGround site with a visible twig pile, land, and look there; gathering is optional.");
	FString Result = bCarryingTwigs ? TEXT(" You are carrying a small bundle of fallen twigs.") : FString();
	const AActor* Site = FindPerchedNestSite();
	if (!Site) return Result;
	const UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
	const FIslandNestRecord* Nest = WorldState ? WorldState->FindNest(Site->Tags[0]) : nullptr;
	if (!WorldState) return Result;
	if (Nest && Nest->Layers >= UIslandWorldStateSubsystem::MaxNestLayers)
		return Result + TEXT(" The nest at this roost is complete; there is no room to weave in more.");
	if (const double* Until = WovenUntil.Find(Site->Tags[0]); Until && *Until > FPlatformTime::Seconds())
		return Result + TEXT(" The layer you just wove here is still settling; more weaving is not possible yet.");
	if (!bCarryingTwigs)
		return Result + TEXT(" To weave a nest here you would first need twigs gathered from the ground. None lie up on this perch. If you wish to forage, use land with a listed ArrangingGround target to fly to that open-ground site and descend; after landing, build with GatherTwigs. You can then move_to a listed roost and weave. This is optional.");
	return Result + FString::Printf(TEXT(" While perched here you may weave them into %s (build target: %s). This is a small lasting change that stays after this session."),
		Nest ? TEXT("the nest at this roost") : TEXT("the start of a nest"), *Site->Tags[0].ToString());
}

void ARavenAgentAIController::Build(FName Target)
{
	if (Target == FName(TEXT("GatherTwigs")))
	{
		if (LocomotionState != ERavenLocomotionState::Grounded) { ReportAction(TEXT("Twigs can only be gathered while standing on the ground. Nothing was gathered.")); return; }
		if (bCarryingTwigs) { ReportAction(TEXT("You are already carrying a bundle of twigs; there is no room in your beak for more.")); return; }
		AIslandArrangement* Patch = FindForageableTwigPatch();
		UIslandWorldStateSubsystem* WorldState = GetWorld() ? GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>() : nullptr;
		const UAgentMemoryComponent* Memory = GetPawn() ? GetPawn()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
		const FString AgentId = Memory ? Memory->GetResolvedAgentId() : (GetPawn() ? GetPawn()->GetName() : FString());
		const int32 Today = UIslandWorldStateSubsystem::CurrentIslandDay(GetWorld());
		if (!Patch || !Patch->HasForageableTwigs() || !WorldState ||
			!WorldState->GatherArrangementTwigs(Patch->GetSiteId(), AgentId, Today))
		{
			ReportAction(TEXT("There are no available fallen twigs within reach here, or this site's bundle was already gathered today; no bundle was gathered. Find another open-ground site with a visible twig pile."));
			return;
		}
		// Keep this observed patch visually honest too if a fixture or map contains a second actor with the same site ID.
		Patch->GatherForageableTwigs();
		bCarryingTwigs = true;
		ReportAction(TEXT("GatherTwigs: You gathered the visible fallen-twig bundle into your beak; the site is depleted until a later Island day. Nothing else was found, and nothing has been built yet."));
		return;
	}
	AActor* Site = FindPerchedNestSite();
	if (!Site || !Site->ActorHasTag(Target))
	{
		ReportAction(TEXT("Weaving is only possible while perched at a roost site offered as a build target; move_to that roost first. Nothing changed."));
		return;
	}
	if (!bCarryingTwigs) { ReportAction(TEXT("You have no twigs to weave; gather some from the ground first. Nothing changed.")); return; }
	if (const double* Until = WovenUntil.Find(Target); Until && *Until > FPlatformTime::Seconds())
	{
		ReportAction(TEXT("The last layer here is still settling; weaving again is not possible yet. Nothing changed."));
		return;
	}
	UIslandWorldStateSubsystem* WorldState = GetWorld()->GetSubsystem<UIslandWorldStateSubsystem>();
	if (!WorldState) { ReportAction(TEXT("Nothing lasting can be built in this world. Nothing changed.")); return; }
	if (const FIslandNestRecord* Existing = WorldState->FindNest(Target); Existing && Existing->Layers >= UIslandWorldStateSubsystem::MaxNestLayers)
	{
		ReportAction(TEXT("The nest here is already complete; there is no room to weave in more. You are still carrying your twigs."));
		return;
	}
	// The woven material rests on the support beneath the perched body, not on the marker in the air.
	const ACharacter* RavenCharacter = Cast<ACharacter>(GetPawn());
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	FVector SupportLocation = GetPawn()->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
	FHitResult Support;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenNestSupport), false, GetPawn());
	if (GetWorld()->LineTraceSingleByChannel(Support, GetPawn()->GetActorLocation(), SupportLocation - FVector(0.f, 0.f, 12.f), ECC_Visibility, Query))
		SupportLocation = Support.ImpactPoint;
	const UAgentMemoryComponent* Memory = GetPawn()->FindComponentByClass<UAgentMemoryComponent>();
	const int32 NewLayers = WorldState->AddNestLayer(Target, SupportLocation, Memory ? Memory->GetResolvedAgentId() : GetPawn()->GetName());
	if (NewLayers == 0) { ReportAction(TEXT("Weaving failed: the change could not be kept, so nothing lasting occurred. You are still carrying your twigs.")); return; }
	bCarryingTwigs = false;
	WovenUntil.Add(Target, FPlatformTime::Seconds() + 240);
	const FString Fact = FString::Printf(TEXT("%s: You wove your twigs into %s; it now has %d of %d layers. This change stays in the world after this session. It is a nest you made, not an assigned home, and it does not change how you rest."),
		*Target.ToString(), NewLayers == 1 ? TEXT("the first ring of a new nest") : TEXT("the nest"), NewLayers, UIslandWorldStateSubsystem::MaxNestLayers);
	ReportAction(Fact);
	if (UAgentMemoryComponent* Writable = GetPawn()->FindComponentByClass<UAgentMemoryComponent>())
		Writable->AppendMemory(Writable->MakeMemory(EAgentMemoryType::Observation, Fact, 0.6f, {TEXT("action-result"), TEXT("nest"), Target.ToString()}));
}

bool ARavenAgentAIController::BeginPerchAt(AActor* Perch)
{
	if (!Perch || !GetPawn()) return false;
	if (LocomotionState == ERavenLocomotionState::Perched && FVector::DistSquared(GetPawn()->GetActorLocation(), Perch->GetActorLocation()) < FMath::Square(15.f))
	{
		ReportAction(TEXT("Already perched at this site. Arrival is complete; you can rest, inspect once, or depart."));
		return true;
	}
	PerchTarget = Perch->GetActorLocation();
	// Rise vertically, cross above the landing point, then descend. This is a
	// simple approach, not obstacle pathfinding; a blocked segment safely aborts.
	const float ApproachZ = FMath::Max(GetPawn()->GetActorLocation().Z, PerchTarget.Z) + FMath::Max(100.f, TakeoffHeight);
	CruiseTarget = FVector(PerchTarget.X, PerchTarget.Y, ApproachZ);
	BeginTakeoff(CruiseTarget);
	MovementTarget.Z = ApproachZ;
	bApproachingPerch = true;
	bHasMovementTarget = true;
	bTargetIsPerch = true;
	ReportAction(TEXT("Roost approach started; arrival is not yet complete."));
	return true;
}

void ARavenAgentAIController::SetGrounded()
{
	FlightWaypoints.Reset();
	bHasTakeoffEscapeTarget = false;
	bHasMovementTarget = false;
	bTargetIsPerch = false;
	bApproachingPerch = false;
	bLandingAtArrangementSite = false;
	LocomotionState = ERavenLocomotionState::Grounded;
	SetFlyingMovement(false);
}

FVector ARavenAgentAIController::PlanFlightLeg(const FVector& From, const FVector& To)
{
	FlightWaypoints.Reset();
	APawn* Raven = GetPawn();
	if (!Raven || !GetWorld()) return To;
	const ACharacter* RavenCharacter = Cast<ACharacter>(Raven);
	const float Radius = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 30.f;
	const float HalfHeight = RavenCharacter ? RavenCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenFlightPlan), false, Raven);
	FHitResult Hit;
	if (!GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_WorldStatic, Body, Query)) return To;

	// Something is in the way: find the highest solid surface along the route and cross above it.
	float Highest = FMath::Max(From.Z, To.Z);
	const int32 Samples = FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(From, To) / 250.f), 2, 64);
	for (int32 Index = 0; Index <= Samples; ++Index)
	{
		const FVector Probe = FMath::Lerp(From, To, Index / static_cast<float>(Samples));
		FHitResult Top;
		if (GetWorld()->LineTraceSingleByChannel(Top, FVector(Probe.X, Probe.Y, Highest + 20000.f), FVector(Probe.X, Probe.Y, Probe.Z - 1000.f), ECC_WorldStatic, Query))
			Highest = FMath::Max(Highest, static_cast<float>(Top.ImpactPoint.Z));
	}
	const float Cruise = Highest + HalfHeight + 250.f;
	const FVector Up(From.X, From.Y, FMath::Max(Cruise, From.Z));
	const FVector Over(To.X, To.Y, Up.Z);
	// If even the climb is blocked (a roof overhead), fly as before and let the obstruction be reported.
	if (GetWorld()->SweepSingleByChannel(Hit, From, Up, FQuat::Identity, ECC_WorldStatic, Body, Query)) return To;
	FlightWaypoints = { Over, To };
	return Up;
}

bool ARavenAgentAIController::AdvanceTowardsTarget(float DeltaSeconds)
{
	APawn* Raven = GetPawn();
	const FVector Delta = MovementTarget - Raven->GetActorLocation();
	const float ArrivalRadius = LocomotionState == ERavenLocomotionState::Landing ? 2.f : 15.f;
	if (Delta.SizeSquared() < FMath::Square(ArrivalRadius)) return true;
	const FVector Direction = Delta.GetSafeNormal();
	FHitResult Hit;
	FVector Wind = FVector::ZeroVector;
	if (LocomotionState == ERavenLocomotionState::Flying)
	{
		for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
		{
			Wind = It->GetLocalWind(Raven->GetActorLocation(), Raven);
			break;
		}
	}
	// Fade drift near arrival and clamp steps so low frame rates cannot overshoot a target.
	const FVector Velocity = Direction * FMath::Max(0.f, FlightSpeed) + Wind * FMath::Clamp(Delta.Size() / 300.f, 0.f, 1.f);
	const FVector Step = (Velocity * FMath::Max(0.f, DeltaSeconds)).GetClampedToMaxSize(Delta.Size());
	Raven->SetActorLocation(Raven->GetActorLocation() + Step, true, &Hit);
	if (!Direction.IsNearlyZero()) Raven->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
	if (Hit.bBlockingHit)
	{
		// Obstruction is not a successful landing/perch. Stop and allow another decision.
		FlightWaypoints.Reset();
		bHasTakeoffEscapeTarget = false;
		bHasMovementTarget = false;
		bTargetIsPerch = false;
		bApproachingPerch = false;
		bLandingAtArrangementSite = false;
		LocomotionState = ERavenLocomotionState::Flying;
		ReportAction(TEXT("Flight was blocked by geometry; this is not a successful arrival. Choose a different approach."));
		return false;
	}
	return FVector::DistSquared(Raven->GetActorLocation(), MovementTarget) < FMath::Square(ArrivalRadius);
}

void ARavenAgentAIController::ActOnDecision(const FAgentDecision& Decision)
{
	if (!GetPawn()) return;
	if (IsResting()) return;
	if (Decision.ActionType == EAgentActionType::Idle)
	{
		bHasMovementTarget = bApproachingPerch = bTargetIsPerch = false;
		bLandingAtArrangementSite = false;
		bHasTakeoffEscapeTarget = false;
		FlightWaypoints.Reset();
		if (LocomotionState == ERavenLocomotionState::TakingOff || LocomotionState == ERavenLocomotionState::Landing || LocomotionState == ERavenLocomotionState::Hopping)
		{
			LocomotionState = ERavenLocomotionState::Flying;
			SetFlyingMovement(true);
		}
	}
	if (Decision.ActionType == EAgentActionType::Land)
	{
		BeginGroundLandingAt(FName(*Decision.ActionTarget));
		return;
	}

	if (Decision.ActionType == EAgentActionType::Build && !Decision.ActionTarget.StartsWith(TEXT("ArrangingGround")))
	{
		if (Decision.ActionTarget == TEXT("GuestBook"))
		{
			Super::ActOnDecision(Decision);
			return;
		}
		Build(FName(*Decision.ActionTarget));
		return;
	}

	if (Decision.ActionType == EAgentActionType::Wander)
	{
		if (LocomotionState == ERavenLocomotionState::Grounded)
		{
			if (FMath::FRand() < 0.4f) BeginHop(); else BeginTakeoff(MakeCruiseTarget());
		}
		else if (LocomotionState == ERavenLocomotionState::Perched)
		{
			BeginTakeoff(MakeCruiseTarget());
		}
		else if (LocomotionState == ERavenLocomotionState::Flying)
		{
			const float Choice = FMath::FRand();
			if (Choice < 0.18f)
			{
				if (!BeginLanding(GetPawn()->GetActorLocation())) ReportAction(TEXT("No safe ground was found below; choose another flight or landing site."));
			}
			else if (Choice < 0.32f && BeginPerch()) {}
			else { MovementTarget = PlanFlightLeg(GetPawn()->GetActorLocation(), MakeCruiseTarget()); bHasMovementTarget = true; bApproachingPerch = bTargetIsPerch = false; }
		}
		return;
	}

	if (Decision.ActionType == EAgentActionType::MoveTo)
	{
		const FName TargetTag(*Decision.ActionTarget);
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->ActorHasTag(TargetTag)) continue;
			if (!IslandInteractionUtility::IsMovementTargetAllowed(*It))
			{
				ReportAction(TEXT("Wildlife are not flight destinations. Watch from a respectful distance; do not chase, feed, touch, or claim them."));
				return;
			}
			if (AAutonomousAgentCharacter* OtherResident = Cast<AAutonomousAgentCharacter>(*It); OtherResident && OtherResident != GetPawn())
			{
				const FVector RavenLocation = GetPawn()->GetActorLocation();
				if (FVector::Dist2D(RavenLocation, OtherResident->GetActorLocation()) <= ResidentApproachStandOffDistance)
				{
					ReportAction(TEXT("Already near the other resident. Speaking remains optional; perch, observe, or choose another activity."));
					return;
				}
				const FVector Destination = BuildResidentApproachPoint(RavenLocation, OtherResident->GetActorLocation());
				ReportAction(TEXT("Flight toward the other resident's conversational space started; arriving does not begin a conversation."));
				if (LocomotionState == ERavenLocomotionState::Grounded || LocomotionState == ERavenLocomotionState::Perched)
				{
					BeginTakeoff(Destination);
				}
				else
				{
					MovementTarget = PlanFlightLeg(RavenLocation, Destination);
					bHasMovementTarget = true;
					bApproachingPerch = bTargetIsPerch = false;
					LocomotionState = ERavenLocomotionState::Flying;
					SetFlyingMovement(true);
				}
				return;
			}
			if (It->ActorHasTag(TEXT("RavenPerch")))
			{
				BeginPerchAt(*It);
				return;
			}
			const FVector Destination = It->GetActorLocation() + FVector(0.f, 0.f, 180.f);
			if (FVector::DistSquared(GetPawn()->GetActorLocation(), Destination) < FMath::Square(35.f)) { ReportAction(TEXT("Already at this landmark. Movement is complete; inspect once, wait, or choose a different destination.")); return; }
			ReportAction(TEXT("Flight to the landmark started; arrival is not yet complete."));
			if (LocomotionState == ERavenLocomotionState::Grounded || LocomotionState == ERavenLocomotionState::Perched)
				BeginTakeoff(Destination);
			else { MovementTarget = PlanFlightLeg(GetPawn()->GetActorLocation(), Destination); bHasMovementTarget = true; bApproachingPerch = bTargetIsPerch = false; LocomotionState = ERavenLocomotionState::Flying; SetFlyingMovement(true); }
			return;
		}
	}

	Super::ActOnDecision(Decision);
}

void ARavenAgentAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Raven = GetPawn();
	if (!Raven) return;
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ListeningStoneAttentionRemaining = FMath::Max(0.f, ListeningStoneAttentionRemaining - SafeDelta);
	ListeningStoneCheckRemaining -= SafeDelta;
	if (ListeningStoneCheckRemaining <= 0.f)
	{
		ListeningStoneCheckRemaining = 0.25f;
		CheckForNearbyListeningStoneChime();
	}
	UpdateWingAnimation(DeltaSeconds);
	UpdateHeadAnimation(DeltaSeconds);
	UpdateCarriedTwigVisual();
	UpdateCrowAnimation();
	if (IsResting()) return;

	if (LocomotionState == ERavenLocomotionState::Hopping)
	{
		HopElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(HopElapsed / HopDuration, 0.f, 1.f);
		FVector Position = FMath::Lerp(HopStart, HopEnd, Alpha);
		Position.Z += FMath::Sin(Alpha * PI) * HopHeight;
		Raven->SetActorLocation(Position, true);
		if (Alpha >= 1.f)
		{
			SetGrounded();
			ReportAction(TEXT("Completed a short ground hop."));
		}
		return;
	}

	if (!bHasMovementTarget) return;
	if (!AdvanceTowardsTarget(DeltaSeconds)) return;

	bHasMovementTarget = false;
	// A planned flight continues leg by leg before any arrival handling.
	if (LocomotionState == ERavenLocomotionState::Flying && FlightWaypoints.Num() > 0)
	{
		MovementTarget = FlightWaypoints[0];
		FlightWaypoints.RemoveAt(0);
		bHasMovementTarget = true;
		return;
	}
	if (LocomotionState == ERavenLocomotionState::TakingOff)
	{
		if (bHasTakeoffEscapeTarget)
		{
			MovementTarget = TakeoffEscapeTarget;
			bHasTakeoffEscapeTarget = false;
		}
		else
		{
			LocomotionState = ERavenLocomotionState::Flying;
			MovementTarget = PlanFlightLeg(Raven->GetActorLocation(), CruiseTarget);
		}
		bHasMovementTarget = true;
	}
	else if (LocomotionState == ERavenLocomotionState::Flying && bApproachingPerch)
	{
		MovementTarget = PerchTarget;
		bHasMovementTarget = true;
		bApproachingPerch = false;
		LocomotionState = ERavenLocomotionState::Landing;
	}
	else if (LocomotionState == ERavenLocomotionState::Landing)
	{
		if (bTargetIsPerch)
		{
			// A marker in empty air is not a perch. Verify close support below.
			const ACharacter* PerchingCharacter = Cast<ACharacter>(Raven);
			const float HalfHeight = PerchingCharacter ? PerchingCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 45.f;
			FHitResult Support;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(RavenPerchSupport), false, Raven);
			if (!GetWorld()->LineTraceSingleByChannel(Support, Raven->GetActorLocation(), Raven->GetActorLocation() - FVector(0.f, 0.f, HalfHeight + 12.f), ECC_Visibility, Query) || Support.ImpactNormal.Z < 0.5f)
			{
				bTargetIsPerch = false;
				LocomotionState = ERavenLocomotionState::Flying;
				ReportAction(TEXT("Perch rejected: no suitable support below the landing point."));
				return;
			}
			LocomotionState = ERavenLocomotionState::Perched;
			SetFlyingMovement(true);
			ReportAction(TEXT("Landed and perched on solid support. Arrival is complete; you may rest here or choose to depart."));
		}
		else
		{
			const bool bLandedAtArrangementSite = bLandingAtArrangementSite;
			SetGrounded();
			if (bLandedAtArrangementSite)
				ReportAction(FindForageableTwigPatch()
					? TEXT("Landed on the verified open-ground site. A visible bundle of fallen twigs is within reach if you choose to gather it.")
					: TEXT("Landed on the verified open-ground site, but there is no visible twig bundle within reach. No resource was gathered."));
		}
	}
	else if (LocomotionState == ERavenLocomotionState::Flying && bLandingAtArrangementSite)
	{
		if (!BeginLanding(ArrangementLandingTarget))
		{
			bLandingAtArrangementSite = false;
			ReportAction(TEXT("The open-ground site had no safe landing surface on arrival. You remain in flight; choose another visible site."));
		}
		else ReportAction(TEXT("Reached the open-ground site; descending to the surface now."));
	}
	else if (LocomotionState == ERavenLocomotionState::Flying) ReportAction(TEXT("Reached the flight destination. No further movement is needed to arrive."));
}
