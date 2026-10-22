#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IslandSurfaceSwimMovementComponent.generated.h"

/**
 * Surface swimming for Water plugin bodies, which provide height queries but
 * do not automatically supply the PhysicsVolume required by stock swimming.
 */
UCLASS()
class CAPTIVESKY_2_API UIslandSurfaceSwimMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:
	friend class FIslandOceanTest;
	virtual void PhysSwimming(float DeltaTime, int32 Iterations) override;
};
