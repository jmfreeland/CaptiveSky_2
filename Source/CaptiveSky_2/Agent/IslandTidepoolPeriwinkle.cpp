#include "IslandTidepoolPeriwinkle.h"

#include "IslandCommonFrog.h"
#include "IslandDayNight.h"
#include "IslandTideglassDragonfly.h"
#include "IslandTideglassSubsystem.h"
#include "IslandTidepoolMinnows.h"
#include "IslandTidepoolCrab.h"
#include "IslandWeather.h"
#include "RatAgentAIController.h"
#include "RavenAgentAIController.h"
#include "ProceduralMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIslandTidepoolPeriwinkle::AIslandTidepoolPeriwinkle()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("TidepoolPeriwinkle"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeInstance(TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseShapeMaterial = BasicShapeInstance.Succeeded() ? BasicShapeInstance.Object : (BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr);

	Foot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foot"));
	Foot->SetupAttachment(RootComponent);
	Foot->SetRelativeLocation(FVector(1.f, 0.f, 2.3f));
	Foot->SetRelativeScale3D(FVector(0.25f, 0.17f, 0.04f));
	Foot->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);

	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(RootComponent);
	Shell->SetRelativeLocation(FVector(-4.f, 0.f, 13.f));
	Shell->SetRelativeScale3D(FVector(0.21f, 0.19f, 0.18f));
	Shell->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);

	Aperture = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellAperture"));
	Aperture->SetupAttachment(RootComponent);
	Aperture->SetRelativeLocation(FVector(8.f, 0.f, 8.f));
	Aperture->SetRelativeScale3D(FVector(0.035f, 0.105f, 0.10f));
	Aperture->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);

	for (int32 Side : {-1, 1})
	{
		const FName StalkName(*FString::Printf(TEXT("EyeStalk_%d"), Side));
		UStaticMeshComponent* Stalk = CreateDefaultSubobject<UStaticMeshComponent>(StalkName);
		Stalk->SetupAttachment(RootComponent);
		Stalk->SetRelativeLocation(FVector(9.f, Side * 6.f, 7.f));
		Stalk->SetRelativeRotation(FRotator(0.f, Side * 10.f, -Side * 12.f));
		Stalk->SetRelativeScale3D(FVector(0.018f, 0.018f, 0.10f));
		Stalk->SetStaticMesh(Cylinder.Succeeded() ? Cylinder.Object : nullptr);
		EyeStalks.Add(Stalk);

		const FName EyeName(*FString::Printf(TEXT("Eye_%d"), Side));
		UStaticMeshComponent* Eye = CreateDefaultSubobject<UStaticMeshComponent>(EyeName);
		Eye->SetupAttachment(Stalk);
		Eye->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
		Eye->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.035f));
		Eye->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		Eyes.Add(Eye);
	}

	ShellSpiral = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShellGrowthSpiral"));
	ShellSpiral->SetupAttachment(RootComponent);
	ShellSpiral->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShellSpiral->SetCanEverAffectNavigation(false);
	ShellSpiral->SetGenerateOverlapEvents(false);
	ShellSpiral->SetCastShadow(false);

	const TArray<UPrimitiveComponent*> NonBlockingParts = { Foot.Get(), Shell.Get(), Aperture.Get(), ShellSpiral.Get() };
	for (UPrimitiveComponent* Part : NonBlockingParts)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
	}
	for (UStaticMeshComponent* Part : EyeStalks)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
	}
	for (UStaticMeshComponent* Part : Eyes)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
	}

	BuildShellSpiral();
}

void AIslandTidepoolPeriwinkle::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	Phase = FMath::Fmod(FMath::Abs(HomeLocation.X * 0.001f + HomeLocation.Y * 0.0017f), 2.f * PI);
	RavenCheckRemaining = 0.4f * (Phase / (2.f * PI));
	if (const UStaticMeshComponent* PoolSurface = UIslandTideglassSubsystem::FindPoolSurface(GetWorld()))
	{
		SeaLevelZ = PoolSurface->GetComponentLocation().Z;
		bHasSeaLevel = FMath::IsFinite(SeaLevelZ);
	}
	for (TActorIterator<AIslandDayNight> It(GetWorld()); It; ++It)
	{
		IslandClock = *It;
		break;
	}
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		IslandWeather = *It;
		break;
	}
	if (!BaseShapeMaterial)
	{
		BaseShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial_Inst.BasicShapeMaterial_Inst"));
		if (!BaseShapeMaterial)
			BaseShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	if (BaseShapeMaterial)
	{
		FootMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		ShellMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		SpiralMaterial = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		if (FootMaterial) Foot->SetMaterial(0, FootMaterial);
		if (ShellMaterial) Shell->SetMaterial(0, ShellMaterial);
		if (SpiralMaterial) ShellSpiral->SetMaterial(0, SpiralMaterial);
		if (Aperture && SpiralMaterial) Aperture->SetMaterial(0, SpiralMaterial);
	}
	ApplyPalette();
}

float AIslandTidepoolPeriwinkle::ExposureRestAmount(float TideOffsetCm)
{
	if (!FMath::IsFinite(TideOffsetCm)) return 0.f;
	const float NormalizedTide = FMath::Clamp(
		TideOffsetCm / UIslandTideglassSubsystem::MaximumTideOffsetCm, -1.f, 1.f);
	// Stay open while water is returning; tuck in only through the exposed ebb.
	return 1.f - FMath::SmoothStep(-0.72f, -0.12f, NormalizedTide);
}

float AIslandTidepoolPeriwinkle::LocalWaterlineOffsetCm(float TideOffsetCm, float GroundZCm, float SeaLevelZCm)
{
	if (!FMath::IsFinite(TideOffsetCm)) return 0.f;
	if (!FMath::IsFinite(GroundZCm) || !FMath::IsFinite(SeaLevelZCm)) return TideOffsetCm;

	// A positive result means the current waterline is high relative to this
	// snail's rock; a high rock instead experiences a more severe local ebb.
	const double RelativeWaterline = static_cast<double>(TideOffsetCm) +
		static_cast<double>(SeaLevelZCm) - static_cast<double>(GroundZCm);
	return static_cast<float>(FMath::Clamp(RelativeWaterline, -1000000.0, 1000000.0));
}

float AIslandTidepoolPeriwinkle::RainRestAmount(float RainIntensity)
{
	if (!FMath::IsFinite(RainIntensity)) return 0.f;
	return 0.42f * FMath::SmoothStep(0.45f, 0.9f, FMath::Clamp(RainIntensity, 0.f, 1.f));
}

float AIslandTidepoolPeriwinkle::WindRestAmount(float HorizontalWindSpeed)
{
	if (!FMath::IsFinite(HorizontalWindSpeed)) return 0.f;
	// The Island's ordinary breeze only softens grazing; the maximum tuck is
	// reserved for a hard storm gust. This is a broad weather sample, not a
	// per-snail shelter trace, so a colony adds no extra collision queries.
	return 0.38f * FMath::SmoothStep(90.f, 330.f, FMath::Max(0.f, HorizontalWindSpeed));
}

bool AIslandTidepoolPeriwinkle::ShouldTuckForCrab(bool bCrabScurrying, float Distance, float VerticalOffset)
{
	constexpr float ThreatRadius = 175.f;
	constexpr float MaximumVerticalOffset = 100.f;
	return bCrabScurrying && FMath::IsFinite(Distance) && Distance >= 0.f && FMath::IsFinite(VerticalOffset) &&
		Distance <= ThreatRadius && FMath::Abs(VerticalOffset) <= MaximumVerticalOffset;
}

bool AIslandTidepoolPeriwinkle::ShouldTuckForRat(float Distance, float VerticalOffset)
{
	constexpr float ThreatRadius = 190.f;
	constexpr float MaximumVerticalOffset = 120.f;
	return FMath::IsFinite(Distance) && Distance >= 0.f && FMath::IsFinite(VerticalOffset) &&
		Distance <= ThreatRadius && FMath::Abs(VerticalOffset) <= MaximumVerticalOffset;
}

float AIslandTidepoolPeriwinkle::WetShellAmount(float TideOffsetCm, float RainIntensity)
{
	const float TideWetness = FMath::IsFinite(TideOffsetCm)
		? 1.f - ExposureRestAmount(TideOffsetCm)
		: 0.f;
	const float RainWetness = FMath::IsFinite(RainIntensity)
		? 0.55f * FMath::SmoothStep(0.45f, 0.9f, FMath::Clamp(RainIntensity, 0.f, 1.f))
		: 0.f;
	return FMath::Clamp(FMath::Max(TideWetness, RainWetness), 0.f, 1.f);
}

FVector2D AIslandTidepoolPeriwinkle::GrazingOffset(float ActiveSeconds, float InitialPhaseRadians)
{
	if (!FMath::IsFinite(ActiveSeconds) || !FMath::IsFinite(InitialPhaseRadians))
	{
		return FVector2D::ZeroVector;
	}

	constexpr float PathRadiusCm = GrazingMaxExcursionCm * 0.5f;
	constexpr float CycleSeconds = 120.f;
	const float StartAngle = InitialPhaseRadians;
	const float CurrentAngle = StartAngle + FMath::Fmod(ActiveSeconds, CycleSeconds) * (2.f * PI / CycleSeconds);
	const FVector2D Start(FMath::Cos(StartAngle), FMath::Sin(StartAngle));
	const FVector2D Current(FMath::Cos(CurrentAngle), FMath::Sin(CurrentAngle));
	return (Current - Start) * PathRadiusCm;
}

float AIslandTidepoolPeriwinkle::AdvanceGrazingTime(float ActiveSeconds, float DeltaSeconds, float ExposureRest)
{
	if (!FMath::IsFinite(ActiveSeconds) || !FMath::IsFinite(DeltaSeconds) || !FMath::IsFinite(ExposureRest))
	{
		return 0.f;
	}
	const float Activity = 1.f - FMath::Clamp(ExposureRest, 0.f, 1.f);
	return FMath::Max(0.f, ActiveSeconds) + FMath::Max(0.f, DeltaSeconds) * Activity;
}

float AIslandTidepoolPeriwinkle::GrazingActivityForHour(float CurrentHour)
{
	if (!FMath::IsFinite(CurrentHour)) return 1.f;

	// Keep the snail quietly active by day, but let its grazing path progress
	// more readily through the cool, dim hours without a hard dawn/dusk switch.
	constexpr float DayActivity = 0.45f;
	constexpr float AdditionalNightActivity = 1.f - DayActivity;
	return DayActivity + AdditionalNightActivity * AIslandDayNight::NightAmount(CurrentHour);
}

bool AIslandTidepoolPeriwinkle::RespondToQuietObservation()
{
	if (ObservationCooldownRemaining > 0.f) return false;
	ObservationRestRemaining = 1.6f;
	ObservationCooldownRemaining = 4.2f;
	return true;
}

void AIslandTidepoolPeriwinkle::CheckForNearbyCrabDisturbance()
{
	if (!GetWorld()) return;

	constexpr float CrabRearmRadius = 500.f;
	for (TActorIterator<AIslandTidepoolCrab> It(GetWorld()); It; ++It)
	{
		AIslandTidepoolCrab* Crab = *It;
		if (!IsValid(Crab)) continue;

		const FVector Offset = Crab->GetActorLocation() - GetActorLocation();
		const float Distance = Offset.Size2D();
		if (CrabPresenceLatch.Get() == Crab)
		{
			// One tuck per scurry; a calm interval or a clear retreat rearms the cue.
			if (!Crab->IsScurrying() || Distance > CrabRearmRadius)
				CrabPresenceLatch.Reset();
			else
				continue;
		}

		if (!ShouldTuckForCrab(Crab->IsScurrying(), Distance, Offset.Z)) continue;
		if (!RespondToQuietObservation()) continue;
		CrabPresenceLatch = Crab;
		return;
	}
}

void AIslandTidepoolPeriwinkle::CheckForNearbyRavenDisturbance()
{
	if (!GetWorld()) return;
	constexpr float RavenRadius = 300.f;
	constexpr float RavenRearmRadius = 520.f;
	constexpr float MaximumVerticalOffset = 180.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		const float DistanceSquared = Offset.SizeSquared2D();
		if (RavenPresenceLatch.Get() == *It && DistanceSquared > FMath::Square(RavenRearmRadius))
			RavenPresenceLatch.Reset();

		const bool bSettledRaven = It->LocomotionState == ERavenLocomotionState::Grounded ||
			It->LocomotionState == ERavenLocomotionState::Hopping ||
			It->LocomotionState == ERavenLocomotionState::Perched;
		if (!bSettledRaven || FMath::Abs(Offset.Z) > MaximumVerticalOffset ||
			DistanceSquared > FMath::Square(RavenRadius) || RavenPresenceLatch.Get() == *It)
			continue;

		// Notice only the first settled approach. A bird lingering nearby does not
		// keep the snail tucked, and an airborne pass never counts as a threat cue.
		RavenPresenceLatch = *It;
		RespondToQuietObservation();
		return;
	}
}

void AIslandTidepoolPeriwinkle::CheckForNearbyRatDisturbance()
{
	if (!GetWorld()) return;

	constexpr float RatRearmRadius = 520.f;
	for (TActorIterator<ARatAgentAIController> It(GetWorld()); It; ++It)
	{
		ARatAgentAIController* RatController = *It;
		const APawn* Rat = IsValid(RatController) ? RatController->GetPawn() : nullptr;
		if (!IsValid(Rat)) continue;

		const FVector Offset = Rat->GetActorLocation() - GetActorLocation();
		const float Distance = Offset.Size2D();
		if (RatPresenceLatch.Get() == RatController)
		{
			if (Distance > RatRearmRadius)
				RatPresenceLatch.Reset();
			else
				continue;
		}

		if (!ShouldTuckForRat(Distance, Offset.Z)) continue;
		// A single close approach is enough; the rat must leave the wider ring
		// before a later encounter can startle the snail again.
		RatPresenceLatch = RatController;
		RespondToQuietObservation();
		return;
	}
}

void AIslandTidepoolPeriwinkle::CheckForNearbyPlayerDisturbance(APawn* PlayerPawn)
{
	if (!IsValid(PlayerPawn))
	{
		PlayerPresenceLatch.Reset();
		return;
	}

	constexpr float PlayerRadius = 240.f;
	constexpr float PlayerRearmRadius = 520.f;
	constexpr float MaximumVerticalOffset = 220.f;
	const FVector Offset = PlayerPawn->GetActorLocation() - GetActorLocation();
	const float DistanceSquared = Offset.SizeSquared2D();
	if (PlayerPresenceLatch.Get() == PlayerPawn && DistanceSquared > FMath::Square(PlayerRearmRadius))
		PlayerPresenceLatch.Reset();

	if (FMath::Abs(Offset.Z) > MaximumVerticalOffset ||
		DistanceSquared > FMath::Square(PlayerRadius) || PlayerPresenceLatch.Get() == PlayerPawn)
	{
		return;
	}

	// A single close approach is enough; lingering beside the snail does not
	// keep it tucked, and the player must leave the wider ring before rearming.
	PlayerPresenceLatch = PlayerPawn;
	RespondToQuietObservation();
}

void AIslandTidepoolPeriwinkle::SetColorVariant(int32 Variant)
{
	ColorVariant = FMath::Clamp(Variant, 0, 2);
	ApplyPalette();
}

void AIslandTidepoolPeriwinkle::ApplyPalette()
{
	static const FLinearColor ShellPalettes[] = {
		FLinearColor(0.46f, 0.24f, 0.095f),
		FLinearColor(0.31f, 0.34f, 0.19f),
		FLinearColor(0.48f, 0.34f, 0.18f)
	};
	const FLinearColor ShellColor = ShellPalettes[FMath::Clamp(ColorVariant, 0, 2)];
	const float Wetness = FMath::Clamp(ShellWetness, 0.f, 1.f);
	const FLinearColor DampShellColor = FMath::Lerp(ShellColor,
		FLinearColor(ShellColor.R * 0.78f, ShellColor.G * 0.84f, ShellColor.B * 0.9f, 1.f), Wetness);
	if (FootMaterial) FootMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.22f, 0.25f, 0.15f));
	if (ShellMaterial) ShellMaterial->SetVectorParameterValue(TEXT("Color"), DampShellColor);
	if (SpiralMaterial) SpiralMaterial->SetVectorParameterValue(TEXT("Color"), DampShellColor * 0.42f);
	for (UStaticMeshComponent* Eye : Eyes)
		if (Eye && BaseShapeMaterial) Eye->SetMaterial(0, BaseShapeMaterial);
}

void AIslandTidepoolPeriwinkle::BuildShellSpiral()
{
	if (!ShellSpiral) return;

	constexpr int32 SampleCount = 72;
	constexpr int32 TubeSides = 5;
	constexpr float ShellCenterX = -4.f;
	constexpr float ShellCenterZ = 13.f;
	constexpr float ShellRadiusX = 10.5f;
	constexpr float ShellRadiusY = 9.5f;
	constexpr float ShellRadiusZ = 9.f;
	constexpr float StripeRadius = 0.48f;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(SampleCount * TubeSides);
	Triangles.Reserve((SampleCount - 1) * TubeSides * 6);
	Normals.Reserve(SampleCount * TubeSides);
	UVs.Reserve(SampleCount * TubeSides);
	Colors.Reserve(SampleCount * TubeSides);
	Tangents.Reserve(SampleCount * TubeSides);

	for (int32 Sample = 0; Sample < SampleCount; ++Sample)
	{
		const float T = static_cast<float>(Sample) / (SampleCount - 1);
		const float Angle = T * 4.4f * PI;
		const float Radius = FMath::Lerp(0.45f, 8.25f, T);
		const float XOffset = Radius * FMath::Cos(Angle);
		const float Y = Radius * FMath::Sin(Angle);
		const float SurfaceTerm = FMath::Max(0.06f, 1.f - FMath::Square(XOffset / ShellRadiusX) - FMath::Square(Y / ShellRadiusY));
		const float Z = ShellCenterZ + ShellRadiusZ * FMath::Sqrt(SurfaceTerm) + 0.35f;
		const FVector Center(ShellCenterX + XOffset, Y, Z);
		const FVector SurfaceNormal(XOffset / FMath::Square(ShellRadiusX), Y / FMath::Square(ShellRadiusY), (Z - ShellCenterZ) / FMath::Square(ShellRadiusZ));
		const FVector Normal = SurfaceNormal.GetSafeNormal();
		const FVector Tangent(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		const FVector Side = FVector::CrossProduct(Tangent, Normal).GetSafeNormal();
		for (int32 Ring = 0; Ring < TubeSides; ++Ring)
		{
			const float RingAngle = 2.f * PI * Ring / TubeSides;
			const FVector Offset = (Normal * FMath::Cos(RingAngle) + Side * FMath::Sin(RingAngle)) * StripeRadius;
			Vertices.Add(Center + Offset);
			Normals.Add((Normal * FMath::Cos(RingAngle) + Side * FMath::Sin(RingAngle)).GetSafeNormal());
			UVs.Add(FVector2D(T, static_cast<float>(Ring) / TubeSides));
			Colors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(Tangent, false));
		}
	}

	for (int32 Sample = 0; Sample < SampleCount - 1; ++Sample)
	{
		for (int32 Ring = 0; Ring < TubeSides; ++Ring)
		{
			const int32 A = Sample * TubeSides + Ring;
			const int32 B = Sample * TubeSides + (Ring + 1) % TubeSides;
			const int32 C = (Sample + 1) * TubeSides + (Ring + 1) % TubeSides;
			const int32 D = (Sample + 1) * TubeSides + Ring;
			Triangles.Append({ A, B, C, A, C, D, A, C, B, A, D, C });
		}
	}
	ShellSpiral->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void AIslandTidepoolPeriwinkle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ElapsedSeconds += FMath::Max(0.f, DeltaSeconds);
	const float TideOffset = IslandClock.IsValid()
		? UIslandTideglassSubsystem::TideOffsetCm(IslandClock->CurrentHour, IslandClock->DayNumber)
		: UIslandTideglassSubsystem::MaximumTideOffsetCm;
	const float LocalTideOffset = bHasSeaLevel
		? LocalWaterlineOffsetCm(TideOffset, HomeLocation.Z, SeaLevelZ)
		: TideOffset;
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ObservationRestRemaining = FMath::Max(0.f, ObservationRestRemaining - SafeDelta);
	ObservationCooldownRemaining = FMath::Max(0.f, ObservationCooldownRemaining - SafeDelta);
	RavenCheckRemaining = FMath::Max(0.f, RavenCheckRemaining - SafeDelta);
	MinnowAttentionRemaining = FMath::Max(0.f, MinnowAttentionRemaining - SafeDelta);
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.4f;
		FVector TargetLocation = FVector::ZeroVector;
		WildlifeAttentionTarget = FindNearbyWildlifeAttentionTarget(TargetLocation);
		if (Cast<AIslandTidepoolMinnows>(WildlifeAttentionTarget.Get()))
		{
			MinnowAttentionLocation = TargetLocation;
			MinnowAttentionRemaining = MinnowAttentionHoldSeconds;
		}
		else if (WildlifeAttentionTarget.IsValid())
		{
			MinnowAttentionRemaining = 0.f;
		}
		if (WildlifeAttentionTarget.IsValid())
			WildlifeAttentionLocation = TargetLocation;
		else if (MinnowAttentionRemaining <= 0.f)
			WildlifeAttentionLocation = FVector::ZeroVector;
		CheckForNearbyRavenDisturbance();
		CheckForNearbyRatDisturbance();
		CheckForNearbyPlayerDisturbance(UGameplayStatics::GetPlayerPawn(this, 0));
		CheckForNearbyCrabDisturbance();
	}
	const float TideRest = ExposureRestAmount(LocalTideOffset);
	const float ObservationRest = FMath::SmoothStep(0.f, 0.35f, ObservationRestRemaining);
	const float RainIntensity = IslandWeather.IsValid() && GetWorld()
		? IslandWeather->SampleRainIntensity(GetWorld()->GetTimeSeconds())
		: 0.f;
	const float HorizontalWindSpeed = IslandWeather.IsValid() && GetWorld()
		? IslandWeather->SampleWind(HomeLocation, GetWorld()->GetTimeSeconds()).Size2D()
		: 0.f;
	const float TargetShellWetness = WetShellAmount(LocalTideOffset, RainIntensity);
	const float PreviousShellWetness = ShellWetness;
	ShellWetness = FMath::FInterpTo(ShellWetness, TargetShellWetness, SafeDelta, 0.8f);
	if (FMath::Abs(ShellWetness - PreviousShellWetness) > 0.001f)
		ApplyPalette();
	const float WeatherRest = RainRestAmount(RainIntensity);
	const float WindRest = WindRestAmount(HorizontalWindSpeed);
	const float ExposureRest = FMath::Max(TideRest, FMath::Max(ObservationRest, FMath::Max(WeatherRest, WindRest)));
	const float StalkExtension = FMath::Lerp(1.f, 0.46f, ExposureRest);
	const float ApertureExtension = FMath::Lerp(1.f, 0.5f, ExposureRest);
	const float AttentionScale = FMath::Lerp(1.f, 0.35f, ExposureRest);
	// Close visitors draw a quiet glance; the animal stays planted on its rock.
	float TargetAttentionYaw = 0.f;
	bool bPlayerInAttentionRange = false;
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const float PlayerDistance = FVector::Dist2D(GetActorLocation(), PlayerPawn->GetActorLocation());
		if (FMath::IsFinite(PlayerDistance) && PlayerDistance <= 220.f)
		{
			bPlayerInAttentionRange = true;
			TargetAttentionYaw = CalculateAttentionYaw(PlayerPawn->GetActorLocation());
		}
	}
	if (!bPlayerInAttentionRange)
	{
		if (WildlifeAttentionTarget.IsValid())
			TargetAttentionYaw = CalculateAttentionYaw(WildlifeAttentionLocation);
		else if (MinnowAttentionRemaining > 0.f)
			TargetAttentionYaw = CalculateAttentionYaw(MinnowAttentionLocation);
	}
	UpdateAttentionYaw(TargetAttentionYaw, DeltaSeconds);
	for (UStaticMeshComponent* EyeStalk : EyeStalks)
		if (EyeStalk) EyeStalk->SetRelativeScale3D(FVector(0.018f, 0.018f, 0.10f * StalkExtension));
	if (Aperture) Aperture->SetRelativeScale3D(FVector(0.035f, 0.105f, 0.10f * ApertureExtension));

	// A periwinkle grazes over a small patch only while conditions let it stay
	// extended. The closed two-minute path returns to its starting point and
	// remains within a 16 cm radius of its home, so it cannot wander off its rock.
	const float CrawlActivity = 1.f - FMath::Clamp(ExposureRest, 0.f, 1.f);
	const float CircadianActivity = IslandClock.IsValid()
		? GrazingActivityForHour(IslandClock->CurrentHour)
		: 1.f;
	GrazingActiveSeconds = AdvanceGrazingTime(
		GrazingActiveSeconds, SafeDelta * CircadianActivity, ExposureRest);
	FVector CrawlGround(GetActorLocation().X, GetActorLocation().Y, HomeLocation.Z);
	if (CrawlActivity > 0.01f)
	{
		const FVector2D CrawlOffsetCm = GrazingOffset(GrazingActiveSeconds, Phase);
		const FVector DesiredGround = HomeLocation + FVector(CrawlOffsetCm.X, CrawlOffsetCm.Y, 0.f);
		if (UWorld* World = GetWorld())
		{
			FHitResult GroundHit;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(PeriwinkleGrazing), false, this);
			const FVector TraceStart = DesiredGround + FVector(0.f, 0.f, 80.f);
			const FVector TraceEnd = DesiredGround - FVector(0.f, 0.f, 140.f);
			if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Query) &&
				GroundHit.ImpactNormal.Z >= 0.45f &&
				FMath::Abs(GroundHit.ImpactPoint.Z - HomeLocation.Z) <= 24.f)
			{
				CrawlGround = GroundHit.ImpactPoint;
			}
		}
	}

		// Barely perceptible antenna and body motion rewards a close look without
		// turning naturally slow intertidal life into a conspicuous moving prop.
		const float ActivityScale = FMath::Lerp(1.f, 0.3f, ExposureRest);
	const float Sway = FMath::Sin(ElapsedSeconds * 0.8f + Phase) * 2.4f * ActivityScale;
	for (int32 Index = 0; Index < EyeStalks.Num(); ++Index)
	{
		if (EyeStalks[Index])
			EyeStalks[Index]->SetRelativeRotation(FRotator(0.f, (Index == 0 ? 1.f : -1.f) * (12.f + Sway) + AttentionYaw * AttentionScale, (Index == 0 ? -1.f : 1.f) * 12.f));
	}
	SetActorRotation(FRotator(0.f, Sway * 0.45f, 0.f));
	const float BodyBob = FMath::Sin(ElapsedSeconds * 0.55f + Phase) * FMath::Lerp(0.8f, 0.2f, ExposureRest);
	SetActorLocation(CrawlGround + FVector(0.f, 0.f, BodyBob));
}

void AIslandTidepoolPeriwinkle::UpdateAttentionYaw(float TargetYaw, float DeltaSeconds)
{
	// Turn toward a nearby visitor attentively, but relax the gaze more slowly.
	const float InterpSpeed = FMath::IsNearlyZero(TargetYaw) ? 1.2f : 2.4f;
	AttentionYaw = FMath::FInterpTo(AttentionYaw, TargetYaw, DeltaSeconds, InterpSpeed);
}

float AIslandTidepoolPeriwinkle::CalculateAttentionYaw(const FVector& TargetLocation) const
{
	constexpr float AttentionRadius = 220.f;
	constexpr float MaximumYaw = 35.f;
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero() || ToTarget.SizeSquared() > FMath::Square(AttentionRadius))
		return 0.f;

	const FVector LocalDirection = GetActorTransform().InverseTransformVectorNoScale(ToTarget);
	if (LocalDirection.X <= 0.f)
		return 0.f;
	return FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(LocalDirection.Y, LocalDirection.X)), -MaximumYaw, MaximumYaw);
}

bool AIslandTidepoolPeriwinkle::ShouldAttendWildlife(float Distance, float VerticalOffset)
{
	constexpr float AttentionRadius = 220.f;
	constexpr float MinimumVerticalOffset = -80.f;
	constexpr float MaximumVerticalOffset = 220.f;
	return FMath::IsFinite(Distance) && Distance >= 0.f && Distance <= AttentionRadius &&
		FMath::IsFinite(VerticalOffset) && VerticalOffset >= MinimumVerticalOffset &&
		VerticalOffset <= MaximumVerticalOffset;
}

AActor* AIslandTidepoolPeriwinkle::FindNearbyWildlifeAttentionTarget(FVector& OutTargetLocation) const
{
	UWorld* World = GetWorld();
	OutTargetLocation = FVector::ZeroVector;
	if (!World) return nullptr;

	AActor* BestTarget = nullptr;
	FVector BestTargetLocation = FVector::ZeroVector;
	float BestDistanceSquared = FMath::Square(220.f);
	const FVector EyeBase = GetActorLocation();
	const auto Consider = [this, &BestTarget, &BestTargetLocation, &BestDistanceSquared, &EyeBase](AActor* Candidate,
		const FVector& CandidateLocation)
	{
		if (!IsValid(Candidate)) return;
		const FVector Offset = CandidateLocation - EyeBase;
		const float DistanceSquared = Offset.SizeSquared2D();
		const float Distance = FMath::Sqrt(DistanceSquared);
		if (!ShouldAttendWildlife(Distance, Offset.Z) || DistanceSquared > BestDistanceSquared) return;

		// Eye stalks can follow a nearby animal in front of the shell without
		// rotating the whole body or adding a visibility trace per snail.
		const FVector LocalDirection = GetActorTransform().InverseTransformVectorNoScale(Offset);
		if (LocalDirection.X <= 0.f) return;
		BestTarget = Candidate;
		BestTargetLocation = CandidateLocation;
		BestDistanceSquared = DistanceSquared;
	};

	for (TActorIterator<AIslandCommonFrog> It(World); It; ++It)
		Consider(*It, It->GetActorLocation());
	for (TActorIterator<AIslandTideglassDragonfly> It(World); It; ++It)
		Consider(*It, It->GetActorLocation());
	for (TActorIterator<AIslandTidepoolMinnows> It(World); It; ++It)
	{
		AIslandTidepoolMinnows* Minnows = *It;
		if (Minnows && Minnows->IsMakingSurfaceBreak())
			Consider(Minnows, Minnows->GetSurfaceBreakWorldLocation());
	}

	if (BestTarget)
		OutTargetLocation = BestTargetLocation;
	return BestTarget;
}
