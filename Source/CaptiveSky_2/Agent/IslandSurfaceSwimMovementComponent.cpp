#include "IslandSurfaceSwimMovementComponent.h"

void UIslandSurfaceSwimMovementComponent::PhysSwimming(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME || !HasValidData())
	{
		return;
	}

	// WaterBody actors answer wave/surface queries, but do not behave like a
	// PhysicsVolume. Reuse CharacterMovement's swept, gravity-free horizontal
	// flight solver while retaining MOVE_Swimming for gameplay and animation.
	Super::PhysFlying(DeltaTime, Iterations);

	// Surface height is corrected by ACaptiveSky_2Character after this movement
	// tick. Do not let accumulated input or root motion push the capsule below it.
	Velocity.Z = 0.f;
}
