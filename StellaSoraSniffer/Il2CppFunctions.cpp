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

void PrintAllImageNames()
{
    if (!il2cpp_domain_get || !il2cpp_domain_get_assemblies || !il2cpp_assembly_get_image)
        return;

    Il2CppDomain* domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);

    size_t asmCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &asmCount);

    DebugPrintA("Loaded Assemblies:\n");

    for (size_t i = 0; i < asmCount; i++)
    {
        const Il2CppAssembly* assembly = assemblies[i];
        const Il2CppImage* image = il2cpp_assembly_get_image(assembly);

        if (image && image->name)
        {
            DebugPrintA("%s\n", image->name);
        }
    }
}