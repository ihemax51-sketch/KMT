#pragma once

namespace GameServerConsole
{
    void Initialize();
    void WriteInfo(const char* text);
    void WriteWarning(const char* text);
    void WriteSuccess(const char* text);
    void WriteFailure(const char* text);
}
