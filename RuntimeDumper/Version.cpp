#include "pch.h"
#include "Version.h"
#include "PrintHelper.h"
#include <vector>
#include <string>

static void* OriginalFuncs[17];

#define DEFINE_PROXY(i, name, ret, params, args) \
ret WINAPI p##name params { \
    using func = ret (WINAPI*) params; \
    return ((func)OriginalFuncs[i]) args; \
}

static const char* ExportNames[] = {
    "GetFileVersionInfoA",
    "GetFileVersionInfoByHandle",
    "GetFileVersionInfoExA",
    "GetFileVersionInfoExW",
    "GetFileVersionInfoSizeA",
    "GetFileVersionInfoSizeExA",
    "GetFileVersionInfoSizeExW",
    "GetFileVersionInfoSizeW",
    "GetFileVersionInfoW",
    "VerFindFileA",
    "VerFindFileW",
    "VerInstallFileA",
    "VerInstallFileW",
    "VerLanguageNameA",
    "VerLanguageNameW",
    "VerQueryValueA",
    "VerQueryValueW"
};

void LoadVersionDll()
{
    wchar_t szSystemDirectory[MAX_PATH]{};
    GetSystemDirectoryW(szSystemDirectory, MAX_PATH);

    std::wstring OriginalPath = szSystemDirectory;
    OriginalPath += L"\\version.dll";

    HMODULE version = LoadLibraryW(OriginalPath.c_str());
    if (!version)
    {
        DebugPrint(L"Failed to load system version.dll\n");
        return;
    }

    for (int i = 0; i < 17; i++)
    {
        OriginalFuncs[i] = GetProcAddress(version, ExportNames[i]);
        if (!OriginalFuncs[i])
        {
            DebugPrint(L"Failed to get address of %s\n", ExportNames[i]);
            return;
        }
    }

    DebugPrint(L"Loaded system version.dll\n");
}

DEFINE_PROXY(0, GetFileVersionInfoA,
    BOOL, (LPCSTR f, DWORD h, DWORD l, LPVOID d),
    (f, h, l, d));

DEFINE_PROXY(1, GetFileVersionInfoByHandle,
    BOOL, (DWORD a, DWORD b, DWORD c, LPVOID d),
    (a, b, c, d));

DEFINE_PROXY(2, GetFileVersionInfoExA,
    BOOL, (DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e),
    (a, b, c, d, e));

DEFINE_PROXY(3, GetFileVersionInfoExW,
    BOOL, (DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e),
    (a, b, c, d, e));

DEFINE_PROXY(4, GetFileVersionInfoSizeA,
    DWORD, (LPCSTR a, LPDWORD b),
    (a, b));

DEFINE_PROXY(5, GetFileVersionInfoSizeExA,
    DWORD, (DWORD a, LPCSTR b, LPDWORD c),
    (a, b, c));

DEFINE_PROXY(6, GetFileVersionInfoSizeExW,
    DWORD, (DWORD a, LPCWSTR b, LPDWORD c),
    (a, b, c));

DEFINE_PROXY(7, GetFileVersionInfoSizeW,
    DWORD, (LPCWSTR a, LPDWORD b),
    (a, b));

DEFINE_PROXY(8, GetFileVersionInfoW,
    BOOL, (LPCWSTR a, DWORD b, DWORD c, LPVOID d),
    (a, b, c, d));

DEFINE_PROXY(9, VerFindFileA,
    DWORD, (DWORD a, LPCSTR b, LPCSTR c, LPSTR d, LPSTR e,
        UINT f, UINT g, UINT h),
    (a, b, c, d, e, f, g, h));

DEFINE_PROXY(10, VerFindFileW,
    DWORD, (DWORD a, LPCWSTR b, LPCWSTR c, LPWSTR d, LPWSTR e,
        UINT f, UINT g, UINT h),
    (a, b, c, d, e, f, g, h));

DEFINE_PROXY(11, VerInstallFileA,
    DWORD, (DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e,
        LPCSTR f, LPCSTR g, LPSTR h),
    (a, b, c, d, e, f, g, h));

DEFINE_PROXY(12, VerInstallFileW,
    DWORD, (DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e,
        LPCWSTR f, LPCWSTR g, LPWSTR h),
    (a, b, c, d, e, f, g, h));

DEFINE_PROXY(13, VerLanguageNameA,
    DWORD, (DWORD a, LPSTR b, DWORD c),
    (a, b, c));

DEFINE_PROXY(14, VerLanguageNameW,
    DWORD, (DWORD a, LPWSTR b, DWORD c),
    (a, b, c));

DEFINE_PROXY(15, VerQueryValueA,
    BOOL, (LPCVOID a, LPCSTR b, LPVOID* c, PUINT d),
    (a, b, c, d));

DEFINE_PROXY(16, VerQueryValueW,
    BOOL, (LPCVOID a, LPCWSTR b, LPVOID* c, PUINT d),
    (a, b, c, d));
