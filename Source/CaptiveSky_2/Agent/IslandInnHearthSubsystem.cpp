#include "IslandInnHearthSubsystem.h"

#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UIslandInnHearthSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIslandInnHearthSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	for (TActorIterator<APointLight> It(&InWorld); It; ++It)
	{
		if (!It->ActorHasTag(TEXT("InnHearthLight"))) continue;
		HearthLight = It->PointLightComponent;
		if (HearthLight.IsValid())
		{
			BaseIntensity = FMath::Max(0.f, HearthLight->Intensity);
			SetBanked();
			return;
		}
	}
}

bool UIslandInnHearthSubsystem::TendHearth(FString& OutFact)
{
	OutFact.Reset();
	if (!HearthLight.IsValid())
	{
		OutFact = TEXT("You reached the inn hearth, but its tagged light is not present. Nothing changed.");
		return false;
	}

	if (bLit)
	{
		SetBanked();
		OutFact = TEXT("You banked the hearth. Its light is out; it will stay that way until someone tends it again. No permanent change was made.");
		return true;
	}

	bLit = true;
	SecondsRemaining = BurnDurationSeconds;
	FlickerTime = 0.f;
	ApplyFlicker();
	OutFact = TEXT("You kindled the inn hearth's warm light. It will flicker gently for about five minutes of play (roughly three Island hours at the current clock rate), then bank itself unless someone tends it again. This is light only, not simulated heat or a visible flame; no persistent hearth change was saved.");
	return true;
}

FString UIslandInnHearthSubsystem::DescribeHearth() const
{
	if (bLit && HearthLight.IsValid())
	{
		return FString::Printf(TEXT(" The inn hearth is lit; its tagged light is flickering, with about %.0f seconds of play left before it banks itself. You may move_to/interact with target InnHearth to bank it early, or leave it be."), SecondsRemaining);
	}
	return TEXT(" The inn hearth is dark and banked. You may move_to/interact with target InnHearth if you choose to kindle its light; no one has to tend it.");
}

void UIslandInnHearthSubsystem::Tick(float DeltaTime)
{
	if (!bLit || !HearthLight.IsValid())
	{
		if (bLit) SetBanked();
		return;
	}
	SecondsRemaining = FMath::Max(0.f, SecondsRemaining - FMath::Max(0.f, DeltaTime));
	if (SecondsRemaining <= 0.f)
	{
		SetBanked();
		return;
	}
	FlickerTime += FMath::Max(0.f, DeltaTime);
	ApplyFlicker();
}

TStatId UIslandInnHearthSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UIslandInnHearthSubsystem, STATGROUP_Tickables);
}

bool UIslandInnHearthSubsystem::IsTickable() const
{
	return bLit;
}

void UIslandInnHearthSubsystem::Deinitialize()
{
	SetBanked();
	HearthLight.Reset();
	Super::Deinitialize();
}

void UIslandInnHearthSubsystem::SetBanked()
{
	bLit = false;
	SecondsRemaining = 0.f;
	FlickerTime = 0.f;
	if (HearthLight.IsValid()) HearthLight->SetIntensity(0.f);
}

void UIslandInnHearthSubsystem::ApplyFlicker()
{
	if (!HearthLight.IsValid()) return;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(FlickerTime * 17.f + FMath::Sin(FlickerTime * 5.3f));
	HearthLight->SetIntensity(BaseIntensity * FMath::Lerp(0.78f, 1.08f, Pulse));
}
