#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "IslandSpectator.generated.h"

class ACameraActor;
class APlayerController;
class AAutonomousAgentCharacter;
class STextBlock;
class SWidget;

UENUM()
enum class EIslandShotKind : uint8
{
	/** A slow drift through one of the journey viewpoints. */
	Establishing,
	/** Framing a resident while they speak (a two-shot when another resident is close). */
	Speech,
	/** Looking at something a resident has just made or changed. */
	Made
};

USTRUCT()
struct FIslandShot
{
	GENERATED_BODY()

	EIslandShotKind Kind = EIslandShotKind::Establishing;
	FString Title;
	FVector From = FVector::ZeroVector;
	FVector To = FVector::ZeroVector;   // the camera drifts from From to To over the shot
	FVector LookAt = FVector::ZeroVector;
	float FieldOfView = 60.f;
	float Duration = 20.f;
	/** Where the (hidden) player pawn waits during the shot, so subtitles and weather follow the subject. */
	FVector Focus = FVector::ZeroVector;
};

/**
 * Spectator mode for an unattended screen. Takes over the local player's view with a camera that
 * drifts slowly through the journey viewpoints (Config/IslandViewpoints.json), cuts to residents when
 * they speak, and cuts to anything a resident makes or changes in the lasting world state. The player
 * pawn is hidden, without collision or input, and travels with the shot's subject so that ambient
 * subtitles, rain, and wind sampling follow what is on screen. Ending spectator mode restores it all.
 */
UCLASS()
class CAPTIVESKY_2_API AIslandSpectatorDirector : public AActor
{
	GENERATED_BODY()

public:
	AIslandSpectatorDirector();

	static constexpr float MinimumHoldSeconds = 4.f;
	static constexpr float EstablishingSeconds = 24.f;

	/** Tests point this at their own viewpoint file; empty uses Config/IslandViewpoints.json. */
	FString ViewpointFileOverride;

	void BeginSpectating(APlayerController* Player);
	void EndSpectating();

	/** Cut to a speaking resident (any actor with a location; residents arrive through the speech bus). */
	void FocusOnSpeaker(AActor* Speaker, const FString& Speech);
	/** Cut to something that was just made or changed at Location. */
	void FocusOnChange(const FVector& Location, const FString& Title);

	const FIslandShot& GetCurrentShot() const { return Current; }
	ACameraActor* GetCamera() const { return Camera; }
	int32 GetEstablishingCount() const { return Establishing.Num(); }
	/** Camera placement for a subject at Head seen from Distance, avoiding anything that would block the view. */
	static FVector FrameSubject(UWorld* World, const FVector& Head, const FVector& Facing, float Distance, const TArray<const AActor*>& Ignore);
	/** Resolves -SpectatorScreenshotDir; an empty override preserves the historical output folder. */
	static FString ResolveScreenshotDirectory(const FString& ProjectSavedDir, const FString& Override);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> Camera;

	TWeakObjectPtr<APlayerController> Viewer;
	TWeakObjectPtr<APawn> HiddenPawn;
	FVector PawnHome = FVector::ZeroVector;
	TArray<FIslandShot> Establishing;
	FIslandShot Current;
	float Elapsed = 0.f;
	int32 NextEstablishing = 0;
	int32 NextInn = 0;
	bool bInnTurn = false;
	float SincePoll = 0.f;
	int32 ShotIndex = 0;
	bool bShotCaptured = false;
	FString WorldStateSignature;
	TSharedPtr<STextBlock> CaptionText;
	TSharedPtr<SWidget> CaptionWidget;

	void LoadEstablishingShots();
	void StartShot(const FIslandShot& Shot);
	void NextEstablishingShot();
	void PollLastingChanges();
	friend class FIslandSpectatorTest;
	FString DescribeClock() const;

	UFUNCTION()
	void HandleSpeech(AAutonomousAgentCharacter* Speaker, const FString& Speech);
};

/** Starts spectator mode with -Spectator on the command line, or toggles it with Island.Spectate. */
UCLASS()
class CAPTIVESKY_2_API UIslandSpectatorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool IsSpectating() const { return Director.IsValid(); }
	AIslandSpectatorDirector* GetDirector() const { return Director.Get(); }
	void StartSpectating();
	void StopSpectating();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	TWeakObjectPtr<AIslandSpectatorDirector> Director;
	FTimerHandle StartRetry;
};
