#include "IslandTidepoolCrab.h"
#include "IslandWeather.h"
#include "RavenAgentAIController.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "IslandPoolRippleEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AIslandTidepoolCrab::AIslandTidepoolCrab()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.08f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Tags.AddUnique(TEXT("IslandLife"));
	Tags.AddUnique(TEXT("TidepoolCrab"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseShapeMaterial = BasicMaterial.Succeeded() ? BasicMaterial.Object : nullptr;

	Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	Shell->SetupAttachment(RootComponent);
	Shell->SetRelativeLocation(FVector(-5.f, 0.f, 22.f));
	Shell->SetRelativeScale3D(FVector(0.30f, 0.24f, 0.16f));
	Shell->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
	Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shell->SetCastShadow(false);

	for (int32 Side : {-1, 1})
	{
		for (int32 LegIndex = 0; LegIndex < 3; ++LegIndex)
		{
			const FName Name(*FString::Printf(TEXT("Leg_%d_%d"), Side, LegIndex));
			UStaticMeshComponent* Leg = CreateDefaultSubobject<UStaticMeshComponent>(Name);
			Leg->SetupAttachment(RootComponent);
			Leg->SetRelativeLocation(FVector(-18.f + LegIndex * 11.f, Side * 20.f, 13.f));
			Leg->SetRelativeRotation(FRotator(0.f, Side * (28.f + LegIndex * 5.f), 0.f));
			Leg->SetRelativeScale3D(FVector(0.15f, 0.035f, 0.04f));
			Leg->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
			Leg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Leg->SetCastShadow(false);
			Legs.Add(Leg);
		}

		const FName ClawName(*FString::Printf(TEXT("Claw_%d"), Side));
		UStaticMeshComponent* Claw = CreateDefaultSubobject<UStaticMeshComponent>(ClawName);
		Claw->SetupAttachment(RootComponent);
		Claw->SetRelativeLocation(FVector(18.f, Side * 25.f, 15.f));
		Claw->SetRelativeScale3D(FVector(0.095f, 0.075f, 0.08f));
		Claw->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		Claw->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Claw->SetCastShadow(false);
		Claws.Add(Claw);

		const FName EyeName(*FString::Printf(TEXT("Eye_%d"), Side));
		UStaticMeshComponent* Eye = CreateDefaultSubobject<UStaticMeshComponent>(EyeName);
		Eye->SetupAttachment(RootComponent);
		Eye->SetRelativeLocation(FVector(18.f, Side * 10.f, 35.f));
		Eye->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.035f));
		Eye->SetStaticMesh(Sphere.Succeeded() ? Sphere.Object : nullptr);
		Eye->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Eye->SetCastShadow(false);
		Eyes.Add(Eye);
	}
}

void AIslandTidepoolCrab::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	Phase = FMath::FRandRange(0.f, 2.f * PI);
	for (TActorIterator<AIslandWeather> It(GetWorld()); It; ++It)
	{
		Weather = *It;
		break;
	}
	if (BaseShapeMaterial)
	{
		UMaterialInstanceDynamic* ShellTint = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		UMaterialInstanceDynamic* LegTint = UMaterialInstanceDynamic::Create(BaseShapeMaterial, this);
		if (ShellTint) ShellTint->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.16f, 0.10f));
		if (LegTint) LegTint->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.42f, 0.27f, 0.14f));
		if (ShellTint && Shell) Shell->SetMaterial(0, ShellTint);
		for (UStaticMeshComponent* Leg : Legs) if (Leg) Leg->SetMaterial(0, LegTint);
		for (UStaticMeshComponent* Claw : Claws) if (Claw) Claw->SetMaterial(0, LegTint);
		for (UStaticMeshComponent* Eye : Eyes) if (Eye) Eye->SetMaterial(0, ShellTint);
	}
}

void AIslandTidepoolCrab::RespondToQuietObservation(const FVector& ObserverLocation)
{
	if (bIsSheltered) return;
	ScurryDirection = (GetActorLocation() - ObserverLocation).GetSafeNormal2D();
	if (ScurryDirection.IsNearlyZero()) ScurryDirection = GetActorRightVector();
	ScurryRemaining = 2.4f;
}

void AIslandTidepoolCrab::CheckForLowRavenFlyby()
{
	if (!GetWorld() || bIsSheltered || ScurryRemaining > 0.f || RavenFlybyCooldownRemaining > 0.f) return;

	constexpr float FlybyRadius = 425.f;
	constexpr float MinimumHeight = 120.f;
	constexpr float MaximumHeight = 480.f;
	for (TActorIterator<ARavenAgentAIController> It(GetWorld()); It; ++It)
	{
		if (It->LocomotionState != ERavenLocomotionState::Flying) continue;
		const APawn* Raven = It->GetPawn();
		if (!IsValid(Raven)) continue;

		const FVector Offset = Raven->GetActorLocation() - GetActorLocation();
		if (Offset.Z < MinimumHeight || Offset.Z > MaximumHeight || Offset.SizeSquared2D() > FMath::Square(FlybyRadius))
			continue;

		// A close, low pass briefly startles the crab away from the bird; it remains
		// wild and resumes its local routine without becoming a target or changing state.
		RespondToQuietObservation(Raven->GetActorLocation());
		RavenFlybyCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTidepoolCrab::CheckForNearbyNaturalRipple()
{
	if (!GetWorld() || bIsSheltered || ScurryRemaining > 0.f || RippleResponseCooldownRemaining > 0.f) return;

	constexpr float RippleResponseRadius = 275.f;
	for (TActorIterator<AIslandPoolRippleEffect> It(GetWorld()); It; ++It)
	{
		AIslandPoolRippleEffect* Ripple = *It;
		if (!IsValid(Ripple) || (!Ripple->ActorHasTag(TEXT("WindImpact")) && !Ripple->ActorHasTag(TEXT("RainImpact")))) continue;
		if (FVector::DistSquared2D(GetActorLocation(), Ripple->GetActorLocation()) > FMath::Square(RippleResponseRadius)) continue;

		// Natural weather ripples briefly send the shore crab back from the water's edge.
		// The cue is local, transient and deliberately ignores untagged visitor ripples.
		RespondToQuietObservation(Ripple->GetActorLocation());
		RippleResponseCooldownRemaining = 8.f;
		return;
	}
}

void AIslandTidepoolCrab::SetSheltered(bool bSheltered)
{
	if (bIsSheltered == bSheltered) return;
	bIsSheltered = bSheltered;
	SetActorLocation(bIsSheltered ? HomeLocation - FVector(0.f, 0.f, 55.f) : HomeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(bIsSheltered);
	SetActorTickEnabled(!bIsSheltered);
}

float AIslandTidepoolCrab::RainMovementScale(float RainIntensity)
{
	const float RainActivity = FMath::SmoothStep(0.3f, 0.8f, FMath::Clamp(RainIntensity, 0.f, 1.f));
	return FMath::Lerp(1.f, 0.38f, RainActivity);
}

FVector AIslandTidepoolCrab::ResolveGroundPath(const FVector& Start, const FVector& Desired) const
{
	UWorld* World = GetWorld();
	if (!World || Start.Equals(Desired)) return Desired;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(10.f);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(IslandTidepoolCrabScurry), false, this);
	FHitResult Hit;
	if (!World->SweepSingleByChannel(Hit, Start, Desired, FQuat::Identity, ECC_WorldStatic, Shape, Query)) return Desired;
	if (Hit.bStartPenetrating) return Start;
	const FVector Contact = Hit.Location + Hit.Normal * 2.f;
	const FVector Remaining = Desired - Contact;
	const FVector Slide = Remaining - Hit.Normal * FVector::DotProduct(Remaining, Hit.Normal);
	if (Slide.IsNearlyZero()) return Contact;
	FHitResult SlideHit;
	if (World->SweepSingleByChannel(SlideHit, Contact, Contact + Slide, FQuat::Identity, ECC_WorldStatic, Shape, Query))
		return SlideHit.bStartPenetrating ? Contact : SlideHit.Location;
	return Contact + Slide;
}

void AIslandTidepoolCrab::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()) return;
	const float Time = GetWorld()->GetTimeSeconds();
	const float SafeDelta = FMath::Max(0.f, DeltaSeconds);
	ScurryRemaining = FMath::Max(0.f, ScurryRemaining - SafeDelta);
	RavenFlybyCooldownRemaining = FMath::Max(0.f, RavenFlybyCooldownRemaining - SafeDelta);
	RippleResponseCooldownRemaining = FMath::Max(0.f, RippleResponseCooldownRemaining - SafeDelta);
	RavenCheckRemaining -= SafeDelta;
	if (RavenCheckRemaining <= 0.f)
	{
		RavenCheckRemaining = 0.35f;
		CheckForLowRavenFlyby();
	}
	RippleCheckRemaining -= SafeDelta;
	if (RippleCheckRemaining <= 0.f)
	{
		RippleCheckRemaining = 0.35f;
		CheckForNearbyNaturalRipple();
	}
	const float Angle = Time * 0.12f + Phase;
	const float Rain = Weather.IsValid() ? Weather->SampleRainIntensity(Time) : 0.f;
	const float RainScale = RainMovementScale(Rain);
	const FVector IdleDrift(FMath::Sin(Angle) * 90.f * RainScale, FMath::Cos(Angle * 0.73f) * 80.f * RainScale, 0.f);
	const float ScurryAlpha = ScurryRemaining > 1.7f
		? FMath::SmoothStep(0.f, 0.7f, 2.4f - ScurryRemaining)
		: FMath::SmoothStep(0.f, 1.7f, ScurryRemaining);
	const FVector Desired = HomeLocation + IdleDrift + ScurryDirection * (95.f * ScurryAlpha);
	const FVector PreviousLocation = GetActorLocation();
	const FVector SafeLocation = ResolveGroundPath(PreviousLocation, Desired);
	SetActorLocation(SafeLocation, false);
	const FVector Facing = (SafeLocation - PreviousLocation).GetSafeNormal2D();
	if (!Facing.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Facing.Rotation(), DeltaSeconds, 3.f));
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
		if (Legs[Index])
		{
			const int32 Side = Index < 3 ? -1 : 1;
			const int32 LegIndex = Index % 3;
			const float Swing = FMath::Sin(Time * 11.f * RainScale + Index * PI * 0.5f) * (1.5f + ScurryAlpha * 5.f) * RainScale;
			Legs[Index]->SetRelativeRotation(FRotator(0.f, Side * (28.f + LegIndex * 5.f) + Swing, 0.f));
		}
}
