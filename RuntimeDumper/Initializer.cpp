#include "pch.h"
#include "Initializer.h"
#include "PrintHelper.h"
#include "Il2CppFunctions.h"
#include "Il2CppDumper.h"
#include "JsonGenerator.h"
#include "MetaDumper.h"

void TestPrintAllImageNames()
{
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

DWORD WINAPI MainThread(LPVOID) {
    DebugPrintA("[INFO] RuntimeDumper\n");
    DebugPrintA("[INFO] Waiting for GameAssembly.dll...\n");

    while (!GetModuleHandle(L"GameAssembly.dll")) {
        Sleep(200);
    }

    DebugPrintA("[INFO] GameAssembly.dll loaded, Starting dump ...\n");

    Sleep(5000);
    InitIl2CppFunctions();
    Il2CppDomain* domain = nullptr;
    Il2CppThread* thread = nullptr;
    if (!AttachIl2Cpp(domain, thread))
    {
        return 1;
    }

 //   DumpCs(".\\output\\dump.cs");
 //   DumpJsonOutputToFile(".\\output\\script.json");
	//DumpMetaFile(".\\output\\global-metadata.dat");
    GetCodeRegistration();

    return 0;
}
