// Visual-regression captures of fixed Island viewpoints, rendered offscreen from the open editor
// map. Runs in the editor world only: no play session, agents, gateway, or world-state writes.
// Viewpoints live in Config/IslandViewpoints.json so compositions can change without recompiling.
// Run through Scripts/Capture-Viewpoints.ps1 (it needs a real RHI, so never with -nullrhi).

#include "Misc/AutomationTest.h"
#include "AgentDataPaths.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "IslandDayNight.h"
#include "IslandWorldStateSubsystem.h"
#include "IslandNest.h"
#include "IslandCurio.h"
#include "IslandArrangement.h"
#include "IslandGuestBook.h"
#include "IslandFirefly.h"
#include "IslandWeather.h"
#include "IslandEnvironmentSubsystem.h"
#include "IslandTideglassSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "ContentStreaming.h"
#include "ShaderCompiler.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "LandscapeComponent.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FLandscapePreviewBackup
	{
		TWeakObjectPtr<ULandscapeComponent> Component;
		TArray<TStrongObjectPtr<UMaterialInterface>> Materials;
	};

	struct FReusedLandscapeMIDBackup
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Material;
		float Wetness = 0.f;
	};

	struct FLandmarkRockPreviewBackup
	{
		TWeakObjectPtr<UStaticMeshComponent> OriginalComponent;
		TWeakObjectPtr<UStaticMesh> OriginalMesh;
		TWeakObjectPtr<UStaticMeshComponent> PreviewComponent;
		ECollisionEnabled::Type OriginalCollisionEnabled = ECollisionEnabled::NoCollision;
		FTransform OriginalTransform = FTransform::Identity;
		bool bOriginalWasVisible = true;
	};

	struct FTideglassWeatherPreview
	{
		TWeakObjectPtr<UMaterialParameterCollectionInstance> Instance;
		float OriginalWindSpeed = 0.f;
		float OriginalStorm = 0.f;
		float OriginalRainIntensity = 0.f;
		float PreviewWindSpeed = 0.f;
		float PreviewStorm = 0.f;
		float PreviewRainIntensity = 0.f;
		bool bEnabled = false;

		void Apply(UWorld* World) const
		{
			if (!bEnabled || !World || !Instance.IsValid()) return;
			Instance->SetScalarParameterValue(TEXT("WindSpeed"), PreviewWindSpeed);
			Instance->SetScalarParameterValue(TEXT("Storm"), PreviewStorm);
			Instance->SetScalarParameterValue(TEXT("RainIntensity"), PreviewRainIntensity);
			World->UpdateParameterCollectionInstances(true, false);
		}

		bool Restore(UWorld* World, FString& OutError) const
		{
			if (!bEnabled) return true;
			if (!World || !Instance.IsValid())
			{
				OutError = TEXT("the world or environment collection instance was destroyed before restoration");
				return false;
			}
			Instance->SetScalarParameterValue(TEXT("WindSpeed"), OriginalWindSpeed);
			Instance->SetScalarParameterValue(TEXT("Storm"), OriginalStorm);
			Instance->SetScalarParameterValue(TEXT("RainIntensity"), OriginalRainIntensity);
			World->UpdateParameterCollectionInstances(true, false);
			float WindSpeed = 0.f, Storm = 0.f, RainIntensity = 0.f;
			if (!Instance->GetScalarParameterValue(TEXT("WindSpeed"), WindSpeed) ||
				!Instance->GetScalarParameterValue(TEXT("Storm"), Storm) ||
				!Instance->GetScalarParameterValue(TEXT("RainIntensity"), RainIntensity) ||
				!FMath::IsNearlyEqual(WindSpeed, OriginalWindSpeed) ||
				!FMath::IsNearlyEqual(Storm, OriginalStorm) ||
				!FMath::IsNearlyEqual(RainIntensity, OriginalRainIntensity))
			{
				OutError = FString::Printf(TEXT("wind/storm/rain were %.3f/%.3f/%.3f, expected %.3f/%.3f/%.3f"),
					WindSpeed, Storm, RainIntensity, OriginalWindSpeed, OriginalStorm, OriginalRainIntensity);
				return false;
			}
			return true;
		}
	};

	// Landscape components render from instances baked in the map, so a SetMaterial swap never reaches the
	// screen. Previewing a different base graph therefore reparents the assigned landscape instance in memory only
	// (nothing is saved) and puts the authored parent back afterwards.
	struct FLandscapeParentSwap
	{
		TWeakObjectPtr<ALandscapeProxy> Proxy;
		TWeakObjectPtr<UMaterialInstanceConstant> Instance;
		TStrongObjectPtr<UMaterialInterface> Parent;
	};
	TArray<FLandscapeParentSwap> GLandscapeParentSwaps;

	int32 SwapLandscapeParent(UWorld* World, UMaterialInterface* NewParent)
	{
		for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
		{
			UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(It->LandscapeMaterial);
			if (!Instance || NewParent == Instance->Parent) continue;
			const bool bKnown = GLandscapeParentSwaps.ContainsByPredicate([Instance](const FLandscapeParentSwap& Swap) { return Swap.Instance == Instance; });
			if (bKnown) continue;
			FLandscapeParentSwap& Swap = GLandscapeParentSwaps.AddDefaulted_GetRef();
			Swap.Proxy = *It;
			Swap.Instance = Instance;
			Swap.Parent.Reset(Instance->Parent);
			Instance->SetParentEditorOnly(NewParent);
			It->UpdateAllComponentMaterialInstances();
			// Each component renders from its own baked instance, which keeps the old parent's shader until recached.
			for (TObjectIterator<UMaterialInstanceConstant> Mic; Mic; ++Mic)
				if (Mic->GetOuter() == *It) Mic->InitStaticPermutation();
			TArray<ULandscapeComponent*> Comps; It->GetComponents<ULandscapeComponent>(Comps);
			for (ULandscapeComponent* C : Comps) C->MarkRenderStateDirty();
		}
		return GLandscapeParentSwaps.Num();
	}

	// Landscape instances are map-baked, so wetness reaches them through the environment collection, not a MID.
	// The environment subsystem does not run in editor worlds, so the capture writes the collection itself.
	void HoldEnvironmentWetness(UWorld* World, float Wetness)
	{
		UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, UIslandEnvironmentSubsystem::CollectionPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!World || !Collection) return;
		World->GetParameterCollectionInstance(Collection)->SetScalarParameterValue(TEXT("Wetness"), FMath::Max(Wetness, 0.f));
		World->UpdateParameterCollectionInstances(true, false);
	}

	void RestoreLandscapeParents()
	{
		for (const FLandscapeParentSwap& Swap : GLandscapeParentSwaps)
		{
			if (!Swap.Instance.IsValid()) continue;
			Swap.Instance->SetParentEditorOnly(Swap.Parent.Get());
			if (!Swap.Proxy.IsValid()) continue;
			ALandscapeProxy* Proxy = Swap.Proxy.Get();
			Proxy->UpdateAllComponentMaterialInstances();
			for (TObjectIterator<UMaterialInstanceConstant> Mic; Mic; ++Mic)
				if (Mic->GetOuter() == Proxy) Mic->InitStaticPermutation();
			TArray<ULandscapeComponent*> Components;
			Proxy->GetComponents<ULandscapeComponent>(Components);
			for (ULandscapeComponent* Component : Components) Component->MarkRenderStateDirty();
		}
		GLandscapeParentSwaps.Reset();
	}

	void RestoreLandscapeWetnessPreview(const TArray<FLandscapePreviewBackup>& Backups,
		const TArray<FReusedLandscapeMIDBackup>& ReusedInstances)
	{
		for (const FReusedLandscapeMIDBackup& Backup : ReusedInstances)
			if (Backup.Material.IsValid())
				Backup.Material->SetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, Backup.Wetness);
		for (const FLandscapePreviewBackup& Backup : Backups)
			if (Backup.Component.IsValid())
				for (int32 Index = 0; Index < Backup.Materials.Num(); ++Index)
					Backup.Component->SetMaterial(Index, Backup.Materials[Index].Get());
	}

	bool IsLandscapeWetnessPreviewRestored(const TArray<FLandscapePreviewBackup>& Backups,
		const TArray<FReusedLandscapeMIDBackup>& ReusedInstances, FString& OutError)
	{
		for (const FLandscapePreviewBackup& Backup : Backups)
		{
			if (!Backup.Component.IsValid()) continue;
			for (int32 Index = 0; Index < Backup.Materials.Num(); ++Index)
			{
				UMaterialInterface* Expected = Backup.Materials[Index].Get();
				UMaterialInterface* Actual = Backup.Component->GetMaterial(Index);
				if (Actual == Expected) continue;

				float ExpectedWetness = 0.f;
				float ActualWetness = 0.f;
				const UMaterialInstanceDynamic* ActualMID = Cast<UMaterialInstanceDynamic>(Actual);
				const bool bEquivalentLandscapeMaterial = ActualMID && ActualMID->Parent == Expected &&
					Expected && Expected->GetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, ExpectedWetness) &&
					ActualMID->GetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, ActualWetness) &&
					FMath::IsNearlyEqual(ExpectedWetness, ActualWetness);
				if (!bEquivalentLandscapeMaterial)
				{
					const FString ActualParent = ActualMID && ActualMID->Parent ? ActualMID->Parent->GetPathName() : TEXT("<no parent>");
					OutError = FString::Printf(TEXT("%s slot %d expected %s, got %s (parent %s, wetness %.3f vs %.3f)"),
						*Backup.Component->GetPathName(), Index, Expected ? *Expected->GetPathName() : TEXT("null"),
						Actual ? *Actual->GetPathName() : TEXT("null"), *ActualParent, ActualWetness, ExpectedWetness);
					return false;
				}
			}
		}
		for (const FReusedLandscapeMIDBackup& Backup : ReusedInstances)
		{
			float RestoredWetness = 0.f;
			if (!Backup.Material.IsValid() || !Backup.Material->GetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, RestoredWetness) ||
				!FMath::IsNearlyEqual(RestoredWetness, Backup.Wetness))
			{
				OutError = FString::Printf(TEXT("reused MID %s did not return to its authored wetness %.3f (got %.3f)"),
					Backup.Material.IsValid() ? *Backup.Material->GetPathName() : TEXT("<invalid>"), Backup.Wetness, RestoredWetness);
				return false;
			}
		}
		return true;
	}

	int32 ApplyLandscapeWetnessPreview(UWorld* World, float Wetness, UMaterialInterface* PreviewParent,
		float PuddleDepthOverride, TArray<FLandscapePreviewBackup>& OutBackups,
		TArray<FReusedLandscapeMIDBackup>& OutReusedInstances, TArray<TWeakObjectPtr<UMaterialInstanceDynamic>>& OutPreviewInstances)
	{
		if (!World) return 0;
		static const FName PuddleDepthParameter(TEXT("Puddle Depth"));
		TMap<UMaterialInterface*, UMaterialInstanceDynamic*> PreviewInstances;
		// Landscape material overrides can be shared by every component on a proxy.
		// Snapshot all slots before the first SetMaterial call so later reads cannot
		// accidentally capture an already-mutated proxy-wide material.
		for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
		{
			TArray<ULandscapeComponent*> Components;
			It->GetComponents<ULandscapeComponent>(Components);
			for (ULandscapeComponent* Component : Components)
			{
				if (!Component) continue;
				FLandscapePreviewBackup& Backup = OutBackups.AddDefaulted_GetRef();
				Backup.Component = Component;
				const int32 MaterialCount = Component->GetNumMaterials();
				Backup.Materials.Reserve(MaterialCount);
				for (int32 Index = 0; Index < MaterialCount; ++Index)
					Backup.Materials.Emplace(Component->GetMaterial(Index));
			}
		}

		int32 ChangedSlots = 0;
		for (const FLandscapePreviewBackup& Backup : OutBackups)
		{
			ULandscapeComponent* Component = Backup.Component.Get();
			if (!Component) continue;
			for (int32 Index = 0; Index < Backup.Materials.Num(); ++Index)
			{
				UMaterialInterface* Original = Backup.Materials[Index].Get();
				UMaterialInterface* Source = PreviewParent ? PreviewParent : Original;
				float AuthoredWetness = 0.f;
				if (!Original || !Source || !Source->GetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, AuthoredWetness)) continue;

				UMaterialInstanceDynamic* Preview = PreviewInstances.FindRef(Source);
				if (!Preview)
				{
					bool bReused = false;
					Preview = PreviewParent
						? UMaterialInstanceDynamic::Create(PreviewParent, Component)
						: UIslandEnvironmentSubsystem::GetOrCreateLandscapeWetnessInstance(Original, Component, bReused);
					if (!Preview) continue;
					PreviewInstances.Add(Source, Preview);
					if (bReused)
					{
						FReusedLandscapeMIDBackup& ReusedBackup = OutReusedInstances.AddDefaulted_GetRef();
						ReusedBackup.Material = Preview;
						ReusedBackup.Wetness = AuthoredWetness;
					}
					Preview->SetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter,
						Wetness >= 0.f ? Wetness : AuthoredWetness);
					if (PuddleDepthOverride >= 0.f)
						Preview->SetScalarParameterValue(PuddleDepthParameter, PuddleDepthOverride);
					OutPreviewInstances.Add(Preview);
				}
				Component->SetMaterial(Index, Preview);
				++ChangedSlots;
			}
		}
		return ChangedSlots;
	}

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
		FIslandViewpointCaptureCommand(UWorld* InWorld, TArray<FIslandViewpoint> InViewpoints, float InHour, FIntPoint InSize, FString InDirectory,
		FAutomationTestBase* InTest, AIslandDayNight* InClock, float InOriginalStartHour, int32 InOriginalDayNumber, TArray<TWeakObjectPtr<AActor>> InPreviewActors,
			AIslandWeather* InGroundCoverWeather, bool bInClearGroundCover, TArray<FLandscapePreviewBackup> InLandscapeBackups,
			TArray<FReusedLandscapeMIDBackup> InReusedLandscapeInstances, bool bInCompareLandscapeWetness,
			bool bInPuddlePreview,
			TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> InLandscapePreviewInstances,
			FTideglassWeatherPreview InTideglassWeatherPreview)
			: PreviewActors(MoveTemp(InPreviewActors)), World(InWorld), Viewpoints(MoveTemp(InViewpoints)), Hour(InHour), Size(InSize),
			  Test(InTest), Clock(InClock), GroundCoverWeather(InGroundCoverWeather), BaseDirectory(MoveTemp(InDirectory)), Directory(BaseDirectory),
			  OriginalStartHour(InOriginalStartHour), OriginalDayNumber(InOriginalDayNumber), bClearGroundCover(bInClearGroundCover),
			  LandscapeBackups(MoveTemp(InLandscapeBackups)), ReusedLandscapeInstances(MoveTemp(InReusedLandscapeInstances)),
			  LandscapePreviewInstances(MoveTemp(InLandscapePreviewInstances)), bCompareLandscapeWetness(bInCompareLandscapeWetness),
			  bPuddlePreview(bInPuddlePreview), TideglassWeatherPreview(MoveTemp(InTideglassWeatherPreview))
		{
			if (bCompareLandscapeWetness) Directory = BaseDirectory + TEXT("_authored_dry");
		}

		virtual bool Update() override
		{
			if (!World.IsValid()) return true;
			TideglassWeatherPreview.Apply(World.Get());
			if (!bStarted)
			{
				bStarted = true;
				for (TActorIterator<AIslandDayNight> It(World.Get()); It; ++It)
				{
					// Preview lighting at the requested hour, exactly as editing Start Hour would; restored afterwards.
					if (!Clock.IsValid())
					{
						Clock = *It;
						OriginalStartHour = It->StartHour;
					}
					It->StartHour = Hour;
					It->OnConstruction(It->GetActorTransform());
					break;
				}
				SpawnLastingChanges();
				Target.Reset(NewObject<UTextureRenderTarget2D>());
				Target->RenderTargetFormat = RTF_RGBA8_SRGB;
				Target->InitAutoFormat(Size.X, Size.Y);
				Target->UpdateResourceImmediate(true);
			}
			if (Index >= Viewpoints.Num())
			{
				if (bCompareLandscapeWetness && !bCapturingWetLandscape)
				{
					bCapturingWetLandscape = true;
					Index = 0;
					Directory = BaseDirectory + TEXT("_fully_wet");
					HoldEnvironmentWetness(World.Get(), 1.f);
					for (const TWeakObjectPtr<UMaterialInstanceDynamic>& Material : LandscapePreviewInstances)
						if (Material.IsValid())
							Material->SetScalarParameterValue(UIslandEnvironmentSubsystem::LandscapeWetnessParameter, 1.f);
					Test->AddInfo(bPuddlePreview
						? TEXT("Capturing the puddle-enabled prototype at full collection wetness; the paired dry capture used zero collection wetness on the same baked-parent graph.")
						: TEXT("Capturing the same viewpoints fully wet in the same editor world; authored-dry capture is retained for comparison."));
					return false;
				}
				RestoreLandscapeWetnessPreview(LandscapeBackups, ReusedLandscapeInstances);
				FString WeatherRestoreError;
				if (!TideglassWeatherPreview.Restore(World.Get(), WeatherRestoreError))
					Test->AddError(FString::Printf(TEXT("Tideglass weather preview failed to restore the environment collection: %s"), *WeatherRestoreError));
				RestoreLandscapeParents();
				HoldEnvironmentWetness(World.Get(), -1.f);
				FString RestoreError;
				if (!IsLandscapeWetnessPreviewRestored(LandscapeBackups, ReusedLandscapeInstances, RestoreError))
					Test->AddError(FString::Printf(TEXT("Landscape wetness preview failed to restore original material assignments and values: %s"), *RestoreError));
				LandscapeBackups.Reset();
				ReusedLandscapeInstances.Reset();
				if (Clock.IsValid())
				{
					Clock->StartHour = OriginalStartHour;
					Clock->DayNumber = OriginalDayNumber;
					Clock->OnConstruction(Clock->GetActorTransform());
				}
				for (const TWeakObjectPtr<AActor>& Preview : PreviewActors)
					if (Preview.IsValid()) Preview->Destroy();
				PreviewActors.Reset();
				if (bClearGroundCover && GroundCoverWeather.IsValid()) GroundCoverWeather->ClearGroundCoverPreview();
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
			// A swapped-in landscape graph can take minutes to compile; capturing earlier shows the previous look.
			if (Frames == 1 && GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
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
		/**
		 * Shows what residents have left behind (nests, curios, stone arrangements, and guest-book lines)
		 * by spawning transient previews from WorldState/<Map>.json into the editor world for the capture.
		 * Reading the file changes nothing; -ViewpointNoWorldState captures the bare map instead.
		 */
		void SpawnLastingChanges()
		{
			AIslandGuestBook* Book = nullptr;
			for (TActorIterator<AActor> It(World.Get()); It; ++It)
				if (It->ActorHasTag(TEXT("IslandInn")) && It->ActorHasTag(TEXT("InnCounter")))
				{
					const FVector Location = It->GetActorTransform().TransformPosition(FVector(0.f, 0.f, 60.f));
					FActorSpawnParameters BookSpawn;
					BookSpawn.ObjectFlags |= RF_Transient;
					BookSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					Book = World->SpawnActor<AIslandGuestBook>(AIslandGuestBook::StaticClass(), FTransform(It->GetActorRotation(), Location), BookSpawn);
					if (Book) PreviewActors.Add(Book);
					break;
				}
			if (FParse::Param(FCommandLine::Get(), TEXT("ViewpointNoWorldState"))) return;
			const FString Path = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("WorldState") / (World->GetMapName() + TEXT(".json")));
			UIslandWorldStateSubsystem* Reader = NewObject<UIslandWorldStateSubsystem>(GetTransientPackage());
			if (!FPaths::FileExists(Path) || !Reader->ReadStateFile(Path))
			{
				Test->AddInfo(FString::Printf(TEXT("No readable world state at %s; capturing the bare map."), *Path));
				return;
			}
			if (Book)
			{
				FString Display;
				const TArray<FIslandGuestBookEntry>& Entries = Reader->GetGuestBookEntries();
				for (int32 EntryIndex = FMath::Max(0, Entries.Num() - 3); EntryIndex < Entries.Num(); ++EntryIndex)
				{
					const FIslandGuestBookEntry& Entry = Entries[EntryIndex];
					if (!Display.IsEmpty()) Display += TEXT("\n");
					Display += FString::Printf(TEXT("Day %d - %s\n%s"), Entry.Day, *Entry.AgentId.Left(12), *Entry.Line.Left(34));
				}
				Book->SetDisplayText(Display);
			}
			const int32 Today = Reader->GetSavedDay().Get(1);
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			for (const FIslandNestRecord& Nest : Reader->GetNests())
				if (AIslandNest* Actor = World->SpawnActor<AIslandNest>(Nest.Location, FRotator::ZeroRotator, Spawn))
				{
					Actor->SetWoven(Nest.SiteTag, Nest.Layers);
					PreviewActors.Add(Actor);
				}
			for (const FIslandCurioRecord& Curio : Reader->GetCurios())
				if (AIslandCurio* Actor = World->SpawnActor<AIslandCurio>(Curio.Location + FVector(0.f, 0.f, AIslandCurio::GroundClearance), FRotator::ZeroRotator, Spawn))
				{
					Actor->ShowRecord(Curio);
					PreviewActors.Add(Actor);
				}
			for (const FIslandArrangementSite& Site : Reader->GetArrangementSites())
				if (AIslandArrangement* Actor = World->SpawnActor<AIslandArrangement>(Site.Location, FRotator::ZeroRotator, Spawn))
				{
					Actor->ShowSite(Site, Today);
					PreviewActors.Add(Actor);
				}
			Test->AddInfo(FString::Printf(TEXT("Showing %d nest(s), %d curio(s), and %d arranging ground(s) from %s (Island day %d)."),
				Reader->GetNests().Num(), Reader->GetCurios().Num(), Reader->GetArrangementSites().Num(), *Path, Today));
			if (Book) Test->AddInfo(TEXT("The transient open guest book is showing its latest saved lines at the inn counter."));
		}

		TArray<TWeakObjectPtr<AActor>> PreviewActors;
		TWeakObjectPtr<UWorld> World;
		TArray<FIslandViewpoint> Viewpoints;
		float Hour;
		FIntPoint Size;
		FAutomationTestBase* Test;
		TStrongObjectPtr<UTextureRenderTarget2D> Target;
		TWeakObjectPtr<ASceneCapture2D> Capture;
		TWeakObjectPtr<AIslandDayNight> Clock;
		TWeakObjectPtr<AIslandWeather> GroundCoverWeather;
		FString BaseDirectory;
		FString Directory;
		float OriginalStartHour = 9.f;
		int32 OriginalDayNumber = 1;
		int32 Index = 0;
		int32 Frames = 0;
		bool bStarted = false;
		bool bClearGroundCover = false;
		TArray<FLandscapePreviewBackup> LandscapeBackups;
		TArray<FReusedLandscapeMIDBackup> ReusedLandscapeInstances;
		TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> LandscapePreviewInstances;
		bool bCompareLandscapeWetness = false;
		bool bCapturingWetLandscape = false;
		bool bPuddlePreview = false;
		FTideglassWeatherPreview TideglassWeatherPreview;
	};

	/** Puts a previewed static-mesh material back once the captures are done. */
	class FRestoreStaticMeshMaterialCommand : public IAutomationLatentCommand
	{
	public:
		FRestoreStaticMeshMaterialCommand(UStaticMeshComponent* InComponent, UMaterialInterface* InOriginal)
			: Component(InComponent), Original(InOriginal) {}

		virtual bool Update() override
		{
			if (Component.IsValid()) Component->SetMaterial(0, Original.Get());
			return true;
		}

	private:
		TWeakObjectPtr<UStaticMeshComponent> Component;
		TStrongObjectPtr<UMaterialInterface> Original;
	};

	/** Removes transient rock visuals after the capture and restores each blockout component's visibility. */
	class FRestoreLandmarkRockPreviewCommand : public IAutomationLatentCommand
	{
	public:
		FRestoreLandmarkRockPreviewCommand(TArray<FLandmarkRockPreviewBackup> InBackups, FAutomationTestBase* InTest)
			: Backups(MoveTemp(InBackups)), Test(InTest) {}

		virtual bool Update() override
		{
			for (FLandmarkRockPreviewBackup& Backup : Backups)
			{
				if (Backup.OriginalComponent.IsValid())
				{
					Backup.OriginalComponent->SetVisibility(Backup.bOriginalWasVisible, false);
					const bool bOriginalStatePreserved = Backup.OriginalComponent->IsVisible() == Backup.bOriginalWasVisible &&
						Backup.OriginalComponent->GetStaticMesh() == Backup.OriginalMesh.Get() &&
						Backup.OriginalComponent->GetCollisionEnabled() == Backup.OriginalCollisionEnabled &&
						Backup.OriginalComponent->GetComponentTransform().Equals(Backup.OriginalTransform);
					if (!bOriginalStatePreserved && Test)
						Test->AddError(TEXT("Landmark rock preview did not restore the blockout component's original visibility or preserve its mesh, collision, and transform."));
				}
				if (Backup.PreviewComponent.IsValid()) Backup.PreviewComponent->DestroyComponent();
			}
			Backups.Reset();
			return true;
		}

	private:
		TArray<FLandmarkRockPreviewBackup> Backups;
		FAutomationTestBase* Test = nullptr;
	};

	class FRestoreTideglassSurfacePreviewCommand : public IAutomationLatentCommand
	{
	public:
		FRestoreTideglassSurfacePreviewCommand(UStaticMeshComponent* InBlockout, UProceduralMeshComponent* InPreview,
			bool bInWasVisible, bool bInWasHiddenInGame)
			: Blockout(InBlockout), Preview(InPreview), bWasVisible(bInWasVisible), bWasHiddenInGame(bInWasHiddenInGame) {}

		virtual bool Update() override
		{
			if (Preview.IsValid())
				if (AActor* Owner = Preview->GetOwner()) Owner->Destroy();
			if (Blockout.IsValid())
			{
				Blockout->SetVisibility(bWasVisible);
				Blockout->SetHiddenInGame(bWasHiddenInGame);
			}
			return true;
		}

	private:
		TWeakObjectPtr<UStaticMeshComponent> Blockout;
		TStrongObjectPtr<UProceduralMeshComponent> Preview;
		bool bWasVisible = true;
		bool bWasHiddenInGame = false;
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
	int32 PreviewDayNumber = 0;
	FParse::Value(FCommandLine::Get(), TEXT("ViewpointDay="), PreviewDayNumber);
	double LandscapeWetness = -1.0;
	const bool bLandscapeWetnessPreview = FParse::Value(FCommandLine::Get(), TEXT("ViewpointLandscapeWetness="), LandscapeWetness);
	const bool bCompareLandscapeWetness = FParse::Param(FCommandLine::Get(), TEXT("ViewpointLandscapeWetnessPair"));
	const bool bPuddlePreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointLandscapePuddlePreview"));
	FString LandscapeMaterialPath;
	const bool bLandscapeMaterialOverride = FParse::Value(FCommandLine::Get(), TEXT("ViewpointLandscapeMaterial="), LandscapeMaterialPath);
	FString LandscapeParentPath;
	const bool bLandscapeParentOverride = FParse::Value(FCommandLine::Get(), TEXT("ViewpointLandscapeParent="), LandscapeParentPath);
	UMaterialInterface* LandscapePreviewParent = nullptr;
	if (bPuddlePreview)
	{
		if (!bCompareLandscapeWetness || !bLandscapeMaterialOverride)
		{
			AddError(TEXT("Landscape puddle preview requires -ViewpointLandscapeWetnessPair and -ViewpointLandscapeMaterial."));
			return false;
		}
		LandscapePreviewParent = LoadObject<UMaterialInterface>(nullptr, *LandscapeMaterialPath);
		if (!TestNotNull(TEXT("Puddle preview material asset loaded"), LandscapePreviewParent))
		{
			AddError(FString::Printf(TEXT("Could not load puddle preview material %s."), *LandscapeMaterialPath));
			return false;
		}
		if (bLandscapeParentOverride)
		{
			AddError(TEXT("Choose either -ViewpointLandscapeParent or -ViewpointLandscapePuddlePreview; the latter now swaps its material through the baked landscape parent."));
			return false;
		}
	}
	else if (bLandscapeMaterialOverride)
	{
		AddError(TEXT("-ViewpointLandscapeMaterial is only supported with -ViewpointLandscapePuddlePreview."));
		return false;
	}
	if (bLandscapeWetnessPreview && bCompareLandscapeWetness)
	{
		AddError(TEXT("Choose either one -LandscapeWetness value or -CompareLandscapeWetness, not both."));
		return false;
	}
	if (bLandscapeWetnessPreview && (LandscapeWetness < 0.0 || LandscapeWetness > 1.0))
	{
		AddError(TEXT("Landscape wetness preview must be between 0 and 1."));
		return false;
	}
	const bool bNightFireflyPreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointNightFireflies"));
	const bool bGroundCoverSwayPreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointGroundCoverSway"));
	const bool bGroundCoverPreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointGroundCover")) || bGroundCoverSwayPreview;
	const bool bLandmarkRockPreview = FParse::Param(FCommandLine::Get(), TEXT("ViewpointLandmarkRockPreview"));
	FString Only;
	FParse::Value(FCommandLine::Get(), TEXT("ViewpointOnly="), Only);
	FString TideglassMaterialPath;
	const bool bTideglassMaterialOverride = FParse::Value(FCommandLine::Get(), TEXT("ViewpointTideglassMaterial="), TideglassMaterialPath);
	FString TideglassWeatherMode;
	const bool bTideglassWeatherOverride = FParse::Value(FCommandLine::Get(), TEXT("ViewpointTideglassWeather="), TideglassWeatherMode);
	if (bTideglassWeatherOverride && (!bTideglassMaterialOverride || (!Only.IsEmpty() && Only != TEXT("Tideglass"))))
	{
		AddError(TEXT("Tideglass weather preview requires a Tideglass material and -ViewpointOnly=Tideglass."));
		return false;
	}
	if (bTideglassWeatherOverride && TideglassWeatherMode != TEXT("Calm") && TideglassWeatherMode != TEXT("Storm"))
	{
		AddError(TEXT("ViewpointTideglassWeather must be Calm or Storm."));
		return false;
	}
	AIslandDayNight* PreviewClock = nullptr;
	float OriginalStartHour = 9.f;
	int32 OriginalDayNumber = 1;
	TArray<TWeakObjectPtr<AActor>> PreviewActors;
	AIslandWeather* PreviewWeather = nullptr;
	if (bNightFireflyPreview)
	{
		if (!(Hour >= 19.0 || Hour < 5.0))
		{
			AddError(TEXT("Night firefly preview requires a night hour (19:00–05:00); pass -Hour 20."));
			return false;
		}
		for (TActorIterator<AIslandDayNight> It(Island); It; ++It)
		{
			PreviewClock = *It;
			OriginalStartHour = It->StartHour;
			OriginalDayNumber = It->DayNumber;
			It->StartHour = Hour;
			if (PreviewDayNumber > 0) It->DayNumber = PreviewDayNumber;
			It->OnConstruction(It->GetActorTransform());
			break;
		}
		for (TActorIterator<AIslandWeather> It(Island); It; ++It) { PreviewWeather = *It; break; }
		if (!PreviewClock || !PreviewWeather)
		{
			if (PreviewClock)
			{
				PreviewClock->StartHour = OriginalStartHour;
				PreviewClock->DayNumber = OriginalDayNumber;
				PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
			}
			AddError(TEXT("Night firefly preview needs the Island day/night and weather actors."));
			return false;
		}
		TSet<AIslandFirefly*> ExistingFireflies;
		for (TActorIterator<AIslandFirefly> It(Island); It; ++It) ExistingFireflies.Add(*It);
		PreviewWeather->RefreshNightEcology();
		AActor* Stones = nullptr;
		for (TActorIterator<AActor> It(Island); It; ++It)
			if (It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"))) { Stones = *It; break; }
		AIslandFirefly* RouteFirefly = nullptr;
		float NearestStoneDistance = TNumericLimits<float>::Max();
		for (TActorIterator<AIslandFirefly> It(Island); It; ++It)
		{
			if (ExistingFireflies.Contains(*It)) continue;
			PreviewActors.Add(*It);
			if (UPointLightComponent* Glow = It->FindComponentByClass<UPointLightComponent>())
			{
				It->DayNight = PreviewClock;
				It->Phase = PI / 2.f - static_cast<float>(Island->GetTimeSeconds()) * 4.2f;
				It->PulseRate = 1.f;
				It->ObservationPulseRemaining = 0.f;
				It->ChimeResponseRemaining = 0.f;
				It->UpdateGlow(Island->GetTimeSeconds(), PreviewWeather->SampleRainIntensity(Island->GetTimeSeconds()));
				Glow->SetVisibility(true);
			}
			if (Stones)
			{
				const float Distance = FVector::Dist(It->GetActorLocation(), Stones->GetActorLocation());
				if (Distance < NearestStoneDistance) { NearestStoneDistance = Distance; RouteFirefly = *It; }
			}
		}
		if (!RouteFirefly || PreviewActors.IsEmpty())
		{
			for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
			PreviewActors.Reset();
			PreviewClock->StartHour = OriginalStartHour;
			PreviewClock->DayNumber = OriginalDayNumber;
			PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
			AddError(TEXT("The Island map did not produce a route-side night firefly for capture."));
			return false;
		}
		RouteFirefly->Tags.AddUnique(TEXT("NightPreviewFirefly"));
		AddInfo(FString::Printf(TEXT("Night ecology preview spawned %d transient firefly actors; route-side light is %.0f cm from ListeningStones."),
			PreviewActors.Num(), NearestStoneDistance));
	}
	if (bGroundCoverPreview)
	{
		if (!PreviewWeather)
			for (TActorIterator<AIslandWeather> It(Island); It; ++It) { PreviewWeather = *It; break; }
		if (!PreviewWeather)
		{
			for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
			if (PreviewClock)
			{
				PreviewClock->StartHour = OriginalStartHour;
				PreviewClock->DayNumber = OriginalDayNumber;
				PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
			}
			AddError(TEXT("Ground-cover preview needs the Island weather actor."));
			return false;
		}
		PreviewWeather->InitializeGroundCover();
		TestTrue(TEXT("The Island landscape receives at least one separated meadow patch"), PreviewWeather->GroundCoverMeadowInstanceCount > 0);
		TestTrue(TEXT("Landscape and landmark instances stay within the bounded 46,764-instance budget"),
			PreviewWeather->GroundCoverMeadowInstanceCount <= 46080 && PreviewWeather->GroundCoverInstanceCount <= 46764);
		TestTrue(TEXT("Existing interactive-spruce forest mesh is available"),
			PreviewWeather->IslandSpruce && PreviewWeather->IslandSpruce->GetStaticMesh() != nullptr);
		TestTrue(TEXT("Deterministic spruce groves add a bounded population without affecting collision or navigation"),
			PreviewWeather->GroundCoverTreeCount > 0 && PreviewWeather->GroundCoverTreeCount <= 160 && PreviewWeather->IslandSpruce &&
			PreviewWeather->IslandSpruce->GetInstanceCount() == PreviewWeather->GroundCoverTreeCount &&
			PreviewWeather->IslandSpruce->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
			!PreviewWeather->IslandSpruce->CanEverAffectNavigation());
		AddInfo(FString::Printf(TEXT("Transient ground-cover preview placed %d nonblocking instances (%d + %d grass clumps, %d + %d + %d ground plants), including %d exposed-hillside patch instances and %d spruce trees."),
			PreviewWeather->GroundCoverInstanceCount, PreviewWeather->ShoreGrassA->GetInstanceCount(), PreviewWeather->ShoreGrassB->GetInstanceCount(),
			PreviewWeather->ShoreGroundPlants->GetInstanceCount(), PreviewWeather->ShoreGroundPlantLowA->GetInstanceCount(),
			PreviewWeather->ShoreGroundPlantLowB->GetInstanceCount(), PreviewWeather->GroundCoverMeadowInstanceCount, PreviewWeather->GroundCoverTreeCount));
		if (bGroundCoverSwayPreview)
		{
			AActor* Tideglass = nullptr;
			AActor* ListeningStones = nullptr;
			for (TActorIterator<AActor> It(Island); It; ++It)
			{
				if (!Tideglass && It->ActorHasTag(TEXT("TideglassPool")) && It->ActorHasTag(TEXT("IslandLandmark"))) Tideglass = *It;
				if (!ListeningStones && It->ActorHasTag(TEXT("ListeningStones")) && It->ActorHasTag(TEXT("IslandLandmark"))) ListeningStones = *It;
			}
			if (!Tideglass || !ListeningStones)
			{
				PreviewWeather->ClearGroundCoverPreview();
				for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
				if (PreviewClock)
				{
					PreviewClock->StartHour = OriginalStartHour;
					PreviewClock->DayNumber = OriginalDayNumber;
					PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
				}
				AddError(TEXT("Ground-cover sway preview requires both tagged TideglassPool and ListeningStones landmarks."));
				return false;
			}
			PreviewWeather->AddTransientGust(Tideglass->GetActorLocation(), FVector(0.857f, -0.514f, 0.f), 300.f, 1800.f, 18.f);
			PreviewWeather->AddTransientGust(ListeningStones->GetActorLocation(), FVector(0.f, 1.f, 0.f), 300.f, 1800.f, 18.f);
			PreviewWeather->UpdateGroundCoverSway();
			float MaximumVisibleSwayDegrees = 0.f;
			auto MeasureMaximumSway = [&MaximumVisibleSwayDegrees](UHierarchicalInstancedStaticMeshComponent* Grass, const TArray<FTransform>& Baselines)
			{
				for (int32 Index = 0; Grass && Index < Baselines.Num(); ++Index)
				{
					FTransform Current;
					if (Grass->GetInstanceTransform(Index, Current, false))
						MaximumVisibleSwayDegrees = FMath::Max(MaximumVisibleSwayDegrees,
							FMath::RadiansToDegrees(Baselines[Index].GetRotation().AngularDistance(Current.GetRotation())));
				}
			};
			MeasureMaximumSway(PreviewWeather->ShoreGrassA, PreviewWeather->ShoreGrassABaseTransforms);
			MeasureMaximumSway(PreviewWeather->ShoreGrassB, PreviewWeather->ShoreGrassBBaseTransforms);
			MeasureMaximumSway(PreviewWeather->ShoreGroundPlants, PreviewWeather->ShoreGroundPlantBaseTransforms);
			MeasureMaximumSway(PreviewWeather->ShoreGroundPlantLowA, PreviewWeather->ShoreGroundPlantLowABaseTransforms);
			MeasureMaximumSway(PreviewWeather->ShoreGroundPlantLowB, PreviewWeather->ShoreGroundPlantLowBBaseTransforms);
			TestTrue(TEXT("Fixed preview gusts move at least one shore-grass clump"), MaximumVisibleSwayDegrees > 0.1f);
			TestTrue(TEXT("Fixed preview gusts respect the ten-degree response limit"), MaximumVisibleSwayDegrees <= 10.01f);
			AddInfo(FString::Printf(TEXT("Applied two fixed transient preview gusts; maximum measured clump sway is %.2f degrees. No weather/world state was saved."), MaximumVisibleSwayDegrees));
		}
	}

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
			bool bRequiresNightFireflies = false;
			(*Object)->TryGetBoolField(TEXT("night_fireflies"), bRequiresNightFireflies);
			if (bRequiresNightFireflies && !bNightFireflyPreview) continue;
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
			// Keep the camera at eye level or above whatever ground lies beneath it; a negative min_height
			// (interior shots) opts out, since the trace from above would land on the roof.
			FHitResult Ground;
			if (MinimumHeight >= 0.0 && Island->LineTraceSingleByChannel(Ground, View.From + FVector(0, 0, 5000), View.From - FVector(0, 0, 20000), ECC_Visibility) &&
				View.From.Z < Ground.ImpactPoint.Z + MinimumHeight)
				View.From.Z = Ground.ImpactPoint.Z + MinimumHeight;
			Viewpoints.Add(View);
		}
	}
	if (!TestTrue(TEXT("At least one viewpoint resolved"), Viewpoints.Num() > 0))
	{
		for (const TWeakObjectPtr<AActor>& Preview : PreviewActors) if (Preview.IsValid()) Preview->Destroy();
		if (bGroundCoverPreview && PreviewWeather) PreviewWeather->ClearGroundCoverPreview();
		if (PreviewClock)
		{
			PreviewClock->StartHour = OriginalStartHour;
			PreviewClock->DayNumber = OriginalDayNumber;
			PreviewClock->OnConstruction(PreviewClock->GetActorTransform());
		}
		return false;
	}
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Viewpoints") /
		FString::Printf(TEXT("%s_h%04.1f"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H%M%S")), Hour));
	TArray<FLandscapePreviewBackup> LandscapeBackups;
	TArray<FReusedLandscapeMIDBackup> ReusedLandscapeInstances;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> LandscapePreviewInstances;
	if (bLandscapeParentOverride || bPuddlePreview)
	{
		UMaterialInterface* Parent = bPuddlePreview
			? LandscapePreviewParent
			: LoadObject<UMaterialInterface>(nullptr, *LandscapeParentPath);
		const FString& ParentPath = bPuddlePreview ? LandscapeMaterialPath : LandscapeParentPath;
		if (!TestNotNull(TEXT("Landscape preview parent material loaded"), Parent)) return false;
		if (!TestTrue(TEXT("Landscape parent swap reached an assigned landscape instance"), SwapLandscapeParent(Island, Parent) > 0))
			return false;
		AddInfo(FString::Printf(TEXT("Previewing landscape graph %s through the baked component instances in memory; the authored parent will be restored after capture."), *ParentPath));
	}
	if (bLandscapeWetnessPreview || bCompareLandscapeWetness)
	{
		// The environment subsystem lazily replaces authored landscape materials with
		// its own wetness MIDs on first tick. Prime it before taking the preview backup
		// so this test cannot race that one-time assignment during capture.
		if (UIslandEnvironmentSubsystem* Environment = Island->GetSubsystem<UIslandEnvironmentSubsystem>())
			Environment->Tick(0.f);

		const float PreviewWetness = bCompareLandscapeWetness ? 0.f : static_cast<float>(LandscapeWetness);
		if (bLandscapeWetnessPreview || bCompareLandscapeWetness) HoldEnvironmentWetness(Island, PreviewWetness);
		if (bPuddlePreview)
		{
			AddInfo(TEXT("Puddle preview uses the baked-parent swap and environment collection; no ineffective per-component material override is applied."));
		}
		else
		{
			const int32 ChangedSlots = ApplyLandscapeWetnessPreview(Island, PreviewWetness, nullptr,
				-1.f, LandscapeBackups, ReusedLandscapeInstances, LandscapePreviewInstances);
			if (!TestTrue(TEXT("Landscape wetness preview affects at least one parameterized material slot"), ChangedSlots > 0))
			{
				RestoreLandscapeWetnessPreview(LandscapeBackups, ReusedLandscapeInstances);
				RestoreLandscapeParents();
				HoldEnvironmentWetness(Island, -1.f);
				return false;
			}
			if (bCompareLandscapeWetness)
				AddInfo(FString::Printf(TEXT("Prepared an authored-dry/full-wet landscape comparison across %d transient material slot(s); authored material parent and wetness will be restored after capture."), ChangedSlots));
			else
				AddInfo(FString::Printf(TEXT("Applied transient Ground Wetness %.2f to %d landscape material slot(s); authored material parent and wetness will be restored after capture."), LandscapeWetness, ChangedSlots));
		}
	}
	UStaticMeshComponent* OceanMesh = nullptr;
	UMaterialInterface* OriginalOceanMaterial = nullptr;
	FString OceanMaterialPath;
	if (FParse::Value(FCommandLine::Get(), TEXT("ViewpointOceanMaterial="), OceanMaterialPath))
	{
		UMaterialInterface* OceanMaterial = LoadObject<UMaterialInterface>(nullptr, *OceanMaterialPath);
		for (TActorIterator<AStaticMeshActor> It(Island); It && !OceanMesh; ++It)
			if (It->GetActorLabel() == TEXT("OceanPlane")) OceanMesh = It->GetStaticMeshComponent();
		if (!OceanMaterial || !OceanMesh)
		{
			AddError(FString::Printf(TEXT("Ocean preview needs a loadable material (%s) and an actor labelled OceanPlane."), *OceanMaterialPath));
			return false;
		}
		OriginalOceanMaterial = OceanMesh->GetMaterial(0);
		OceanMesh->SetMaterial(0, OceanMaterial);
		AddInfo(FString::Printf(TEXT("Transient ocean material preview %s on OceanPlane; the authored material is restored after capture."), *OceanMaterialPath));
	}
	UStaticMeshComponent* TideglassMesh = nullptr;
	UProceduralMeshComponent* TideglassPreviewSurface = nullptr;
	bool bTideglassWasVisible = true;
	bool bTideglassWasHiddenInGame = false;
	FTideglassWeatherPreview TideglassWeather;
	if (bTideglassMaterialOverride)
	{
		UMaterialInterface* TideglassMaterial = LoadObject<UMaterialInterface>(nullptr, *TideglassMaterialPath);
		TideglassMesh = UIslandTideglassSubsystem::FindPoolSurface(Island);
		if (!TideglassMaterial || !TideglassMesh)
		{
			if (OceanMesh) OceanMesh->SetMaterial(0, OriginalOceanMaterial);
			AddError(FString::Printf(TEXT("Tideglass preview needs a loadable material (%s) and the flattened sphere beside the TideglassPool marker."), *TideglassMaterialPath));
			return false;
		}
		bTideglassWasVisible = TideglassMesh->IsVisible();
		bTideglassWasHiddenInGame = TideglassMesh->bHiddenInGame;
		TideglassPreviewSurface = UIslandTideglassSubsystem::CreatePoolSurfaceMesh(TideglassMesh);
		if (!TideglassPreviewSurface)
		{
			if (OceanMesh) OceanMesh->SetMaterial(0, OriginalOceanMaterial);
			AddError(TEXT("Could not create the transient irregular Tideglass surface over the blockout footprint."));
			return false;
		}
		TideglassMesh->SetVisibility(false);
		TideglassMesh->SetHiddenInGame(true);
		TideglassPreviewSurface->SetMaterial(0, TideglassMaterial);
		const FProcMeshSection* TideglassSection = TideglassPreviewSurface->GetProcMeshSection(0);
		AddInfo(FString::Printf(TEXT("Transient Tideglass preview %s: surface at %s bounds origin %s extent %s, registered=%s visible=%s hiddenInGame=%s actorHidden=%s, section vertices=%d indices=%d; footprint at %s."),
			*TideglassMaterialPath, *TideglassPreviewSurface->GetComponentLocation().ToString(),
			*TideglassPreviewSurface->Bounds.Origin.ToString(), *TideglassPreviewSurface->Bounds.BoxExtent.ToString(),
			TideglassPreviewSurface->IsRegistered() ? TEXT("true") : TEXT("false"),
			TideglassPreviewSurface->IsVisible() ? TEXT("true") : TEXT("false"),
			TideglassPreviewSurface->bHiddenInGame ? TEXT("true") : TEXT("false"),
			TideglassPreviewSurface->GetOwner()->IsHidden() ? TEXT("true") : TEXT("false"),
			TideglassSection ? TideglassSection->ProcVertexBuffer.Num() : 0,
			TideglassSection ? TideglassSection->ProcIndexBuffer.Num() : 0,
			*TideglassMesh->GetComponentLocation().ToString()));
		if (bTideglassWeatherOverride)
		{
			UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, UIslandEnvironmentSubsystem::CollectionPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
			UMaterialParameterCollectionInstance* Instance = Collection ? Island->GetParameterCollectionInstance(Collection) : nullptr;
			if (!Instance ||
				!Instance->GetScalarParameterValue(TEXT("WindSpeed"), TideglassWeather.OriginalWindSpeed) ||
				!Instance->GetScalarParameterValue(TEXT("Storm"), TideglassWeather.OriginalStorm) ||
				!Instance->GetScalarParameterValue(TEXT("RainIntensity"), TideglassWeather.OriginalRainIntensity))
			{
				if (AActor* Owner = TideglassPreviewSurface->GetOwner()) Owner->Destroy();
				TideglassPreviewSurface = nullptr;
				TideglassMesh->SetVisibility(bTideglassWasVisible);
				TideglassMesh->SetHiddenInGame(bTideglassWasHiddenInGame);
				if (OceanMesh) OceanMesh->SetMaterial(0, OriginalOceanMaterial);
				AddError(TEXT("Could not load and snapshot the environment collection's wind/storm/rain values."));
				return false;
			}
			TideglassWeather.Instance = Instance;
			TideglassWeather.bEnabled = true;
			const bool bStormPreview = TideglassWeatherMode == TEXT("Storm");
			TideglassWeather.PreviewWindSpeed = bStormPreview ? 300.f : 0.f;
			TideglassWeather.PreviewStorm = bStormPreview ? 1.f : 0.f;
			TideglassWeather.PreviewRainIntensity = bStormPreview ? 1.f : 0.f;
			AddInfo(FString::Printf(TEXT("Tideglass material-response diagnostic: forcing %s collection values during capture, then restoring their prior values; this is not a gameplay weather simulation."), *TideglassWeatherMode));
		}
	}
	TArray<FLandmarkRockPreviewBackup> LandmarkRockPreviewBackups;
	if (bLandmarkRockPreview)
	{
		UStaticMesh* RockMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
		UMaterialInterface* RockMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/StarterContent/Props/Materials/M_Rock.M_Rock"));
		if (!TestNotNull(TEXT("Starter Content rock mesh for the landmark prototype loaded"), RockMesh) ||
			!TestNotNull(TEXT("Starter Content rock material for the landmark prototype loaded"), RockMaterial)) return false;

		const TSet<FString> LandmarkLabels = {
			TEXT("WindArch_Pillar_A"), TEXT("WindArch_Pillar_B"), TEXT("WindArch_Beam"),
			TEXT("ListeningStone_A"), TEXT("ListeningStone_B"), TEXT("ListeningStone_C")
		};
		TMap<FString, UStaticMeshComponent*> ComponentsByLabel;
		for (TActorIterator<AStaticMeshActor> It(Island); It; ++It)
		{
			const FString Label = It->GetActorLabel();
			if (!LandmarkLabels.Contains(Label)) continue;
			if (UStaticMeshComponent* Component = It->GetStaticMeshComponent()) ComponentsByLabel.Add(Label, Component);
		}
		if (!TestEqual(TEXT("All six blockout landmark components are present for the transient rock prototype"),
			ComponentsByLabel.Num(), LandmarkLabels.Num())) return false;

		for (const FString& Label : LandmarkLabels)
		{
			UStaticMeshComponent* Original = ComponentsByLabel.FindRef(Label);
			if (!Original || !Original->GetStaticMesh())
			{
				AddError(FString::Printf(TEXT("Landmark %s has no original static mesh to preserve."), *Label));
				for (FLandmarkRockPreviewBackup& Backup : LandmarkRockPreviewBackups)
				{
					if (Backup.OriginalComponent.IsValid()) Backup.OriginalComponent->SetVisibility(Backup.bOriginalWasVisible, false);
					if (Backup.PreviewComponent.IsValid()) Backup.PreviewComponent->DestroyComponent();
				}
				return false;
			}

			AActor* Owner = Original->GetOwner();
			UStaticMeshComponent* Preview = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
			if (!Preview)
			{
				AddError(FString::Printf(TEXT("Could not create the transient rock visual for %s."), *Label));
				for (FLandmarkRockPreviewBackup& Backup : LandmarkRockPreviewBackups)
				{
					if (Backup.OriginalComponent.IsValid()) Backup.OriginalComponent->SetVisibility(Backup.bOriginalWasVisible, false);
					if (Backup.PreviewComponent.IsValid()) Backup.PreviewComponent->DestroyComponent();
				}
				return false;
			}
			Preview->SetupAttachment(Original);
			Preview->SetStaticMesh(RockMesh);
			Preview->SetMaterial(0, RockMaterial);
			Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Preview->SetCanEverAffectNavigation(false);
			Preview->SetGenerateOverlapEvents(false);
			Preview->SetWorldTransform(Original->GetComponentTransform());
			const FVector OriginalMeshExtent = Original->GetStaticMesh()->GetBounds().BoxExtent;
			const FVector RockMeshExtent = RockMesh->GetBounds().BoxExtent;
			const FVector BoundsFitScale(
				OriginalMeshExtent.X / FMath::Max(RockMeshExtent.X, 1.f),
				OriginalMeshExtent.Y / FMath::Max(RockMeshExtent.Y, 1.f),
				OriginalMeshExtent.Z / FMath::Max(RockMeshExtent.Z, 1.f));
			Preview->SetRelativeScale3D(BoundsFitScale * 0.9f);
			Preview->RegisterComponent();

			FLandmarkRockPreviewBackup& Backup = LandmarkRockPreviewBackups.AddDefaulted_GetRef();
			Backup.OriginalComponent = Original;
			Backup.OriginalMesh = Original->GetStaticMesh();
			Backup.PreviewComponent = Preview;
			Backup.OriginalCollisionEnabled = Original->GetCollisionEnabled();
			Backup.OriginalTransform = Original->GetComponentTransform();
			Backup.bOriginalWasVisible = Original->IsVisible();
			Original->SetVisibility(false, false);
			TestTrue(FString::Printf(TEXT("%s preserves its original mesh, collision, and transform while the noncolliding rock silhouette is visible"), *Label),
				Original->GetStaticMesh() == Backup.OriginalMesh.Get() &&
				Original->GetCollisionEnabled() == Backup.OriginalCollisionEnabled &&
				Original->GetComponentTransform().Equals(Backup.OriginalTransform) && !Original->IsVisible() && Preview->IsVisible() &&
				Preview->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Preview->CanEverAffectNavigation());
		}
		AddInfo(TEXT("Transient art preview only: six Wind Arch and Listening Stones render components use Starter Content SM_Rock/M_Rock; original blockout meshes remain as collision, and the preview visuals will be removed after capture."));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FIslandViewpointCaptureCommand(Island, MoveTemp(Viewpoints), static_cast<float>(Hour),
		FIntPoint(FMath::Clamp(static_cast<int32>(Width), 64, 3840), FMath::Clamp(static_cast<int32>(Height), 64, 2160)), Directory, this,
		PreviewClock, OriginalStartHour, OriginalDayNumber, MoveTemp(PreviewActors), PreviewWeather, bGroundCoverPreview,
		MoveTemp(LandscapeBackups), MoveTemp(ReusedLandscapeInstances), bCompareLandscapeWetness, bPuddlePreview,
		MoveTemp(LandscapePreviewInstances), MoveTemp(TideglassWeather)));
	if (bLandmarkRockPreview)
		ADD_LATENT_AUTOMATION_COMMAND(FRestoreLandmarkRockPreviewCommand(MoveTemp(LandmarkRockPreviewBackups), this));
	if (OceanMesh) ADD_LATENT_AUTOMATION_COMMAND(FRestoreStaticMeshMaterialCommand(OceanMesh, OriginalOceanMaterial));
	if (TideglassMesh && TideglassPreviewSurface)
		ADD_LATENT_AUTOMATION_COMMAND(FRestoreTideglassSurfacePreviewCommand(TideglassMesh, TideglassPreviewSurface,
			bTideglassWasVisible, bTideglassWasHiddenInGame));
	return true;
}

#endif
