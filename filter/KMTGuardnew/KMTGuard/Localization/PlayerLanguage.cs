using System.Globalization;
using System.Text.RegularExpressions;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace KMTGuard.Localization;

public static class PlayerLanguage
{
    private const string FallbackLanguage = "English";
    private static readonly object Sync = new();
    private static readonly Regex PlaceholderPattern =
        new(@"\{(?<index>\d+)(?:,[^}:]+)?(?:\:[^}]+)?\}", RegexOptions.Compiled);
    private static readonly HashSet<string> ReportedMissingKeys = new(StringComparer.Ordinal);
    private static LanguageState _state = LanguageState.Empty;
    private static string _configuredLanguage = FallbackLanguage;

    public static string CurrentLanguage => Volatile.Read(ref _state).Language;
    public static string LanguagesDirectory => Path.Combine(AppContext.BaseDirectory, "Languages");

    public static LanguageLoadResult Initialize(string? configuredLanguage)
    {
        lock (Sync)
        {
            _configuredLanguage = NormalizeLanguageName(configuredLanguage);
            return LoadLocked();
        }
    }

    public static LanguageLoadResult Reload()
    {
        lock (Sync)
            return LoadLocked();
    }

    public static LanguageLoadResult Switch(string? configuredLanguage)
    {
        lock (Sync)
        {
            var previousLanguage = _configuredLanguage;
            _configuredLanguage = NormalizeLanguageName(configuredLanguage);
            try
            {
                return LoadLocked();
            }
            catch
            {
                _configuredLanguage = previousLanguage;
                throw;
            }
        }
    }

    public static string Get(string key, params object?[] args)
    {
        if (string.IsNullOrWhiteSpace(key))
            throw new ArgumentException("A player-language key is required.", nameof(key));

        var state = Volatile.Read(ref _state);
        if (!state.Active.TryGetValue(key, out var template) &&
            !state.Fallback.TryGetValue(key, out template))
        {
            ReportMissingKey(key);
            return $"[{key}]";
        }

        if (args.Length == 0)
            return template;

        try
        {
            return string.Format(CultureInfo.InvariantCulture, template, args);
        }
        catch (FormatException ex)
        {
            Log.Error(ex, "Invalid player-language format for key {LanguageKey}", key);
            if (state.Fallback.TryGetValue(key, out var fallbackTemplate) &&
                !string.Equals(template, fallbackTemplate, StringComparison.Ordinal))
            {
                try
                {
                    return string.Format(CultureInfo.InvariantCulture, fallbackTemplate, args);
                }
                catch (FormatException)
                {
                }
            }

            return template;
        }
    }

    public static bool TryGet(string key, out string value)
    {
        var state = Volatile.Read(ref _state);
        if (state.Active.TryGetValue(key, out value!))
            return true;

        return state.Fallback.TryGetValue(key, out value!);
    }

    public static string FormatDurationSeconds(int seconds)
    {
        var safeSeconds = Math.Max(0, seconds);
        if (safeSeconds == 60)
            return Get("Common.DurationMinute", 1);
        if (safeSeconds > 0 && safeSeconds % 60 == 0)
            return Get("Common.DurationMinutes", safeSeconds / 60);
        return safeSeconds == 1
            ? Get("Common.DurationSecond", 1)
            : Get("Common.DurationSeconds", safeSeconds);
    }

    public static string ResolveSystemNotice(string name, string? databaseText)
    {
        var key = $"SystemNotices.{name}";
        return TryGet(key, out _) ? Get(key) : databaseText ?? string.Empty;
    }

    private static LanguageLoadResult LoadLocked()
    {
        var fallbackPath = GetLanguagePath(FallbackLanguage);
        var fallback = LoadFile(fallbackPath);
        var activePath = GetLanguagePath(_configuredLanguage);
        var active = string.Equals(_configuredLanguage, FallbackLanguage, StringComparison.OrdinalIgnoreCase)
            ? fallback
            : LoadFile(activePath);

        var invalidKeys = new HashSet<string>(StringComparer.Ordinal);
        foreach (var pair in active)
        {
            if (!fallback.TryGetValue(pair.Key, out var fallbackValue))
                continue;

            if (!HaveSamePlaceholders(fallbackValue, pair.Value))
                invalidKeys.Add(pair.Key);
        }

        if (invalidKeys.Count > 0)
        {
            active = new Dictionary<string, string>(active, StringComparer.Ordinal);
            foreach (var key in invalidKeys)
                active.Remove(key);
        }

        var missingKeys = fallback.Keys
            .Where(key => !active.ContainsKey(key))
            .OrderBy(key => key, StringComparer.Ordinal)
            .ToArray();

        var state = new LanguageState(
            _configuredLanguage,
            fallback,
            active,
            fallbackPath,
            activePath);
        Volatile.Write(ref _state, state);
        ReportedMissingKeys.Clear();

        if (missingKeys.Length > 0)
            Log.Warning(
                "Player language {Language} is missing {MissingCount} key(s); English fallback will be used",
                _configuredLanguage,
                missingKeys.Length);

        if (invalidKeys.Count > 0)
            Log.Warning(
                "Player language {Language} has {InvalidCount} invalid placeholder set(s); English fallback will be used for those keys",
                _configuredLanguage,
                invalidKeys.Count);

        Log.Information(
            "Player language loaded :: language={Language} :: active={ActiveCount} :: fallback={FallbackCount}",
            _configuredLanguage,
            active.Count,
            fallback.Count);

        return new LanguageLoadResult(
            _configuredLanguage,
            active.Count,
            fallback.Count,
            missingKeys.Length,
            invalidKeys.Count,
            activePath);
    }

    private static Dictionary<string, string> LoadFile(string path)
    {
        if (!File.Exists(path))
            throw new FileNotFoundException(
                $"Player language file was not found: {path}",
                path);

        JObject values;
        try
        {
            using var textReader = new StringReader(File.ReadAllText(path));
            using var jsonReader = new JsonTextReader(textReader)
            {
                DateParseHandling = DateParseHandling.None
            };
            values = JObject.Load(
                jsonReader,
                new JsonLoadSettings
                {
                    CommentHandling = CommentHandling.Ignore,
                    DuplicatePropertyNameHandling = DuplicatePropertyNameHandling.Error,
                    LineInfoHandling = LineInfoHandling.Load
                });
        }
        catch (JsonException ex)
        {
            throw new InvalidDataException($"Player language file is invalid: {path}", ex);
        }

        if (values.Count == 0)
            throw new InvalidDataException($"Player language file is empty: {path}");

        var normalized = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var property in values.Properties())
        {
            var key = property.Name.Trim();
            if (string.IsNullOrWhiteSpace(key))
                throw new InvalidDataException($"Player language file contains an empty key: {path}");
            if (property.Value.Type != JTokenType.String)
                throw new InvalidDataException(
                    $"Player language key '{key}' must contain a string value: {path}");

            var value = property.Value.Value<string>();
            if (string.IsNullOrEmpty(value))
                throw new InvalidDataException($"Player language key '{key}' has an empty value: {path}");
            if (!normalized.TryAdd(key, value))
                throw new InvalidDataException($"Player language file contains duplicate key '{key}': {path}");
        }

        return normalized;
    }

    private static string NormalizeLanguageName(string? configuredLanguage)
    {
        var value = string.IsNullOrWhiteSpace(configuredLanguage)
            ? FallbackLanguage
            : configuredLanguage.Trim();

        value = value.ToLowerInvariant() switch
        {
            "en" or "en-us" or "en-gb" or "english" => "English",
            "tr" or "tr-tr" or "turkish" or "t\u00fcrk\u00e7e" => "Turkish",
            _ => value
        };

        if (value.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 ||
            value.Contains(Path.DirectorySeparatorChar) ||
            value.Contains(Path.AltDirectorySeparatorChar) ||
            value is "." or "..")
        {
            throw new InvalidDataException($"Invalid player language name: {value}");
        }

        return value;
    }

    private static string GetLanguagePath(string language)
    {
        var path = Path.GetFullPath(Path.Combine(LanguagesDirectory, $"{language}.json"));
        var root = Path.GetFullPath(LanguagesDirectory) + Path.DirectorySeparatorChar;
        if (!path.StartsWith(root, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException($"Invalid player language path: {path}");
        return path;
    }

    private static bool HaveSamePlaceholders(string fallback, string translated)
    {
        static int[] ReadIndexes(string value) =>
            PlaceholderPattern.Matches(value)
                .Select(match => int.Parse(match.Groups["index"].Value, CultureInfo.InvariantCulture))
                .Distinct()
                .OrderBy(index => index)
                .ToArray();

        return ReadIndexes(fallback).SequenceEqual(ReadIndexes(translated));
    }

    private static void ReportMissingKey(string key)
    {
        lock (Sync)
        {
            if (ReportedMissingKeys.Add(key))
                Log.Error("Missing player-language key: {LanguageKey}", key);
        }
    }

    private sealed record LanguageState(
        string Language,
        IReadOnlyDictionary<string, string> Fallback,
        IReadOnlyDictionary<string, string> Active,
        string FallbackPath,
        string ActivePath)
    {
        public static LanguageState Empty { get; } = new(
            FallbackLanguage,
            new Dictionary<string, string>(StringComparer.Ordinal),
            new Dictionary<string, string>(StringComparer.Ordinal),
            string.Empty,
            string.Empty);
    }
}

public sealed record LanguageLoadResult(
    string Language,
    int ActiveKeyCount,
    int FallbackKeyCount,
    int MissingKeyCount,
    int InvalidPlaceholderCount,
    string FilePath);
