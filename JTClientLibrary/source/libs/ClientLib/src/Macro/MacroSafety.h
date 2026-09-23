#pragma once

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <string>
#include <support/SafePath.h>

inline std::n_wstring KmtSanitizeMacroCharacterName(const wchar_t* value)
{
    std::n_wstring result;
    if (!value)
        return result;
    for (const wchar_t* p = value; *p && result.size() < 64; ++p)
    {
        const wchar_t c = *p;
        if (c < 32 || c == L'<' || c == L'>' || c == L':' || c == L'"' ||
            c == L'/' || c == L'\\' || c == L'|' || c == L'?' || c == L'*')
            result.push_back(L'_');
        else
            result.push_back(c);
    }
    return result;
}

inline FILE* KmtOpenAtomicTextFile(const char* finalPath, char* temporaryPath, size_t capacity)
{
    if (!finalPath || !temporaryPath || !KmtFormatPath(temporaryPath, capacity, "%s.tmp.%lu", finalPath, GetCurrentProcessId()))
        return NULL;
    return fopen(temporaryPath, "w");
}

inline bool KmtCommitAtomicTextFile(FILE* file, const char* temporaryPath, const char* finalPath)
{
    if (!file || !temporaryPath || !finalPath)
        return false;
    const bool flushed = fflush(file) == 0 && _commit(_fileno(file)) == 0;
    const bool closed = fclose(file) == 0;
    if (!flushed || !closed || !MoveFileExA(temporaryPath, finalPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileA(temporaryPath);
        return false;
    }
    return true;
}

inline int KmtClampMacroSetting(int value, int minimum, int maximum, int fallback)
{
    return value >= minimum && value <= maximum ? value : fallback;
}
