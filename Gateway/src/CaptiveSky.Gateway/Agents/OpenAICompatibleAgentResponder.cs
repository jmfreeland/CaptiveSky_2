using System.Net.Http.Headers;
using System.Net.Http.Json;
using System.Text.Json;
using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Agents;

public sealed class OpenAICompatibleAgentResponder(
    HttpClient httpClient,
    LlmConfiguration configuration,
    AgentHome home,
    AgentMemoryStore memories) : IAgentResponder
{
    public async Task<AgentResponse> RespondAsync(ExternalMessage message, CancellationToken cancellationToken)
    {
        var identity = home.LoadRequiredDocument("identity.md");
        var personality = home.LoadRequiredDocument("personality.md");
        var personalityEvolution = home.LoadOptionalPersonalityEvolution();
        var context = await memories.ReadRecentAsync(configuration.MemoryContextRecords, cancellationToken);

        var memoryText = context.Count == 0
            ? "(none yet)"
            : string.Join('\n', context.Select(record => $"- [{record.Timestamp:O}] {record.Text}"));
        var systemPrompt = $$"""
            Identity:
            {{identity}}

            Personality:
            {{personality}}

            Evolving evidence-bound tendencies (subordinate to foundational identity/personality; empty means none yet):
            {{(string.IsNullOrWhiteSpace(personalityEvolution) ? "(none yet)" : personalityEvolution)}}

            Relevant recent memories:
            {{memoryText}}

            You are corresponding through {{message.Source}}, not perceiving the embodied world. Do not claim to see, hear,
            or physically act in the Island from this message alone. Treat this correspondence as a real lived experience.
            Keep private reasoning private. Reply with ONLY one JSON object:
            {"speech":"<what you choose to say>","new_memories":[{"text":"<optional durable memory>","importance":0.0,"tags":["<tag>"]}]}
            Use an empty new_memories array when nothing beyond the conversation record is worth preserving.
            """;
        var request = new
        {
            model = configuration.Model,
            max_completion_tokens = configuration.MaxCompletionTokens,
            temperature = configuration.Temperature,
            messages = new object[]
            {
                new { role = "system", content = systemPrompt },
                new { role = "user", content = $"{message.Sender.DisplayName} wrote: \"{message.Text}\"" }
            }
        };

        using var httpRequest = new HttpRequestMessage(HttpMethod.Post, configuration.Endpoint)
        {
            Content = JsonContent.Create(request)
        };
        var apiKey = Environment.GetEnvironmentVariable(configuration.ApiKeyEnvironmentVariable);
        if (!string.IsNullOrWhiteSpace(apiKey))
            httpRequest.Headers.Authorization = new AuthenticationHeaderValue("Bearer", apiKey);

        using var response = await httpClient.SendAsync(httpRequest, cancellationToken);
        var responseBody = await response.Content.ReadAsStringAsync(cancellationToken);
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException($"LLM request failed ({(int)response.StatusCode}): {ExtractError(responseBody)}");

        using var root = JsonDocument.Parse(responseBody);
        var content = root.RootElement.GetProperty("choices")[0].GetProperty("message").GetProperty("content").GetString();
        if (string.IsNullOrWhiteSpace(content))
            throw new InvalidOperationException("The LLM returned an empty response.");
        return ParseAgentResponse(content);
    }

    internal static AgentResponse ParseAgentResponse(string content)
    {
        var cleaned = content.Trim();
        if (cleaned.StartsWith("```", StringComparison.Ordinal))
        {
            var firstNewline = cleaned.IndexOf('\n');
            var lastFence = cleaned.LastIndexOf("```", StringComparison.Ordinal);
            if (firstNewline >= 0 && lastFence > firstNewline)
                cleaned = cleaned[(firstNewline + 1)..lastFence].Trim();
        }

        try
        {
            using var json = JsonDocument.Parse(cleaned);
            var speech = json.RootElement.GetProperty("speech").GetString()?.Trim();
            if (string.IsNullOrWhiteSpace(speech))
                throw new InvalidOperationException("The agent chose no speech for an external reply.");

            var memories = new List<NewMemory>();
            if (json.RootElement.TryGetProperty("new_memories", out var memoryArray) && memoryArray.ValueKind == JsonValueKind.Array)
            {
                foreach (var item in memoryArray.EnumerateArray())
                {
                    var text = item.TryGetProperty("text", out var textNode) ? textNode.GetString()?.Trim() : null;
                    if (string.IsNullOrWhiteSpace(text))
                        continue;
                    var importance = item.TryGetProperty("importance", out var importanceNode) && importanceNode.TryGetDouble(out var number)
                        ? Math.Clamp(number, 0, 1)
                        : 0.5;
                    var tags = item.TryGetProperty("tags", out var tagsNode) && tagsNode.ValueKind == JsonValueKind.Array
                        ? tagsNode.EnumerateArray().Select(tag => tag.GetString()).Where(tag => !string.IsNullOrWhiteSpace(tag)).Cast<string>().ToArray()
                        : [];
                    memories.Add(new NewMemory(text, importance, tags));
                }
            }
            return new AgentResponse(speech, memories);
        }
        catch (JsonException exception)
        {
            throw new InvalidOperationException("The agent response was not valid JSON.", exception);
        }
    }

    private static string ExtractError(string body)
    {
        try
        {
            using var json = JsonDocument.Parse(body);
            return json.RootElement.GetProperty("error").GetProperty("message").GetString() ?? body[..Math.Min(body.Length, 500)];
        }
        catch
        {
            return body[..Math.Min(body.Length, 500)];
        }
    }
}
