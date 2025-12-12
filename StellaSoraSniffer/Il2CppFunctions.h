#pragma once
#include "il2cpp/il2cpp-class-internals.h"

typedef Il2CppDomain* (*t_il2cpp_domain_get)();
typedef const Il2CppAssembly** (*t_il2cpp_domain_get_assemblies)(const Il2CppDomain* domain, size_t* size);
typedef const Il2CppImage* (*t_il2cpp_assembly_get_image)(const Il2CppAssembly* assembly);
typedef Il2CppThread *(*t_il2cpp_thread_attach)(Il2CppDomain* domain);

inline t_il2cpp_domain_get il2cpp_domain_get = nullptr;
inline t_il2cpp_domain_get_assemblies il2cpp_domain_get_assemblies = nullptr;
inline t_il2cpp_assembly_get_image il2cpp_assembly_get_image = nullptr;
inline t_il2cpp_thread_attach il2cpp_thread_attach = nullptr;


void InitIl2CppFunctions();

bool AttachIl2Cpp(Il2CppDomain*& outDomain, Il2CppThread*& outThread);

void PrintAllImageNames();