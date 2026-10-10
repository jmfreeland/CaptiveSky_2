#include "Misc/AutomationTest.h"
#include "Agent/AgentBrainComponent.h"
#include "Agent/IslandFirefly.h"
#include "Agent/IslandListeningStonePresentation.h"
#include "Agent/IslandListeningStonesChime.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandListeningStonePresentationTest, "CaptiveSky2.Agent.ListeningStonePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandListeningStonePresentationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A clear rising gust can awaken the Listening Stones"), AListeningStonePresentation::ShouldResonateForWind(70.f, 120.f));
	TestFalse(TEXT("Steady wind does not repeatedly chime"), AListeningStonePresentation::ShouldResonateForWind(120.f, 121.f));
	TestFalse(TEXT("A small gust below the audible wind threshold stays quiet"), AListeningStonePresentation::ShouldResonateForWind(20.f, 80.f));
	TestFalse(TEXT("A falling wind does not trigger a new resonance"), AListeningStonePresentation::ShouldResonateForWind(160.f, 100.f));
	TestFalse(TEXT("Non-finite wind samples cannot trigger ambience"), AListeningStonePresentation::ShouldResonateForWind(std::numeric_limits<float>::quiet_NaN(), 150.f));

	UWorld* Island = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* Candidate = Context.World();
		if (Context.WorldType == EWorldType::Editor && Candidate && Candidate->GetMapName() == TEXT("Island"))
		{
			Island = Candidate;
			break;
		}
	}
	if (Island)
	{
		AActor* Marker = nullptr;
		TArray<AStaticMeshActor*> Proxies;
		TestTrue(TEXT("The saved Island resolves its tagged Listening Stones and three cube proxies"),
			UIslandListeningStonePresentationSubsystem::FindStoneProxies(Island, Marker, Proxies));
		TestEqual(TEXT("The saved landmark has exactly three visual proxies"), Proxies.Num(), 3);
		for (const AStaticMeshActor* Proxy : Proxies)
		{
			if (Proxy)
				AddInfo(FString::Printf(TEXT("Saved Listening Stone %s at %s, extent %s"), *Proxy->GetActorLabel(),
					*Proxy->GetActorLocation().ToCompactString(), *Proxy->GetStaticMeshComponent()->Bounds.BoxExtent.ToCompactString()));
		}
	}
	else AddInfo(TEXT("The editor Island is not loaded; saved-map proxy recognition was not exercised."));

	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Listening Stones fixture world is created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	ATargetPoint* Marker = World->SpawnActor<ATargetPoint>(FVector::ZeroVector, FRotator::ZeroRotator);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Synthetic Listening Stones marker spawns"), Marker) || !TestNotNull(TEXT("Engine cube resolves"), Cube))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Marker->Tags = {TEXT("ListeningStones"), TEXT("IslandLandmark")};

	const FVector Locations[] = { FVector(-250.f, -100.f, 150.f), FVector(-250.f, 200.f, 112.f), FVector(200.f, 100.f, 134.f) };
	const float HalfHeights[] = { 151.f, 112.f, 134.f };
	TArray<AStaticMeshActor*> Proxies;
	TArray<ECollisionEnabled::Type> OriginalCollision;
	TArray<bool> OriginalNavigation;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Locations); ++Index)
	{
		AStaticMeshActor* Proxy = World->SpawnActor<AStaticMeshActor>(Locations[Index], FRotator::ZeroRotator);
		if (!TestNotNull(FString::Printf(TEXT("Stone proxy %d spawns"), Index), Proxy)) continue;
		Proxy->GetStaticMeshComponent()->SetStaticMesh(Cube);
		Proxy->SetActorScale3D(FVector(0.8f, 0.7f, HalfHeights[Index] / 50.f));
		Proxies.Add(Proxy);
		OriginalCollision.Add(Proxy->GetStaticMeshComponent()->GetCollisionEnabled());
		OriginalNavigation.Add(Proxy->GetStaticMeshComponent()->CanEverAffectNavigation());
	}
	if (Proxies.Num() != 3)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	Proxies[1]->SetActorHiddenInGame(true);

	World->BeginPlay();
	Marker->Tags.Remove(TEXT("ListeningStones"));
	Marker->Tags.Remove(TEXT("IslandLandmark"));
	UIslandListeningStonePresentationSubsystem* Subsystem = World->GetSubsystem<UIslandListeningStonePresentationSubsystem>();
	TestNotNull(TEXT("Game world has the transient landmark subsystem"), Subsystem);
	AListeningStonePresentation* Presentation = nullptr;
	for (TActorIterator<AListeningStonePresentation> It(World); It; ++It) { Presentation = *It; break; }
	TestNotNull(TEXT("Game start creates a transient stone presentation"), Presentation);
	if (Presentation)
	{
		TestEqual(TEXT("Each blockout proxy becomes one independent standing-stone mesh"), Presentation->GetStoneCount(), 3);
		UMaterialInterface* CandidateRockSurface = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Materials/M_StandingStoneRockSurface.M_StandingStoneRockSurface"));
		TestNotNull(TEXT("The project rock-surface candidate loads for the transient stones"), CandidateRockSurface);
		bool bCandidateHasTintParameter = false;
		if (CandidateRockSurface)
		{
			TArray<FMaterialParameterInfo> VectorParameters;
			TArray<FGuid> VectorParameterIds;
			CandidateRockSurface->GetAllVectorParameterInfo(VectorParameters, VectorParameterIds);
			for (const FMaterialParameterInfo& Parameter : VectorParameters)
			{
				bCandidateHasTintParameter |= Parameter.Name == TEXT("Color") || Parameter.Name == TEXT("BaseColor");
				AddInfo(FString::Printf(TEXT("Standing-stone vector parameter: %s"), *Parameter.Name.ToString()));
			}
			AddInfo(FString::Printf(TEXT("Standing-stone surface exposes %d vector parameter(s); tint parameter found: %s"),
				VectorParameters.Num(), bCandidateHasTintParameter ? TEXT("yes") : TEXT("no")));
		}
		TestTrue(TEXT("The project rock-surface candidate exposes a supported tint parameter"), bCandidateHasTintParameter);
		const FLinearColor ExpectedStoneColors[] = {
			FLinearColor(0.12f, 0.135f, 0.15f), FLinearColor(0.14f, 0.135f, 0.12f), FLinearColor(0.125f, 0.14f, 0.155f)
		};
		for (int32 Index = 0; Index < Presentation->GetStoneCount(); ++Index)
		{
			UProceduralMeshComponent* Stone = Presentation->Stones[Index];
			TestNotNull(FString::Printf(TEXT("Standing stone %d has a procedural mesh"), Index), Stone);
			if (!Stone) continue;
			TestTrue(FString::Printf(TEXT("Standing stone %d is collisionless"), Index),
				Stone->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestFalse(FString::Printf(TEXT("Standing stone %d does not affect navigation"), Index),
				Stone->CanEverAffectNavigation());
			const FProcMeshSection* Section = Stone->GetProcMeshSection(0);
			TestTrue(FString::Printf(TEXT("Standing stone %d has a closed, detailed mesh"), Index),
				Section && Section->ProcIndexBuffer.Num() >= 200 && Section->ProcVertexBuffer.Num() >= 50);
			const FProcMeshSection* LichenSection = Stone->GetProcMeshSection(1);
			TestTrue(FString::Printf(TEXT("Standing stone %d has six broader collisionless lichen islands"), Index),
				LichenSection && LichenSection->ProcVertexBuffer.Num() == 54 && LichenSection->ProcIndexBuffer.Num() == 144);
			if (Section && Section->ProcVertexBuffer.Num() >= 62)
			{
				bool bTextureSeamIsClosed = true;
				for (int32 Ring = 0; Ring < 6; ++Ring)
				{
					const FProcMeshVertex& First = Section->ProcVertexBuffer[Ring * 10];
					const FProcMeshVertex& Seam = Section->ProcVertexBuffer[Ring * 10 + 9];
					bTextureSeamIsClosed &= First.Position.Equals(Seam.Position, 0.01f) &&
						FMath::IsNearlyEqual(First.UV0.X, 0.f) && FMath::IsNearlyEqual(Seam.UV0.X, 1.f);
				}
				TestTrue(FString::Printf(TEXT("Standing stone %d has a position-matched, non-smearing texture seam"), Index),
					bTextureSeamIsClosed);
			}
			UMaterialInstanceDynamic* StoneSurface = Cast<UMaterialInstanceDynamic>(Stone->GetMaterial(0));
			TestNotNull(FString::Printf(TEXT("Standing stone %d uses its transient stone surface"), Index), StoneSurface);
			if (StoneSurface && CandidateRockSurface)
			{
				TestTrue(FString::Printf(TEXT("Standing stone %d uses the local PBR rock candidate"), Index),
					StoneSurface->Parent == CandidateRockSurface);
			}
			if (StoneSurface && ExpectedStoneColors[Index].R > 0.f)
			{
				TestTrue(FString::Printf(TEXT("Standing stone %d retains its calibrated rock tint"), Index),
					StoneSurface->K2_GetVectorParameterValue(TEXT("Color")).Equals(ExpectedStoneColors[Index], 0.001f));
			}
			UMaterialInstanceDynamic* LichenSurface = Cast<UMaterialInstanceDynamic>(Stone->GetMaterial(1));
			TestNotNull(FString::Printf(TEXT("Standing stone %d uses a transient textured lichen overlay"), Index), LichenSurface);
			if (LichenSurface)
			{
				TestTrue(FString::Printf(TEXT("Standing stone %d keeps the lichen tint subdued and moss-green"), Index),
					LichenSurface->K2_GetVectorParameterValue(TEXT("Color")).Equals(FLinearColor(0.21f, 0.33f, 0.08f), 0.001f));
			}
			if (!Section || !Proxies.IsValidIndex(Index)) continue;

			const FBoxSphereBounds& ProxyBounds = Proxies[Index]->GetStaticMeshComponent()->Bounds;
			const float ProxyBottom = ProxyBounds.Origin.Z - ProxyBounds.BoxExtent.Z;
			const float ExpectedTop = ProxyBottom + 2.f * ProxyBounds.BoxExtent.Z * AListeningStonePresentation::StoneHeightRatio;
			float MinimumZ = TNumericLimits<float>::Max();
			float MaximumZ = -TNumericLimits<float>::Max();
			bool bInsideProxyFootprint = true;
			for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
			{
				const FVector WorldPosition = Stone->GetComponentTransform().TransformPosition(Vertex.Position);
				MinimumZ = FMath::Min(MinimumZ, WorldPosition.Z);
				MaximumZ = FMath::Max(MaximumZ, WorldPosition.Z);
				bInsideProxyFootprint &= FMath::Abs(WorldPosition.X - ProxyBounds.Origin.X) <= ProxyBounds.BoxExtent.X + 1.f &&
					FMath::Abs(WorldPosition.Y - ProxyBounds.Origin.Y) <= ProxyBounds.BoxExtent.Y + 1.f;
			}
			TestTrue(FString::Printf(TEXT("Standing stone %d stays inside the original proxy footprint"), Index), bInsideProxyFootprint);
			TestTrue(FString::Printf(TEXT("Standing stone %d is planted within the shortened proxy silhouette"), Index),
				MinimumZ >= ProxyBottom - 1.f && MaximumZ <= ExpectedTop + 1.f && MaximumZ > MinimumZ);
			if (LichenSection)
			{
				bool bLichenInsideProxy = true;
				for (const FProcMeshVertex& Vertex : LichenSection->ProcVertexBuffer)
				{
					const FVector WorldPosition = Stone->GetComponentTransform().TransformPosition(Vertex.Position);
					bLichenInsideProxy &= FMath::Abs(WorldPosition.X - ProxyBounds.Origin.X) <= ProxyBounds.BoxExtent.X + 2.f &&
						FMath::Abs(WorldPosition.Y - ProxyBounds.Origin.Y) <= ProxyBounds.BoxExtent.Y + 2.f &&
						WorldPosition.Z >= ProxyBottom - 1.f && WorldPosition.Z <= ExpectedTop + 2.f;
				}
				TestTrue(FString::Printf(TEXT("Standing stone %d lichen hugs the visual surface and remains within the proxy bounds"), Index),
					bLichenInsideProxy);
			}
		}
		for (int32 Index = 0; Index < Proxies.Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("Proxy %d collision setting is unchanged"), Index),
				Proxies[Index]->GetStaticMeshComponent()->GetCollisionEnabled(), OriginalCollision[Index]);
			TestEqual(FString::Printf(TEXT("Proxy %d navigation setting is unchanged"), Index),
				Proxies[Index]->GetStaticMeshComponent()->CanEverAffectNavigation(), OriginalNavigation[Index]);
		}
	}
	TestTrue(TEXT("Presentation hides the map visuals only in Game"), Proxies[0]->IsHidden() && Proxies[1]->IsHidden() && Proxies[2]->IsHidden());

	if (Subsystem && Presentation)
	{
		TestEqual(TEXT("Natural wind monitoring stays on with no resonance active"), Presentation->GetResonanceRemaining(), 0.f);
		Presentation->ObserveAmbientWind(70.f);
		TestEqual(TEXT("The first wind sample establishes a quiet baseline"), Presentation->GetResonanceRemaining(), 0.f);
		Presentation->ObserveAmbientWind(120.f);
		TestTrue(TEXT("A rising natural gust starts the transient stone resonance"), Presentation->GetResonanceRemaining() > 2.7f);
		int32 NaturalChimeCount = 0;
		AIslandListeningStonesChime* NaturalChime = nullptr;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
		{
			++NaturalChimeCount;
			NaturalChime = *It;
		}
		TestEqual(TEXT("A natural gust creates one finite chime actor"), NaturalChimeCount, 1);
		AIslandFirefly* NearbyFirefly = World->SpawnActor<AIslandFirefly>(FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator);
		AIslandFirefly* DistantFirefly = World->SpawnActor<AIslandFirefly>(
			FVector(AIslandListeningStonesChime::AudibleRadius + 100.f, 0.f, 0.f), FRotator::ZeroRotator);
		if (NearbyFirefly)
		{
			NearbyFirefly->CheckForNearbyStoneChime();
			TestTrue(TEXT("A nearby firefly notices the natural chime and lifts its glow pulse"), NearbyFirefly->ChimeResponseRemaining > 0.f);
			TestEqual(TEXT("The firefly remembers which transient tone it answered"), NearbyFirefly->RespondedChimes.Num(), 1);
		}
		else AddError(TEXT("A nearby firefly is required to verify natural chime response."));
		if (DistantFirefly)
		{
			DistantFirefly->CheckForNearbyStoneChime();
			TestTrue(TEXT("A firefly beyond the acoustic radius does not respond"), DistantFirefly->ChimeResponseRemaining <= 0.f);
		}
		else AddError(TEXT("A distant firefly is required to verify acoustic range."));

		ATargetPoint* Resident = World->SpawnActor<ATargetPoint>(FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator);
		UAgentBrainComponent* ResidentBrain = Resident ? NewObject<UAgentBrainComponent>(Resident) : nullptr;
		if (Resident && ResidentBrain)
		{
			Resident->AddInstanceComponent(ResidentBrain);
			ResidentBrain->RegisterComponent();
			FAgentConversationContext NoMessageContext;
			const FString NearbySituation = ResidentBrain->BuildSituationSummary(NoMessageContext);
			TestTrue(TEXT("A nearby resident's situation includes the fading Listening Stones tone"),
				NearbySituation.Contains(TEXT("soft, layered tone is fading from the ListeningStones")) && NearbySituation.Contains(TEXT("5 metres away")));
			Resident->SetActorLocation(FVector(1200.f, 0.f, 0.f));
			const FString DistantSituation = ResidentBrain->BuildSituationSummary(NoMessageContext);
			TestFalse(TEXT("A resident outside the audible radius gets no tone in their situation"),
				DistantSituation.Contains(TEXT("soft, layered tone is fading from the ListeningStones")));
		}
		else
		{
			AddError(TEXT("A resident and brain component are required to verify ephemeral sound perception."));
		}
		if (NaturalChime)
		{
			const FString NearbyTone = NaturalChime->DescribeForListener(FVector(500.f, 0.f, 0.f));
			TestTrue(TEXT("A nearby resident can notice the fading local tone"), NearbyTone.Contains(TEXT("ListeningStones")) && NearbyTone.Contains(TEXT("5 metres away")));
			TestTrue(TEXT("A resident beyond the sound radius gets no tone fact"), NaturalChime->DescribeForListener(FVector(1200.f, 0.f, 0.f)).IsEmpty());
			TestTrue(TEXT("The resident context spans one full maximum decision interval"),
				FMath::IsNearlyEqual(NaturalChime->GetLifeSpan(), AIslandListeningStonesChime::ResidentContextLifetimeSeconds));
		}
		Presentation->ObserveAmbientWind(40.f);
		Presentation->ObserveAmbientWind(120.f);
		int32 ChimeCountDuringCooldown = 0;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) ++ChimeCountDuringCooldown;
		TestEqual(TEXT("A second gust cannot retrigger during the resonance and cooldown"), ChimeCountDuringCooldown, 1);
		if (NaturalChime)
		{
			NaturalChime->Tick(2.9f);
			TestFalse(TEXT("A faded chime no longer produces audio or ticks"), NaturalChime->IsAudibleAt(FVector(500.f, 0.f, 0.f)) || NaturalChime->IsActorTickEnabled());
			TestTrue(TEXT("The faded tone remains available for the next scheduled resident thought"),
				NaturalChime->DescribeForListener(FVector(500.f, 0.f, 0.f)).Contains(TEXT("within the last few minutes")));
			NaturalChime->ContextExpiresAt = World->GetTimeSeconds() - 0.1;
			TestTrue(TEXT("The transient resident context expires without a persistent record"),
				NaturalChime->DescribeForListener(FVector(500.f, 0.f, 0.f)).IsEmpty());
		}
		Presentation->Tick(2.9f);
		TestEqual(TEXT("Natural resonance lights expire without persistent state"), Presentation->GetResonanceRemaining(), 0.f);
		Subsystem->NotifyChime(180.f);
		TestTrue(TEXT("A chime starts a finite stone resonance"), Presentation->IsActorTickEnabled() && Presentation->GetResonanceRemaining() > 2.7f);
		TestTrue(TEXT("Resonance lights are briefly visible"), Presentation->ResonanceLights[0]->IsVisible() && Presentation->ResonanceLights[1]->IsVisible() && Presentation->ResonanceLights[2]->IsVisible());
		Presentation->Tick(2.9f);
		TestTrue(TEXT("The lightweight wind watch remains active after the bounded resonance"), Presentation->IsActorTickEnabled());
		TestEqual(TEXT("The bounded resonance has fully elapsed"), Presentation->GetResonanceRemaining(), 0.f);
		TestFalse(TEXT("The resonance lights switch off after the chime"), Presentation->ResonanceLights[0]->IsVisible() || Presentation->ResonanceLights[1]->IsVisible() || Presentation->ResonanceLights[2]->IsVisible());
		Subsystem->RestorePresentation();
	}
	TestFalse(TEXT("An initially visible map proxy restores visible"), Proxies[0]->IsHidden());
	TestTrue(TEXT("An initially hidden map proxy stays hidden"), Proxies[1]->IsHidden());
	TestFalse(TEXT("The third proxy restores visible"), Proxies[2]->IsHidden());
	bool bPresentationRemains = false;
	for (TActorIterator<AListeningStonePresentation> It(World); It; ++It) bPresentationRemains = true;
	TestFalse(TEXT("Transient presentation actor is destroyed on restore"), bPresentationRemains);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
