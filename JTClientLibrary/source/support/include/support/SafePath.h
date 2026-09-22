#pragma once

#include <stdio.h>
#include <stdarg.h>

inline bool KmtFormatPath(char* destination, size_t capacity, const char* format, ...)
{
    if (!destination || capacity == 0 || !format)
        return false;
    destination[0] = '\0';
    va_list arguments;
    va_start(arguments, format);
    const int written = _vsnprintf(destination, capacity - 1, format, arguments);
    va_end(arguments);
    destination[capacity - 1] = '\0';
    return written >= 0 && static_cast<size_t>(written) < capacity;
}
