#include "pch.h"
#include "CSharpRender3.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <il2cpp-class-internals.h>
#include <il2cpp-object-internals.h>
#include <il2cpp-tabledefs.h>
#include "Il2CppFunctions.h"
#include "PrintHelper.h"
#include "Util.h"

static std::string GetTypeName(const Il2CppType* type) {
    if (!type) return "<void>";
    switch (type->type) {
    case IL2CPP_TYPE_VOID: return "<void>";
    case IL2CPP_TYPE_BOOLEAN: return "<bool>";
    case IL2CPP_TYPE_CHAR: return "<char>";
    case IL2CPP_TYPE_I1: return "<sbyte>";
    case IL2CPP_TYPE_U1: return "<byte>";
    case IL2CPP_TYPE_I2: return "<short>";
    case IL2CPP_TYPE_U2: return "<ushort>";
    case IL2CPP_TYPE_I4: return "<int>";
    case IL2CPP_TYPE_U4: return "<uint>";
    case IL2CPP_TYPE_I8: return "<long>";
    case IL2CPP_TYPE_U8: return "<ulong>";
    case IL2CPP_TYPE_R4: return "<float>";
    case IL2CPP_TYPE_R8: return "<double>";
    case IL2CPP_TYPE_STRING: return "<string>";
    default:
        const char* typeName = il2cpp_type_get_name(type);
        return typeName ? std::string("<") + typeName + ">" : "<object>";
    }
}

static bool IsFieldStatic(const FieldInfo* field) {
    return field->type->attrs & FIELD_ATTRIBUTE_STATIC;
}

static bool IsMethodStatic(const MethodInfo* method) {
    return (method->flags & METHOD_ATTRIBUTE_STATIC) != 0;
}

static std::string ToBinary32(uint32_t value) {
    std::string result = "";
    for (int i = 31; i >= 0; --i)
        result += ((value >> i) & 1) ? "1" : "0";
    return result;
}

static void RenderDumper3(std::ostringstream& os) {
    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain) return;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);
    if (!assemblies) return;

    uintptr_t baseAddr = GetModuleBase();

    os << "// Runtime Dumper Render3\n\n";

    int assemblyIndex = 0;
    for (size_t i = 0; i < assemblyCount; ++i) {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image) continue;

        std::string assemblyName = image->name ? image->name : "<Unknown>";
        size_t classCount = il2cpp_image_get_class_count(image);

        ++assemblyIndex;  // 当前 assembly 序号

        int globalClassIndex = 0;
        for (size_t c = 0; c < classCount; ++c) {
            Il2CppClass* klass = const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
            if (!klass) continue;

            globalClassIndex++; // 类序号

            std::string ns = klass->namespaze ? klass->namespaze : "";
            std::string clsName = klass->name ? klass->name : "<Unknown>";
            std::string parentName = (klass->parent && klass->parent->name) ? klass->parent->name : "";

            DebugPrint(L"[DumpCs3] Dumping class: %ls\n", Utf8ToUtf16(clsName).c_str());

            // 输出 Assembly / 类信息
            os << "[" << std::setw(4) << std::setfill('0') << assemblyIndex << "|"
                << std::setw(4) << std::setfill('0') << assemblyCount << "] "
                << assemblyName << "\n";
            os << "using namespace " << ns << ";\n";
            os << "[" << std::setw(4) << std::setfill('0') << globalClassIndex << "|"
                << std::setw(4) << std::setfill('0') << classCount << "] "
                << "class " << clsName;
            if (!parentName.empty()) os << " : " << parentName;
            os << "\n{\n";

            // 字段
            void* fieldIter = nullptr;
            FieldInfo* field;
            std::vector<FieldInfo*> fields;
            while ((field = il2cpp_class_get_fields(klass, &fieldIter))) {
                fields.push_back(field);
            }

            os << "    // 字段数量:" << fields.size() << "\n";
            int fieldIndex = 0;
            for (auto& f : fields) {
                std::string typeName = GetTypeName(f->type);
                os << "    ["
                    << std::setw(4) << std::setfill('0') << (++fieldIndex) << "|"
                    << std::setw(4) << std::setfill('0') << fields.size() << "] "
                    << "|Offset: " << std::showpos << std::hex << std::uppercase << f->offset << std::dec << std::noshowpos << "| "
                    << (IsFieldStatic(f) ? "static" : "      ") << " "
                    << typeName << " "
                    << (f->name ? f->name : "<Unknown>") << ";\n";
            }
            os << "\n";

            // 函数
            void* methodIter = nullptr;
            const MethodInfo* method;
            std::vector<const MethodInfo*> methods;
            while ((method = il2cpp_class_get_methods(klass, &methodIter))) {
                methods.push_back(method);
            }

            os << "    // 函数数量:" << methods.size() << "\n";
            int methodIndex = 0;
            for (auto& m : methods) {
                std::string retType = GetTypeName(m->return_type);
                uintptr_t rva = reinterpret_cast<uintptr_t>(m->methodPointer) - baseAddr;

                os << "    [Flags: " << ToBinary32(m->flags) << "] [ParamCount: "
                    << std::setw(4) << std::setfill('0') << il2cpp_method_get_param_count(m) << "]\n";
                os << "    ["
                    << std::setw(4) << std::setfill('0') << (++methodIndex) << "|"
                    << std::setw(4) << std::setfill('0') << methods.size() << "] "
                    << "|RVA: +" << std::hex << std::showbase << std::uppercase << rva << std::dec << "| "
                    << (IsMethodStatic(m) ? "static" : "      ") << " "
                    << retType << " "
                    << (m->name ? m->name : "<Unknown>") << "(";

                int paramCount = il2cpp_method_get_param_count(m);
                for (int p = 0; p < paramCount; ++p) {
                    const Il2CppType* pt = il2cpp_method_get_param(m, p);
                    const char* pn = il2cpp_method_get_param_name(m, p);
                    os << GetTypeName(pt) << " " << (pn ? pn : "<param>");
                    if (p + 1 < paramCount) os << ", ";
                }
                os << ");\n\n";
            }

            os << "}\n\n";
        }
    }
}

void DumpCs3(std::string path) {
    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::ostringstream ss;
    RenderDumper3(ss);

    std::ofstream file(path, std::ios::out);
    if (file.is_open()) {
        file << ss.str();
        file.close();
        DebugPrintA("[DumpCs3] dump done\n");
    }
    else {
        DebugPrintA("[DumpCs3] Failed to open file: %s\n", path.c_str());
    }
}
