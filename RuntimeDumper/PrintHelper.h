#pragma once

void DebugPrintA(const char* fmt, ...);
void DebugPrintW(const wchar_t* fmt, ...);

void DebugPrintLockA(const char* fmt, ...);
void DebugPrintLockW(const wchar_t* fmt, ...);

void InitializeConsole();
void UninitializeConsole();

#ifdef UNICODE
#define DebugPrint DebugPrintW
#define DebugPrintLock DebugPrintLockW
#else
#define DebugPrint DebugPrintA
#define DebugPrintLock DebugPrintLockA
#endif
