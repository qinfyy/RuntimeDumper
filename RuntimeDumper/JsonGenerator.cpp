#include "pch.h"
#include <Windows.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <string>
#include "Il2CppFunctions.h"
#include "Util.h"
#include <il2cpp-tabledefs.h>
#include "PrintHelper.h"

std::string GetMethodTypeSignature(int typeEnum)
{
    switch (typeEnum)
    {
    case IL2CPP_TYPE_VOID: return "v";
    case IL2CPP_TYPE_BOOLEAN:
    case IL2CPP_TYPE_CHAR:
    case IL2CPP_TYPE_I1:
    case IL2CPP_TYPE_U1:
    case IL2CPP_TYPE_I2:
    case IL2CPP_TYPE_U2:
    case IL2CPP_TYPE_I4:
    case IL2CPP_TYPE_U4: return "i";
    case IL2CPP_TYPE_I8:
    case IL2CPP_TYPE_U8: return "j";
    case IL2CPP_TYPE_R4: return "f";
    case IL2CPP_TYPE_R8: return "d";
    default: return "i";
    }
}

std::string BuildTypeSignature(const MethodInfo* method)
{
    std::string sig;

    // 返回值
    const Il2CppType* returnType = il2cpp_method_get_return_type(method);
    if (returnType)
        sig += GetMethodTypeSignature(returnType->byref ? IL2CPP_TYPE_PTR : returnType->type);

    // this 指针
    if (!(method->flags & METHOD_ATTRIBUTE_STATIC))
        sig += GetMethodTypeSignature(IL2CPP_TYPE_OBJECT);

    // 参数
    int paramCount = il2cpp_method_get_param_count(method);
    for (int i = 0; i < paramCount; i++)
    {
        const Il2CppType* pType = il2cpp_method_get_param(method, i);
        if (pType)
            sig += GetMethodTypeSignature(pType->byref ? IL2CPP_TYPE_PTR : pType->type);
    }

    // MethodInfo*
    sig += GetMethodTypeSignature(IL2CPP_TYPE_PTR);

    return sig;
}

std::string BuildMethodName(Il2CppClass* klass, const MethodInfo* method)
{
    const char* className = il2cpp_class_get_name(klass);
    const char* methodName = il2cpp_method_get_name(method);
    if (!className) className = "UnknownClass";
    if (!methodName) methodName = "UnknownMethod";
    return std::string(className) + "$$" + methodName;
}

void WriteScriptMethods(std::ostringstream& os)
{
    uintptr_t moduleBase = GetModuleBase();
    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain) { os << "[]"; return; }

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);

    os << "[\n";
    bool firstMethod = true;

    for (size_t a = 0; a < assemblyCount; ++a)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[a]);
        if (!image) continue;

        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t c = 0; c < classCount; ++c)
        {
            Il2CppClass* klass = const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
            if (!klass) continue;

            void* iter = nullptr;
            const MethodInfo* method = nullptr;
            while ((method = il2cpp_class_get_methods(klass, &iter)))
            {
                if (!method->methodPointer) continue;

                if (!firstMethod) os << ",\n";
                firstMethod = false;

                uintptr_t va = reinterpret_cast<uintptr_t>(method->methodPointer);
                uint64_t rva = (va >= moduleBase) ? (va - moduleBase) : va;
                std::string name = AsciiEscapeToEscapeLiterals(BuildMethodName(klass, method));
				std::string signature = BuildTypeSignature(method);
                DebugPrintA("[JsonGen] ScriptMethod %s\n", name.c_str());

                os << "\t{\n";
                os << "\t\t\"Address\": " << rva << ",\n";
                os << "\t\t\"Name\": \"" << name << "\",\n";
                os << "\t\t\"Signature\": \"\",\n";
                os << "\t\t\"TypeSignature\": \"" << signature << "\"\n";
                os << "\t}";
            }
        }
    }

    os << "\n\t]";
}

void DumpJsonOutput(std::ostringstream& os)
{
    os << "{\n";

    os << "\t\"ScriptMethod\": ";
    WriteScriptMethods(os);
    os << ",\n";
    os << "\t\"ScriptString\": []";
    os << ",\n";
    os << "\t\"ScriptMetadata\": []";
    os << ",\n";
    os << "\t\"ScriptMetadataMethod\": []";
    os << ",\n";
    os << "\t\"Addresses\": []";
    os << "\n";
    os << "}\n";
}

void DumpJsonOutputToFile(const std::string& path)
{
    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        DebugPrintA("[JsonGen] start writing JSON...\n");

        std::ostringstream ss;
        DumpJsonOutput(ss);

        file << ss.str();
        file.close();
        DebugPrintA("[JsonGen] JSON output complete!\n");
    }
    else
    {
        DebugPrintA("[ERROR] Failed to open file for writing: %s\n", path);
    }
}
