#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AgentBrainComponent.h"
#include "AutonomousAgentCharacter.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ContentStreaming.h"
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
#include "IslandForestStag.h"
#include "IslandWeather.h"
#include "LandscapeProxy.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "TextureResource.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFoxStagAwarenessCaptureTest,
	"CaptiveSky2.Visual.WoodlandFoxStagAwareness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FIslandFoxStagAwarenessCaptureTest::RunTest(const FString& Parameters)
{
	struct FCaptureCleanup
	{
		TWeakObjectPtr<AIslandWeather> Weather;
		TWeakObjectPtr<AIslandDayNight> DayNight;
		float OriginalStartHour = 9.f;
		float OriginalCurrentHour = 9.f;
		bool bClearGroundCoverPreview = false;
		TArray<TWeakObjectPtr<AActor>> Actors;
		TArray<TPair<TWeakObjectPtr<AActor>, bool>> HiddenActors;

		~FCaptureCleanup()
		{
			for (const TWeakObjectPtr<AActor>& Actor : Actors)
				if (Actor.IsValid()) Actor->Destroy();
			for (const TPair<TWeakObjectPtr<AActor>, bool>& HiddenActor : HiddenActors)
				if (HiddenActor.Key.IsValid()) HiddenActor.Key->SetActorHiddenInGame(HiddenActor.Value);
			if (Weather.IsValid() && bClearGroundCoverPreview) Weather->ClearGroundCoverPreview();
			if (DayNight.IsValid())
			{
				DayNight->StartHour = OriginalStartHour;
				DayNight->CurrentHour = OriginalCurrentHour;
				DayNight->Tick(0.f);
			}
		}
	} Cleanup;

	if (!TestNotNull(TEXT("The Unreal engine is available for the transient wildlife capture"), GEngine))
		return false;

	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World() &&
			Context.World()->GetMapName() == TEXT("Island"))
		{
			Island = Context.World();
			break;
		}
	if (!Island)
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
			if (Context.WorldType == EWorldType::Editor && Context.World() &&
				Context.World()->GetMapName() == TEXT("Island"))
			{
				Island = Context.World();
				break;
			}
	if (!TestNotNull(TEXT("The saved Island world is loaded for the fox–stag capture"), Island))
		return false;

	AActor* WindArch = nullptr;
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->ActorHasTag(TEXT("WindArch")))
		{
			WindArch = *It;
			break;
		}
	if (!TestNotNull(TEXT("The woodland scene has its authored Wind Arch anchor"), WindArch))
		return false;

	UClass* ResidentClass = LoadClass<AAutonomousAgentCharacter>(nullptr,
		TEXT("/Game/Agents/BP_Agent_Placeholder.BP_Agent_Placeholder_C"));
	if (!TestNotNull(TEXT("Aster's resident placeholder is available for the transient capture"), ResidentClass))
		return false;

	// In Game context, keep the staged pair distinct from the live ambient wildlife.
	// Restore their exact visibility states after the screenshot is saved.
	for (TActorIterator<AIslandForestStag> It(Island); It; ++It)
	{
		Cleanup.HiddenActors.Emplace(*It, It->IsHidden());
		It->SetActorHiddenInGame(true);
	}
	for (TActorIterator<AIslandForestFox> It(Island); It; ++It)
	{
		Cleanup.HiddenActors.Emplace(*It, It->IsHidden());
		It->SetActorHiddenInGame(true);
	}

	// Start from the real runtime fox site, then find a small, flat, visible three-actor
	// composition nearby. No saved actor, level, or world-state location is authored.
	const FVector RuntimeWoodlandEdgeSite(-101434.42f, 101633.89f, 2608.63f);
	const FVector SearchOffsets[] = {
		FVector::ZeroVector, FVector(300.f, 0.f, 0.f), FVector(-300.f, 0.f, 0.f),
		FVector(0.f, 300.f, 0.f), FVector(0.f, -300.f, 0.f),
		FVector(300.f, 300.f, 0.f), FVector(-300.f, 300.f, 0.f),
		FVector(300.f, -300.f, 0.f), FVector(-300.f, -300.f, 0.f)
	};
	const FVector PairOffsets[] = {
		FVector(450.f, 0.f, 0.f), FVector(-450.f, 0.f, 0.f),
		FVector(0.f, 450.f, 0.f), FVector(0.f, -450.f, 0.f),
		FVector(320.f, 320.f, 0.f), FVector(-320.f, 320.f, 0.f),
		FVector(320.f, -320.f, 0.f), FVector(-320.f, -320.f, 0.f)
	};
	auto FindGround = [Island, WindArch](const FVector& Candidate, FVector& OutGround)
	{
		FHitResult GroundHit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(FoxStagCaptureGround), false, WindArch);
		if (!Island->LineTraceSingleByChannel(GroundHit,
			Candidate + FVector(0.f, 0.f, 1600.f),
			Candidate - FVector(0.f, 0.f, 3000.f), ECC_WorldStatic, Query) ||
			!Cast<ALandscapeProxy>(GroundHit.GetActor()) || GroundHit.ImpactNormal.Z < 0.8f)
			return false;
		OutGround = GroundHit.ImpactPoint;
		return true;
	};
	auto HasClearSight = [Island](const FVector& Start, const FVector& End, AActor* IgnoreA, AActor* IgnoreB)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(FoxStagCaptureSight), false);
		if (IgnoreA) Query.AddIgnoredActor(IgnoreA);
		if (IgnoreB) Query.AddIgnoredActor(IgnoreB);
		FHitResult Hit;
		return !Island->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
	};

	FVector StagGround = FVector::ZeroVector;
	FVector FoxGround = FVector::ZeroVector;
	FVector ResidentGround = FVector::ZeroVector;
	bool bFoundVisibleComposition = false;
	for (const FVector& SearchOffset : SearchOffsets)
	{
		FVector CandidateStagGround;
		if (!FindGround(RuntimeWoodlandEdgeSite + SearchOffset, CandidateStagGround)) continue;
		for (const FVector& PairOffset : PairOffsets)
		{
			FVector CandidateFoxGround;
			if (!FindGround(CandidateStagGround + PairOffset, CandidateFoxGround) ||
				FVector::Dist2D(CandidateStagGround, CandidateFoxGround) < 350.f ||
				FVector::Dist2D(CandidateStagGround, CandidateFoxGround) > 650.f ||
				FMath::Abs(CandidateStagGround.Z - CandidateFoxGround.Z) > 140.f ||
				!HasClearSight(CandidateStagGround + FVector(0.f, 0.f, 85.f),
					CandidateFoxGround + FVector(0.f, 0.f, 70.f), nullptr, nullptr))
				continue;

			const FVector Midpoint = (CandidateStagGround + CandidateFoxGround) * 0.5f;
			const FVector PairDirection = (CandidateFoxGround - CandidateStagGround).GetSafeNormal2D();
			const FVector SideDirection = FVector::CrossProduct(PairDirection, FVector::UpVector).GetSafeNormal();
			const FVector ResidentOffsets[] = {
				SideDirection * 300.f, -SideDirection * 300.f,
				SideDirection * 400.f, -SideDirection * 400.f,
				SideDirection * 500.f, -SideDirection * 500.f
			};
			for (const FVector& ResidentOffset : ResidentOffsets)
			{
				FVector CandidateResidentGround;
				if (!FindGround(Midpoint + ResidentOffset, CandidateResidentGround) ||
					FMath::Abs(CandidateResidentGround.Z - Midpoint.Z) > 140.f ||
					FVector::Dist2D(CandidateResidentGround, CandidateStagGround) < 220.f ||
					FVector::Dist2D(CandidateResidentGround, CandidateFoxGround) < 220.f ||
					FVector::Dist2D(CandidateResidentGround, CandidateStagGround) > 550.f ||
					FVector::Dist2D(CandidateResidentGround, CandidateFoxGround) > 550.f ||
					!HasClearSight(CandidateResidentGround + FVector(0.f, 0.f, 88.f),
						CandidateStagGround + FVector(0.f, 0.f, 75.f), nullptr, nullptr) ||
					!HasClearSight(CandidateResidentGround + FVector(0.f, 0.f, 88.f),
						CandidateFoxGround + FVector(0.f, 0.f, 65.f), nullptr, nullptr))
					continue;

				StagGround = CandidateStagGround;
				FoxGround = CandidateFoxGround;
				ResidentGround = CandidateResidentGround;
				bFoundVisibleComposition = true;
				break;
			}
			if (bFoundVisibleComposition) break;
		}
		if (bFoundVisibleComposition) break;
	}
	if (!TestTrue(TEXT("Nearby meadow ground provides a flat, visible three-actor composition"), bFoundVisibleComposition))
		return false;

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandForestStag* Stag = Island->SpawnActor<AIslandForestStag>(
		StagGround + FVector(0.f, 0.f, 3.f), (FoxGround - StagGround).Rotation(), Spawn);
	if (Stag) Cleanup.Actors.Add(Stag);
	AIslandForestFox* Fox = Island->SpawnActor<AIslandForestFox>(
		FoxGround + FVector(0.f, 0.f, 3.f), (StagGround - FoxGround).Rotation(), Spawn);
	if (Fox) Cleanup.Actors.Add(Fox);
	const FRotator AsterFacing = ((StagGround + FoxGround) * 0.5f - ResidentGround).Rotation();
	const FTransform AsterTransform(AsterFacing, ResidentGround + FVector(0.f, 0.f, 88.f));
	AAutonomousAgentCharacter* Aster = Island->SpawnActorDeferred<AAutonomousAgentCharacter>(
		ResidentClass, AsterTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Aster)
	{
		Aster->AutoPossessAI = EAutoPossessAI::Disabled;
		Aster->FinishSpawning(AsterTransform);
	}
	if (Aster) Cleanup.Actors.Add(Aster);
	if (!TestNotNull(TEXT("A transient grazing stag is staged at the woodland edge"), Stag) ||
		!TestNotNull(TEXT("A transient awake fox is staged nearby"), Fox) ||
		!TestNotNull(TEXT("Aster is staged as a quiet witness"), Aster))
		return false;

	if (UCharacterMovementComponent* Movement = Aster->GetCharacterMovement()) Movement->Velocity = FVector::ZeroVector;
	UAgentBrainComponent* AsterBrain = Aster->FindComponentByClass<UAgentBrainComponent>();
	if (!TestNotNull(TEXT("Aster's existing brain can receive the fleeting local observation"), AsterBrain))
		return false;

	TestTrue(TEXT("The fox is awake for this encounter"), Fox->CanRespondToQuietObservation());
	Stag->CheckForNearbyFox();
	TestTrue(TEXT("The visible, awake fox draws one brief look from the grazing stag"), Stag->IsQuietlyNoticingFox());
	TestTrue(TEXT("The cue neither startles nor moves the stag"), !Stag->IsStartled() && Stag->GetActorLocation().Equals(StagGround + FVector(0.f, 0.f, 3.f), 1.f));
	const FString Situation = AsterBrain->BuildSituationSummary(FAgentConversationContext());
	TestTrue(TEXT("Aster can see the brief wildlife moment while its reason and outcome remain uncertain"),
		Situation.Contains(TEXT("The stag has briefly lifted its head toward the nearby fox")) &&
		Situation.Contains(TEXT("you cannot know what it will do next")));
	TestFalse(TEXT("The wildlife observation does not imply predation or a chase"),
		Situation.Contains(TEXT("predator")) || Situation.Contains(TEXT("will chase")));

	AIslandWeather* Weather = nullptr;
	for (TActorIterator<AIslandWeather> It(Island); It; ++It) { Weather = *It; break; }
	if (!TestNotNull(TEXT("The Island weather actor can supply its reversible meadow preview"), Weather))
		return false;
	Cleanup.Weather = Weather;
	if (Weather->GroundCoverInstanceCount == 0)
	{
		Weather->InitializeGroundCover();
		Cleanup.bClearGroundCoverPreview = true;
	}
	TestTrue(TEXT("The capture retains live meadow ecology around the animals"),
		Weather->GroundCoverInstanceCount > 0 && Weather->GroundCoverMeadowInstanceCount > 0);
	AddInfo(FString::Printf(TEXT("Transient scene ecology: %d ground-cover instances, %d meadow instances, %d trees, and %d shrubs."),
		Weather->GroundCoverInstanceCount, Weather->GroundCoverMeadowInstanceCount,
		Weather->GroundCoverTreeCount, Weather->GroundCoverShrubCount));

	AIslandDayNight* DayNight = nullptr;
	for (TActorIterator<AIslandDayNight> It(Island); It; ++It) { DayNight = *It; break; }
	if (!TestNotNull(TEXT("The Island day/night actor provides a reversible daylight capture"), DayNight))
		return false;
	Cleanup.DayNight = DayNight;
	Cleanup.OriginalStartHour = DayNight->StartHour;
	Cleanup.OriginalCurrentHour = DayNight->CurrentHour;
	DayNight->StartHour = 11.f;
	DayNight->CurrentHour = 11.f;
	DayNight->Tick(0.f);

	const float LookLength = Stag->LookAroundAnimation ? Stag->LookAroundAnimation->GetPlayLength() : 0.f;
	TestTrue(TEXT("The stag's authored look-around animation is loaded"), LookLength > 0.f);
	if (LookLength > 0.f)
	{
		Stag->GetDeerMesh()->SetPosition(FMath::Clamp(LookLength * 0.62f, 0.f, LookLength - KINDA_SMALL_NUMBER), false);
		Stag->GetDeerMesh()->RefreshBoneTransforms();
	}

	constexpr int32 CaptureWidth = 1600;
	constexpr int32 CaptureHeight = 900;
	UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	Target->RenderTargetFormat = RTF_RGBA8_SRGB;
	Target->InitAutoFormat(CaptureWidth, CaptureHeight);
	Target->UpdateResourceImmediate(true);
	const FVector Focus = (StagGround + FoxGround + ResidentGround) / 3.f + FVector(0.f, 0.f, 105.f);
	const FVector AwayFromArch = (Focus - WindArch->GetActorLocation()).GetSafeNormal2D();
	const FVector PairDirection = (FoxGround - StagGround).GetSafeNormal2D();
	const FVector SideDirection = FVector::CrossProduct(PairDirection, FVector::UpVector).GetSafeNormal();
	const FVector CameraOffsets[] = {
		AwayFromArch * 1050.f + FVector(0.f, 0.f, 220.f),
		-AwayFromArch * 1050.f + FVector(0.f, 0.f, 220.f),
		SideDirection * 1100.f + FVector(0.f, 0.f, 240.f),
		-SideDirection * 1100.f + FVector(0.f, 0.f, 240.f)
	};
	TArray<FVector> SubjectTargets;
	auto AddVisibilitySamples = [&SubjectTargets](const AActor* Actor, float BodyHeight, float HeadHeight, float HalfWidth)
	{
		const FVector Right = Actor->GetActorRightVector() * HalfWidth;
		const FVector Body = Actor->GetActorLocation() + FVector(0.f, 0.f, BodyHeight);
		const FVector Head = Actor->GetActorLocation() + FVector(0.f, 0.f, HeadHeight);
		SubjectTargets.Add(Body);
		SubjectTargets.Add(Body + Right);
		SubjectTargets.Add(Body - Right);
		SubjectTargets.Add(Head);
		SubjectTargets.Add(Head + Right * 0.6f);
		SubjectTargets.Add(Head - Right * 0.6f);
	};
	AddVisibilitySamples(Stag, 80.f, 145.f, 65.f);
	AddVisibilitySamples(Fox, 30.f, 65.f, 36.f);
	AddVisibilitySamples(Aster, 80.f, 150.f, 30.f);
	FVector CameraLocation = Focus + CameraOffsets[0];
	FCollisionQueryParams CameraQuery(SCENE_QUERY_STAT(FoxStagCaptureCamera), false, WindArch);
	CameraQuery.AddIgnoredActor(Stag);
	CameraQuery.AddIgnoredActor(Fox);
	CameraQuery.AddIgnoredActor(Aster);
	int32 BestSubjectBlockers = MAX_int32;
	for (const FVector& Offset : CameraOffsets)
	{
		const FVector Candidate = Focus + Offset;
		int32 Blockers = 0;
		for (const FVector& Subject : SubjectTargets)
		{
			FHitResult Hit;
			if (Island->LineTraceSingleByChannel(Hit, Candidate, Subject, ECC_Visibility, CameraQuery) &&
				Hit.GetActor() != Stag && Hit.GetActor() != Fox && Hit.GetActor() != Aster)
				++Blockers;
		}
		if (Blockers < BestSubjectBlockers)
		{
			BestSubjectBlockers = Blockers;
			CameraLocation = Candidate;
		}
		if (Blockers == 0) break;
	}
	ASceneCapture2D* Camera = Island->SpawnActor<ASceneCapture2D>(
		CameraLocation, (Focus - CameraLocation).Rotation(), Spawn);
	if (Camera) Cleanup.Actors.Add(Camera);
	USceneCaptureComponent2D* Capture = Camera ? Camera->GetCaptureComponent2D() : nullptr;
	if (!TestNotNull(TEXT("The transient real-RHI fox–stag scene capture camera was spawned"), Capture))
		return false;
	Capture->TextureTarget = Target;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->FOVAngle = 46.f;
	Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
	Capture->PostProcessSettings.AutoExposureBias = 0.8f;
	Capture->PostProcessBlendWeight = 1.f;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	IStreamingManager::Get().StreamAllResources(2.f);
	Capture->CaptureScene();
	FlushRenderingCommands();
	TArray<FColor> Pixels;
	const bool bReadPixels = Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) &&
		Pixels.Num() == CaptureWidth * CaptureHeight;
	for (FColor& Pixel : Pixels) Pixel.A = 255;
	const FString CaptureDirectory = FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),
		TEXT("Viewpoints"), TEXT("WoodlandFoxStagAwareness_"), FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
	const FString ImagePath = CaptureDirectory / TEXT("01_Aster_Witnesses_FoxStagGlance.png");
	const bool bSaved = bReadPixels && FImageUtils::SaveImageByExtension(*ImagePath,
		FImageView(Pixels.GetData(), CaptureWidth, CaptureHeight));
	AddInfo(FString::Printf(TEXT("Fox–stag awareness capture: %s; subject blockers=%d; hour=11.0; Aster observation verified."),
		*ImagePath, BestSubjectBlockers));
	TestTrue(TEXT("The 1600x900 real-RHI fox–stag awareness frame is saved"), bSaved);
	return bSaved;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
