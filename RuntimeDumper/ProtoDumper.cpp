#include "pch.h"
#include "ProtoDumper.h"
#include "Il2CppFunctions.h"
#include <vector>
#include <sstream>
#include "PrintHelper.h"
#include "Util.h"
#include <filesystem>
#include <unordered_map>
#include <fstream>

const bool FIX_ENUM = true;
const char* PROTOBUF_IMAGE = "Assembly-CSharp.dll";

static Il2CppClass* _originalNameAttr = nullptr;

bool HasGoogleProtobuf()
{
    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain)
        return false;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);

    for (size_t i = 0; i < assemblyCount; ++i)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image)
            continue;

        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t c = 0; c < classCount; ++c)
        {
            Il2CppClass* klass = const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
			const char* nsp = il2cpp_class_get_namespace(klass);
            if (!klass || !nsp)
                continue;

            if (strncmp(nsp, "Google.Protobuf", 15) == 0)
            {
                DebugPrintA("[ProtoDump] Found Google.Protobuf namespace in %s (%s.%s)\n",image->name, nsp, klass->name);
                return true;
            }
        }
    }

    return false;
}

Il2CppClass* FindClass(const char* imageName, const char* namespaze, const char* className)
{
    if (!className)
        return nullptr;

    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain)
        return nullptr;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);

    for (size_t i = 0; i < assemblyCount; ++i)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image || !image->name)
            continue;

        if (imageName && strcmp(image->name, imageName) != 0)
            continue;

        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t c = 0; c < classCount; ++c)
        {
            Il2CppClass* klass = const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
            if (!klass)
                continue;

            if (strcmp(klass->name, className) != 0)
                continue;

            if (namespaze)
            {
                if (!klass->namespaze || strcmp(klass->namespaze, namespaze) != 0)
                    continue;
            }

            return klass;
        }
    }

    return nullptr;
}

void InitProtobufAttributes()
{
    if (!_originalNameAttr)
        _originalNameAttr = FindClass(nullptr, "Google.Protobuf.Reflection", "OriginalNameAttribute");

    if (!_originalNameAttr)
        DebugPrintA("[ProtoDump] OriginalNameAttribute not found\n");
}

bool IsInGoogleProtobufNamespace(Il2CppClass* klass)
{
    const char* ns = il2cpp_class_get_namespace(klass);
    if (!ns || !*ns)
        return false;

    return strcmp(ns, "Google.Protobuf") == 0 || strncmp(ns, "Google.Protobuf.", 16) == 0;
}

bool IsProtobufMessage(Il2CppClass* klass)
{
    if (!klass) return false;

    if (IsInGoogleProtobufNamespace(klass)) return false;
    if (il2cpp_class_is_valuetype(klass) || klass->enumtype) return false;

    void* iter = nullptr;
    Il2CppClass* iface = nullptr;

    while ((iface = il2cpp_class_get_interfaces(klass, &iter)))
    {
        if (!iface || !iface->namespaze || !iface->name)
            continue;

        if (strcmp(iface->namespaze, "Google.Protobuf") != 0)
            continue;

        if (strncmp(iface->name, "IMessage", 8) == 0)
            return true;
    }

    return false;
}

bool IsProtobufEnum(Il2CppClass* klass)
{
    if (!il2cpp_class_is_enum(klass))
        return false;

    if (il2cpp_class_get_declaring_type(klass) != nullptr)
        return false;

    if (IsInGoogleProtobufNamespace(klass))
        return false;

    void* iter = nullptr;
    FieldInfo* field = nullptr;

    while ((field = il2cpp_class_get_fields(klass, &iter)) != nullptr)
    {
        if (!il2cpp_field_is_literal(field))
            continue;

        if (il2cpp_field_has_attribute(field, _originalNameAttr))
            return true;
    }

    return false;
}

std::vector<Il2CppClass*> GetAllProtobufMessages()
{
    std::vector<Il2CppClass*> messages;

    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain) return messages;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);

    for (size_t i = 0; i < assemblyCount; ++i)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image)
            continue;

        if (PROTOBUF_IMAGE && strcmp(image->name, PROTOBUF_IMAGE) != 0)
            continue;

        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t c = 0; c < classCount; ++c)
        {
            Il2CppClass* klass =
                const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
            if (!klass) continue;

            if (il2cpp_class_is_valuetype(klass) || klass->enumtype)
                continue;

            if (klass->declaringType != nullptr)
                continue;

            if (IsProtobufMessage(klass))
                messages.push_back(klass);
        }
    }

    return messages;
}

std::vector<Il2CppClass*> GetAllProtobufEnums()
{
    std::vector<Il2CppClass*> result;
    InitProtobufAttributes();
    if (!_originalNameAttr)
        return result;

    Il2CppDomain* domain = il2cpp_domain_get();
    if (!domain)
        return result;

    size_t assemblyCount = 0;
    const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(domain, &assemblyCount);

    for (size_t i = 0; i < assemblyCount; ++i)
    {
        const Il2CppImage* image = il2cpp_assembly_get_image(assemblies[i]);
        if (!image)
            continue;

        if (PROTOBUF_IMAGE && strcmp(image->name, PROTOBUF_IMAGE) != 0)
            continue;

        size_t classCount = il2cpp_image_get_class_count(image);
        for (size_t c = 0; c < classCount; ++c)
        {
            Il2CppClass* klass = const_cast<Il2CppClass*>(il2cpp_image_get_class(image, c));
            if (!klass) continue;

            if (IsProtobufEnum(klass))
                result.push_back(klass);
        }
    }

    return result;
}

std::string EnumToString(Il2CppClass* enumClass, int value)
{
    if (!enumClass)
        return "";

    void* iter = nullptr;
    FieldInfo* field = nullptr;

    while ((field = il2cpp_class_get_fields(enumClass, &iter)) != nullptr)
    {
        // 枚举的 value__ 字段不要
        if (strcmp(field->name, "value__") == 0)
            continue;

        // 只处理字面量字段
        if (!il2cpp_field_is_literal(field))
            continue;

        int64_t fieldValue = 0;
        il2cpp_field_static_get_value(field, &fieldValue);

        if (fieldValue == value)
        {
            return field->name;
        }
    }

    // 找不到匹配值就返回 unknown
    return "";
}

// 获取 FieldType 名称
std::wstring GetFieldTypeString(Il2CppObject* field)
{
    if (!field)
        return L"unknown";

    Il2CppClass* fieldClass = il2cpp_object_get_class(field);
    if (!fieldClass)
        return L"unknown";

    // 获取 FieldType 枚举值
    Il2CppObject* fieldTypeValue = nullptr;
    {
        void* iter = nullptr;
        const MethodInfo* method = nullptr;
        while ((method = il2cpp_class_get_methods(fieldClass, &iter)) != nullptr)
        {
            const char* name = il2cpp_method_get_name(method);
            if (!name) continue;
            if (strcmp(name, "get_FieldType") != 0) continue;
            if (!il2cpp_method_is_instance(method)) continue;
            if (il2cpp_method_get_param_count(method) != 0) continue;

            Il2CppException* exc = nullptr;
            fieldTypeValue = il2cpp_runtime_invoke(method, field, nullptr, &exc);
            if (exc || !fieldTypeValue)
                return L"unknown";
            break;
        }
    }

    if (!fieldTypeValue)
        return L"unknown";

    // 获取 FieldType 枚举名称
    Il2CppClass* fieldTypeEnumClass = FindClass(nullptr, "Google.Protobuf.Reflection", "FieldType");
    if (!fieldTypeEnumClass)
        return L"unknown";

    int fieldTypeInt = *(int*)il2cpp_object_unbox(fieldTypeValue);
    std::string fieldTypeName = EnumToString(fieldTypeEnumClass, fieldTypeInt);
    if (fieldTypeName.empty())
        return L"unknown";

    for (auto& c : fieldTypeName) c = tolower(c); // 转小写
    std::wstring wFieldType = Utf8ToUtf16(fieldTypeName);

    // 处理基本类型
    if (wFieldType == L"int32") return L"int32";
    if (wFieldType == L"int64") return L"int64";
    if (wFieldType == L"uint32") return L"uint32";
    if (wFieldType == L"uint64") return L"uint64";
    if (wFieldType == L"sint32") return L"sint32";
    if (wFieldType == L"sint64") return L"sint64";
    if (wFieldType == L"fixed32") return L"fixed32";
    if (wFieldType == L"fixed64") return L"fixed64";
    if (wFieldType == L"sfixed32") return L"sfixed32";
    if (wFieldType == L"sfixed64") return L"sfixed64";
    if (wFieldType == L"float") return L"float";
    if (wFieldType == L"double") return L"double";
    if (wFieldType == L"bool") return L"bool";
    if (wFieldType == L"string") return L"string";
    if (wFieldType == L"bytes") return L"bytes";

    // enum 类型
    if (wFieldType == L"enum")
    {
        // 获取 EnumDescriptor
        const MethodInfo* getEnumType = il2cpp_class_get_method_from_name(fieldClass, "get_EnumType", 0);
        if (!getEnumType) return L"unknown_enum";

        Il2CppException* exc = nullptr;
        Il2CppObject* enumDescriptor = il2cpp_runtime_invoke(getEnumType, field, nullptr, &exc);
        if (exc || !enumDescriptor) return L"unknown_enum";

        // 调用 EnumDescriptor.get_Name()
        Il2CppClass* enumDescClass = il2cpp_object_get_class(enumDescriptor);
        const MethodInfo* getNameMethod = il2cpp_class_get_method_from_name(enumDescClass, "get_Name", 0);
        if (!getNameMethod) return L"unknown_enum";

        exc = nullptr;
        Il2CppObject* nameObj = il2cpp_runtime_invoke(getNameMethod, enumDescriptor, nullptr, &exc);
        if (exc || !nameObj) return L"unknown_enum";

        return Il2cppToWString((Il2CppString*)nameObj);
    }

    if (wFieldType == L"message")
    {
        // 获取 MessageDescriptor
        const MethodInfo* getMessageType = il2cpp_class_get_method_from_name(fieldClass, "get_MessageType", 0);
        if (!getMessageType) return L"unknown_message";

        Il2CppException* exc = nullptr;
        Il2CppObject* msgDescriptor = il2cpp_runtime_invoke(getMessageType, field, nullptr, &exc);
        if (exc || !msgDescriptor) return L"unknown_message";

        // 调用 MessageDescriptor.get_Name()
        Il2CppClass* msgDescClass = il2cpp_object_get_class(msgDescriptor);
        const MethodInfo* getNameMethod = il2cpp_class_get_method_from_name(msgDescClass, "get_Name", 0);
        if (!getNameMethod) return L"unknown_message";

        exc = nullptr;
        Il2CppObject* nameObj = il2cpp_runtime_invoke(getNameMethod, msgDescriptor, nullptr, &exc);
        if (exc || !nameObj) return L"unknown_message";

        return Il2cppToWString((Il2CppString*)nameObj);
    }

    return wFieldType;
}

// 获取 protobuf 字段名字
std::wstring GetFieldName(Il2CppObject* field)
{
    if (!field)
        return L"unknown";

    Il2CppClass* klass = il2cpp_object_get_class(field);
    if (!klass)
        return L"unknown";

    // 找 Name 属性
    const MethodInfo* getNameMethod = il2cpp_class_get_method_from_name(klass, "get_Name", 0);
    if (!getNameMethod)
        return L"unknown";

    Il2CppException* exc = nullptr;
    Il2CppObject* result = il2cpp_runtime_invoke(getNameMethod, field, nullptr, &exc);
    if (exc || !result)
        return L"unknown";

    return Il2cppToWString((Il2CppString*)result);
}

// 获取 protobuf 字段编号
int GetFieldNumber(Il2CppObject* field)
{
    if (!field)
        return 0;

    Il2CppClass* klass = il2cpp_object_get_class(field);
    if (!klass)
        return 0;

    // 找 FieldNumber 属性
    const MethodInfo* getNumberMethod = il2cpp_class_get_method_from_name(klass, "get_FieldNumber", 0);
    if (!getNumberMethod)
        return 0;

    Il2CppException* exc = nullptr;
    Il2CppObject* result = il2cpp_runtime_invoke(getNumberMethod, field, nullptr, &exc);
    if (exc || !result)
        return 0;

    // 枚举值一般是 int
    return *(int*)il2cpp_object_unbox(result);
}

Il2CppObject* GetFieldsCollection(Il2CppObject* messageDescriptor)
{
    if (!messageDescriptor) return nullptr;

    Il2CppClass* descriptorClass = il2cpp_object_get_class(messageDescriptor);
    if (!descriptorClass) return nullptr;

    // 获取 get_Fields 方法
    const MethodInfo* getFieldsMethod = il2cpp_class_get_method_from_name(descriptorClass, "get_Fields", 0);
    if (!getFieldsMethod)
    {
        DebugPrintA("[ProtoDump] get_Fields method not found for %s\n", il2cpp_class_get_name(descriptorClass));
        return nullptr;
    }

    Il2CppException* exc = nullptr;
    Il2CppObject* fieldsCollection = il2cpp_runtime_invoke(getFieldsMethod, messageDescriptor, nullptr, &exc);

    if (exc)
    {
        DebugPrintA("[ProtoDump] Exception when calling get_Fields\n");
        return nullptr;
    }

    if (!fieldsCollection)
    {
        DebugPrintA("[ProtoDump] get_Fields returned null\n");
        return nullptr;
    }

    //DebugPrintA("[ProtoDump] Successfully got FieldCollection for %s\n", il2cpp_class_get_name(descriptorClass));
    return fieldsCollection;
}

std::vector<Il2CppObject*> GetAllFields(Il2CppObject* messageDescriptor)
{
    std::vector<Il2CppObject*> fields;

    if (!messageDescriptor)
        return fields;

    Il2CppObject* fieldCollection = GetFieldsCollection(messageDescriptor);
    if (!fieldCollection)
        return fields;

    Il2CppClass* fcClass = il2cpp_object_get_class(fieldCollection);
    if (!fcClass)
        return fields;

    // FieldCollection.InDeclarationOrder() 方法返回 IList<FieldDescriptor>
    const MethodInfo* inDeclOrderMethod = il2cpp_class_get_method_from_name(fcClass, "InDeclarationOrder", 0);
    if (!inDeclOrderMethod)
    {
        DebugPrintA("[ProtoDump] InDeclarationOrder method not found\n");
        return fields;
    }

    Il2CppException* exc = nullptr;
    Il2CppObject* fieldListObj = il2cpp_runtime_invoke(inDeclOrderMethod, fieldCollection, nullptr, &exc);
    if (exc || !fieldListObj)
    {
        DebugPrintA("[ProtoDump] Failed to get fields in declaration order\n");
        return fields;
    }

    Il2CppClass* listClass = il2cpp_object_get_class(fieldListObj);
    if (!listClass)
        return fields;

    const MethodInfo* getItemMethod = il2cpp_class_get_method_from_name(listClass, "get_Item", 1);
    if (!getItemMethod)
    {
        DebugPrintA("[ProtoDump] IList get_Item method not found\n");
        return fields;
    }

    const MethodInfo* countMethod = il2cpp_class_get_method_from_name(listClass, "get_Count", 0);
    if (!countMethod)
    {
        DebugPrintA("[ProtoDump] IList get_Count method not found\n");
        return fields;
    }

    exc = nullptr;
    Il2CppObject* countObj = il2cpp_runtime_invoke(countMethod, fieldListObj, nullptr, &exc);
    if (exc || !countObj)
        return fields;

    int count = *(int*)il2cpp_object_unbox(countObj);

    for (int i = 0; i < count; ++i)
    {
        void* args[1] = { &i };
        Il2CppException* excField = nullptr;
        Il2CppObject* field = il2cpp_runtime_invoke(getItemMethod, fieldListObj, args, &excField);
        if (!excField && field)
        {
            fields.push_back(field);
        }
    }

    return fields;
}

Il2CppObject* GetMessageDescriptor(Il2CppClass* messageClass)
{
    void* iter = nullptr;
    const MethodInfo* method = nullptr;

    while ((method = il2cpp_class_get_methods(messageClass, &iter)) != nullptr)
    {
        const char* name = il2cpp_method_get_name(method);
        if (!name || strcmp(name, "get_Descriptor") != 0)
            continue;

        if (il2cpp_method_is_instance(method)) // must be static
            continue;

        if (il2cpp_method_get_param_count(method) != 0)
            continue;

        Il2CppException* exc = nullptr;
        Il2CppObject* descriptor = il2cpp_runtime_invoke(method, nullptr, nullptr, &exc);
        if (exc) return nullptr;

        return descriptor;
    }

    return nullptr;
}

std::wstring GetDescriptorName(Il2CppObject* descriptor)
{
    if (!descriptor) return L"";

    Il2CppClass* klass = il2cpp_object_get_class(descriptor);

    void* iter = nullptr;
    const MethodInfo* method = nullptr;

    while ((method = il2cpp_class_get_methods(klass, &iter)) != nullptr)
    {
        const char* name = il2cpp_method_get_name(method);
        if (!name || strcmp(name, "get_Name") != 0)
            continue;

        if (!il2cpp_method_is_instance(method))
            continue;

        if (il2cpp_method_get_param_count(method) != 0)
            continue;

        Il2CppException* exc = nullptr;
        Il2CppObject* result = il2cpp_runtime_invoke(method, descriptor, nullptr, &exc);
        if (exc || !result)
            return L"";

        return Il2cppToWString((Il2CppString*)result);
    }

    return L"";
}

std::wstring GetFieldDefinition(Il2CppObject* field)
{
    if (!field) return L"";

    std::wstring fieldName = GetFieldName(field);
    int fieldNumber = GetFieldNumber(field);

    // 判断是否 Map
    Il2CppClass* fieldClass = il2cpp_object_get_class(field);
    const MethodInfo* isMapMethod = il2cpp_class_get_method_from_name(fieldClass, "get_IsMap", 0);
    bool isMap = false;

    if (isMapMethod)
    {
        Il2CppException* exc = nullptr;
        Il2CppObject* res = il2cpp_runtime_invoke(isMapMethod, field, nullptr, &exc);
        if (!exc && res)
            isMap = *(bool*)il2cpp_object_unbox(res);
    }

    if (isMap)
    {
        // 获取 MessageType，即 MapEntryDescriptor
        const MethodInfo* getMessageType = il2cpp_class_get_method_from_name(fieldClass, "get_MessageType", 0);
        if (!getMessageType) return L"// Error: Map MessageType missing";

        Il2CppException* exc = nullptr;
        Il2CppObject* mapEntryDescriptor = il2cpp_runtime_invoke(getMessageType, field, nullptr, &exc);
        if (exc || !mapEntryDescriptor) return L"// Error: Map MessageType invoke failed";

        // 获取字段集合 FieldCollection
        Il2CppObject* mapFieldsCollection = GetFieldsCollection(mapEntryDescriptor);
        if (!mapFieldsCollection) return L"// Error: Map FieldCollection missing";

        auto mapFields = GetAllFields(mapEntryDescriptor);
        if (mapFields.size() < 2)
            return L"// Error: Map fields count != 2";

        // 第一个是 key，第二个是 value
        std::wstring keyType = GetFieldTypeString(mapFields[0]);
        std::wstring valueType = GetFieldTypeString(mapFields[1]);

        return L"map<" + keyType + L", " + valueType + L"> " + fieldName + L" = " + std::to_wstring(fieldNumber) + L";";
    }
    else
    {
        // 判断是否 repeated
        const MethodInfo* isRepeatedMethod = il2cpp_class_get_method_from_name(fieldClass, "get_IsRepeated", 0);
        bool isRepeated = false;

        if (isRepeatedMethod)
        {
            Il2CppException* exc = nullptr;
            Il2CppObject* res = il2cpp_runtime_invoke(isRepeatedMethod, field, nullptr, &exc);
            if (!exc && res)
                isRepeated = *(bool*)il2cpp_object_unbox(res);
        }

        std::wstring typeStr = GetFieldTypeString(field);
        std::wstring repeatedStr = isRepeated ? L"repeated " : L"";

        return repeatedStr + typeStr + L" " + fieldName + L" = " + std::to_wstring(fieldNumber) + L";";
    }
}

std::wstring GetIndent(int indentLevel)
{
    return std::wstring(indentLevel * 4, L' ');
}

std::vector<Il2CppObject*> GetAllNestedTypes(Il2CppObject* messageDescriptor)
{
    std::vector<Il2CppObject*> result;
    if (!messageDescriptor) return result;

    Il2CppClass* descClass = il2cpp_object_get_class(messageDescriptor);
    if (!descClass) return result;

    const MethodInfo* getNestedMethod = il2cpp_class_get_method_from_name(descClass, "get_NestedTypes", 0);
    if (!getNestedMethod) return result;

    Il2CppException* exc = nullptr;
    Il2CppObject* listObj = il2cpp_runtime_invoke(getNestedMethod, messageDescriptor, nullptr, &exc);
    if (exc || !listObj) return result;

    // listObj 是 IList<MessageDescriptor>
    Il2CppClass* listClass = il2cpp_object_get_class(listObj);
    const MethodInfo* countMethod = il2cpp_class_get_method_from_name(listClass, "get_Count", 0);
    const MethodInfo* itemMethod = il2cpp_class_get_method_from_name(listClass, "get_Item", 1);
    if (!countMethod || !itemMethod) return result;

    exc = nullptr;
    Il2CppObject* countObj = il2cpp_runtime_invoke(countMethod, listObj, nullptr, &exc);
    if (exc || !countObj) return result;
    int count = *(int*)il2cpp_object_unbox(countObj);

    for (int i = 0; i < count; ++i)
    {
        void* args[1] = { &i };
        Il2CppException* excField = nullptr;
        Il2CppObject* nested = il2cpp_runtime_invoke(itemMethod, listObj, args, &excField);
        if (!excField && nested)
            result.push_back(nested);
    }

    return result;
}

std::vector<Il2CppObject*> GetAllEnumTypes(Il2CppObject* messageDescriptor)
{
    std::vector<Il2CppObject*> result;
    if (!messageDescriptor) return result;

    Il2CppClass* descClass = il2cpp_object_get_class(messageDescriptor);
    if (!descClass) return result;

    const MethodInfo* getEnumMethod = il2cpp_class_get_method_from_name(descClass, "get_EnumTypes", 0);
    if (!getEnumMethod) return result;

    Il2CppException* exc = nullptr;
    Il2CppObject* listObj = il2cpp_runtime_invoke(getEnumMethod, messageDescriptor, nullptr, &exc);
    if (exc || !listObj) return result;

    // listObj 是 IList<EnumDescriptor>
    Il2CppClass* listClass = il2cpp_object_get_class(listObj);
    const MethodInfo* countMethod = il2cpp_class_get_method_from_name(listClass, "get_Count", 0);
    const MethodInfo* itemMethod = il2cpp_class_get_method_from_name(listClass, "get_Item", 1);
    if (!countMethod || !itemMethod) return result;

    exc = nullptr;
    Il2CppObject* countObj = il2cpp_runtime_invoke(countMethod, listObj, nullptr, &exc);
    if (exc || !countObj) return result;
    int count = *(int*)il2cpp_object_unbox(countObj);

    for (int i = 0; i < count; ++i)
    {
        void* args[1] = { &i };
        Il2CppException* excField = nullptr;
        Il2CppObject* e = il2cpp_runtime_invoke(itemMethod, listObj, args, &excField);
        if (!excField && e)
            result.push_back(e);
    }

    return result;
}

void GenerateEnumDefinitionByDescriptor(Il2CppObject* enumDescriptor, std::wstringstream& out, int indentLevel = 0)
{
    if (!enumDescriptor) return;

    std::wstring indent = GetIndent(indentLevel);

    Il2CppClass* descClass = il2cpp_object_get_class(enumDescriptor);
    if (!descClass) return;

    // 获取 Name
    const MethodInfo* getNameMethod = il2cpp_class_get_method_from_name(descClass, "get_Name", 0);
    if (!getNameMethod) return;

    Il2CppException* exc = nullptr;
    Il2CppObject* nameObj = il2cpp_runtime_invoke(getNameMethod, enumDescriptor, nullptr, &exc);
    if (exc || !nameObj) return;

    std::wstring enumName = Il2cppToWString((Il2CppString*)nameObj);

    // 输出注释
    out << indent << L"// Enum Descriptor Generation\n";
    out << indent << L"enum " << enumName << L" {\n";

    // 获取 Values
    const MethodInfo* getValuesMethod = il2cpp_class_get_method_from_name(descClass, "get_Values", 0);
    if (!getValuesMethod) return;

    exc = nullptr;
    Il2CppObject* valueListObj = il2cpp_runtime_invoke(getValuesMethod, enumDescriptor, nullptr, &exc);
    if (exc || !valueListObj) return;

    Il2CppClass* listClass = il2cpp_object_get_class(valueListObj);
    const MethodInfo* countMethod = il2cpp_class_get_method_from_name(listClass, "get_Count", 0);
    const MethodInfo* itemMethod = il2cpp_class_get_method_from_name(listClass, "get_Item", 1);
    if (!countMethod || !itemMethod) return;

    exc = nullptr;
    Il2CppObject* countObj = il2cpp_runtime_invoke(countMethod, valueListObj, nullptr, &exc);
    if (exc || !countObj) return;

    int count = *(int*)il2cpp_object_unbox(countObj);

    for (int i = 0; i < count; ++i)
    {
        void* args[1] = { &i };
        Il2CppException* excValue = nullptr;
        Il2CppObject* val = il2cpp_runtime_invoke(itemMethod, valueListObj, args, &excValue);
        if (!excValue && val)
        {
            // FieldDescriptor 的 Name 和 Number
            Il2CppClass* valClass = il2cpp_object_get_class(val);
            const MethodInfo* getFieldName = il2cpp_class_get_method_from_name(valClass, "get_Name", 0);
            const MethodInfo* getFieldNumber = il2cpp_class_get_method_from_name(valClass, "get_Number", 0);
            if (!getFieldName || !getFieldNumber) continue;

            Il2CppObject* fNameObj = il2cpp_runtime_invoke(getFieldName, val, nullptr, &excValue);
            Il2CppObject* fNumObj = il2cpp_runtime_invoke(getFieldNumber, val, nullptr, &excValue);
            if (excValue || !fNameObj || !fNumObj) continue;

            std::wstring fieldName = Il2cppToWString((Il2CppString*)fNameObj);
            int fieldNumber = *(int*)il2cpp_object_unbox(fNumObj);

            out << indent << L"    " << fieldName << L" = " << fieldNumber << L";\n";
        }
    }

    out << indent << L"}\n\n";
}

void GenerateMessageDefinitionByDescriptor(Il2CppObject* descriptor, std::wstringstream& out, int indentLevel = 0)
{
    if (!descriptor) return;

    // 获取 Message 名称
    std::wstring name = GetDescriptorName(descriptor);
    if (name.empty()) return;

    // 过滤 MapEntry 消息
    if (name.size() >= 5 && std::equal(name.end() - 5, name.end(), L"Entry"))
        return;

    std::wstring indent = GetIndent(indentLevel);

    out << indent << L"// Message Descriptor Generation\n";
    out << indent << L"message " << name << L" {\n";

    // --- oneof + 普通字段 ---
    auto fields = GetAllFields(descriptor);
    std::unordered_map<Il2CppObject*, std::vector<Il2CppObject*>> oneofGroups;

    for (auto field : fields)
    {
        Il2CppObject* containingOneof = nullptr;
        const MethodInfo* getOneof = il2cpp_class_get_method_from_name(il2cpp_object_get_class(field), "get_ContainingOneof", 0);
        if (getOneof)
        {
            Il2CppException* exc = nullptr;
            containingOneof = il2cpp_runtime_invoke(getOneof, field, nullptr, &exc);
        }
        if (containingOneof)
            oneofGroups[containingOneof].push_back(field);
    }

    for (auto& [oneofDesc, fieldList] : oneofGroups)
    {
        const MethodInfo* getName = il2cpp_class_get_method_from_name(il2cpp_object_get_class(oneofDesc), "get_Name", 0);
        std::wstring oneofName = L"unknown_oneof";
        if (getName)
        {
            Il2CppException* exc = nullptr;
            Il2CppObject* nameObj = il2cpp_runtime_invoke(getName, oneofDesc, nullptr, &exc);
            if (!exc && nameObj)
                oneofName = Il2cppToWString((Il2CppString*)nameObj);
        }

        out << indent << L"    oneof " << oneofName << L" {\n";
        for (auto field : fieldList)
            out << indent << L"        " << GetFieldDefinition(field) << L"\n";
        out << indent << L"    }\n";
    }

    for (auto field : fields)
    {
        Il2CppObject* containingOneof = nullptr;
        const MethodInfo* getOneof = il2cpp_class_get_method_from_name(il2cpp_object_get_class(field), "get_ContainingOneof", 0);
        if (getOneof)
        {
            Il2CppException* exc = nullptr;
            containingOneof = il2cpp_runtime_invoke(getOneof, field, nullptr, &exc);
        }
        if (containingOneof) continue;

        out << indent << L"    " << GetFieldDefinition(field) << L"\n";
    }

    // 嵌套枚举
    auto nestedEnums = GetAllEnumTypes(descriptor);
    for (auto e : nestedEnums)
        GenerateEnumDefinitionByDescriptor(e, out, indentLevel + 1);

    // 嵌套消息
    auto nestedMessages = GetAllNestedTypes(descriptor);
    for (auto nested : nestedMessages)
        GenerateMessageDefinitionByDescriptor(nested, out, indentLevel + 1);

    out << indent << L"}\n\n";
}

void GenerateEnumDefinition(Il2CppClass* enumClass, std::wstringstream& out, int indentLevel = 0)
{
    std::wstring indent = GetIndent(indentLevel);
    std::string className = il2cpp_class_get_name(enumClass);
    std::wstring wClassName = Utf8ToUtf16(className);
    std::wstring wNsp = Utf8ToUtf16(enumClass->namespaze);
    std::wstring wAsm = Utf8ToUtf16(enumClass->image->name);

    out << indent << L"// Class: " << wClassName << L", Namespaze: " << wNsp << L", Assembly: " << wAsm << L"\n";
    out << indent << L"// Il2Cpp Class Generation\n";
    out << indent << L"enum " << wClassName << L" {\n";

    void* iter = nullptr;
    FieldInfo* field = nullptr;
    while ((field = il2cpp_class_get_fields(enumClass, &iter)) != nullptr)
    {
        if (!il2cpp_field_is_literal(field)) continue;
        const char* name = il2cpp_field_get_name(field);
        if (!name) continue;

        std::wstring wname = Utf8ToUtf16(name);
        if (FIX_ENUM) wname = wClassName + L"_" + wname;

        int value;
        il2cpp_field_static_get_value(field, &value);

        out << indent << L"    " << wname << L" = " << value << L";\n";
    }

    out << indent << L"}\n\n";
}

void GenerateMessageDefinition(Il2CppClass* messageClass, std::wstringstream& out, int indentLevel = 0)
{
    if (!messageClass) return;

    Il2CppObject* descriptor = GetMessageDescriptor(messageClass);
    if (!descriptor) return;

    std::string className = il2cpp_class_get_name(messageClass);
    std::wstring wClassName = Utf8ToUtf16(className);
    std::wstring wNsp = Utf8ToUtf16(messageClass->namespaze ? messageClass->namespaze : "");
    std::wstring wAsm = Utf8ToUtf16(messageClass->image ? messageClass->image->name : "");

    std::wstring indent = GetIndent(indentLevel);
    out << indent << L"// Class: " << wClassName << L", Namespaze: " << wNsp << L", Assembly: " << wAsm << L"\n";

    // 解析 descriptor 生成 proto
    GenerateMessageDefinitionByDescriptor(descriptor, out, indentLevel);
}

// 主函数
void DumpProtos(std::string path)
{
    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::wofstream file(path);
    if (file.is_open()) {
        if (!HasGoogleProtobuf())
        {
            DebugPrintA("[ProtoDump] Google.Protobuf not found, skip proto dump\n");
            return;
        }

        auto messages = GetAllProtobufMessages();
        auto enums = GetAllProtobufEnums();

        std::wstringstream ss;
        ss << L"// Runtime Dumper\n\n";
        ss << L"syntax = \"proto3\";\n\n";

        for (auto e : enums) {
            DebugPrintA("[ProtoDump] Enum: %s.%s\n", e->namespaze ? e->namespaze : "", e->name);
            GenerateEnumDefinition(e, ss);
        }

        for (auto m : messages) {
            DebugPrintA("[ProtoDump] Message: %s.%s\n", m->namespaze ? m->namespaze : "", m->name);
            GenerateMessageDefinition(m, ss);
        }

        file << ss.str();
        file.close();
        DebugPrintA("[ProtoDump] dump done!\n");
    }
    else
    {
        DebugPrintA("[ProtoDump] [ERROR] Failed to open file for writing: %s\n", path);
    }
}
