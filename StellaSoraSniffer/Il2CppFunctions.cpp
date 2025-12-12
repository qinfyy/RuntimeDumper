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

    il2cpp_domain_get = (t_il2cpp_domain_get)GetProcAddress(hGameAssembly, "il2cpp_domain_get");
    il2cpp_domain_get_assemblies = (t_il2cpp_domain_get_assemblies)GetProcAddress(hGameAssembly, "il2cpp_domain_get_assemblies");
    il2cpp_assembly_get_image = (t_il2cpp_assembly_get_image)GetProcAddress(hGameAssembly, "il2cpp_assembly_get_image");
    il2cpp_thread_attach = (t_il2cpp_thread_attach)GetProcAddress(hGameAssembly, "il2cpp_thread_attach");

	DebugPrintA("[INFO] il2cpp_domain_get: %p\n", il2cpp_domain_get);
    DebugPrintA("[INFO] il2cpp_domain_get_assemblies: %p\n", il2cpp_domain_get_assemblies);
    DebugPrintA("[INFO] il2cpp_assembly_get_image: %p\n", il2cpp_assembly_get_image);
    DebugPrintA("[INFO] il2cpp_thread_attach: %p\n", il2cpp_thread_attach);
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