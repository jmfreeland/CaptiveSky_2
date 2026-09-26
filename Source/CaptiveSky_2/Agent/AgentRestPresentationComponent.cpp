#include "AgentRestPresentationComponent.h"
#include "AgentConsolidationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAgentRestPresentationComponent::UAgentRestPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAgentRestPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	if (Character.IsValid())
	{
		BodyMesh = Character->GetMesh();
		if (BodyMesh.IsValid())
		{
			AwakeMeshTransform = BodyMesh->GetRelativeTransform();
			bHasCapturedAwakeTransform = true;
		}
	}
	if (GetOwner()) BindToConsciousness(GetOwner()->FindComponentByClass<UAgentConsolidationComponent>());
}

void UAgentRestPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundConsciousness.IsValid())
		BoundConsciousness->OnConsciousStateChanged.RemoveDynamic(this, &UAgentRestPresentationComponent::HandleConsciousStateChanged);
	BoundConsciousness.Reset();
	Super::EndPlay(EndPlayReason);
}

void UAgentRestPresentationComponent::BindToConsciousness(UAgentConsolidationComponent* Consciousness)
{
	if (BoundConsciousness.IsValid())
		BoundConsciousness->OnConsciousStateChanged.RemoveDynamic(this, &UAgentRestPresentationComponent::HandleConsciousStateChanged);
	BoundConsciousness = Consciousness;
	if (Consciousness)
	{
		Consciousness->OnConsciousStateChanged.AddUniqueDynamic(this, &UAgentRestPresentationComponent::HandleConsciousStateChanged);
		ApplyConsciousState(Consciousness->ConsciousState);
	}
}

void UAgentRestPresentationComponent::SetRestPosture(EAgentRestPosture NewPosture)
{
	RestPosture = NewPosture;
	if (BoundConsciousness.IsValid()) ApplyConsciousState(BoundConsciousness->ConsciousState);
}

void UAgentRestPresentationComponent::HandleConsciousStateChanged(EAgentConsciousState PreviousState, EAgentConsciousState NewState)
{
	ApplyConsciousState(NewState);
}

void UAgentRestPresentationComponent::ApplyConsciousState(EAgentConsciousState State)
{
	const bool bShouldRest = State != EAgentConsciousState::Awake;
	ACharacter* CharacterActor = Character.Get();
	if (!CharacterActor && GetOwner()) CharacterActor = Cast<ACharacter>(GetOwner());
	if (!CharacterActor) return;
	Character = CharacterActor;
	if (!BodyMesh.IsValid()) BodyMesh = CharacterActor->GetMesh();
	if (BodyMesh.IsValid() && !bHasCapturedAwakeTransform)
	{
		AwakeMeshTransform = BodyMesh->GetRelativeTransform();
		bHasCapturedAwakeTransform = true;
	}

	if (RestPosture == EAgentRestPosture::GroundedSettle)
	{
		UCharacterMovementComponent* Movement = CharacterActor->GetCharacterMovement();
		if (Movement)
		{
			FNavAgentProperties& NavProperties = Movement->GetNavAgentPropertiesRef();
			if (bShouldRest)
			{
				if (!bHasCapturedCrouchCapability)
				{
					bOriginalCanCrouch = NavProperties.bCanCrouch;
					bWasCrouchedBeforeRest = CharacterActor->bIsCrouched;
					bHasCapturedCrouchCapability = true;
				}
				NavProperties.bCanCrouch = true;
				if (!bWasCrouchedBeforeRest && Movement->IsMovingOnGround()) CharacterActor->Crouch();
			}
			else if (bHasCapturedCrouchCapability)
			{
				// Cancel even a queued-but-not-yet-applied request if wake arrives before CharacterMovement's next update.
				if (!bWasCrouchedBeforeRest) CharacterActor->UnCrouch();
				NavProperties.bCanCrouch = bOriginalCanCrouch;
				bHasCapturedCrouchCapability = false;
				bWasCrouchedBeforeRest = false;
			}
		}
	}

	if (BodyMesh.IsValid() && bHasCapturedAwakeTransform)
	{
		StartMeshTransform = BodyMesh->GetRelativeTransform();
		TargetMeshTransform = AwakeMeshTransform;
		if (bShouldRest && RestPosture == EAgentRestPosture::PerchedBird)
		{
			TargetMeshTransform.AddToTranslation(FVector(0.f, 0.f, -1.5f));
			TargetMeshTransform.SetRotation((TargetMeshTransform.GetRotation() * FQuat(FRotator(7.f, 0.f, 0.f))).GetNormalized());
		}
		TransitionElapsed = 0.f;
		bAnimatingMesh = !StartMeshTransform.Equals(TargetMeshTransform, KINDA_SMALL_NUMBER);
		SetComponentTickEnabled(bAnimatingMesh);
		if (!bAnimatingMesh) BodyMesh->SetRelativeTransform(TargetMeshTransform);
	}
}

void UAgentRestPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bAnimatingMesh || !BodyMesh.IsValid()) return;
	TransitionElapsed += FMath::Max(0.f, DeltaTime);
	const float Alpha = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(TransitionElapsed / TransitionDuration, 0.f, 1.f));
	BodyMesh->SetRelativeLocationAndRotation(
		FMath::Lerp(StartMeshTransform.GetLocation(), TargetMeshTransform.GetLocation(), Alpha),
		FQuat::Slerp(StartMeshTransform.GetRotation(), TargetMeshTransform.GetRotation(), Alpha));
	BodyMesh->SetRelativeScale3D(FMath::Lerp(StartMeshTransform.GetScale3D(), TargetMeshTransform.GetScale3D(), Alpha));
	if (Alpha >= 1.f)
	{
		BodyMesh->SetRelativeTransform(TargetMeshTransform);
		bAnimatingMesh = false;
		SetComponentTickEnabled(false);
	}
}
