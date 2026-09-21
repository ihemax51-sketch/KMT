///******************************************************************************
/// \File BSObj.h
///
/// \Desc
///
///******************************************************************************

#pragma once

/// Critical error that could lead to a crash
#define PUT_CRITICAL 5

/// Non-cricital error that will not lead to a crash
#define PUT_ERROR 4

/// A warning that should be taken care of
#define PUT_WARNING 3

/// Informational message that requires no action
#define PUT_INFO 2

/// Debugging message for developers
#define PUT_DEBUG 1

/// Low level debug messages for developers (consider enabling this on a per-file basis to avoid spam)
#define PUT_DEBUG_LOW 0

#ifndef PUT_LOGLEVEL_DEFAULT
#define PUT_LOGLEVEL_DEFAULT PUT_DEBUG_LOW
#endif

#ifndef PUT_LOGLEVEL
#define PUT_LOGLEVEL PUT_LOGLEVEL_DEFAULT
#endif

#define BS_NONE(fmt, ...) do { ReportConsole(-1, fmt, __VA_ARGS__);} while (0);

/// \def BS_DEBUG(fmt, ...)
/// \copybrief PUT_DEBUG
#if PUT_LOGLEVEL <= PUT_DEBUG
#define BS_DEBUG(fmt, ...) do { ReportConsole(PUT_DEBUG, fmt, __VA_ARGS__);} while (0);
#else
#define BS_DEBUG(fmt, ...)
#endif

/// \def BS_INFO(fmt, ...)
/// \copybrief PUT_INFO
#if PUT_LOGLEVEL <= PUT_INFO
#define BS_INFO(fmt, ...) do { ReportConsole(PUT_INFO, fmt, __VA_ARGS__);} while (0);
#else
#define BS_INFO(fmt, ...)
#endif

/// \def BS_WARNING(fmt, ...)
/// \copybrief PUT_WARNING
#if PUT_LOGLEVEL <= PUT_WARNING
#define BS_WARNING(fmt, ...) do { ReportConsole(PUT_WARNING, fmt, __VA_ARGS__);} while (0);
#else
#define BS_WARNING(fmt, ...)
#endif

/// \def BS_ERROR(fmt, ...)
/// \copybrief PUT_ERROR
#if PUT_LOGLEVEL <= PUT_ERROR
#define BS_ERROR(fmt, ...) do { ReportConsole(PUT_ERROR, fmt, __VA_ARGS__);} while (0);
#else
#define BS_ERROR(fmt, ...)
#endif

/// \def BS_CRITICAL(fmt, ...)
/// \copybrief PUT_CRITICAL
#if PUT_LOGLEVEL <= PUT_CRITICAL
#define BS_CRITICAL(fmt, ...) do { ReportConsole(PUT_CRITICAL, fmt, __VA_ARGS__);} while (0);
#else
#define BS_CRITICAL(fmt, ...)
#endif

void ReportConsole(int nLogLevel, const char *fmt, ...);

const char *GetReportConsoleType(int nType);

void MsgBoxError(const char *lpsczText, ...);