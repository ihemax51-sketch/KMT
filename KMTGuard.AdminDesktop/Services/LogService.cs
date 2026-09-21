using System.IO;
using KMTGuard.AdminDesktop.Models;

namespace KMTGuard.AdminDesktop.Services;

public sealed class LogService
{
    internal const int DefaultTailByteLimit = 512 * 1024;

    public IReadOnlyList<LogFileItem> DiscoverLogs()
    {
        var roots = new[]
        {
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\KMTGuard\logs")),
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\KMTGuard\bin\Release\net8.0\win-x64\logs")),
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\KMTGuard\bin\Debug\net8.0\win-x64\logs")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, @"filter\KMTGuardnew\KMTGuard\bin\Release\net8.0\win-x64\logs")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, @"filter\KMTGuardnew\KMTGuard\bin\Debug\net8.0\win-x64\logs"))
        };

        return roots
            .Where(Directory.Exists)
            .SelectMany(root => Directory.EnumerateFiles(root, "*.log", SearchOption.TopDirectoryOnly))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .Select(path =>
            {
                var file = new FileInfo(path);
                return new LogFileItem
                {
                    Name = file.Name,
                    Path = file.FullName,
                    LastWriteTime = file.LastWriteTime,
                    Length = file.Length
                };
            })
            .OrderByDescending(file => file.LastWriteTime)
            .ToList();
    }

    public string ReadTail(string path, int maxLines = 350) =>
        ReadLastLines(path, maxLines, DefaultTailByteLimit);

    internal static string ReadLastLines(string path, int maxLines, int maxBytes)
    {
        if (!File.Exists(path))
            return "Log file was not found.";

        var boundedLines = Math.Clamp(maxLines, 1, 5000);
        var boundedBytes = Math.Clamp(maxBytes, 16 * 1024, 4 * 1024 * 1024);
        var queue = new Queue<string>(boundedLines);
        using var stream = new FileStream(
            path,
            FileMode.Open,
            FileAccess.Read,
            FileShare.ReadWrite | FileShare.Delete,
            bufferSize: 16 * 1024,
            options: FileOptions.SequentialScan);
        var startOffset = Math.Max(0, stream.Length - boundedBytes);
        stream.Seek(startOffset, SeekOrigin.Begin);
        using var reader = new StreamReader(stream, detectEncodingFromByteOrderMarks: startOffset == 0);

        // A bounded seek can start in the middle of a UTF-8 character or log
        // line. Discard only that partial first line; every complete trailing
        // line remains available without scanning a multi-gigabyte file.
        if (startOffset > 0)
            _ = reader.ReadLine();

        while (reader.ReadLine() is { } line)
        {
            if (queue.Count == boundedLines)
                queue.Dequeue();

            queue.Enqueue(line);
        }

        return string.Join(Environment.NewLine, queue);
    }
}
