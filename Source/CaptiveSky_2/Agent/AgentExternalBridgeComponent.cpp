// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentExternalBridgeComponent.h"
#include "AgentBrainComponent.h"
#include "AgentMemoryComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentExternalBridge, Log, All);

UAgentExternalBridgeComponent::UAgentExternalBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAgentExternalBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	SessionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	if (UAgentBrainComponent* Brain = GetBrain())
	{
		Brain->OnDecisionReady.AddDynamic(this, &UAgentExternalBridgeComponent::HandleDecisionReady);
	}
	ClaimOrRefreshHeartbeat();
}

void UAgentExternalBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAgentBrainComponent* Brain = GetBrain())
	{
		Brain->OnDecisionReady.RemoveDynamic(this, &UAgentExternalBridgeComponent::HandleDecisionReady);
	}
	PreserveCurrentTurnForNextEmbodiment();
	RemoveOwnHeartbeat();
	Super::EndPlay(EndPlayReason);
}

void UAgentExternalBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Now = FPlatformTime::Seconds();
	if (Now >= NextHeartbeatAt)
	{
		ClaimOrRefreshHeartbeat();
		NextHeartbeatAt = Now + HeartbeatIntervalSeconds;
	}
	if (!bProcessingTurn && Now >= NextInboxPollAt)
	{
		TryProcessNextTurn();
		NextInboxPollAt = Now + InboxPollIntervalSeconds;
	}
}

UAgentMemoryComponent* UAgentExternalBridgeComponent::GetMemory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UAgentMemoryComponent>() : nullptr;
}

UAgentBrainComponent* UAgentExternalBridgeComponent::GetBrain() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UAgentBrainComponent>() : nullptr;
}

FString UAgentExternalBridgeComponent::GetGatewayDirectory() const
{
	if (const UAgentMemoryComponent* Memory = GetMemory())
	{
		return Memory->GetAgentDirectory() / TEXT(".gateway");
	}
	return FString();
}

void UAgentExternalBridgeComponent::ClaimOrRefreshHeartbeat()
{
	const UAgentMemoryComponent* Memory = GetMemory();
	const FString GatewayDirectory = GetGatewayDirectory();
	if (!Memory || GatewayDirectory.IsEmpty())
	{
		return;
	}

	IFileManager::Get().MakeDirectory(*GatewayDirectory, true);
	const FString PresencePath = GatewayDirectory / TEXT("embodiment.json");
	FString ExistingJson;
	if (FFileHelper::LoadFileToString(ExistingJson, *PresencePath))
	{
		TSharedPtr<FJsonObject> ExistingRoot;
		const TSharedRef<TJsonReader<TCHAR>> ExistingReader = TJsonReaderFactory<TCHAR>::Create(ExistingJson);
		FString ExistingSession;
		FString ExistingExpiryString;
		FDateTime ExistingExpiry;
		if (FJsonSerializer::Deserialize(ExistingReader, ExistingRoot) && ExistingRoot.IsValid() &&
			ExistingRoot->TryGetStringField(TEXT("session_id"), ExistingSession) &&
			ExistingRoot->TryGetStringField(TEXT("expires_at"), ExistingExpiryString) &&
			FDateTime::ParseIso8601(*ExistingExpiryString, ExistingExpiry) &&
			ExistingExpiry > FDateTime::UtcNow() && ExistingSession != SessionId)
		{
			bOwnsEmbodimentAuthority = false;
			return;
		}
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schema_version"), 1);
	Root->SetStringField(TEXT("agent_id"), Memory->GetResolvedAgentId());
	Root->SetStringField(TEXT("session_id"), SessionId);
	Root->SetStringField(TEXT("expires_at"), (FDateTime::UtcNow() + FTimespan::FromSeconds(HeartbeatLifetimeSeconds)).ToIso8601());

	FString Json;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);
	const bool bPreviouslyOwned = bOwnsEmbodimentAuthority;
	WriteFileAtomically(PresencePath, Json);
	bOwnsEmbodimentAuthority = PresenceBelongsToThisSession();
	if (bOwnsEmbodimentAuthority && !bPreviouslyOwned)
	{
		RecoverOrphanedTurns();
	}
}

bool UAgentExternalBridgeComponent::PresenceBelongsToThisSession() const
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *(GetGatewayDirectory() / TEXT("embodiment.json"))))
	{
		return false;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	FString ExistingSession;
	return FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid() &&
		Root->TryGetStringField(TEXT("session_id"), ExistingSession) && ExistingSession == SessionId;
}

void UAgentExternalBridgeComponent::RemoveOwnHeartbeat()
{
	const FString PresencePath = GetGatewayDirectory() / TEXT("embodiment.json");
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *PresencePath))
	{
		return;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	FString ExistingSession;
	if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid() &&
		Root->TryGetStringField(TEXT("session_id"), ExistingSession) && ExistingSession == SessionId)
	{
		IFileManager::Get().Delete(*PresencePath, false, true);
	}
}

void UAgentExternalBridgeComponent::RecoverOrphanedTurns()
{
	const FString GatewayDirectory = GetGatewayDirectory();
	const FString ProcessingRoot = GatewayDirectory / TEXT("processing");
	const FString InboxDirectory = GatewayDirectory / TEXT("inbox");
	const FString OutboxDirectory = GatewayDirectory / TEXT("outbox");
	IFileManager::Get().MakeDirectory(*InboxDirectory, true);

	TArray<FString> SessionDirectories;
	IFileManager::Get().FindFiles(SessionDirectories, *(ProcessingRoot / TEXT("*")), false, true);
	for (const FString& OldSession : SessionDirectories)
	{
		if (OldSession == SessionId)
		{
			continue;
		}
		const FString OldSessionDirectory = ProcessingRoot / OldSession;
		TArray<FString> TurnFiles;
		IFileManager::Get().FindFiles(TurnFiles, *(OldSessionDirectory / TEXT("*.json")), true, false);
		for (const FString& FileName : TurnFiles)
		{
			const FString OldPath = OldSessionDirectory / FileName;
			if (FPaths::FileExists(OutboxDirectory / FileName))
			{
				IFileManager::Get().Delete(*OldPath, false, true);
				continue;
			}
			IFileManager::Get().Move(*(InboxDirectory / FileName), *OldPath, false, true, false, true);
		}
	}
}

bool UAgentExternalBridgeComponent::IsHeadlessTurnActive() const
{
	const FString MarkerPath = GetGatewayDirectory() / TEXT("headless-turn.json");
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *MarkerPath))
	{
		return false;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	FString ExpiresAtString;
	FDateTime ExpiresAt;
	return FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid() &&
		Root->TryGetStringField(TEXT("expires_at"), ExpiresAtString) &&
		FDateTime::ParseIso8601(*ExpiresAtString, ExpiresAt) && ExpiresAt > FDateTime::UtcNow();
}

void UAgentExternalBridgeComponent::TryProcessNextTurn()
{
	UAgentBrainComponent* Brain = GetBrain();
	UAgentMemoryComponent* Memory = GetMemory();
	if (!bOwnsEmbodimentAuthority || !PresenceBelongsToThisSession() ||
		!Brain || !Memory || Brain->bRequestInFlight || IsHeadlessTurnActive())
	{
		return;
	}

	const FString GatewayDirectory = GetGatewayDirectory();
	const FString InboxDirectory = GatewayDirectory / TEXT("inbox");
	TArray<FString> TurnFiles;
	IFileManager::Get().FindFiles(TurnFiles, *(InboxDirectory / TEXT("*.json")), true, false);
	TurnFiles.Sort();
	if (TurnFiles.IsEmpty())
	{
		return;
	}

	const FString ProcessingDirectory = GatewayDirectory / TEXT("processing") / SessionId;
	IFileManager::Get().MakeDirectory(*ProcessingDirectory, true);
	for (const FString& FileName : TurnFiles)
	{
		const FString InboxPath = InboxDirectory / FileName;
		const FString ProcessingPath = ProcessingDirectory / FileName;
		if (!IFileManager::Get().Move(*ProcessingPath, *InboxPath, false, true, false, true))
		{
			continue;
		}

		FString Json;
		FAgentExternalUtterance Turn;
		if (!FFileHelper::LoadFileToString(Json, *ProcessingPath) || !FAgentExternalUtterance::FromJson(Json, Turn) ||
			Turn.AgentId != Memory->GetResolvedAgentId())
		{
			UE_LOG(LogAgentExternalBridge, Warning, TEXT("Quarantining invalid external turn: %s"), *ProcessingPath);
			QuarantineInvalidTurn(ProcessingPath);
			continue;
		}

		CurrentTurn = MoveTemp(Turn);
		CurrentProcessingPath = ProcessingPath;
		bProcessingTurn = true;
		Brain->RequestExternalDecision(CurrentTurn);
		return;
	}
}

void UAgentExternalBridgeComponent::HandleDecisionReady(const FAgentDecision& Decision)
{
	if (!bProcessingTurn)
	{
		return;
	}

	FAgentExternalResponse Response;
	Response.TurnId = CurrentTurn.TurnId;
	Response.AgentId = CurrentTurn.AgentId;
	Response.Status = Decision.bValid ? TEXT("completed") : TEXT("failed");
	Response.Speech = Decision.Speech;
	Response.Error = Decision.bValid ? FString() : TEXT("The embodied brain could not complete the turn.");
	Response.CompletedAt = FDateTime::UtcNow();

	const FString OutboxDirectory = GetGatewayDirectory() / TEXT("outbox");
	IFileManager::Get().MakeDirectory(*OutboxDirectory, true);
	if (!WriteFileAtomically(OutboxDirectory / (CurrentTurn.TurnId + TEXT(".json")), Response.ToJson()))
	{
		UE_LOG(LogAgentExternalBridge, Error, TEXT("Failed to write external response for turn %s"), *CurrentTurn.TurnId);
		return;
	}

	IFileManager::Get().Delete(*CurrentProcessingPath, false, true);
	bProcessingTurn = false;
	CurrentTurn = FAgentExternalUtterance();
	CurrentProcessingPath.Reset();
}

void UAgentExternalBridgeComponent::PreserveCurrentTurnForNextEmbodiment()
{
	if (!bProcessingTurn || CurrentProcessingPath.IsEmpty())
	{
		return;
	}
	const FString InboxDirectory = GetGatewayDirectory() / TEXT("inbox");
	IFileManager::Get().MakeDirectory(*InboxDirectory, true);
	IFileManager::Get().Move(*(InboxDirectory / FPaths::GetCleanFilename(CurrentProcessingPath)), *CurrentProcessingPath, false, true, false, true);
	bProcessingTurn = false;
}

void UAgentExternalBridgeComponent::QuarantineInvalidTurn(const FString& ProcessingPath) const
{
	const FString FailedDirectory = GetGatewayDirectory() / TEXT("failed");
	IFileManager::Get().MakeDirectory(*FailedDirectory, true);
	IFileManager::Get().Move(*(FailedDirectory / FPaths::GetCleanFilename(ProcessingPath)), *ProcessingPath, true, true, false, true);
}

bool UAgentExternalBridgeComponent::WriteFileAtomically(const FString& Destination, const FString& Contents)
{
	const FString Temporary = Destination + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Contents, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return false;
	}
	if (!IFileManager::Get().Move(*Destination, *Temporary, true, true, false, true))
	{
		IFileManager::Get().Delete(*Temporary, false, true);
		return false;
	}
	return true;
}
