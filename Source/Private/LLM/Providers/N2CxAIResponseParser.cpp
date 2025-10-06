// Copyright (c) 2025 Nick McClure (Protospatial). All Rights Reserved.

#include "LLM/Providers/N2CxAIResponseParser.h"
#include "Utils/N2CLogger.h"
#include "Serialization/JsonSerializer.h"

bool UN2CxAIResponseParser::ParseLLMResponse(
    const FString& InJson,
    FN2CTranslationResponse& OutResponse)
{
    // Parse JSON string
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJson);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        FN2CLogger::Get().LogError(
            FString::Printf(TEXT("Failed to parse xAI response JSON: %s"), *InJson),
            TEXT("xAIResponseParser")
        );
        return false;
    }

    // Check for error response (xAI aligns with OpenAI shape)
    FString ErrorMessage;
    if (JsonObject->HasField(TEXT("error")))
    {
        if (HandleCommonErrorResponse(JsonObject, TEXT("error"), ErrorMessage))
        {
            FN2CLogger::Get().LogError(ErrorMessage, TEXT("xAIResponseParser"));
        }
        return false;
    }

    // Extract message content from OpenAI-compatible format
    FString MessageContent;
    if (!ExtractStandardMessageContent(JsonObject, TEXT("choices"), TEXT("message"), TEXT("content"), MessageContent))
    {
        FN2CLogger::Get().LogError(TEXT("Failed to extract message content from xAI response"), TEXT("xAIResponseParser"));
        return false;
    }

    // Extract usage information if available
    const TSharedPtr<FJsonObject> UsageObject = JsonObject->GetObjectField(TEXT("usage"));
    if (UsageObject.IsValid())
    {
        int32 PromptTokens = 0;
        int32 CompletionTokens = 0;
        UsageObject->TryGetNumberField(TEXT("prompt_tokens"), PromptTokens);
        UsageObject->TryGetNumberField(TEXT("completion_tokens"), CompletionTokens);

        OutResponse.Usage.InputTokens = PromptTokens;
        OutResponse.Usage.OutputTokens = CompletionTokens;

        FN2CLogger::Get().Log(FString::Printf(TEXT("LLM Token Usage - Input: %d Output: %d"), PromptTokens, CompletionTokens), EN2CLogSeverity::Info);
    }

    FN2CLogger::Get().Log(FString::Printf(TEXT("LLM Response Message Content: %s"), *MessageContent), EN2CLogSeverity::Debug);

    // Parse the extracted content as our expected JSON format
    return Super::ParseLLMResponse(MessageContent, OutResponse);
}
