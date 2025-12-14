#include "pch.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <il2cpp-class-internals.h>
#include <il2cpp-object-internals.h>
#include <il2cpp-tabledefs.h>
#include "Il2CppFunctions.h"
#include "Il2CppDumper.h"
#include "PrintHelper.h"
#include "Util.h"

std::string GetMethodModifiers(uint16_t flags)
{
    std::string str;

    uint16_t access = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access)
    {
    case METHOD_ATTRIBUTE_PRIVATE:
        str += "private ";
        break;
    case METHOD_ATTRIBUTE_PUBLIC:
        str += "public ";
        break;
    case METHOD_ATTRIBUTE_FAMILY:
        str += "protected ";
        break;
    case METHOD_ATTRIBUTE_ASSEM:
    case METHOD_ATTRIBUTE_FAM_AND_ASSEM:
        str += "internal ";
        break;
    case METHOD_ATTRIBUTE_FAM_OR_ASSEM:
        str += "protected internal ";
        break;
    }

    if (flags & METHOD_ATTRIBUTE_STATIC)
        str += "static ";

    if (flags & METHOD_ATTRIBUTE_ABSTRACT)
    {
        str += "abstract ";
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            str += "override ";
    }
    else if (flags & METHOD_ATTRIBUTE_FINAL)
    {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            str += "sealed override ";
    }
    else if (flags & METHOD_ATTRIBUTE_VIRTUAL)
    {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT)
            str += "virtual ";
        else
            str += "override ";
    }

    if (flags & METHOD_ATTRIBUTE_PINVOKE_IMPL)
        str += "extern ";

    return str;
}

std::string GetFieldModifier(const FieldInfo* field)
{
    std::string rets = "";
    uint16_t attrs = field->type->attrs;
    auto access = attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
    switch (access) {
    case FIELD_ATTRIBUTE_PRIVATE:
        rets += "private ";
        break;
    case FIELD_ATTRIBUTE_PUBLIC:
        rets += "public ";
        break;
    case FIELD_ATTRIBUTE_FAMILY:
        rets += "protected ";
        break;
    case FIELD_ATTRIBUTE_ASSEMBLY:
    case FIELD_ATTRIBUTE_FAM_AND_ASSEM:
        rets += "internal ";
        break;
    case FIELD_ATTRIBUTE_FAM_OR_ASSEM:
        rets += "protected internal ";
        break;
    }
    if (attrs & FIELD_ATTRIBUTE_LITERAL) {
        rets += "const ";
    }
    else 
    {
        if (attrs & FIELD_ATTRIBUTE_STATIC) {
            rets += "static ";
        }
        if (attrs & FIELD_ATTRIBUTE_INIT_ONLY) {
            rets += "readonly ";
        }
    }
    return rets;
}

std::string GetClassModifier(Il2CppClass* klass)
{
    std::ostringstream outPut;
    auto flags = klass->flags;

    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE) {
        outPut << "[Serializable]\n";
    }

    auto layout = flags & TYPE_ATTRIBUTE_LAYOUT_MASK;
    switch (layout) {
    case TYPE_ATTRIBUTE_AUTO_LAYOUT:
        break;
    case TYPE_ATTRIBUTE_SEQUENTIAL_LAYOUT:
        outPut << "[StructLayout(LayoutKind.Sequential)]\n";
        break;
    case TYPE_ATTRIBUTE_EXPLICIT_LAYOUT:
        outPut << "[StructLayout(LayoutKind.Explicit)]\n";
        break;
    }

    auto is_valuetype = il2cpp_class_is_valuetype(klass);
    bool is_enum = klass->enumtype;

    auto visibility = flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
    switch (visibility) {
    case TYPE_ATTRIBUTE_PUBLIC:
    case TYPE_ATTRIBUTE_NESTED_PUBLIC:
        outPut << "public ";
        break;
    case TYPE_ATTRIBUTE_NOT_PUBLIC:
    case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
    case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
        outPut << "internal ";
        break;
    case TYPE_ATTRIBUTE_NESTED_PRIVATE:
        outPut << "private ";
        break;
    case TYPE_ATTRIBUTE_NESTED_FAMILY:
        outPut << "protected ";
        break;
    case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:
        outPut << "protected internal ";
        break;
    }

    if (flags & TYPE_ATTRIBUTE_ABSTRACT && flags & TYPE_ATTRIBUTE_SEALED) {
        outPut << "static ";
    }
    else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && flags & TYPE_ATTRIBUTE_ABSTRACT) {
        outPut << "abstract ";
    }
    else if (!is_valuetype && !is_enum && flags & TYPE_ATTRIBUTE_SEALED) {
        outPut << "sealed ";
    }

    if (flags & TYPE_ATTRIBUTE_INTERFACE) {
        outPut << "interface ";
    }
    else if (is_enum) {
        outPut << "enum ";
    }
    else if (is_valuetype) {
        outPut << "struct ";
    }
    else {
        outPut << "class ";
    }

    return outPut.str();
}

std::string TrimCsType(const std::string& type)
{
    std::string result;
    std::string token;
    int genericDepth = 0;

    auto FlushToken = [&]()
    {
        if (token.empty())
            return;

        size_t pos = token.find_last_of('.');
        if (pos != std::string::npos)
            result += token.substr(pos + 1);
        else
            result += token;

        token.clear();
    };

    for (size_t i = 0; i < type.size(); i++)
    {
        char c = type[i];

        if (c == '<')
        {
            FlushToken();
            result += '<';
            genericDepth++;
        }
        else if (c == '>')
        {
            FlushToken();
            result += '>';
            genericDepth--;
        }
        else if (c == ',')
        {
            FlushToken();
            result += ", ";
        }
        else if (c == '[' && i + 1 < type.size() && type[i + 1] == ']')
        {
            FlushToken();
            result += "[]";
            i++;
        }
        else
        {
            token += c;
        }
    }

    FlushToken();
    return result;
}

void DumpCsHeader(std::ostream& os)
{
    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain)
        return;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);
    if (!assemblies)
        return;

    uint32_t runningOffset = 0;

    for (size_t i = 0; i < assemblyCount; i++)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image || !image->name)
            continue;

        size_t classCount = il2cpp_image_get_class_count(image);

        os << "// Image " << i << ": " << image->name << " - " << runningOffset << "\n";
        runningOffset += (uint32_t)classCount;
    }

    os << "\n";
}

std::string GetTypeName(const Il2CppType* type)
{
    if (!type)
        return "void";

    switch (type->type)
    {
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
        break;
    }

    const char* typeName = il2cpp_type_get_name(type);
    if (!typeName)
        return "object";

    return TrimCsType(typeName);
}

void DumpEnum(std::ostream& os, Il2CppClass* klass, size_t tdi)
{
    const char* name = il2cpp_class_get_name(klass);
    const char* nsp = il2cpp_class_get_namespace(klass);

    os << "// Assembly: " << klass->image->name << "\n";

    if (nsp && nsp[0])
        os << "// Namespace: " << nsp << "\n";
    else
        os << "// Namespace:\n";

    os << GetClassModifier(klass) << name << " // TypeDefIndex: " << tdi << "\n" << "{\n";

    void* iter = nullptr;
    FieldInfo* field;

    while ((field = il2cpp_class_get_fields(klass, &iter)))
    {
        const char* fname = field->name;

        if (strcmp(fname, "value__") == 0)
            continue;

        if (!il2cpp_field_is_literal(field))
            continue;

        int64_t val = 0;
        il2cpp_field_static_get_value(field, &val);

        os << "\t" << fname << " = " << val << ",\n";
    }

    os << "}\n\n";
}

std::string DumpLiteralValue(FieldInfo* field)
{
    std::ostringstream os;
    const Il2CppType* type = field->type;

    switch (type->type)
    {
    case IL2CPP_TYPE_BOOLEAN:
    {
        bool v;
        il2cpp_field_static_get_value(field, &v);
        os << (v ? "true" : "false");
        break;
    }
    case IL2CPP_TYPE_CHAR:
    {
        wchar_t v;
        il2cpp_field_static_get_value(field, &v);
        std::wstring wstr(1, v);
        os << "'" << AsciiEscapeToEscapeLiterals(Utf16ToUtf8(wstr)) << "'";
        break;
    }
    case IL2CPP_TYPE_I1:
    {
        int8_t v;
        il2cpp_field_static_get_value(field, &v);
        os << static_cast<int>(v);
        break;
    }
    case IL2CPP_TYPE_U1:
    {
        uint8_t v;
        il2cpp_field_static_get_value(field, &v);
        os << static_cast<unsigned int>(v);
        break;
    }
    case IL2CPP_TYPE_I2:
    {
        int16_t v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_U2:
    {
        uint16_t v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_I4:
    {
        int32_t v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_U4:
    {
        uint32_t v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_I8:
    {
        int64_t v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_U8:
    {
        uint64_t v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_R4:
    {
        float v;
        il2cpp_field_static_get_value(field, &v);
        os << v << "f";
        break;
    }
    case IL2CPP_TYPE_R8:
    {
        double v;
        il2cpp_field_static_get_value(field, &v);
        os << v;
        break;
    }
    case IL2CPP_TYPE_STRING:
    {
        Il2CppString* s;
        il2cpp_field_static_get_value(field, &s);
        os << "\"" << AsciiEscapeToEscapeLiterals(Utf16ToUtf8(Il2cppToWString(s))) << "\"";
        break;
    }
    default:
    {
        // unsupported
        break;
    }
    }

    return os.str();
}

void DumpFields(std::ostream& os, Il2CppClass* klass)
{
    void* iter = nullptr;
    const FieldInfo* field;

    os << "\t// Fields\n";

    while ((field = il2cpp_class_get_fields(klass, &iter)))
    {
        const Il2CppType* type = field->type;
        std::string tname = GetTypeName(type);

        uint32_t offset = field->offset;

        std::string literal;
        if (field->type->attrs & FIELD_ATTRIBUTE_LITERAL) {
            literal = DumpLiteralValue(const_cast<FieldInfo*>(field));
        }

        os << "\t" << GetFieldModifier(field) << tname << " " << field->name;

        if (!literal.empty()) {
            os << " = " << literal;
		}

        os << "; // 0x" << std::hex << offset << std::dec << "\n";
    }

    os << "\n";
}

bool Il2CppTypeIsByref(const Il2CppType* type) {
    auto byref = type->byref;
    if (il2cpp_type_is_byref) {
        byref = il2cpp_type_is_byref(type);
    }
    return byref;
}

void GetMethodParamModifier(std::ostream& os, const Il2CppType* type, uint32_t attrs) {
    if (Il2CppTypeIsByref(type)) {
        if (attrs & PARAM_ATTRIBUTE_OUT && !(attrs & PARAM_ATTRIBUTE_IN)) {
            os << "out ";
        }
        else if (attrs & PARAM_ATTRIBUTE_IN && !(attrs & PARAM_ATTRIBUTE_OUT)) {
            os << "in ";
        }
        else {
            os << "ref ";
        }
    }
    else {
        if (attrs & PARAM_ATTRIBUTE_IN) {
            os << "[In] ";
        }
        if (attrs & PARAM_ATTRIBUTE_OUT) {
            os << "[Out] ";
        }
    }
}

void DumpMethods(std::ostream& os, Il2CppClass* klass) {
    void* iter = nullptr;
    const MethodInfo* method;

    os << "\t// Methods\n\n";

    while ((method = il2cpp_class_get_methods(klass, &iter))) {
        uintptr_t va = (uintptr_t)method->methodPointer;
        uintptr_t rva = va - (uintptr_t)GetModuleHandle(L"GameAssembly.dll");

        os << "\t// RVA: 0x" << std::hex << rva << " VA: 0x" << va << std::dec << " // Slot: " << method->slot << "\n";

        auto returnType = method->return_type;
        auto flags = method->flags;
        std::string ret = GetTypeName(returnType);
        std::string modifier = GetMethodModifiers(method->flags);
		std::string methodName = method->name;

        os << "\t" << modifier;
        if (Il2CppTypeIsByref(returnType)) {
            os << "ref ";
        }

        os << ret << " " << methodName << "(";

        auto param_count = il2cpp_method_get_param_count(method);
        for (int i = 0; i < param_count; ++i) {
            const Il2CppType* p = il2cpp_method_get_param(method, i);
            auto attrs = p->attrs;
            const char* pname = il2cpp_method_get_param_name(method, i);
            GetMethodParamModifier(os, p, attrs);

            os << GetTypeName(p) << " " << pname;

            if (i + 1 < param_count)
                os << ", ";
        }

        os << ") { }\n\n";
    }
}

void DumpClass(std::ostream& outPut, Il2CppClass* klass, size_t tdi)
{
    if (klass->enumtype)
    {
        DebugPrintA("[DumpCs] Dumping enum: %s\n", klass->name);
        DumpEnum(outPut, klass, tdi);
        return;
    }

    DebugPrintA("[DumpCs] Dumping class: %s\n", klass->name);

    const char* name = klass->name;
    const char* nsp = klass->namespaze;

    outPut << "// Assembly: " << klass->image->name << "\n";

    if (nsp && nsp[0])
        outPut << "// Namespace: " << nsp << "\n";
    else
        outPut << "// Namespace:\n";

    std::vector<std::string> extends;

    Il2CppClass* parent = klass->parent;
    bool is_valuetype = il2cpp_class_is_valuetype(klass);

    if (!is_valuetype && parent) {
        Il2CppType* parent_type = &parent->byval_arg;

        if (parent_type->type != IL2CPP_TYPE_OBJECT) {
            if (parent->name) {
                extends.emplace_back(parent->name);
            }
        }
    }

    if (klass->interfaces_count > 0 && klass->implementedInterfaces) {
        for (uint16_t i = 0; i < klass->interfaces_count; ++i) {
            Il2CppClass* interfaceClass = klass->implementedInterfaces[i];
            if (interfaceClass && interfaceClass->name) {
                extends.emplace_back(interfaceClass->name);
            }
        }
    }

    outPut << GetClassModifier(klass) << name;

    if (!extends.empty()) {
        outPut << " : " << extends[0];
        for (size_t i = 1; i < extends.size(); ++i) {
            outPut << ", " << extends[i];
        }
    }

    outPut << " // TypeDefIndex: " << tdi << "\n" << "{\n";

    DumpFields(outPut, klass);
    DumpMethods(outPut, klass);

    outPut << "}\n\n";
}

void DumpClasses(std::ostream& os)
{
    Il2CppDomain* domain = il2cpp_domain_get();
    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);

    for (size_t i = 0; i < assemblyCount; i++)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image) continue;

        size_t classCount = il2cpp_image_get_class_count(image);

        for (size_t c = 0; c < classCount; c++)
        {
            const Il2CppClass* klass = il2cpp_image_get_class(image, c);
            if (!klass) continue;

            DumpClass(os, const_cast<Il2CppClass*>(klass), c);
        }
    }
}

void DumpCs(std::string path)
{
    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::ofstream file(path);
    if (file.is_open()) {
		DebugPrintA("[DumpCs] dumping...\n");

        std::ostringstream ss;

        DumpCsHeader(ss);
        DumpClasses(ss);

        file << ss.str();
        file.close();
        DebugPrintA("[DumpCs] dump done!\n");
    }
    else 
    {
        DebugPrintA("[ERROR] Failed to open file for writing: %s\n", path);
    }
}
