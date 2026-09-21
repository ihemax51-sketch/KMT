#include <Windows.h>

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

#include "SettingManagers/Settings.h"
#include "Console/ShardManagerConsole.h"
#include "Runtime/ShardManagerRuntimeSafety.h"

namespace
{
    volatile LONG s_initializationState = 0;

    bool InitializeAddon()
    {
        const LONG previous = InterlockedCompareExchange(&s_initializationState, 1, 0);
        if (previous == 2)
            return true;
        if (previous == 3)
            return false;
        if (previous == 1)
        {
            for (unsigned attempt = 0; attempt < 1200; ++attempt)
            {
                Sleep(100);
                const LONG state = InterlockedCompareExchange(&s_initializationState, 0, 0);
                if (state == 2)
                    return true;
                if (state == 3)
                    return false;
            }
            return false;
        }

        ShardManagerConsole::Initialize(CSettings::ShouldAllocateConsole());

        bool success = false;
        try
        {
            if (!ShardManagerRuntimeSafety::ValidateHost())
                throw "Host compatibility validation failed";
            ShardManagerConsole::WriteSuccess("Supported ShardManager runtime detected");

            if (!CSettings::LoadIniSettings())
                throw "Runtime settings or database initialization failed";
            ShardManagerConsole::WriteSuccess("Database and runtime settings loaded");

            if (!ShardManagerRuntimeSafety::InstallHooks())
                throw "Required message hooks could not be installed";

            success = true;
        }
        catch (const char* reason)
        {
            ShardManagerConsole::WriteFailure(reason);
        }
        catch (...)
        {
            ShardManagerConsole::WriteFailure("Unexpected initialization failure");
        }

        if (!success)
        {
            ShardManagerRuntimeSafety::RollbackHooks();
            CSettings::Shutdown();
            ShardManagerConsole::SetFailed();
            InterlockedExchange(&s_initializationState, 3);
            return false;
        }

        InterlockedExchange(&s_initializationState, 2);
        ShardManagerConsole::SetReady();
        return true;
    }

    DWORD WINAPI InitializeShardManager(LPVOID)
    {
        if (!InitializeAddon())
        {
            ShardManagerConsole::WriteFailure(
                "Fail-closed shutdown: required ShardManager protection did not initialize");
            TerminateProcess(GetCurrentProcess(), ERROR_DLL_INIT_FAILED);
            return ERROR_DLL_INIT_FAILED;
        }
        return ERROR_SUCCESS;
    }
}

EXTERN_DLL_EXPORT void InitializeShardManagerAddon()
{
    if (!InitializeAddon())
        TerminateProcess(GetCurrentProcess(), ERROR_DLL_INIT_FAILED);
}

EXTERN_DLL_EXPORT BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(NULL, 0, InitializeShardManager, NULL, 0, NULL);
        if (thread == NULL)
            return FALSE;
        CloseHandle(thread);
    }
    return TRUE;
}
