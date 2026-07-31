// Copyright Epic Games, Inc. All Rights Reserved.
//
// Console commands for smoke-testing Agent subsystems in isolation, before
// the full Character/AIController/StateTree wiring exists. Not exposed to
// Blueprint on purpose -- these are developer utilities only.

#include "AgentMemoryComponent.h"
#include "HAL/IConsoleManager.h"
#include "UObject/Package.h"

static void AgentTestMemory_Exec(const TArray<FString>& Args)
{
	const FString TestText = Args.Num() > 0
		? FString::Join(Args, TEXT(" "))
		: TEXT("The player waved near the old oak tree.");

	UAgentMemoryComponent* Temp = NewObject<UAgentMemoryComponent>(GetTransientPackage());
	Temp->AgentId = TEXT("ConsoleTest");

	const FAgentMemoryRecord NewRecord = Temp->MakeMemory(EAgentMemoryType::Observation, TestText, 0.6f, { TEXT("test") });
	Temp->AppendMemory(NewRecord);

	const TArray<FAgentMemoryRecord> Relevant = Temp->GetRelevantContext(500, TestText);

	UE_LOG(LogTemp, Log, TEXT("[Agent.TestMemory] store: %s"), *Temp->GetMemoryFilePath());
	UE_LOG(LogTemp, Log, TEXT("[Agent.TestMemory] total records: %d, relevant to query (%d):"), Temp->GetMemoryCount(), Relevant.Num());
	for (const FAgentMemoryRecord& Record : Relevant)
	{
		UE_LOG(LogTemp, Log, TEXT("[Agent.TestMemory]   score=%.2f type=%s text=\"%s\""),
			Record.RelevanceScore, *FAgentMemoryRecord::TypeToString(Record.Type), *Record.Text);
	}
}

static FAutoConsoleCommand CVarAgentTestMemory(
	TEXT("Agent.TestMemory"),
	TEXT("Appends a test memory record to the 'ConsoleTest' agent store and prints GetRelevantContext results. Usage: Agent.TestMemory <text...>"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&AgentTestMemory_Exec)
);
