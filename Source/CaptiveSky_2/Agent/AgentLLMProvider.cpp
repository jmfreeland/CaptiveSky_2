// Copyright Epic Games, Inc. All Rights Reserved.

#include "AgentLLMProvider.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GenericPlatform/GenericPlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentLLM, Log, All);

namespace AgentLLMProviderPrivate
{
	FString GetApiKey()
	{
		const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
		if (!Settings || Settings->ApiKeyEnvVar.IsEmpty())
		{
			return FString();
		}
		return FPlatformMisc::GetEnvironmentVariable(*Settings->ApiKeyEnvVar);
	}

	FString WriteJson(const TSharedRef<FJsonObject>& Root)
	{
		FString Out;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Root, Writer);
		return Out;
	}

	// Extracts {"error":{"message": "..."}} if present, otherwise falls back to the raw body (truncated).
	FString ExtractErrorMessage(const FString& Body)
	{
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Body);
		if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
		{
			const TSharedPtr<FJsonObject>* ErrorObj = nullptr;
			if (Root->TryGetObjectField(TEXT("error"), ErrorObj) && ErrorObj && ErrorObj->IsValid())
			{
				FString Message;
				if ((*ErrorObj)->TryGetStringField(TEXT("message"), Message))
				{
					return Message;
				}
			}
		}
		return Body.Left(500);
	}
}

void FAnthropicLLMProvider::SendRequest(const FAgentLLMRequest& Request, FOnAgentLLMComplete OnComplete)
{
	using namespace AgentLLMProviderPrivate;

	const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
	const FString ApiKey = GetApiKey();
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogAgentLLM, Error, TEXT("Anthropic provider: no API key found in env var '%s'"), Settings ? *Settings->ApiKeyEnvVar : TEXT("<unset>"));
		FAgentLLMResult Result;
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("No API key found in environment variable '%s'."), Settings ? *Settings->ApiKeyEnvVar : TEXT("<unset>"));
		OnComplete.ExecuteIfBound(Result);
		return;
	}

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Request.ModelOverride.IsEmpty() ? Settings->Model : Request.ModelOverride);
	Root->SetNumberField(TEXT("max_tokens"), Request.MaxTokens > 0 ? Request.MaxTokens : Settings->DefaultMaxTokens);
	if (!Request.SystemPrompt.IsEmpty())
	{
		Root->SetStringField(TEXT("system"), Request.SystemPrompt);
	}

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const FAgentLLMMessage& Message : Request.Messages)
	{
		TSharedRef<FJsonObject> MessageObj = MakeShared<FJsonObject>();
		MessageObj->SetStringField(TEXT("role"), Message.Role);

		TArray<TSharedPtr<FJsonValue>> ContentBlocks;
		if (!Message.Text.IsEmpty())
		{
			TSharedRef<FJsonObject> TextBlock = MakeShared<FJsonObject>();
			TextBlock->SetStringField(TEXT("type"), TEXT("text"));
			TextBlock->SetStringField(TEXT("text"), Message.Text);
			ContentBlocks.Add(MakeShared<FJsonValueObject>(TextBlock));
		}
		if (!Message.ImageBase64PNG.IsEmpty())
		{
			TSharedRef<FJsonObject> ImageBlock = MakeShared<FJsonObject>();
			ImageBlock->SetStringField(TEXT("type"), TEXT("image"));
			TSharedRef<FJsonObject> Source = MakeShared<FJsonObject>();
			Source->SetStringField(TEXT("type"), TEXT("base64"));
			Source->SetStringField(TEXT("media_type"), TEXT("image/png"));
			Source->SetStringField(TEXT("data"), Message.ImageBase64PNG);
			ImageBlock->SetObjectField(TEXT("source"), Source);
			ContentBlocks.Add(MakeShared<FJsonValueObject>(ImageBlock));
		}
		MessageObj->SetArrayField(TEXT("content"), ContentBlocks);
		MessagesArray.Add(MakeShared<FJsonValueObject>(MessageObj));
	}
	Root->SetArrayField(TEXT("messages"), MessagesArray);

	const FString Endpoint = Settings->EndpointOverride.IsEmpty() ? TEXT("https://api.anthropic.com/v1/messages") : Settings->EndpointOverride;

	FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("content-type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("x-api-key"), ApiKey);
	HttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
	HttpRequest->SetTimeout(Settings->RequestTimeoutSeconds);
	HttpRequest->SetContentAsString(WriteJson(Root));

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FAgentLLMResult Result;
			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				Result.bSuccess = false;
				Result.ErrorMessage = TEXT("Failed to connect to Anthropic API.");
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			const FString Body = Response->GetContentAsString();
			if (Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), Response->GetResponseCode(), *ExtractErrorMessage(Body));
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			TSharedPtr<FJsonObject> RootObj;
			TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Body);
			if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
			{
				Result.bSuccess = false;
				Result.ErrorMessage = TEXT("Anthropic response was not valid JSON.");
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			FString CombinedText;
			const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
			if (RootObj->TryGetArrayField(TEXT("content"), ContentArray) && ContentArray)
			{
				for (const TSharedPtr<FJsonValue>& Block : *ContentArray)
				{
					const TSharedPtr<FJsonObject>* BlockObj = nullptr;
					if (Block.IsValid() && Block->TryGetObject(BlockObj) && BlockObj && BlockObj->IsValid())
					{
						FString BlockType;
						(*BlockObj)->TryGetStringField(TEXT("type"), BlockType);
						if (BlockType == TEXT("text"))
						{
							FString Text;
							if ((*BlockObj)->TryGetStringField(TEXT("text"), Text))
							{
								CombinedText += Text;
							}
						}
					}
				}
			}

			Result.bSuccess = true;
			Result.ResponseText = CombinedText;
			OnComplete.ExecuteIfBound(Result);
		});

	HttpRequest->ProcessRequest();
}

void FOpenAICompatibleLLMProvider::SendRequest(const FAgentLLMRequest& Request, FOnAgentLLMComplete OnComplete)
{
	using namespace AgentLLMProviderPrivate;

	const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
	const FString ApiKey = GetApiKey();

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Request.ModelOverride.IsEmpty() ? Settings->Model : Request.ModelOverride);
	const TCHAR* MaxTokensField = Settings->bUseMaxCompletionTokensParam ? TEXT("max_completion_tokens") : TEXT("max_tokens");
	Root->SetNumberField(MaxTokensField, Request.MaxTokens > 0 ? Request.MaxTokens : Settings->DefaultMaxTokens);
	Root->SetNumberField(TEXT("temperature"), Settings->Temperature);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;

	if (!Request.SystemPrompt.IsEmpty())
	{
		TSharedRef<FJsonObject> SystemMessage = MakeShared<FJsonObject>();
		SystemMessage->SetStringField(TEXT("role"), TEXT("system"));
		SystemMessage->SetStringField(TEXT("content"), Request.SystemPrompt);
		MessagesArray.Add(MakeShared<FJsonValueObject>(SystemMessage));
	}

	for (const FAgentLLMMessage& Message : Request.Messages)
	{
		TSharedRef<FJsonObject> MessageObj = MakeShared<FJsonObject>();
		MessageObj->SetStringField(TEXT("role"), Message.Role);

		if (Message.ImageBase64PNG.IsEmpty())
		{
			// Plain string content when there's no image -- simplest valid shape.
			MessageObj->SetStringField(TEXT("content"), Message.Text);
		}
		else
		{
			TArray<TSharedPtr<FJsonValue>> ContentBlocks;
			if (!Message.Text.IsEmpty())
			{
				TSharedRef<FJsonObject> TextBlock = MakeShared<FJsonObject>();
				TextBlock->SetStringField(TEXT("type"), TEXT("text"));
				TextBlock->SetStringField(TEXT("text"), Message.Text);
				ContentBlocks.Add(MakeShared<FJsonValueObject>(TextBlock));
			}
			TSharedRef<FJsonObject> ImageBlock = MakeShared<FJsonObject>();
			ImageBlock->SetStringField(TEXT("type"), TEXT("image_url"));
			TSharedRef<FJsonObject> ImageUrl = MakeShared<FJsonObject>();
			ImageUrl->SetStringField(TEXT("url"), FString::Printf(TEXT("data:image/png;base64,%s"), *Message.ImageBase64PNG));
			ImageBlock->SetObjectField(TEXT("image_url"), ImageUrl);
			ContentBlocks.Add(MakeShared<FJsonValueObject>(ImageBlock));

			MessageObj->SetArrayField(TEXT("content"), ContentBlocks);
		}

		MessagesArray.Add(MakeShared<FJsonValueObject>(MessageObj));
	}
	Root->SetArrayField(TEXT("messages"), MessagesArray);

	const FString Endpoint = Settings->EndpointOverride.IsEmpty() ? TEXT("https://api.openai.com/v1/chat/completions") : Settings->EndpointOverride;

	FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("content-type"), TEXT("application/json"));
	if (!ApiKey.IsEmpty())
	{
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	}
	HttpRequest->SetTimeout(Settings->RequestTimeoutSeconds);
	HttpRequest->SetContentAsString(WriteJson(Root));

	HttpRequest->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			FAgentLLMResult Result;
			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				Result.bSuccess = false;
				Result.ErrorMessage = TEXT("Failed to connect to OpenAI-compatible endpoint.");
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			const FString Body = Response->GetContentAsString();
			if (Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), Response->GetResponseCode(), *ExtractErrorMessage(Body));
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			TSharedPtr<FJsonObject> RootObj;
			TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Body);
			if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
			{
				Result.bSuccess = false;
				Result.ErrorMessage = TEXT("Response was not valid JSON.");
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			FString ResponseText;
			const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
			if (RootObj->TryGetArrayField(TEXT("choices"), Choices) && Choices && Choices->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* FirstChoice = nullptr;
				if ((*Choices)[0].IsValid() && (*Choices)[0]->TryGetObject(FirstChoice) && FirstChoice && FirstChoice->IsValid())
				{
					const TSharedPtr<FJsonObject>* MessageObj = nullptr;
					if ((*FirstChoice)->TryGetObjectField(TEXT("message"), MessageObj) && MessageObj && MessageObj->IsValid())
					{
						(*MessageObj)->TryGetStringField(TEXT("content"), ResponseText);
					}
				}
			}

			Result.bSuccess = true;
			Result.ResponseText = ResponseText;
			OnComplete.ExecuteIfBound(Result);
		});

	HttpRequest->ProcessRequest();
}

TUniquePtr<IAgentLLMProvider> CreateAgentLLMProvider()
{
	const UAgentLLMSettings* Settings = GetDefault<UAgentLLMSettings>();
	const EAgentLLMProviderType ProviderType = Settings ? Settings->Provider : EAgentLLMProviderType::Anthropic;

	switch (ProviderType)
	{
	case EAgentLLMProviderType::OpenAICompatible:
		return MakeUnique<FOpenAICompatibleLLMProvider>();
	case EAgentLLMProviderType::Anthropic:
	default:
		return MakeUnique<FAnthropicLLMProvider>();
	}
}
