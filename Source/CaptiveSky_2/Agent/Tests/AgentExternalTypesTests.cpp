// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "AgentExternalTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentExternalTypesTest, "CaptiveSky2.Agent.ExternalTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentExternalTypesTest::RunTest(const FString& Parameters)
{
	const FString TurnJson = TEXT("{\"schema_version\":1,\"turn_id\":\"turn-1\",\"agent_id\":\"Agent_Raven_01\","
		"\"source\":\"discord\",\"conversation_id\":\"discord:dm:10\",\"message_id\":\"20\","
		"\"sender\":{\"id\":\"42\",\"display_name\":\"Visitor\"},\"text\":\"Hello\","
		"\"received_at\":\"2026-08-05T12:00:00Z\",\"metadata\":{}}");
	FAgentExternalUtterance Turn;
	if (TestTrue(TEXT("A gateway turn parses"), FAgentExternalUtterance::FromJson(TurnJson, Turn)))
	{
		TestEqual(TEXT("Turn id parses"), Turn.TurnId, FString(TEXT("turn-1")));
		TestEqual(TEXT("Agent id parses"), Turn.AgentId, FString(TEXT("Agent_Raven_01")));
		TestEqual(TEXT("Source parses"), Turn.Source, FString(TEXT("discord")));
		TestEqual(TEXT("Participant id parses"), Turn.ParticipantId, FString(TEXT("42")));
		TestEqual(TEXT("Participant name parses"), Turn.ParticipantName, FString(TEXT("Visitor")));
		TestEqual(TEXT("Text parses"), Turn.Text, FString(TEXT("Hello")));
	}

	FAgentExternalResponse Response;
	Response.TurnId = TEXT("turn-1");
	Response.AgentId = TEXT("Agent_Raven_01");
	Response.Status = TEXT("completed");
	Response.Speech = TEXT("Hello from the Island.");
	Response.CompletedAt = FDateTime(2026, 8, 5, 12, 0, 1);
	const FString ResponseJson = Response.ToJson();
	TestTrue(TEXT("Response contains the turn id"), ResponseJson.Contains(TEXT("\"turn_id\":\"turn-1\"")));
	TestTrue(TEXT("Response contains speech"), ResponseJson.Contains(TEXT("Hello from the Island.")));

	FAgentExternalUtterance Invalid;
	TestFalse(TEXT("Unknown schema versions are rejected"),
		FAgentExternalUtterance::FromJson(TEXT("{\"schema_version\":2}"), Invalid));
	return true;
}
