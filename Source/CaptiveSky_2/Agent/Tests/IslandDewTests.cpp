#include "Misc/AutomationTest.h"
#include "IslandDew.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandDewTest, "CaptiveSky2.Agent.IslandDew",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandDewTest::RunTest(const FString& Parameters)
{
	FIslandDewInputs Dawn;
	Dawn.SunHeight = 0.2f;
	Dawn.Hour = 7.f;
	Dawn.Wetness = 0.6f;
	Dawn.Rain = 0.f;
	Dawn.CloudCover = 0.2f;
	Dawn.Storm = 0.f;
	TestTrue(TEXT("Clear low morning sun over damp grass glitters"), ComputeDewStrength(Dawn) > 0.7f);

	FIslandDewInputs BreezyDawn = Dawn;
	BreezyDawn.WindSpeed = 160.f;
	TestTrue(TEXT("A breeze softens the dew without removing it"),
		ComputeDewStrength(BreezyDawn) > 0.f && ComputeDewStrength(BreezyDawn) < ComputeDewStrength(Dawn));
	FIslandDewInputs WindyDawn = Dawn;
	WindyDawn.WindSpeed = 320.f;
	TestEqual(TEXT("Wind at the established mist-clearance speed suppresses dew"), ComputeDewStrength(WindyDawn), 0.f);

	FIslandDewInputs DryDawn = Dawn;
	DryDawn.Wetness = 0.f;
	TestTrue(TEXT("Dry ground still carries some dew, less than wet"), ComputeDewStrength(DryDawn) > 0.f && ComputeDewStrength(DryDawn) < ComputeDewStrength(Dawn));

	FIslandDewInputs Evening = Dawn;
	Evening.Hour = 17.f;
	TestEqual(TEXT("No dew in the evening"), ComputeDewStrength(Evening), 0.f);

	FIslandDewInputs Noon = Dawn;
	Noon.SunHeight = 1.f;
	Noon.Hour = 11.5f;
	TestEqual(TEXT("The high sun has burnt it off"), ComputeDewStrength(Noon), 0.f);

	FIslandDewInputs Night = Dawn;
	Night.SunHeight = -0.3f;
	Night.Hour = 3.f;
	TestEqual(TEXT("Nothing to catch the light at night"), ComputeDewStrength(Night), 0.f);

	FIslandDewInputs Raining = Dawn;
	Raining.Rain = 0.8f;
	TestEqual(TEXT("No glitter in falling rain"), ComputeDewStrength(Raining), 0.f);

	FIslandDewInputs Grey = Dawn;
	Grey.CloudCover = 1.f;
	TestEqual(TEXT("No glitter under solid cloud"), ComputeDewStrength(Grey), 0.f);

	FIslandDewInputs Stormy = Dawn;
	Stormy.Storm = 1.f;
	TestEqual(TEXT("No glitter in a storm"), ComputeDewStrength(Stormy), 0.f);

	FIslandDewInputs Nonsense;
	Nonsense.SunHeight = 0.2f;
	Nonsense.Hour = 7.f;
	Nonsense.Wetness = 9.f;
	Nonsense.Rain = -3.f;
	Nonsense.CloudCover = -2.f;
	Nonsense.Storm = -1.f;
	const float Clamped = ComputeDewStrength(Nonsense);
	TestTrue(TEXT("Out-of-range inputs stay within 0..1"), Clamped >= 0.f && Clamped <= 1.f);

	FIslandDewInputs InvalidWeather = Dawn;
	InvalidWeather.CloudCover = std::numeric_limits<float>::quiet_NaN();
	TestEqual(TEXT("A non-finite weather sample suppresses dew instead of propagating NaN"),
		ComputeDewStrength(InvalidWeather), 0.f);
	InvalidWeather = Dawn;
	InvalidWeather.WindSpeed = std::numeric_limits<float>::quiet_NaN();
	TestEqual(TEXT("A non-finite wind sample suppresses dew instead of propagating NaN"), ComputeDewStrength(InvalidWeather), 0.f);
	InvalidWeather = Dawn;
	InvalidWeather.Hour = std::numeric_limits<float>::infinity();
	TestEqual(TEXT("A non-finite clock sample suppresses dew"), ComputeDewStrength(InvalidWeather), 0.f);

	TestEqual(TEXT("No strength, no glints"), AIslandDewActor::DesiredCount(0.f), 0);
	TestEqual(TEXT("Full strength fills every slot"), AIslandDewActor::DesiredCount(1.f), AIslandDewActor::MaxGlints);
	TestEqual(TEXT("Strength beyond range is clamped"), AIslandDewActor::DesiredCount(7.f), AIslandDewActor::MaxGlints);
	TestTrue(TEXT("Half strength is about half the glints"), FMath::Abs(AIslandDewActor::DesiredCount(0.5f) - AIslandDewActor::MaxGlints / 2) <= 1);
	TestEqual(TEXT("A non-finite strength cannot request glint instances"),
		AIslandDewActor::DesiredCount(std::numeric_limits<float>::quiet_NaN()), 0);
	TestTrue(TEXT("Far glints are larger than near ones"), AIslandDewActor::GlintSize(1200.f) > AIslandDewActor::GlintSize(100.f));
	TestTrue(TEXT("Glint size stays bounded"), AIslandDewActor::GlintSize(1e6f) <= AIslandDewActor::GlintSize(AIslandDewActor::Radius) + 0.001f);
	TestTrue(TEXT("A non-finite distance resolves to a small finite glint"),
		FMath::IsFinite(AIslandDewActor::GlintSize(std::numeric_limits<float>::quiet_NaN())));

	TestTrue(TEXT("Nothing is said without dew"), UIslandDewSubsystem::DescribeDew(0.1f).IsEmpty());
	TestTrue(TEXT("A little dew is mentioned"), UIslandDewSubsystem::DescribeDew(0.5f).Contains(TEXT("little dew")));
	TestTrue(TEXT("Heavy dew is mentioned"), UIslandDewSubsystem::DescribeDew(0.9f).Contains(TEXT("glitters")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandDewRuntimeTest, "CaptiveSky2.Agent.IslandDewRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandDewRuntimeTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Dew fixture world created"), World) || !TestNotNull(TEXT("Engine is available"), GEngine))
	{
		if (World) World->DestroyWorld(false);
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandDewActor* Dew = World->SpawnActor<AIslandDewActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Transient dew actor can spawn without external services"), Dew))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	UInstancedStaticMeshComponent* Glints = Dew->FindComponentByClass<UInstancedStaticMeshComponent>();
	TestNotNull(TEXT("The transient actor owns one glint instance component"), Glints);
	if (Glints)
	{
		TestTrue(TEXT("Glints do not block the player"), Glints->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Glints do not affect navigation"), Glints->CanEverAffectNavigation());
		const FVector FirstView(1200.f, -500.f, 1400.f);
		Dew->Advance(FirstView, 0.5f);
		if (!Dew->HasMaterial())
		{
			TestFalse(TEXT("Missing optional material leaves glints hidden"), Glints->IsVisible());
			TestEqual(TEXT("Missing optional material creates no instances"), Glints->GetInstanceCount(), 0);
		}
		else
		{
			TestTrue(TEXT("A compiled optional material reveals the glint pool"), Glints->IsVisible());
			TestEqual(TEXT("The instance pool is allocated once and bounded"), Glints->GetInstanceCount(), AIslandDewActor::MaxGlints);
			TestTrue(TEXT("Initial seating uploads only a bounded batch"), Dew->GetLastTransformUploadCount() <= 48);

			// Seating is rate-limited per tick, so let the pool settle (bounded) before checking it stays quiet.
			for (int32 SeatPass = 0; SeatPass < 8 && Dew->GetLastTransformUploadCount() > 0; ++SeatPass)
			{
				Dew->Advance(FirstView, 0.5f);
			}
			Dew->Advance(FirstView, 0.5f);
			TestEqual(TEXT("A stationary view does not re-upload unchanged glint transforms"), Dew->GetLastTransformUploadCount(), 0);

			FTransform FirstTransform;
			TestTrue(TEXT("The first active glint has a transform"), Glints->GetInstanceTransform(0, FirstTransform, true));
			TestTrue(TEXT("A lit glint is scaled visibly above the inactive sentinel"), FirstTransform.GetScale3D().X > 0.001f);

			const FVector SecondView(FirstView.X + 5000.f, FirstView.Y, FirstView.Z);
			Dew->Advance(SecondView, 0.5f);
			TestEqual(TEXT("Moving the view reuses rather than grows the fixed pool"), Glints->GetInstanceCount(), AIslandDewActor::MaxGlints);
			TestTrue(TEXT("Viewer movement uploads newly seated glints"), Dew->GetLastTransformUploadCount() > 0);
			FTransform RepositionedTransform;
			TestTrue(TEXT("The active glint remains queryable after relocation"), Glints->GetInstanceTransform(0, RepositionedTransform, true));
			TestTrue(TEXT("A re-seated glint stays in the viewer's bounded disc"),
				FVector::Dist2D(RepositionedTransform.GetLocation(), SecondView) <= AIslandDewActor::Radius + 0.1f);
			for (int32 SeatPass = 0; SeatPass < 3; ++SeatPass)
			{
				Dew->Advance(SecondView, 0.5f);
			}
			bool bAllActiveGlintsWithinRadius = true;
			for (int32 Slot = 0; Slot < AIslandDewActor::DesiredCount(0.5f); ++Slot)
			{
				FTransform ActiveTransform;
				bAllActiveGlintsWithinRadius &= Glints->GetInstanceTransform(Slot, ActiveTransform, true) &&
					ActiveTransform.GetScale3D().X > 0.001f &&
					FVector::Dist2D(ActiveTransform.GetLocation(), SecondView) <= AIslandDewActor::Radius + 0.1f;
			}
			TestTrue(TEXT("Bounded seat batches eventually keep every active glint around the new view"), bAllActiveGlintsWithinRadius);
			Dew->Advance(SecondView, 0.5f);
			TestEqual(TEXT("A settled relocated scatter uploads no redundant transforms"), Dew->GetLastTransformUploadCount(), 0);

			Dew->Advance(SecondView, 0.f);
			TestFalse(TEXT("Zero strength hides the glints"), Glints->IsVisible());
			TestEqual(TEXT("Fading out preserves the bounded reusable pool"), Glints->GetInstanceCount(), AIslandDewActor::MaxGlints);
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
