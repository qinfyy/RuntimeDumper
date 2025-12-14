#include "pch.h"
#include "PrintHelper.h"
#include <windows.h>
#include <mutex>
#include <cstdio>
#include <cstdarg>

constexpr int MAX_BUFFER_SIZE = 512;

static bool g_consoleInitialized = false;
static std::mutex g_consoleLock;

void InitializeConsole()
{
    if (g_consoleInitialized)
        return;

    if (GetConsoleWindow() == NULL)
        AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    g_consoleInitialized = true;
}

void DebugPrintVA(const char* fmt, va_list args)
{
    InitializeConsole();

    va_list copy;
    va_copy(copy, args);
    int len = _vscprintf(fmt, copy);
    va_end(copy);
    if (len <= 0) return;

    char stackBuf[MAX_BUFFER_SIZE];
    char* buf = (len <= MAX_BUFFER_SIZE) ? stackBuf : (char*)malloc(len + 1);
    if (!buf) return;

    vsprintf_s(buf, len + 1, fmt, args);

    printf("%s", buf);

    if (len > MAX_BUFFER_SIZE)
        free(buf);

    fflush(stdout);
}

void DebugPrintVW(const wchar_t* fmt, va_list args)
{
    InitializeConsole();

    va_list copy;
    va_copy(copy, args);
    int len = _vscwprintf(fmt, copy);
    va_end(copy);
    if (len <= 0) return;

    wchar_t stackBuf[MAX_BUFFER_SIZE];
    wchar_t* buf = (len <= MAX_BUFFER_SIZE) ? stackBuf : (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (!buf) return;

    vswprintf_s(buf, len + 1, fmt, args);

    wprintf(L"%ls", buf);

    if (len > MAX_BUFFER_SIZE)
        free(buf);

    fflush(stdout);
}

void DebugPrintA(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DebugPrintVA(fmt, args);
    va_end(args);
}

void DebugPrintLockA(const char* fmt, ...)
{
    std::lock_guard guard(g_consoleLock);
    va_list args;
    va_start(args, fmt);
    DebugPrintVA(fmt, args);
    va_end(args);
}

void DebugPrintW(const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DebugPrintVW(fmt, args);
    va_end(args);
}

void DebugPrintLockW(const wchar_t* fmt, ...)
{
    std::lock_guard guard(g_consoleLock);
    va_list args;
    va_start(args, fmt);
    DebugPrintVW(fmt, args);
    va_end(args);
}

void UninitializeConsole()
{
    if (!g_consoleInitialized)
        return;

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    FreeConsole();

    g_consoleInitialized = false;
}
