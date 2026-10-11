// Copyright Epic Games, Inc. All Rights Reserved.

#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "AgentBrainComponent.h"
#include "AgentExternalBridgeComponent.h"
#include "AgentRelationshipComponent.h"
#include "AgentSocialComponent.h"
#include "AgentConsolidationComponent.h"
#include "AgentRestPresentationComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Base64.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogAutonomousAgent, Log, All);

AAutonomousAgentCharacter::AAutonomousAgentCharacter()
{
	Memory = CreateDefaultSubobject<UAgentMemoryComponent>(TEXT("Memory"));
	Brain = CreateDefaultSubobject<UAgentBrainComponent>(TEXT("Brain"));
	ExternalBridge = CreateDefaultSubobject<UAgentExternalBridgeComponent>(TEXT("ExternalBridge"));
	Relationships = CreateDefaultSubobject<UAgentRelationshipComponent>(TEXT("Relationships"));
	Social = CreateDefaultSubobject<UAgentSocialComponent>(TEXT("Social"));
	Consolidation = CreateDefaultSubobject<UAgentConsolidationComponent>(TEXT("Consolidation"));
	RestPresentation = CreateDefaultSubobject<UAgentRestPresentationComponent>(TEXT("RestPresentation"));

	EyeCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("EyeCapture"));
	EyeCapture->SetupAttachment(GetMesh());
	EyeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	EyeCapture->bCaptureEveryFrame = false;
	EyeCapture->bCaptureOnMovement = false;
	EyeCapture->FOVAngle = 90.f;
}

FString AAutonomousAgentCharacter::GetAgentDisplayName() const
{
	return DisplayName.IsEmpty() ? GetActorNameOrLabel() : DisplayName;
}

void AAutonomousAgentCharacter::BeginPlay()
{
	Super::BeginPlay();
	RegisterApproachTargetTags();
	ApplyMetaHumanBodyIfBuilt();

	// Re-parent onto the actual assigned mesh's socket now that the Blueprint CDO's mesh is in effect.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (!EyeSocketName.IsNone() && MeshComp->DoesSocketExist(EyeSocketName))
		{
			EyeCapture->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, EyeSocketName);
		}
		else
		{
			// No named socket on this body (e.g. the placeholder capsule) -- use a plausible head-height forward offset.
			EyeCapture->AttachToComponent(MeshComp, FAttachmentTransformRules::KeepRelativeTransform);
			EyeCapture->SetRelativeLocation(FVector(30.f, 0.f, 60.f));
		}
	}

	if (!EyeRenderTarget)
	{
		EyeRenderTarget = NewObject<UTextureRenderTarget2D>(this);
		EyeRenderTarget->InitAutoFormat(EyeCaptureResolution, EyeCaptureResolution);
		EyeRenderTarget->UpdateResourceImmediate(true);
		EyeCapture->TextureTarget = EyeRenderTarget;
	}
}

void AAutonomousAgentCharacter::ApplyMetaHumanBodyIfBuilt()
{
	// A map-placed resident (e.g. Aster) keeps its placeholder Blueprint; when the built MetaHuman for that resident exists in
	// this checkout (Content/ is local-only, Scripts/Create-MetaHumans.py), show it instead. No-op otherwise.
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !Memory) return;
	const FString AgentId = Memory->GetResolvedAgentId();
	if (!AgentId.Equals(TEXT("Agent_Aster_01"), ESearchCase::IgnoreCase)) return;
	const FString Who = AgentId.RightChop(6).LeftChop(3);   // "Aster"
	const FString Base = FString::Printf(TEXT("/Game/Characters/MetaHumans/Built/%s/MHC_%s"), *Who, *Who);
	USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s/Body/SKM_MHC_%s_BodyMesh.SKM_MHC_%s_BodyMesh"), *Base, *Who, *Who), nullptr, LOAD_NoWarn | LOAD_Quiet);
	USkeletalMesh* Face = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s/Face/SKM_MHC_%s_FaceMesh.SKM_MHC_%s_FaceMesh"), *Base, *Who, *Who), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Body || !Face || MeshComp->GetSkeletalMeshAsset() == Body) return;
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComp->SetSkeletalMesh(Body);
	USkeletalMeshComponent* FaceComp = NewObject<USkeletalMeshComponent>(this, TEXT("MetaHumanFace"));
	FaceComp->SetupAttachment(MeshComp);
	FaceComp->RegisterComponent();
	FaceComp->SetSkeletalMesh(Face);
	UE_LOG(LogAutonomousAgent, Display, TEXT("%s is using its built MetaHuman body."), *AgentId);
}

void AAutonomousAgentCharacter::RegisterApproachTargetTags()
{
	// Give other residents a stable, body-independent movement target. Movement
	// brings an agent within conversational range but never initiates speech by itself.
	if (Memory)
	{
		const FString AgentId = Memory->GetResolvedAgentId();
		if (!AgentId.IsEmpty())
		{
			const FName ApproachTag(*FString::Printf(TEXT("ApproachAgent_%s"), *AgentId));
			Tags.AddUnique(ApproachTag);
			// Models tidy the doubled word in "ApproachAgent_Agent_Aster_01" into "ApproachAgent_Aster_01";
			// answer to that form too, or the move silently fails as "not found".
			if (AgentId.StartsWith(TEXT("Agent_")))
				Tags.AddUnique(FName(*FString::Printf(TEXT("ApproachAgent_%s"), *AgentId.RightChop(6))));
		}
	}
}

FString AAutonomousAgentCharacter::CaptureFirstPersonSnapshot() const
{
	// Headless runs (including -NullRHI playtests) can still exercise agent
	// decisions, but cannot render the scene capture into its texture target.
	// Keep the provider request text-only instead of crashing in image export.
	if (!FApp::CanEverRender())
	{
		UE_LOG(LogAutonomousAgent, Verbose, TEXT("Skipping first-person snapshot because this process cannot render."));
		return FString();
	}

	if (!EyeCapture || !EyeRenderTarget)
	{
		UE_LOG(LogAutonomousAgent, Warning, TEXT("CaptureFirstPersonSnapshot called before eye capture was initialized."));
		return FString();
	}

	EyeCapture->CaptureScene();

	TArray<uint8> PNGBytes;
	FMemoryWriter MemWriter(PNGBytes);
	FImageUtils::ExportRenderTarget2DAsPNG(EyeRenderTarget, MemWriter);

	if (PNGBytes.Num() == 0)
	{
		UE_LOG(LogAutonomousAgent, Warning, TEXT("First-person snapshot produced no PNG data."));
		return FString();
	}

	return FBase64::Encode(PNGBytes);
}
