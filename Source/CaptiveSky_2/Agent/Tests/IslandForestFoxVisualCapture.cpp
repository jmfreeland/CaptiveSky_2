#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AgentBrainComponent.h"
#include "AutonomousAgentCharacter.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "IslandDayNight.h"
#include "IslandForestFox.h"
#include "IslandWeather.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "ContentStreaming.h"
#include "TextureResource.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFoxResidentGlanceCaptureTest,
	"CaptiveSky2.Visual.WoodlandFoxResidentGlance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFoxResidentGlanceCaptureTest::RunTest(const FString& Parameters)
{
	struct FGroundCoverPreviewCleanup
	{
		TWeakObjectPtr<AIslandWeather> Weather;
		~FGroundCoverPreviewCleanup()
		{
			if (Weather.IsValid()) Weather->ClearGroundCoverPreview();
		}
	} GroundCoverCleanup;

	if (!TestNotNull(TEXT("The Unreal engine is available for the transient wildlife capture"), GEngine))
		return false;

	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() &&
			Context.World()->GetMapName() == TEXT("Island"))
		{
			Island = Context.World();
			break;
		}
	if (!TestNotNull(TEXT("The saved Island editor world is loaded for the fox glance capture"), Island))
		return false;

	AActor* WindArch = nullptr;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("WindArch")))
		{
			WindArch = *It;
			break;
		}
	if (!TestNotNull(TEXT("The meadow scene has its authored Wind Arch anchor"), WindArch))
		return false;

	UClass* ResidentClass = LoadClass<AAutonomousAgentCharacter>(nullptr,
		TEXT("/Game/Agents/BP_Agent_Placeholder.BP_Agent_Placeholder_C"));
	if (!TestNotNull(TEXT("Aster's resident placeholder is available for the transient capture"), ResidentClass))
		return false;

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Tangents[] = {
		FVector(-400.f, 0.f, 0.f), FVector(400.f, 0.f, 0.f),
		FVector(0.f, -400.f, 0.f), FVector(0.f, 400.f, 0.f),
		FVector(300.f, 300.f, 0.f), FVector(-300.f, 300.f, 0.f),
		FVector(300.f, -300.f, 0.f), FVector(-300.f, -300.f, 0.f)
	};
	// This is the grounded fallback site selected and logged by the runtime fox in
	// Codex_FoxWoodlandCover_Game_20261010.log; retain the place it actually uses.
	const FVector RuntimeWoodlandEdgeSite(-101434.42f, 101633.89f, 2608.63f);
	const FVector SearchOffsets[] = {
		FVector::ZeroVector, FVector(500.f, 0.f, 0.f), FVector(-500.f, 0.f, 0.f),
		FVector(0.f, 500.f, 0.f), FVector(0.f, -500.f, 0.f),
		FVector(500.f, 500.f, 0.f), FVector(-500.f, 500.f, 0.f),
		FVector(500.f, -500.f, 0.f), FVector(-500.f, -500.f, 0.f)
	};
	FVector FoxLocation = FVector::ZeroVector;
	FVector ResidentLocation = FVector::ZeroVector;
	bool bFoundVisiblePair = false;
	for (const FVector& SearchOffset : SearchOffsets)
	{
		const FVector FoxCandidate = RuntimeWoodlandEdgeSite + SearchOffset;
		FHitResult FoxGroundHit;
		FCollisionQueryParams FoxGroundQuery(SCENE_QUERY_STAT(FoxResidentCaptureGround), false, WindArch);
		if (!Island->LineTraceSingleByChannel(FoxGroundHit,
			FoxCandidate + FVector(0.f, 0.f, 1600.f),
			FoxCandidate - FVector(0.f, 0.f, 3000.f), ECC_Visibility, FoxGroundQuery) ||
			FoxGroundHit.ImpactNormal.Z < 0.8f)
			continue;

		for (const FVector& Tangent : Tangents)
		{
			const FVector ResidentCandidate = FoxCandidate + Tangent;
			FHitResult ResidentGroundHit;
			FCollisionQueryParams ResidentGroundQuery(SCENE_QUERY_STAT(FoxResidentCaptureGround), false, WindArch);
			if (!Island->LineTraceSingleByChannel(ResidentGroundHit,
				ResidentCandidate + FVector(0.f, 0.f, 1600.f),
				ResidentCandidate - FVector(0.f, 0.f, 3000.f), ECC_Visibility, ResidentGroundQuery) ||
				ResidentGroundHit.ImpactNormal.Z < 0.8f)
				continue;

			if (FMath::Abs(ResidentGroundHit.ImpactPoint.Z - FoxGroundHit.ImpactPoint.Z) > 180.f)
				continue;
			const FVector FoxEye = FoxGroundHit.ImpactPoint + FVector(0.f, 0.f, 72.f);
			const FVector ResidentHead = ResidentGroundHit.ImpactPoint + FVector(0.f, 0.f, 155.f);
			FCollisionQueryParams SightQuery(SCENE_QUERY_STAT(FoxResidentCaptureSight), false, WindArch);
			FHitResult SightHit;
			if (Island->LineTraceSingleByChannel(SightHit, FoxEye, ResidentHead, ECC_Visibility, SightQuery))
				continue;

			FoxLocation = FoxGroundHit.ImpactPoint + FVector(0.f, 0.f, 2.f);
			ResidentLocation = ResidentGroundHit.ImpactPoint + FVector(0.f, 0.f, 88.f);
			bFoundVisiblePair = true;
			break;
		}
		if (bFoundVisiblePair) break;
	}
	if (!TestTrue(TEXT("Nearby walkable ground provides a clear fox–resident sightline"), bFoundVisiblePair))
		return false;

	const FRotator AsterFacing = (FoxLocation - ResidentLocation).Rotation();
	AAutonomousAgentCharacter* Aster = Island->SpawnActorDeferred<AAutonomousAgentCharacter>(
		ResidentClass, FTransform(AsterFacing, ResidentLocation), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Aster)
	{
		Aster->AutoPossessAI = EAutoPossessAI::Disabled;
		Aster->FinishSpawning(FTransform(AsterFacing, ResidentLocation));
		if (UCharacterMovementComponent* Movement = Aster->GetCharacterMovement())
			Movement->Velocity = FVector::ZeroVector;
	}
	AIslandForestFox* Fox = Island->SpawnActor<AIslandForestFox>(FoxLocation, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("A transient Aster placeholder is staged in the Wind Arch meadow"), Aster) ||
		!TestNotNull(TEXT("A transient woodland fox is staged in the Wind Arch meadow"), Fox))
	{
		if (Fox) Fox->Destroy();
		if (Aster) Aster->Destroy();
		return false;
	}

	Fox->HomeLocation = Fox->GetActorLocation();
	UAgentBrainComponent* AsterBrain = Aster->FindComponentByClass<UAgentBrainComponent>();
	TestNotNull(TEXT("Aster's existing brain can receive the fleeting local observation"), AsterBrain);
	TestTrue(TEXT("The transient fox has its imported look animation"), Fox->LookAroundAnimation && Fox->FoxMesh);
	const bool bFoxLookStarted = Fox->RespondToQuietObservation(Aster->GetActorLocation());
	TestTrue(TEXT("The awake fox enters the brief look before its retreat"),
		bFoxLookStarted && Fox->IsRespondingToQuietObserver() && !Fox->bMoving);
	const float HomeRadius = FVector::Dist2D(Fox->HomeLocation, Fox->TargetLocation);
	TestTrue(TEXT("The staged retreat remains within the fox's local woodland patch"), HomeRadius <= 650.1f);
	const FString ResidentSummary = AsterBrain
		? AsterBrain->BuildSituationSummary(FAgentConversationContext()) : FString();
	TestTrue(TEXT("Aster can perceive the actual fleeting glance without inferred intent"),
		ResidentSummary.Contains(TEXT("briefly looking toward a nearby observer")) &&
		ResidentSummary.Contains(TEXT("cannot know what it intends")));

	AIslandWeather* PreviewWeather = nullptr;
	for (TActorIterator<AIslandWeather> It(Island); It; ++It) { PreviewWeather = *It; break; }
	if (!TestNotNull(TEXT("The Island weather actor can provide its reversible vegetation preview"), PreviewWeather))
	{
		Fox->Destroy();
		Aster->Destroy();
		return false;
	}
	PreviewWeather->InitializeGroundCover();
	GroundCoverCleanup.Weather = PreviewWeather;
	TestTrue(TEXT("The transient scene includes the same bounded ground-cover scatter used in play"),
		PreviewWeather->GroundCoverInstanceCount > 0 && PreviewWeather->GroundCoverMeadowInstanceCount > 0);
	AddInfo(FString::Printf(TEXT("Transient capture ecology: %d ground-cover instances, %d meadow instances, %d trees, and %d shrubs."),
		PreviewWeather->GroundCoverInstanceCount, PreviewWeather->GroundCoverMeadowInstanceCount,
		PreviewWeather->GroundCoverTreeCount, PreviewWeather->GroundCoverShrubCount));

	AIslandDayNight* DayNight = nullptr;
	for (TActorIterator<AIslandDayNight> It(Island); It; ++It) { DayNight = *It; break; }
	if (!TestNotNull(TEXT("The Island day/night actor provides a reversible noon meadow capture"), DayNight))
	{
		Fox->Destroy();
		Aster->Destroy();
		return false;
	}
	const float OriginalStartHour = DayNight->StartHour;
	const float OriginalCurrentHour = DayNight->CurrentHour;
	DayNight->StartHour = 12.f;
	DayNight->CurrentHour = 12.f;
	DayNight->Tick(0.f);

	constexpr int32 CaptureWidth = 1600;
	constexpr int32 CaptureHeight = 900;
	UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	Target->RenderTargetFormat = RTF_RGBA8_SRGB;
	Target->InitAutoFormat(CaptureWidth, CaptureHeight);
	Target->UpdateResourceImmediate(true);
	const FVector LookAt = (Fox->GetActorLocation() + Aster->GetActorLocation()) * 0.5f + FVector(0.f, 0.f, 75.f);
	const FVector PairDirection = (Aster->GetActorLocation() - Fox->GetActorLocation()).GetSafeNormal2D();
	const FVector SideDirection = FVector::CrossProduct(PairDirection, FVector::UpVector).GetSafeNormal();
	const FVector CameraOffsets[] = {
		SideDirection * 950.f + FVector(0.f, 0.f, 420.f),
		-SideDirection * 950.f + FVector(0.f, 0.f, 420.f),
		SideDirection * 1200.f + FVector(0.f, 0.f, 500.f),
		-SideDirection * 1200.f + FVector(0.f, 0.f, 500.f)
	};
	FVector CameraLocation = LookAt + CameraOffsets[0];
	FCollisionQueryParams CameraQuery(SCENE_QUERY_STAT(FoxResidentCaptureCamera), false, WindArch);
	CameraQuery.AddIgnoredActor(Aster);
	CameraQuery.AddIgnoredActor(Fox);
	int32 BestBlockers = MAX_int32;
	bool bBestBackgroundBlocked = true;
	for (const FVector& Offset : CameraOffsets)
	{
		const FVector Candidate = LookAt + Offset;
		FHitResult FoxHit;
		FHitResult FoxHeadHit;
		FHitResult ResidentHit;
		FHitResult ResidentHeadHit;
		const bool bFoxBlocked = Island->LineTraceSingleByChannel(FoxHit, Candidate,
			Fox->GetActorLocation() + FVector(0.f, 0.f, 55.f), ECC_Visibility, CameraQuery);
		const bool bFoxHeadBlocked = Island->LineTraceSingleByChannel(FoxHeadHit, Candidate,
			Fox->GetActorLocation() + FVector(0.f, 0.f, 110.f), ECC_Visibility, CameraQuery);
		const bool bResidentBlocked = Island->LineTraceSingleByChannel(ResidentHit, Candidate,
			Aster->GetActorLocation() + FVector(0.f, 0.f, 90.f), ECC_Visibility, CameraQuery);
		const bool bResidentHeadBlocked = Island->LineTraceSingleByChannel(ResidentHeadHit, Candidate,
			Aster->GetActorLocation() + FVector(0.f, 0.f, 160.f), ECC_Visibility, CameraQuery);
		const int32 Blockers = static_cast<int32>(bFoxBlocked) + static_cast<int32>(bFoxHeadBlocked) +
			static_cast<int32>(bResidentBlocked) + static_cast<int32>(bResidentHeadBlocked);
		const FVector ViewDirection = (LookAt - Candidate).GetSafeNormal();
		FHitResult BackgroundHit;
		const FVector BackgroundStart = LookAt + ViewDirection * 180.f + FVector(0.f, 0.f, 110.f);
		const bool bBackgroundBlocked = Island->LineTraceSingleByChannel(BackgroundHit, BackgroundStart,
			BackgroundStart + ViewDirection * 1800.f, ECC_Visibility, CameraQuery);
		if (Blockers < BestBlockers || (Blockers == BestBlockers && bBestBackgroundBlocked && !bBackgroundBlocked))
		{
			BestBlockers = Blockers;
			bBestBackgroundBlocked = bBackgroundBlocked;
			CameraLocation = Candidate;
		}
		if (Blockers == 0 && !bBackgroundBlocked) break;
	}
	ASceneCapture2D* Camera = Island->SpawnActor<ASceneCapture2D>(
		CameraLocation, (LookAt - CameraLocation).Rotation(), Spawn);
	USceneCaptureComponent2D* Capture = Camera ? Camera->GetCaptureComponent2D() : nullptr;
	if (!TestNotNull(TEXT("The transient real-RHI Aster–fox capture camera was spawned"), Capture))
	{
		DayNight->StartHour = OriginalStartHour;
		DayNight->CurrentHour = OriginalCurrentHour;
		DayNight->Tick(0.f);
		if (Camera) Camera->Destroy();
		Fox->Destroy();
		Aster->Destroy();
		return false;
	}

	Capture->TextureTarget = Target;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 48.f;
	Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
	Capture->PostProcessSettings.AutoExposureBias = 0.6f;
	Capture->PostProcessBlendWeight = 1.f;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	if (Fox->LookAroundAnimation && Fox->FoxMesh)
	{
		const float PoseTime = FMath::Clamp(Fox->LookAroundAnimation->GetPlayLength() * 0.35f, 0.f,
			FMath::Max(0.f, Fox->LookAroundAnimation->GetPlayLength() - KINDA_SMALL_NUMBER));
		Fox->FoxMesh->SetPosition(PoseTime, false);
		Fox->FoxMesh->RefreshBoneTransforms();
	}
	IStreamingManager::Get().StreamAllResources(2.f);
	Capture->CaptureScene();
	FlushRenderingCommands();
	TArray<FColor> Pixels;
	const bool bReadPixels = Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) &&
		Pixels.Num() == CaptureWidth * CaptureHeight;
	for (FColor& Pixel : Pixels) Pixel.A = 255;
	const FString CaptureDirectory = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),
		TEXT("Viewpoints"), TEXT("WoodlandFoxResidentGlance_"), FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
	const FString ImagePath = CaptureDirectory / TEXT("01_Aster_Fox_WoodlandMeadow.png");
	const bool bSaved = bReadPixels && FImageUtils::SaveImageByExtension(*ImagePath,
		FImageView(Pixels.GetData(), CaptureWidth, CaptureHeight));
	AddInfo(FString::Printf(TEXT("Fox resident glance capture: %s; subject blockers=%d; background blocked=%s; hour=12.0"),
		*ImagePath, BestBlockers, bBestBackgroundBlocked ? TEXT("true") : TEXT("false")));
	TestTrue(TEXT("The 1600x900 real-RHI fox glance frame is saved"), bSaved);

	DayNight->StartHour = OriginalStartHour;
	DayNight->CurrentHour = OriginalCurrentHour;
	DayNight->Tick(0.f);
	Camera->Destroy();
	Fox->Destroy();
	Aster->Destroy();
	return bSaved;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
