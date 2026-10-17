// Copyright Epic Games, Inc. All Rights Reserved.

#include "CaptiveSky_2Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "CollisionShape.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "WaterBodyComponent.h"
#include "WaterSubsystem.h"
#include "IslandOceanSubsystem.h"
#include "IslandPoolRippleEffect.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "CaptiveSky_2.h"

ACaptiveSky_2Character::ACaptiveSky_2Character()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ACaptiveSky_2Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) return;
	const bool bWalkingAtGroundLevel = Movement->IsMovingOnGround();
	if (!bSurfaceSwimming && !Movement->IsFalling() && !bWalkingAtGroundLevel) return;

	UWaterSubsystem* WaterSubsystem = UWaterSubsystem::GetWaterSubsystem(GetWorld());
	UWaterBodyComponent* Ocean = WaterSubsystem ? WaterSubsystem->GetOceanBodyComponent().Get() : nullptr;
	if (!Ocean)
	{
		if (bSurfaceSwimming) LeaveSurfaceSwimming(MOVE_Falling);
		return;
	}

	FVector WaterSurfaceLocation = FVector::ZeroVector;
	FVector WaterSurfaceNormal = FVector::UpVector;
	FVector WaterVelocity = FVector::ZeroVector;
	float WaterDepth = 0.f;
	const bool bHasWaterSurface = Ocean->GetWaterSurfaceInfoAtLocation(
		GetActorLocation(), WaterSurfaceLocation, WaterSurfaceNormal, WaterVelocity, WaterDepth, false);
	if (!bHasWaterSurface)
	{
		if (bSurfaceSwimming) LeaveSurfaceSwimming(MOVE_Falling);
		return;
	}

	if (bSurfaceSwimming)
	{
		// Allow a swimmer to wade out once shallow, walkable ground is directly
		// beneath them. Keep the band narrow so submerged terrain in deep water
		// cannot unexpectedly switch the player back to walking.
		constexpr float MaxShoreStepUp = 20.f;
		constexpr float MaxWadingDepth = 55.f;
		const FVector TraceStart(GetActorLocation().X, GetActorLocation().Y,
			WaterSurfaceLocation.Z + MaxShoreStepUp);
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, WaterSurfaceLocation.Z - MaxWadingDepth);
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SurfaceSwimShoreExit), false, this);
		if (AActor* WaterOwner = Ocean->GetOwner()) TraceParams.AddIgnoredActor(WaterOwner);
		if (AStaticMeshActor* OceanPlane = UIslandOceanSubsystem::FindOceanPlane(GetWorld()))
		{
			TraceParams.AddIgnoredActor(OceanPlane);
		}

		FHitResult GroundHit;
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams) &&
			GroundHit.ImpactNormal.Z >= Movement->GetWalkableFloorZ())
		{
			const float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector ShoreLocation = GetActorLocation();
			ShoreLocation.Z = GroundHit.ImpactPoint.Z + CapsuleHalfHeight + 2.f;
			FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(SurfaceSwimShoreClearance), false, this);
			if (AActor* WaterOwner = Ocean->GetOwner()) OverlapParams.AddIgnoredActor(WaterOwner);
			if (AStaticMeshActor* OceanPlane = UIslandOceanSubsystem::FindOceanPlane(GetWorld()))
			{
				OverlapParams.AddIgnoredActor(OceanPlane);
			}

			const FCollisionShape Capsule = FCollisionShape::MakeCapsule(
				GetCapsuleComponent()->GetScaledCapsuleRadius(), CapsuleHalfHeight);
			if (!GetWorld()->OverlapBlockingTestByChannel(ShoreLocation, GetActorQuat(), ECC_Pawn, Capsule, OverlapParams))
			{
				SetActorLocation(ShoreLocation, false, nullptr, ETeleportType::TeleportPhysics);
				LeaveSurfaceSwimming(MOVE_Walking);
				return;
			}
		}
	}

	if (!bSurfaceSwimming)
	{
		const float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float CapsuleBottomZ = GetActorLocation().Z - CapsuleHalfHeight;
		// A leftover/duplicate ocean-plane collider can keep the player grounded
		// exactly at sea level. Treat that contact as water entry too, instead of
		// allowing the grounded walk mode to continue across the ocean.
		constexpr float GroundedWaterEntryTolerance = 75.f;
		const bool bFallingIntoWater = Movement->IsFalling() && CapsuleBottomZ <= WaterSurfaceLocation.Z;
		const bool bGroundedOnWaterSurface = bWalkingAtGroundLevel &&
			FMath::Abs(CapsuleBottomZ - WaterSurfaceLocation.Z) <= GroundedWaterEntryTolerance;
		if (bFallingIntoWater || bGroundedOnWaterSurface)
		{
			EnterSurfaceSwimming(WaterSurfaceLocation.Z);
		}
		else
		{
			return;
		}
	}

	// Flying movement keeps horizontal input responsive without requiring a
	// PhysicsVolume. The water plugin supplies the moving surface height.
	FVector TargetLocation = GetActorLocation();
	TargetLocation.Z = FMath::FInterpTo(
		TargetLocation.Z, WaterSurfaceLocation.Z + SurfaceSwimOffset, DeltaSeconds, 7.f);
	FHitResult SurfaceFollowHit;
	SetActorLocation(TargetLocation, true, &SurfaceFollowHit, ETeleportType::None);
	Movement->Velocity.Z = 0.f;
	TrySpawnSurfaceSwimWake(WaterSurfaceLocation);
}

void ACaptiveSky_2Character::TrySpawnSurfaceSwimWake(const FVector& WaterSurfaceLocation)
{
	UWorld* World = GetWorld();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!World || !Movement) return;

	constexpr float MinimumWakeSpeed = 80.f;
	constexpr float WakeIntervalSeconds = 1.15f;
	const FVector HorizontalVelocity = FVector(Movement->Velocity.X, Movement->Velocity.Y, 0.f);
	if (HorizontalVelocity.SizeSquared() < FMath::Square(MinimumWakeSpeed) ||
		World->GetTimeSeconds() < NextSurfaceSwimWakeTime)
	{
		return;
	}

	const FVector WakeDirection = HorizontalVelocity.GetSafeNormal();
	FVector WakeLocation = GetActorLocation() - WakeDirection * 48.f;
	WakeLocation.Z = WaterSurfaceLocation.Z + 2.f;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AIslandPoolRippleEffect* Wake = World->SpawnActor<AIslandPoolRippleEffect>(
		WakeLocation, FRotator::ZeroRotator, SpawnParameters);
	if (!Wake) return;

	Wake->ConfigureAsPlayerSwimImpact();
	NextSurfaceSwimWakeTime = World->GetTimeSeconds() + WakeIntervalSeconds;
}

void ACaptiveSky_2Character::EnterSurfaceSwimming(float WaterSurfaceZ)
{
	if (bSurfaceSwimming) return;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		PreviousMaxFlySpeed = Movement->MaxFlySpeed;
		Movement->MaxFlySpeed = SurfaceSwimSpeed;
		Movement->Velocity.Z = 0.f;
		Movement->SetMovementMode(MOVE_Flying);
	}

	bSurfaceSwimming = true;
	UE_LOG(LogTemp, Log, TEXT("[PlayerWater] %s entered surface swimming at Z=%.1f"), *GetName(), WaterSurfaceZ);

	// A short fall into the surface should not leave the capsule buried below it.
	FVector SurfaceEntryLocation = GetActorLocation();
	SurfaceEntryLocation.Z = FMath::Min(SurfaceEntryLocation.Z, WaterSurfaceZ + SurfaceSwimOffset);
	SetActorLocation(SurfaceEntryLocation, true);
}

void ACaptiveSky_2Character::LeaveSurfaceSwimming(EMovementMode NextMovementMode)
{
	if (!bSurfaceSwimming) return;

	bSurfaceSwimming = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxFlySpeed = PreviousMaxFlySpeed;
		Movement->SetMovementMode(NextMovementMode);
	}
	UE_LOG(LogTemp, Log, TEXT("[PlayerWater] %s left surface swimming; next mode=%d"), *GetName(), static_cast<int32>(NextMovementMode));
}

void ACaptiveSky_2Character::Jump()
{
	if (bSurfaceSwimming)
	{
		const float JumpVelocity = GetCharacterMovement()->JumpZVelocity;
		LeaveSurfaceSwimming(MOVE_Falling);
		LaunchCharacter(FVector(0.f, 0.f, JumpVelocity), false, true);
		return;
	}

	Super::Jump();
}

void ACaptiveSky_2Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACaptiveSky_2Character::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ACaptiveSky_2Character::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACaptiveSky_2Character::Look);
	}
	else
	{
		UE_LOG(LogCaptiveSky_2, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACaptiveSky_2Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ACaptiveSky_2Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ACaptiveSky_2Character::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ACaptiveSky_2Character::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACaptiveSky_2Character::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ACaptiveSky_2Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
