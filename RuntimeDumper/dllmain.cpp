// dllmain.cpp
#include "pch.h"
#include <string>
#include "Version.h"
#include "Initializer.h"

bool IsUnityCrashHandler()
{
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0)
        return false;

    std::string exeName = exePath;
    size_t pos = exeName.find_last_of("\\/");
    if (pos != std::string::npos)
        exeName = exeName.substr(pos + 1);

    return (_stricmp(exeName.c_str(), "UnityCrashHandler64.exe") == 0) ||
        (_stricmp(exeName.c_str(), "UnityCrashHandler32.exe") == 0);
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (IsUnityCrashHandler())
        {
            //LoadVersionDll();
            //break;
            ExitProcess(0);
        }

        LoadVersionDll();
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
