#pragma once

#include <windows.h>

void WriteClientStartupDiagnostic(const char* message);

// Installs a narrowly-scoped guard around CObjChild's redundant final child-list
// clear. Normal objects continue through the original client implementation.
bool InstallCObjChildTeardownCompatibilityGuard();

bool BeginClientInitialization();
void MarkClientInitializationSucceeded();
void MarkClientInitializationFailed();
bool WaitForClientInitialization(DWORD timeoutMilliseconds);
