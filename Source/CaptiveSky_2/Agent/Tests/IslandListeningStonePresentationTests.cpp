#include "Misc/AutomationTest.h"
#include "Agent/AgentBrainComponent.h"
#include "Agent/IslandFirefly.h"
#include "Agent/IslandListeningStonePresentation.h"
#include "Agent/IslandListeningStonesChime.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
	UStaticMesh* Rock = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/StarterContent/Props/SM_Rock.SM_Rock"));
	if (!TestNotNull(TEXT("Synthetic Listening Stones marker spawns"), Marker) || !TestNotNull(TEXT("Engine cube resolves"), Cube) ||
		!TestNotNull(TEXT("Starter Content rock resolves"), Rock))
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
		TestEqual(TEXT("Three monolith forms match the three blockout proxies"), Presentation->GetStoneCount(), 3);
		TestTrue(TEXT("Replacement render component is collisionless"), Presentation->Stones->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Replacement forms do not affect navigation"), Presentation->Stones->CanEverAffectNavigation());
		TestTrue(TEXT("Existing Starter Content rock is used"), Presentation->Stones->GetStaticMesh() == Rock);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FTransform Instance;
			TestTrue(FString::Printf(TEXT("Stone form %d has a fitted transform"), Index), Presentation->Stones->GetInstanceTransform(Index, Instance, true));
			TestTrue(FString::Printf(TEXT("Stone form %d stays centered on its proxy bounds"), Index),
				Instance.GetLocation().Equals(Proxies[Index]->GetStaticMeshComponent()->Bounds.Origin, 1.f));
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
		}
		Presentation->ObserveAmbientWind(40.f);
		Presentation->ObserveAmbientWind(120.f);
		int32 ChimeCountDuringCooldown = 0;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) ++ChimeCountDuringCooldown;
		TestEqual(TEXT("A second gust cannot retrigger during the resonance and cooldown"), ChimeCountDuringCooldown, 1);
		if (NaturalChime)
		{
			NaturalChime->Tick(2.9f);
			TestTrue(TEXT("Residents stop receiving the sound fact after the tone fades"), NaturalChime->DescribeForListener(FVector(500.f, 0.f, 0.f)).IsEmpty());
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
