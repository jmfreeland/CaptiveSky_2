#include "Misc/AutomationTest.h"
#include "Agent/AgentDataPaths.h"
#include "Agent/AgentMemoryComponent.h"
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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandListeningStoneResidentMemoryTest, "CaptiveSky2.Agent.ListeningStoneResidentMemory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandListeningStoneResidentMemoryTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A controlled visitor can pause beside the Stones without movement"),
		AListeningStonePresentation::IsEligibleVisitor(true, false, FVector(300.f, 0.f, 0.f), FVector::ZeroVector));
	TestTrue(TEXT("A conscious resident approaching the Stones slowly may be answered"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector(-100.f, 0.f, 0.f)));
	TestTrue(TEXT("An autonomous resident lingering close to the Stones may become a quiet visit"),
		AListeningStonePresentation::IsEligibleQuietResident(false, true, FVector(120.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("A player pause is not mislabeled as resident etiquette"),
		AListeningStonePresentation::IsEligibleQuietResident(true, false, FVector(120.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("An ambient wildlife pawn without resident memory cannot trigger a quiet-visit response"),
		AListeningStonePresentation::IsEligibleQuietResident(false, false, FVector(120.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("A moving resident is not mistaken for someone listening quietly"),
		AListeningStonePresentation::IsEligibleQuietResident(false, true, FVector(120.f, 0.f, 0.f), FVector(21.f, 0.f, 0.f)));
	TestFalse(TEXT("A falling resident is not mistaken for someone listening quietly"),
		AListeningStonePresentation::IsEligibleQuietResident(false, true, FVector(120.f, 0.f, 0.f), FVector(0.f, 0.f, -21.f)));
	TestFalse(TEXT("A nearby resident must be within the close landmark ring"),
		AListeningStonePresentation::IsEligibleQuietResident(false, true, FVector(191.f, 0.f, 0.f), FVector::ZeroVector));
	TestTrue(TEXT("A quiet visit takes long enough to distinguish lingering from passing"),
		AListeningStonePresentation::QuietVisitDurationSeconds >= 6.f);
	TestFalse(TEXT("An ordinary wild pawn cannot trigger the resident response"),
		AListeningStonePresentation::IsEligibleVisitor(false, false, FVector(300.f, 0.f, 0.f), FVector(-100.f, 0.f, 0.f)));
	TestFalse(TEXT("A resident walking away does not trigger the approach response"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("A resident crossing the Stones does not trigger an approach response"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector(0.f, 100.f, 0.f)));
	TestTrue(TEXT("Resident approach has a quiet, grounded memory description"),
		AIslandListeningStonesChime::ResidentObservationText(EIslandChimeCause::ResidentApproach).Contains(TEXT("quiet approach")));
	TestTrue(TEXT("A quiet visit is described as something directly heard, not a world change"),
		AIslandListeningStonesChime::ResidentObservationText(EIslandChimeCause::ResidentListening).Contains(TEXT("quiet pause")));
	TestTrue(TEXT("An audible chime becomes a private observation for a nearby resident"),
		AIslandListeningStonesChime::ShouldRecordResidentObservation(false, true, true, false));
	TestFalse(TEXT("A player does not receive a private resident memory from the same tone"),
		AIslandListeningStonesChime::ShouldRecordResidentObservation(true, true, true, false));
	TestFalse(TEXT("A resident without an agent memory store is not written to"),
		AIslandListeningStonesChime::ShouldRecordResidentObservation(false, false, true, false));
	TestFalse(TEXT("Residents outside the chime's acoustic radius do not remember it"),
		AIslandListeningStonesChime::ShouldRecordResidentObservation(false, true, false, false));
	TestFalse(TEXT("A similar chime within twelve hours is not added again"),
		AIslandListeningStonesChime::ShouldRecordResidentObservation(false, true, true, true));
	TestTrue(TEXT("Gust memories retain the directly witnessed cause"),
		AIslandListeningStonesChime::ResidentObservationText(EIslandChimeCause::NaturalGust).Contains(TEXT("rising gust")));
	TestTrue(TEXT("Visitor memories retain the directly witnessed cause"),
		AIslandListeningStonesChime::ResidentObservationText(EIslandChimeCause::VisitorApproach).Contains(TEXT("quiet approach")));

	const FString TestAgentId = TEXT("Automation_StoneWitness_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TestDirectory = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / TestAgentId);
	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Resident-memory fixture world is created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACharacter* NearbyResident = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
	ACharacter* DistantResident = World->SpawnActor<ACharacter>(FVector(AIslandListeningStonesChime::AudibleRadius + 100.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Nearby autonomous resident fixture spawned"), NearbyResident) ||
		!TestNotNull(TEXT("Distant autonomous resident fixture spawned"), DistantResident))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	UAgentMemoryComponent* Memory = NewObject<UAgentMemoryComponent>(NearbyResident);
	NearbyResident->AddInstanceComponent(Memory);
	Memory->AgentId = TestAgentId;
	Memory->RegisterComponent();
	UAgentMemoryComponent* DistantMemory = NewObject<UAgentMemoryComponent>(DistantResident);
	DistantResident->AddInstanceComponent(DistantMemory);
	DistantMemory->AgentId = TestAgentId + TEXT("_Distant");
	DistantMemory->RegisterComponent();
	AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A test chime can be spawned"), Chime))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
		return false;
	}
	Chime->BeginChime(100.f, EIslandChimeCause::NaturalGust);
	TestEqual(TEXT("A nearby autonomous resident keeps one directly heard chime as private memory"), Memory->GetMemoryCount(), 1);
	TestEqual(TEXT("A resident outside the acoustic radius keeps no chime memory"), DistantMemory->GetMemoryCount(), 0);
	AIslandListeningStonesChime* RepeatedChime = World->SpawnActor<AIslandListeningStonesChime>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (RepeatedChime) RepeatedChime->BeginChime(100.f, EIslandChimeCause::NaturalGust);
	TestEqual(TEXT("A similar tone inside twelve hours does not create repetitive memory"), Memory->GetMemoryCount(), 1);
	AListeningStonePresentation* Presentation = World->SpawnActor<AListeningStonePresentation>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A transient Listening Stones presentation can be staged"), Presentation))
	{
		NearbyResident->SetActorLocation(FVector(300.f, 0.f, 0.f));
		NearbyResident->GetCharacterMovement()->Velocity = FVector(-100.f, 0.f, 0.f);
		Presentation->CheckForVisitorPresence(0.25f);
		AIslandListeningStonesChime* ResidentChime = nullptr;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
			if (It->Cause == EIslandChimeCause::ResidentApproach) { ResidentChime = *It; break; }
		TestNotNull(TEXT("A conscious resident's quiet, directed approach creates a brief response"), ResidentChime);
		TestTrue(TEXT("The response identifies a resident's approach to nearby listeners"), ResidentChime &&
			ResidentChime->DescribeForListener(FVector(300.f, 0.f, 0.f)).Contains(TEXT("resident's quiet approach")));
		TestEqual(TEXT("The approaching resident remembers the distinct response once"), Memory->GetMemoryCount(), 2);
		Presentation->CheckForVisitorPresence(0.25f);
		TestEqual(TEXT("A latched approach does not chime or record again"), Memory->GetMemoryCount(), 2);
	}
	const FString QuietAgentId = TestAgentId + TEXT("_Quiet");
	const FString QuietDirectory = CaptiveSkyDataPaths::ResolveProjectDataPath(TEXT("Agents") / QuietAgentId);
	IFileManager::Get().DeleteDirectory(*QuietDirectory, false, true);
	ACharacter* QuietResident = World->SpawnActor<ACharacter>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A second autonomous resident can quietly visit the Stones"), QuietResident))
	{
		UAgentMemoryComponent* QuietMemory = NewObject<UAgentMemoryComponent>(QuietResident);
		QuietResident->AddInstanceComponent(QuietMemory);
		QuietMemory->AgentId = QuietAgentId;
		QuietMemory->RegisterComponent();
		AListeningStonePresentation* QuietPresentation = World->SpawnActor<AListeningStonePresentation>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (TestNotNull(TEXT("A fresh transient presentation can observe a quiet visit"), QuietPresentation))
		{
			for (int32 Check = 0; Check < 32; ++Check) QuietPresentation->CheckForVisitorPresence(0.25f);
			AIslandListeningStonesChime* ListeningChime = nullptr;
			for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
				if (It->Cause == EIslandChimeCause::ResidentListening) { ListeningChime = *It; break; }
			TestNotNull(TEXT("Eight seconds of stillness creates one brief resident-listening response"), ListeningChime);
			TestEqual(TEXT("The quiet resident remembers the locally heard response once"), QuietMemory->GetMemoryCount(), 1);
			TestTrue(TEXT("Nearby context describes the directly witnessed quiet visit"), ListeningChime &&
				ListeningChime->DescribeForListener(FVector(100.f, 0.f, 0.f)).Contains(TEXT("quiet pause beside the stones")));
			for (int32 Check = 0; Check < 24; ++Check) QuietPresentation->CheckForVisitorPresence(0.25f);
			int32 ListeningChimeCount = 0;
			for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
				if (It->Cause == EIslandChimeCause::ResidentListening) ++ListeningChimeCount;
			TestEqual(TEXT("A resident who continues sitting nearby does not repeatedly chime"), ListeningChimeCount, 1);
			TestEqual(TEXT("Remaining nearby does not create repetitive memory"), QuietMemory->GetMemoryCount(), 1);
			QuietResident->SetActorLocation(FVector(600.f, 0.f, 0.f));
			QuietPresentation->CheckForVisitorPresence(0.25f);
			TestFalse(TEXT("Leaving beyond the wide ring rearms the next quiet visit"), QuietPresentation->bQuietVisitHandled);
			TestFalse(TEXT("The rearmed visit no longer holds its prior resident latch"), QuietPresentation->QuietResidentLatch.IsValid());
			QuietResident->SetActorLocation(FVector(100.f, 0.f, 0.f));
			QuietPresentation->CheckForVisitorPresence(0.25f);
			TestTrue(TEXT("Returning starts a new quiet-visit dwell"), QuietPresentation->QuietResidentLatch.Get() == QuietResident);
		}
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	IFileManager::Get().DeleteDirectory(*QuietDirectory, false, true);
	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandListeningStonePresentationTest, "CaptiveSky2.Agent.ListeningStonePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandListeningStonePresentationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A clear rising gust can awaken the Listening Stones"), AListeningStonePresentation::ShouldResonateForWind(70.f, 120.f));
	TestFalse(TEXT("Steady wind does not repeatedly chime"), AListeningStonePresentation::ShouldResonateForWind(120.f, 121.f));
	TestFalse(TEXT("A small gust below the audible wind threshold stays quiet"), AListeningStonePresentation::ShouldResonateForWind(20.f, 80.f));
	TestFalse(TEXT("A falling wind does not trigger a new resonance"), AListeningStonePresentation::ShouldResonateForWind(160.f, 100.f));
	TestFalse(TEXT("Non-finite wind samples cannot trigger ambience"), AListeningStonePresentation::ShouldResonateForWind(std::numeric_limits<float>::quiet_NaN(), 150.f));
	TestTrue(TEXT("A slow, controlled visitor inside the approach envelope is eligible"),
		AListeningStonePresentation::IsEligibleVisitor(true, false, FVector(300.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("Ambient wildlife cannot qualify as a visitor"),
		AListeningStonePresentation::IsEligibleVisitor(false, false, FVector(300.f, 0.f, 0.f), FVector(-100.f, 0.f, 0.f)));
	TestTrue(TEXT("A conscious resident approaching the Stones slowly may be answered"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector(-100.f, 0.f, 0.f)));
	TestFalse(TEXT("A resident walking away does not trigger an approach response"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("A resident merely crossing past the Stones does not trigger an approach response"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector(0.f, 100.f, 0.f)));
	TestFalse(TEXT("A resident without a directed approach stays unremarked while resting"),
		AListeningStonePresentation::IsEligibleVisitor(false, true, FVector(300.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("Visitors outside the notice radius do not qualify"),
		AListeningStonePresentation::IsEligibleVisitor(true, false, FVector(326.f, 0.f, 0.f), FVector::ZeroVector));
	TestFalse(TEXT("Visitors at a different height do not qualify"),
		AListeningStonePresentation::IsEligibleVisitor(true, false, FVector(100.f, 0.f, 241.f), FVector::ZeroVector));
	TestFalse(TEXT("Fast passers-by do not trigger a quiet approach response"),
		AListeningStonePresentation::IsEligibleVisitor(true, false, FVector(100.f, 0.f, 0.f), FVector(261.f, 0.f, 0.f)));
	TestTrue(TEXT("Resident-approach chimes remain distinct in memory and listener context"),
		AIslandListeningStonesChime::ResidentObservationText(EIslandChimeCause::ResidentApproach).Contains(TEXT("quiet approach")));
	TestFalse(TEXT("A repeated approach, active resonance, or recent tone is cooldown-gated"),
		AListeningStonePresentation::CanBeginVisitorResonance(false, false, 44.f));
	TestFalse(TEXT("An already latched visitor cannot immediately retrigger"),
		AListeningStonePresentation::CanBeginVisitorResonance(true, false, 100.f));
	TestFalse(TEXT("An active resonance cannot be stacked"),
		AListeningStonePresentation::CanBeginVisitorResonance(false, true, 100.f));
	TestTrue(TEXT("A fresh eligible visitor may trigger after cooldown"),
		AListeningStonePresentation::CanBeginVisitorResonance(false, false, 45.f));

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
		// This synthetic Game world has no ULocalPlayer; possession-based integration is reserved
		// for PIE. Keep this test deterministic by exercising the same pure eligibility/cooldown
		// gates above, then verify the visitor response effect directly.
		Presentation->BeginVisitorResonance(false);
		TestTrue(TEXT("The visitor response starts a finite stone resonance"), Presentation->GetResonanceRemaining() > 2.7f);
		int32 VisitorChimeCount = 0;
		AIslandListeningStonesChime* VisitorChime = nullptr;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
		{
			++VisitorChimeCount;
			VisitorChime = *It;
		}
		TestEqual(TEXT("A visitor response creates one transient chime"), VisitorChimeCount, 1);
		TestTrue(TEXT("The transient tone identifies a quiet visitor approach"), VisitorChime &&
			VisitorChime->DescribeForListener(FVector::ZeroVector).Contains(TEXT("quiet visitor approached")));

		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It) It->Destroy();
		Presentation->LastAmbientChimeAt = -1000.0;
		Presentation->VisitorPresenceLatch.Reset();
		Presentation->bIsResonating = false;
		Presentation->ResonanceElapsed = 0.f;

		TestEqual(TEXT("Natural wind monitoring stays on with no resonance active"), Presentation->GetResonanceRemaining(), 0.f);
		Presentation->ObserveAmbientWind(70.f, FVector(70.f, 0.f, 0.f));
		TestEqual(TEXT("The first wind sample establishes a quiet baseline"), Presentation->GetResonanceRemaining(), 0.f);
		const FVector GustDirection(0.f, 120.f, 0.f);
		Presentation->ObserveAmbientWind(120.f, GustDirection);
		TestTrue(TEXT("A rising natural gust starts the transient stone resonance"), Presentation->GetResonanceRemaining() > 2.7f);
		int32 NaturalChimeCount = 0;
		AIslandListeningStonesChime* NaturalChime = nullptr;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
		{
			++NaturalChimeCount;
			NaturalChime = *It;
		}
		TestEqual(TEXT("A natural gust creates one finite chime actor"), NaturalChimeCount, 1);
		TestTrue(TEXT("Nearby residents can identify the natural gust that stirred the stones"), NaturalChime &&
			NaturalChime->DescribeForListener(FVector(500.f, 0.f, 0.f)).Contains(TEXT("natural gust stirred")));
		TestTrue(TEXT("A natural gust's direction shapes its chime pitch"), NaturalChime &&
			FMath::IsNearlyEqual(NaturalChime->AppliedPitchRatio,
				AIslandListeningStonesChime::CalculateWindPitchRatio(GustDirection.Size2D()) *
				AIslandListeningStonesChime::CalculateWindDirectionPitchRatio(GustDirection, Presentation->GetActorForwardVector())));
		TestFalse(TEXT("A visitor cannot stack a response during natural resonance"),
			AListeningStonePresentation::CanBeginVisitorResonance(false, true, 100.f));
		AIslandFirefly* NearbyFirefly = World->SpawnActor<AIslandFirefly>(FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator);
		AIslandFirefly* DistantFirefly = World->SpawnActor<AIslandFirefly>(
			FVector(AIslandListeningStonesChime::AudibleRadius + 100.f, 0.f, 0.f), FRotator::ZeroRotator);
		if (NearbyFirefly && NaturalChime)
		{
			NearbyFirefly->CheckForNearbyStoneChime();
			TestTrue(TEXT("A nearby firefly notices the natural chime and begins a bounded response"),
				NearbyFirefly->ChimeResponseRemaining > 0.f);
			TestEqual(TEXT("The firefly remembers which transient tone it answered"), NearbyFirefly->RespondedChimes.Num(), 1);
			const FVector TowardStones = (NaturalChime->GetActorLocation() - NearbyFirefly->GetActorLocation()).GetSafeNormal2D();
			const FVector ChimeArcStart = AIslandFirefly::EvaluateChimeResponseDisplacement(TowardStones, 0.f);
			const FVector ChimeArcMidpoint = AIslandFirefly::EvaluateChimeResponseDisplacement(
				TowardStones, AIslandFirefly::ChimeResponseDurationSeconds * 0.5f);
			const FVector ChimeArcEnd = AIslandFirefly::EvaluateChimeResponseDisplacement(
				TowardStones, AIslandFirefly::ChimeResponseDurationSeconds);
			TestTrue(TEXT("The firefly makes a 55 cm mid-flight drift toward the stones"),
				FMath::IsNearlyEqual(ChimeArcMidpoint.Size(), AIslandFirefly::ChimeApproachDistanceCm, 0.01f) &&
				FVector::DotProduct(ChimeArcMidpoint, TowardStones) > 0.f && FMath::IsNearlyZero(ChimeArcMidpoint.Z));
		TestTrue(TEXT("The curiosity arc eases in and returns to the ordinary flight path"),
			ChimeArcStart.IsNearlyZero() && ChimeArcEnd.IsNearlyZero());
		NearbyFirefly->Phase = 0.f;
		NearbyFirefly->PulseRate = 1.f;
		NearbyFirefly->UpdateGlow(0.0, 0.f);
		const float ChimeGlowIntensity = NearbyFirefly->Glow->Intensity;
		NearbyFirefly->ChimeResponseRemaining = 0.f;
		NearbyFirefly->UpdateGlow(0.0, 0.f);
		TestTrue(TEXT("A chime raises the firefly's dim-phase glow by a restrained 25 percent"),
			FMath::IsNearlyEqual(ChimeGlowIntensity, NearbyFirefly->Glow->Intensity * 1.25f, 0.01f));
		}
		else AddError(TEXT("A nearby firefly and the natural chime are required to verify the bounded response."));
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
			TestTrue(TEXT("The resident's situation includes why the natural tone sounded"),
				NearbySituation.Contains(TEXT("natural gust stirred the stones")));
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
		AIslandListeningStonesChime* InspectionChime = World->SpawnActor<AIslandListeningStonesChime>(
			FVector::ZeroVector, FRotator::ZeroRotator);
		if (TestNotNull(TEXT("An inspection resonance actor can be staged"), InspectionChime))
		{
			InspectionChime->BeginChime(0.f, EIslandChimeCause::Inspection);
			TestTrue(TEXT("Resident-facing context identifies a tone caused by close inspection"),
				InspectionChime->DescribeForListener(FVector(100.f, 0.f, 0.f)).Contains(TEXT("close inspection")));
		}
		Presentation->ObserveAmbientWind(40.f);
		Presentation->ObserveAmbientWind(120.f);
		int32 NaturalChimeCountDuringCooldown = 0;
		for (TActorIterator<AIslandListeningStonesChime> It(World); It; ++It)
			if (It->Cause == EIslandChimeCause::NaturalGust) ++NaturalChimeCountDuringCooldown;
		TestEqual(TEXT("A second gust cannot retrigger during the resonance and cooldown"), NaturalChimeCountDuringCooldown, 1);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandFireflyChimeCuriosityTest, "CaptiveSky2.Agent.FireflyChimeCuriosity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandFireflyChimeCuriosityTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Firefly curiosity fixture world created"), World) ||
		!TestNotNull(TEXT("Engine is available for firefly curiosity fixture"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandListeningStonesChime* Chime = World->SpawnActor<AIslandListeningStonesChime>(FVector(0.f, 0.f, 80.f), FRotator::ZeroRotator, Spawn);
	AIslandFirefly* Firefly = World->SpawnActor<AIslandFirefly>(FVector(500.f, 0.f, 80.f), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Listening Stones chime spawned"), Chime) ||
		!TestNotNull(TEXT("Nearby firefly spawned"), Firefly))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	World->BeginPlay();
	Chime->BeginChime(0.f);
	TestTrue(TEXT("The nearby firefly answers an audible transient chime"), Firefly->RespondToSoftChime(Chime));
	TestFalse(TEXT("One transient chime cannot trigger repeated curiosity"), Firefly->RespondToSoftChime(Chime));
	TestTrue(TEXT("The response records the direction from the firefly toward the stones"),
		FVector::DotProduct(Firefly->ChimeResponseDirection,
			(Chime->GetActorLocation() - Firefly->GetActorLocation()).GetSafeNormal2D()) > 0.99f);

	Firefly->Phase = 0.f;
	Firefly->PulseRate = 1.f;
	Firefly->Tick(0.f);
	const FVector OrdinaryFlightPosition = Firefly->GetActorLocation();
	Firefly->UpdateGlow(0.0, 0.f);
	const float ChimeGlowIntensity = Firefly->Glow->Intensity;
	Firefly->ChimeResponseRemaining = 0.f;
	Firefly->UpdateGlow(0.0, 0.f);
	const float OrdinaryGlowIntensity = Firefly->Glow->Intensity;
	TestTrue(TEXT("A chime raises the dim-phase glow by a restrained 25 percent"),
		FMath::IsNearlyEqual(ChimeGlowIntensity, OrdinaryGlowIntensity * 1.25f, 0.01f));
	Firefly->ChimeResponseRemaining = Firefly->ChimeResponseDurationSeconds;
	Firefly->ChimeResponseElapsed = 0.f;
	Firefly->Tick(0.45f);
	const FVector FirstCuriousPosition = Firefly->GetActorLocation();
	Firefly->Tick(0.45f);
	const FVector ApproachedPosition = Firefly->GetActorLocation();
	const FVector TowardStones = Firefly->ChimeResponseDirection;
	TestTrue(TEXT("The firefly's flight arc visibly advances toward the chime"),
		FVector::DotProduct(ApproachedPosition - FirstCuriousPosition, TowardStones) > 0.f);
	Firefly->Tick(0.9f);
	TestTrue(TEXT("The firefly returns to its ordinary flight path when curiosity fades"),
		Firefly->GetActorLocation().Equals(OrdinaryFlightPosition, 0.1f));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
