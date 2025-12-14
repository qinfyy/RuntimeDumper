#include "pch.h"
#include "Il2CppFunctions.h"
#include <iostream>
#include "PrintHelper.h"

void InitIl2CppFunctions()
{
    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    if (!hGameAssembly) {
		MessageBoxA(NULL, "GameAssembly.dll not found!", "Error", MB_OK | MB_ICONERROR);
		return;
    }

#define DO_API(r, n, p) \
        n = (r (*) p) GetProcAddress(hGameAssembly, #n); \
        DebugPrintA("[IL2CPP] %-55s -> %p\n", #n, n);

#define DO_API_NO_RETURN(r, n, p) DO_API(r, n, p)
#include "il2cpp-api-functions.h"
#undef DO_API
#undef DO_API_NO_RETURN
}

bool AttachIl2Cpp(Il2CppDomain*& outDomain, Il2CppThread*& outThread)
{
    outDomain = il2cpp_domain_get();
    if (!outDomain)
    {
        DebugPrintA("[ERROR] IL2CPP Domain Not Found!");
        return false;
    }

    outThread = il2cpp_thread_attach(outDomain);
    if (!outThread)
    {
        DebugPrintA("[ERROR] Failed to Attach IL2CPP Thread!");
        return false;
    }

    return true;
}
