#include "pch.h"
#include "HookManager.h"
#include "Hooks.h"
#include "PrintHelper.h"
#include <detours.h>
#include "Il2CppFunctions.h"

//const uintptr_t RVA_get_ClientPublishChannelName = 0x1297B40;
//const uintptr_t RVA_ClientConfig_get_ServerMetaKey = 0x12986E0;
//const uintptr_t RVA_ClientConfig_get_ServerGarbleKey = 0x1298600;
const uintptr_t RVA_readMessage = 0x12F2140;
const uintptr_t RVA_BuildMessage = 0x12EE290;

void InstallHooks() {
    auto base = (uintptr_t)GetModuleHandleA("GameAssembly.dll");

    //o_get_ClientPublishChannelName = (void*)RVA_get_ClientPublishChannelName;
	//o_ClientConfig_get_ServerMetaKey = (void*)(base + RVA_ClientConfig_get_ServerMetaKey);
	//o_ClientConfig_get_ServerGarbleKey = (void*)(base + RVA_ClientConfig_get_ServerGarbleKey);
    o_readMessage = (void*)(base + RVA_readMessage);
    o_BuildMessage = (void*)(base + RVA_BuildMessage);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    //DetourAttach(&(PVOID&)o_get_ClientPublishChannelName, get_ClientPublishChannelName_Hook);
	//DetourAttach(&(PVOID&)o_ClientConfig_get_ServerMetaKey, ClientConfig_get_ServerMetaKey_Hook);
	//DetourAttach(&(PVOID&)o_ClientConfig_get_ServerGarbleKey, ClientConfig_get_ServerGarbleKey_Hook);
    DetourAttach(&(PVOID&)o_readMessage, readMessage_Hook);
    DetourAttach(&(PVOID&)o_BuildMessage, BuildMessage_Hook);

    DetourTransactionCommit();

    DebugPrintLockA("[INFO] Hooks installed\n");
}

void UninstallHooks() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    //DetourDetach(&(PVOID&)o_get_ClientPublishChannelName, get_ClientPublishChannelName_Hook);

    DetourTransactionCommit();
}

DWORD WINAPI WaitForGAModule(LPVOID) {
    DebugPrintLockA("[INFO] Waiting for GameAssembly.dll...\n");
    while (!GetModuleHandleA("GameAssembly.dll")) Sleep(200);

    DebugPrintLockA("[INFO] GameAssembly.dll loaded, installing hooks...\n");
    //InstallHooks();

    Sleep(5000);
    InitIl2CppFunctions();
    Il2CppDomain* domain = nullptr;
    Il2CppThread* thread = nullptr;
    if (!AttachIl2Cpp(domain, thread))
    {
        return 1;
    }

    PrintAllImageNames();
    return 0;
}
