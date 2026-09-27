#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslandLightning.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USoundWaveProcedural;

/**
 * One lightning strike during a storm: a brief flickering flash high over the strike point, a jagged
 * bolt to the ground when it is near enough to see, and thunder that arrives after the time sound
 * takes to travel from the strike to the listener (a procedurally generated rumble, no audio assets).
 * Destroys itself once the thunder has faded.
 */
UCLASS()
class CAPTIVESKY_2_API AIslandLightning : public AActor
{
	GENERATED_BODY()

public:
	AIslandLightning();

	/** Speed of sound used for thunder delay, cm/s. */
	static constexpr float SoundSpeed = 34300.f;
	static constexpr float FlashSeconds = 0.45f;
	static constexpr float BoltVisibleWithin = 400000.f;

	/** Ground is where the bolt lands; Listener is where thunder is heard from (the viewer). */
	void Strike(const FVector& Ground, const FVector& Listener, int32 Seed);

	/** Flash brightness now, 0..1: three quick pulses, then dark. */
	static float FlashAt(float SecondsSinceStrike);
	float GetFlash() const { return FlashAt(Elapsed); }
	float GetThunderDelay() const { return ThunderDelay; }
	bool HasBolt() const;
	bool HasThundered() const { return bThundered; }

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Island|Lightning")
	TObjectPtr<UPointLightComponent> Flash;

	UPROPERTY(VisibleAnywhere, Category = "Island|Lightning")
	TObjectPtr<UInstancedStaticMeshComponent> Bolt;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BoltMaterial;

	UPROPERTY(VisibleAnywhere, Transient, Category = "Island|Lightning")
	TObjectPtr<UAudioComponent> Thunder;

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ThunderWave;

	float Elapsed = 0.f;
	float ThunderDelay = 0.f;
	float ThunderVolume = 0.f;
	float Lifetime = 8.f;
	int32 StrikeSeed = 0;
	bool bThundered = false;

	void StartThunder();
};
