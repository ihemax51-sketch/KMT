using KMTGuard.RuntimeContract;

namespace KMTGuard.GatewayHost;

internal static class GatewayHostProgram
{
    private static async Task<int> Main()
    {
        try
        {
            await global::Program.RunWorkerAsync(FilterRole.Gateway);
            return 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine($"KMTGuard.Gateway failed to start: {exception}");
            return 1;
        }
    }
}
