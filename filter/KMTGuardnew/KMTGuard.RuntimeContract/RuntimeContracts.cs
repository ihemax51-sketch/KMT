using System.IO.Pipes;
using System.Text.Json;

namespace KMTGuard.RuntimeContract;

public enum FilterRole
{
    All,
    Gateway,
    Download,
    Agent
}

public static class RuntimeProtocol
{
    public const int Version = 2;

    public static string GetPipeName(FilterRole role) => $"KMTGuard.Runtime.{role}.v{Version}";
}

public sealed class RuntimeRequest
{
    public string Command { get; set; } = string.Empty;
    public string? Payload { get; set; }
}

public sealed class RuntimeResponse
{
    public bool Success { get; set; }
    public string Message { get; set; } = string.Empty;
    public string? Payload { get; set; }
}

public sealed class RuntimeSnapshot
{
    public FilterRole Role { get; set; }
    public int ProcessId { get; set; }
    public DateTime StartedAtUtc { get; set; }
    public long WorkingSetBytes { get; set; }
    public long PrivateMemoryBytes { get; set; }
    public int SessionCount { get; set; }
    public int ListenerCount { get; set; }
    public bool ListenersHealthy { get; set; }
    public double DatabaseLatencyMs { get; set; }
    public int CommandQueueDepth { get; set; }
    public long? OldestCommandAgeSeconds { get; set; }
    public long RejectedMassivePackets { get; set; }
    public DateTime? CacheLastRefreshUtc { get; set; }
}

public sealed class PlayerLanguageCommandPayload
{
    public string Language { get; set; } = "English";
}

public sealed class PlayerLanguageStatusPayload
{
    public string Language { get; set; } = "English";
    public int ActiveKeyCount { get; set; }
    public int FallbackKeyCount { get; set; }
    public int MissingKeyCount { get; set; }
    public int InvalidPlaceholderCount { get; set; }
}

public sealed class OnlinePlayerSnapshot
{
    public string CharName { get; set; } = string.Empty;
    public string UserId { get; set; } = string.Empty;
    public int CharId { get; set; }
    public uint UniqueId { get; set; }
    public string Ip { get; set; } = string.Empty;
    public string Hwid { get; set; } = string.Empty;
    public int Region { get; set; }
    public int World { get; set; }
    public byte Level { get; set; }
    public byte JobType { get; set; }
    public string Type { get; set; } = string.Empty;
    public DateTime CharacterReadyAtUtc { get; set; }
    public DateTime? LastPing { get; set; }
}

public sealed class ClientlessCommandPayload
{
    public int? Count { get; set; }
    public string? City { get; set; }
    public int? AccountId { get; set; }
}

public sealed class ClientlessPartyFormCommandPayload
{
    public bool Enabled { get; set; }
    public string Mode { get; set; } = "GroupsOf8";
    public string Title { get; set; } = "{CharacterName}";
    public byte MinLevel { get; set; } = 1;
    public byte MaxLevel { get; set; } = 140;
    public byte Purpose { get; set; }
    public byte SettingsFlag { get; set; } = 7;
}

public sealed class ClientlessPartyFormPolicyPayload
{
    public bool Enabled { get; set; }
    public string Mode { get; set; } = "GroupsOf8";
    public string Title { get; set; } = "{CharacterName}";
    public byte MinLevel { get; set; } = 1;
    public byte MaxLevel { get; set; } = 140;
    public byte Purpose { get; set; }
    public byte SettingsFlag { get; set; } = 7;
}

public sealed class SystemClientlessCommandPayload
{
    public string Role { get; set; } = string.Empty;
}

public sealed class QuickLoginAuthPayload
{
    public uint Token { get; set; }
    public string Username { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
    public string ClientIp { get; set; } = string.Empty;
    public byte Locale { get; set; }
    public DateTime ExpiresAtUtc { get; set; }
}

public sealed class OfflineStallAccountPayload
{
    public string Username { get; set; } = string.Empty;
}

public sealed class OfflineStallLookupResponse
{
    public bool IsActive { get; set; }
    public string CharName { get; set; } = string.Empty;
}

public static class RuntimePipeClient
{
    private static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web);

    public static async Task<RuntimeResponse> SendAsync(
        FilterRole role,
        string command,
        object? payload = null,
        TimeSpan? timeout = null,
        CancellationToken cancellationToken = default)
    {
        var effectiveTimeout = timeout ?? TimeSpan.FromSeconds(5);
        using var timeoutSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeoutSource.CancelAfter(effectiveTimeout);

        await using var pipe = new NamedPipeClientStream(
            ".",
            RuntimeProtocol.GetPipeName(role),
            PipeDirection.InOut,
            PipeOptions.Asynchronous);

        await pipe.ConnectAsync(timeoutSource.Token);

        using var reader = new StreamReader(pipe, leaveOpen: true);
        using var writer = new StreamWriter(pipe, leaveOpen: true) { AutoFlush = true };
        var request = new RuntimeRequest
        {
            Command = command,
            Payload = payload is null ? null : JsonSerializer.Serialize(payload, JsonOptions)
        };

        await writer.WriteLineAsync(JsonSerializer.Serialize(request, JsonOptions));
        var responseLine = await reader.ReadLineAsync(timeoutSource.Token);
        if (string.IsNullOrWhiteSpace(responseLine))
            throw new IOException($"The {role} service closed its control pipe without a response.");

        return JsonSerializer.Deserialize<RuntimeResponse>(responseLine, JsonOptions)
               ?? throw new InvalidDataException($"The {role} service returned an invalid control response.");
    }

    public static async Task<T?> SendForPayloadAsync<T>(
        FilterRole role,
        string command,
        object? payload = null,
        TimeSpan? timeout = null,
        CancellationToken cancellationToken = default)
    {
        var response = await SendAsync(role, command, payload, timeout, cancellationToken);
        if (!response.Success)
            throw new InvalidOperationException(response.Message);

        return string.IsNullOrWhiteSpace(response.Payload)
            ? default
            : JsonSerializer.Deserialize<T>(response.Payload, JsonOptions);
    }
}
