#pragma once

#include <Windows.h>

namespace GameServerCrashHandler
{
    void Initialize(HMODULE module);
    LONG HandleException(EXCEPTION_POINTERS* exceptionPointers);
}
