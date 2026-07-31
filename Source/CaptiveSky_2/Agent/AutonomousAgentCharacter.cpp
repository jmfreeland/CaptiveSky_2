// Copyright Epic Games, Inc. All Rights Reserved.

#include "AutonomousAgentCharacter.h"
#include "AgentMemoryComponent.h"
#include "AgentBrainComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Base64.h"

DEFINE_LOG_CATEGORY_STATIC(LogAutonomousAgent, Log, All);

AAutonomousAgentCharacter::AAutonomousAgentCharacter()
{
	Memory = CreateDefaultSubobject<UAgentMemoryComponent>(TEXT("Memory"));
	Brain = CreateDefaultSubobject<UAgentBrainComponent>(TEXT("Brain"));

	EyeCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("EyeCapture"));
	EyeCapture->SetupAttachment(GetMesh());
	EyeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	EyeCapture->bCaptureEveryFrame = false;
	EyeCapture->bCaptureOnMovement = false;
	EyeCapture->FOVAngle = 90.f;
}

void AAutonomousAgentCharacter::BeginPlay()
{
	Super::BeginPlay();

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

FString AAutonomousAgentCharacter::CaptureFirstPersonSnapshot() const
{
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
