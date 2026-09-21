using System.Globalization;
using System.IO;
using System.Threading;
using KMTGuard.RuntimeContract;
using Serilog.Events;
using Serilog.Formatting;

namespace KMTGuard.ConsoleUi;

/// <summary>
/// Gives every split KMTGuard service a compact, recognizable event stream.
/// This changes presentation only; the underlying Serilog events stay intact.
/// </summary>
public sealed class KmtServiceLogFormatter : ITextFormatter
{
    private readonly string _channel;
    private long _sequence;

    public KmtServiceLogFormatter(FilterRole role)
    {
        _channel = role switch
        {
            FilterRole.Agent => "A.GUARD",
            FilterRole.Download => "D.RELAY",
            FilterRole.Gateway => "G.EDGE",
            _ => "CORE"
        };
    }

    public void Format(LogEvent logEvent, TextWriter output)
    {
        var sequence = Interlocked.Increment(ref _sequence);
        var message = logEvent.RenderMessage(CultureInfo.InvariantCulture)
            .Replace("\r\n", "\n", StringComparison.Ordinal)
            .Replace('\r', '\n')
            .Replace("\n", Environment.NewLine + "                 -> ", StringComparison.Ordinal);

        output.Write(logEvent.Timestamp.ToString("HH:mm:ss.fff", CultureInfo.InvariantCulture));
        output.Write("  ");
        output.Write(GetLevelGlyph(logEvent.Level));
        output.Write("  ");
        output.Write(_channel);
        output.Write('#');
        output.Write(sequence.ToString("D6", CultureInfo.InvariantCulture));
        output.Write("  ");
        if (logEvent.Level >= LogEventLevel.Warning)
        {
            output.Write(GetLevelName(logEvent.Level));
            output.Write("  ");
        }
        output.WriteLine(message);

        if (logEvent.Exception is not null)
        {
            var exceptionLines = logEvent.Exception.ToString()
                .Replace("\r\n", "\n", StringComparison.Ordinal)
                .Replace('\r', '\n')
                .Split('\n', StringSplitOptions.RemoveEmptyEntries);

            for (var index = 0; index < exceptionLines.Length; index++)
            {
                output.Write("              ");
                output.Write(index == 0 ? "\\- exception  " : "   trace     ");
                output.WriteLine(exceptionLines[index].TrimEnd());
            }
        }
    }

    private static string GetLevelGlyph(LogEventLevel level) => level switch
    {
        LogEventLevel.Warning => "!!",
        LogEventLevel.Error or LogEventLevel.Fatal => "XX",
        LogEventLevel.Debug or LogEventLevel.Verbose => "--",
        _ => ">>"
    };

    private static string GetLevelName(LogEventLevel level) => level switch
    {
        LogEventLevel.Verbose => "TRACE",
        LogEventLevel.Debug => "DEBUG",
        LogEventLevel.Information => "INFO",
        LogEventLevel.Warning => "WARN",
        LogEventLevel.Error => "ERROR",
        LogEventLevel.Fatal => "FATAL",
        _ => "EVENT"
    };
}
