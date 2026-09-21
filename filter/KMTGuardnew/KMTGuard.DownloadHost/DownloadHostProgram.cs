using KMTGuard.RuntimeContract;

namespace KMTGuard.DownloadHost;

internal static class DownloadHostProgram
{
    private static async Task<int> Main()
    {
        try
        {
            await global::Program.RunWorkerAsync(FilterRole.Download);
            return 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine($"KMTGuard.Download failed to start: {exception}");
            return 1;
        }
    }
}
