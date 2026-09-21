#pragma once

namespace ShardManagerConsole
{
    bool Initialize(bool allocateConsole);
    void SetReady();
    void SetFailed();

    void WriteDebug(const char* text);
    void WriteInfo(const char* text);
    void WriteWarning(const char* text);
    void WriteSuccess(const char* text);
    void WriteFailure(const char* text);
    void WriteFormat(int level, const char* format, ...);
}
