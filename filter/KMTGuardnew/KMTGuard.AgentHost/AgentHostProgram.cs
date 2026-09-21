using KMTGuard.RuntimeContract;

namespace KMTGuard.AgentHost;

internal static class AgentHostProgram
{
    private static async Task<int> Main()
    {
        try
        {
            await ReadRequiredChangelogAsync();
            await global::Program.RunWorkerAsync(FilterRole.Agent);
            return 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine($"KMTGuard.Agent failed to start: {exception.Message}");
            return 1;
        }
    }

    private static async Task ReadRequiredChangelogAsync()
    {
        var changelogPath = Path.Combine(AppContext.BaseDirectory, "CHANGELOG.md");
        if (!File.Exists(changelogPath))
        {
            throw new FileNotFoundException(
                "Required file CHANGELOG.md is missing. Restore it beside KMTGuard.Agent.exe before starting the service.");
        }

        var contents = await File.ReadAllTextAsync(changelogPath);
        if (string.IsNullOrWhiteSpace(contents))
        {
            throw new InvalidDataException(
                "Required file CHANGELOG.md is empty. Add the customer update history before starting the service.");
        }
    }
}
