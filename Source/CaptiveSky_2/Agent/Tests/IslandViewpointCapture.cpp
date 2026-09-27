// Visual-regression captures of fixed Island viewpoints, rendered offscreen from the open editor
// map. Runs in the editor world only: no play session, agents, gateway, or world-state writes.
// Viewpoints live in Config/IslandViewpoints.json so compositions can change without recompiling.
// Run through Scripts/Capture-Viewpoints.ps1 (it needs a real RHI, so never with -nullrhi).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "IslandDayNight.h"
#include "Components/SceneCaptureComponent2D.h"
#include "ContentStreaming.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TextureResource.h"

namespace
{
	struct FIslandViewpoint
	{
		FString Name;
		FVector From = FVector::ZeroVector;
		FVector LookAt = FVector::ForwardVector;
		float FieldOfView = 60.f;
	};

	/** Resolves {"tag": T, "offset": [x,y,z]} or {"at": [x,y,z]} against the actors in World. */
	bool ResolvePoint(UWorld* World, const TSharedPtr<FJsonObject>& Spec, FVector& Out, FString& Error)
	{
		if (!Spec.IsValid()) { Error = TEXT("missing point"); return false; }
		auto ReadVector = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& Vector)
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!Object->TryGetArrayField(Field, Values) || Values->Num() != 3) return false;
			Vector = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
			return true;
		};
		if (ReadVector(Spec, TEXT("at"), Out)) return true;
		FString Tag;
		if (!Spec->TryGetStringField(TEXT("tag"), Tag)) { Error = TEXT("point needs \"tag\" or \"at\""); return false; }
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->ActorHasTag(FName(*Tag))) continue;
			FVector Offset = FVector::ZeroVector;
			ReadVector(Spec, TEXT("offset"), Offset);
			Out = It->GetActorLocation() + Offset;
			return true;
		}
		Error = FString::Printf(TEXT("no actor tagged %s"), *Tag);
		return false;
	}

	/** Captures each viewpoint over several frames (so exposure and streaming settle), then saves a PNG. */
	class FIslandViewpointCaptureCommand : public IAutomationLatentCommand
	{
	public:
		FIslandViewpointCaptureCommand(UWorld* InWorld, TArray<FIslandViewpoint> InViewpoints, float InHour, FIntPoint InSize, FString InDirectory, FAutomationTestBase* InTest)
			: World(InWorld), Viewpoints(MoveTemp(InViewpoints)), Hour(InHour), Size(InSize), Directory(MoveTemp(InDirectory)), Test(InTest) {}

		virtual bool Update() override
		{
			if (!World.IsValid()) return true;
			if (!bStarted)
			{
				bStarted = true;
				for (TActorIterator<AIslandDayNight> It(World.Get()); It; ++It)
				{
					// Preview lighting at the requested hour, exactly as editing Start Hour would; restored afterwards.
					Clock = *It;
					OriginalStartHour = It->StartHour;
					It->StartHour = Hour;
					It->OnConstruction(It->GetActorTransform());
					break;
				}
				Target.Reset(NewObject<UTextureRenderTarget2D>());
				Target->RenderTargetFormat = RTF_RGBA8_SRGB;
				Target->InitAutoFormat(Size.X, Size.Y);
				Target->UpdateResourceImmediate(true);
			}
			if (Index >= Viewpoints.Num())
			{
				if (Clock.IsValid())
				{
					Clock->StartHour = OriginalStartHour;
					Clock->OnConstruction(Clock->GetActorTransform());
				}
				Test->AddInfo(FString::Printf(TEXT("Viewpoint captures saved in %s"), *Directory));
				return true;
			}
			const FIslandViewpoint& View = Viewpoints[Index];
			if (!Capture.IsValid())
			{
				FActorSpawnParameters Spawn;
				Spawn.ObjectFlags |= RF_Transient;
				Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Capture = World->SpawnActor<ASceneCapture2D>(View.From, (View.LookAt - View.From).Rotation(), Spawn);
				USceneCaptureComponent2D* Component = Capture->GetCaptureComponent2D();
				Component->TextureTarget = Target.Get();
				Component->CaptureSource = SCS_FinalColorLDR;
				Component->FOVAngle = View.FieldOfView;
				Component->bCaptureEveryFrame = false;
				Component->bCaptureOnMovement = false;
				Component->bAlwaysPersistRenderingState = true;
				IStreamingManager::Get().StreamAllResources(2.f);
				Frames = 0;
			}
			Capture->GetCaptureComponent2D()->CaptureScene();
			if (++Frames < 60) return false;

			FlushRenderingCommands();
			TArray<FColor> Pixels;
			if (Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) && Pixels.Num() == Size.X * Size.Y)
			{
				for (FColor& Pixel : Pixels) Pixel.A = 255;
				const FString Path = Directory / (View.Name + TEXT(".png"));
				if (FImageUtils::SaveImageByExtension(*Path, FImageView(Pixels.GetData(), Size.X, Size.Y)))
					Test->AddInfo(FString::Printf(TEXT("%s: camera %s looking at %s -> %s"), *View.Name, *View.From.ToString(), *View.LookAt.ToString(), *Path));
				else Test->AddError(FString::Printf(TEXT("Could not write %s"), *Path));
			}
			else Test->AddError(FString::Printf(TEXT("Could not read pixels for %s"), *View.Name));
			Capture->Destroy();
			Capture.Reset();
			++Index;
			return false;
		}

	private:
		TWeakObjectPtr<UWorld> World;
		TArray<FIslandViewpoint> Viewpoints;
		float Hour;
		FIntPoint Size;
		FString Directory;
		FAutomationTestBase* Test;
		TStrongObjectPtr<UTextureRenderTarget2D> Target;
		TWeakObjectPtr<ASceneCapture2D> Capture;
		TWeakObjectPtr<AIslandDayNight> Clock;
		float OriginalStartHour = 9.f;
		int32 Index = 0;
		int32 Frames = 0;
		bool bStarted = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandViewpointCaptureTest, "CaptiveSky2.Visual.Viewpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandViewpointCaptureTest::RunTest(const FString& Parameters)
{
	if (FApp::CanEverRender() == false)
	{
		AddError(TEXT("Viewpoint capture needs a real RHI; run it without -nullrhi (see Scripts/Capture-Viewpoints.ps1)."));
		return false;
	}
	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
		if (Context.WorldType == EWorldType::Editor && Context.World() && Context.World()->GetMapName() == TEXT("Island")) Island = Context.World();
	if (!TestNotNull(TEXT("Island is the open editor map"), Island)) return false;

	const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("IslandViewpoints.json");
	FString Json;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Json, *ConfigPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
	{
		AddError(FString::Printf(TEXT("Could not read %s"), *ConfigPath));
		return false;
	}
	double Hour = 18.5, Width = 1600, Height = 900;
	Root->TryGetNumberField(TEXT("hour"), Hour);
	Root->TryGetNumberField(TEXT("width"), Width);
	Root->TryGetNumberField(TEXT("height"), Height);
	FParse::Value(FCommandLine::Get(), TEXT("ViewpointHour="), Hour);
	FString Only;
	FParse::Value(FCommandLine::Get(), TEXT("ViewpointOnly="), Only);

	// Log the landmarks so compositions can be planned against real coordinates.
	for (TActorIterator<AActor> It(Island); It; ++It)
		if (It->Tags.Num() > 0 && (It->ActorHasTag(TEXT("IslandLandmark")) || It->ActorHasTag(TEXT("RavenPerch")) || It->GetName().Contains(TEXT("Ocean"))))
			AddInfo(FString::Printf(TEXT("Landmark %s (%s) at %s"), *It->Tags[0].ToString(), *It->GetName(), *It->GetActorLocation().ToString()));

	TArray<FIslandViewpoint> Viewpoints;
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (Root->TryGetArrayField(TEXT("viewpoints"), Entries))
	{
		for (const TSharedPtr<FJsonValue>& Entry : *Entries)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry->TryGetObject(Object)) continue;
			FIslandViewpoint View;
			(*Object)->TryGetStringField(TEXT("name"), View.Name);
			if (!Only.IsEmpty() && !View.Name.Contains(Only)) continue;
			double FieldOfView = 60.0, MinimumHeight = 160.0;
			(*Object)->TryGetNumberField(TEXT("fov"), FieldOfView);
			(*Object)->TryGetNumberField(TEXT("min_height"), MinimumHeight);
			View.FieldOfView = FieldOfView;
			const TSharedPtr<FJsonObject>* From = nullptr;
			const TSharedPtr<FJsonObject>* Look = nullptr;
			FString Error;
			if (!(*Object)->TryGetObjectField(TEXT("from"), From) || !(*Object)->TryGetObjectField(TEXT("look"), Look) ||
				!ResolvePoint(Island, *From, View.From, Error) || !ResolvePoint(Island, *Look, View.LookAt, Error))
			{
				AddError(FString::Printf(TEXT("Viewpoint %s: %s"), *View.Name, Error.IsEmpty() ? TEXT("needs \"from\" and \"look\"") : *Error));
				continue;
			}
			// Keep the camera at eye level or above whatever ground lies beneath it.
			FHitResult Ground;
			if (Island->LineTraceSingleByChannel(Ground, View.From + FVector(0, 0, 5000), View.From - FVector(0, 0, 20000), ECC_Visibility) &&
				View.From.Z < Ground.ImpactPoint.Z + MinimumHeight)
				View.From.Z = Ground.ImpactPoint.Z + MinimumHeight;
			Viewpoints.Add(View);
		}
	}
	if (!TestTrue(TEXT("At least one viewpoint resolved"), Viewpoints.Num() > 0)) return false;
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Viewpoints") /
		FString::Printf(TEXT("%s_h%04.1f"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H%M%S")), Hour));
	ADD_LATENT_AUTOMATION_COMMAND(FIslandViewpointCaptureCommand(Island, MoveTemp(Viewpoints), static_cast<float>(Hour), FIntPoint(FMath::Clamp(static_cast<int32>(Width), 64, 3840), FMath::Clamp(static_cast<int32>(Height), 64, 2160)), Directory, this));
	return true;
}

#endif
