// Copyright Epic Games, Inc. All Rights Reserved.
//
// Exercises the configured IAgentLLMProvider against the real API. If the
// configured environment variable (see UAgentLLMSettings::ApiKeyEnvVar) is
// not set, the test logs that it's skipping and passes -- this test only
// asserts on network/parsing behavior when credentials are actually present.

#include "Misc/AutomationTest.h"
#include "AgentLLMProvider.h"
#include "AgentLLMSettings.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/PlatformTime.h"

namespace AgentLLMProviderTestPrivate
{
	struct FSharedState
	{
		bool bComplete = false;
		FAgentLLMResult Result;
	};

	class FValidateResult : public IAutomationLatentCommand
	{
	public:
		FValidateResult(FAutomationTestBase* InTest, TSharedPtr<FSharedState> InState)
			: Test(InTest), State(InState), StartTime(FPlatformTime::Seconds())
		{
		}

		virtual bool Update() override
		{
			if (State->bComplete)
			{
				if (State->Result.bSuccess)
				{
					Test->AddInfo(FString::Printf(TEXT("LLM responded: %s"), *State->Result.ResponseText));
					if (State->Result.ResponseText.IsEmpty())
					{
						Test->AddError(TEXT("LLM call reported success but returned empty text."));
					}
				}
				else
				{
					Test->AddError(FString::Printf(TEXT("LLM call failed: %s"), *State->Result.ErrorMessage));
				}
				return true;
			}

			if (FPlatformTime::Seconds() - StartTime > 25.0)
			{
				Test->AddError(TEXT("Timed out waiting for LLM response after 25 seconds."));
				return true;
			}

			return false;
		}

	private:
		FAutomationTestBase* Test;
		TSharedPtr<FSharedState> State;
		double StartTime;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentLLMProviderTest, "CaptiveSky2.Agent.LLMProviderLive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentLLMProviderTest::RunTest(const FString& Parameters)
{
	using namespace AgentLLMProviderTestPrivate;

	const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
	const FString ApiKey = (Settings && !Settings->ApiKeyEnvVar.IsEmpty())
		? FPlatformMisc::GetEnvironmentVariable(*Settings->ApiKeyEnvVar)
		: FString();

	if (ApiKey.IsEmpty())
	{
		AddInfo(FString::Printf(
			TEXT("Skipping live call: environment variable '%s' is not set. Set it and re-run this test to verify connectivity."),
			Settings ? *Settings->ApiKeyEnvVar : TEXT("<unset>")));
		return true;
	}

	TSharedPtr<FSharedState> State = MakeShared<FSharedState>();

	FAgentLLMRequest Request;
	Request.SystemPrompt = TEXT("You are a terse connectivity check. Reply with exactly one word: PONG");
	FAgentLLMMessage Message;
	Message.Role = TEXT("user");
	Message.Text = TEXT("ping");
	Request.Messages.Add(Message);
	Request.MaxTokens = 16;

	TUniquePtr<IAgentLLMProvider> Provider = CreateAgentLLMProvider();
	Provider->SendRequest(Request, FOnAgentLLMComplete::CreateLambda([State](const FAgentLLMResult& Result)
	{
		State->Result = Result;
		State->bComplete = true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FValidateResult(this, State));
	return true;
}
