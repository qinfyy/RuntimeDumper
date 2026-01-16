#include "pch.h"
#include "CSharpRender2.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <il2cpp-class-internals.h>
#include <il2cpp-object-internals.h>
#include <il2cpp-tabledefs.h>
#include "Il2CppFunctions.h"
#include "PrintHelper.h"
#include "Util.h"

// 获取字段类型字符串
static std::string GetTypeName(const Il2CppType* type) {
    if (!type) return "void";
    switch (type->type) {
    case IL2CPP_TYPE_VOID: return "void";
    case IL2CPP_TYPE_BOOLEAN: return "bool";
    case IL2CPP_TYPE_CHAR: return "char";
    case IL2CPP_TYPE_I1: return "sbyte";
    case IL2CPP_TYPE_U1: return "byte";
    case IL2CPP_TYPE_I2: return "short";
    case IL2CPP_TYPE_U2: return "ushort";
    case IL2CPP_TYPE_I4: return "int";
    case IL2CPP_TYPE_U4: return "uint";
    case IL2CPP_TYPE_I8: return "long";
    case IL2CPP_TYPE_U8: return "ulong";
    case IL2CPP_TYPE_R4: return "float";
    case IL2CPP_TYPE_R8: return "double";
    case IL2CPP_TYPE_STRING: return "string";
    default:
        const char* typeName = il2cpp_type_get_name(type);
        return typeName ? typeName : "object";
    }
}

static bool IsFieldStatic(const FieldInfo* field) {
    return field->type->attrs & FIELD_ATTRIBUTE_STATIC;
}

static bool IsMethodStatic(const MethodInfo* method) {
    return (method->flags & METHOD_ATTRIBUTE_STATIC) != 0;
}

static std::string ToBinary32(uint32_t value) {
    std::string result = "0b";
    for (int i = 31; i >= 0; --i) {
        result += ((value >> i) & 1) ? '1' : '0';
    }
    return result;
}

// 渲染函数
void RenderDumper(std::ostringstream& os) {
    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain) return;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);
    if (!assemblies) return;

    os << "// Runtime Dumper Render2\n\n";

    for (size_t i = 0; i < assemblyCount; ++i) {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image) continue;

        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t c = 0; c < classCount; ++c) {
            Il2CppClass* klass = const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
            if (!klass) continue;

			std::string ns = klass->namespaze ? klass->namespaze : "";
            std::string img = image->name ? image->name : "";
            std::string cls = klass->name ? klass->name : "";

            DebugPrintW(L"[DumpCs2] Dumping class: %ls\n", Utf8ToUtf16(cls).c_str());

            os << "namespace: " << ns << "\n";
            os << "Assembly: " << img << "\n";
            os << "class " << cls;
            if (klass->parent && klass->parent->name) os << " : " << klass->parent->name;
            os << " {\n\n";

            // Fields
            void* fieldIter = nullptr;
            FieldInfo* field;
            while ((field = il2cpp_class_get_fields(klass, &fieldIter))) {
                os << "\t0x" << std::hex << field->offset << std::dec << " | ";
                if (IsFieldStatic(field)) os << "static ";
                std::string typeName = GetTypeName(field->type);
                os << typeName << " "
                    << (field->name ? field->name : "")
                    << ";\n";
            }
            os << "\n";

            // Methods
            void* methodIter = nullptr;
            const MethodInfo* method;
            while ((method = il2cpp_class_get_methods(klass, &methodIter))) {
                uintptr_t funcAddr = reinterpret_cast<uintptr_t>(method->methodPointer);
                uintptr_t baseAddr = GetModuleBase();

                // Flags 二进制，RVA 大写十六进制
                os << "\t[Flags: " << ToBinary32(method->flags) << "] [ParamsCount: "
                    << il2cpp_method_get_param_count(method) << "] |RVA: 0x"
                    << std::uppercase << std::hex << (funcAddr - baseAddr) << std::dec << "|\n";

                os << "\t";
                if (IsMethodStatic(method)) os << "static ";
                std::string retType = GetTypeName(method->return_type);
                os << retType << " "
                    << (method->name ? method->name : "")
                    << "(";

                int paramCount = il2cpp_method_get_param_count(method);
                for (int p = 0; p < paramCount; ++p) {
                    const Il2CppType* paramType = il2cpp_method_get_param(method, p);
                    const char* paramName = il2cpp_method_get_param_name(method, p);
                    std::string pTypeStr = GetTypeName(paramType);
                    os << pTypeStr << " "
                        << (paramName ? paramName : "");
                    if (p + 1 < paramCount) os << ", ";
                }

                os << ");\n\n";
            }

            os << "}\n\n";
        }
    }
}

// 主函数
void DumpCs2(std::string path) {
    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::ofstream file(path);
    if (file.is_open()) {
        DebugPrintA("[DumpCs2] dumping...\n");

        std::ostringstream ss;
        RenderDumper(ss);

        file << ss.str();
        file.close();
        DebugPrintA("[DumpCs2] dump done!\n");
    }
    else
    {
        DebugPrintA("[ERROR] Failed to open file for writing: %s\n", path.c_str());
    }
}
