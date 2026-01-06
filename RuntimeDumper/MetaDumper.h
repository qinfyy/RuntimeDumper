#pragma once
#include <string>
#include <vm/GlobalMetadataFileInternals.h>

struct Metadata
{
    const Il2CppGlobalMetadataHeader* header;

    const Il2CppImageDefinition* imageDefs;
    size_t imageCount;

    const Il2CppAssemblyDefinition* assemblyDefs;
    size_t assemblyCount;

    const Il2CppTypeDefinition* typeDefs;
    size_t typeCount;

    const Il2CppMethodDefinition* methodDefs;
    size_t methodCount;

    const Il2CppParameterDefinition* parameterDefs;
    size_t parameterCount;

    const Il2CppFieldDefinition* fieldDefs;
    size_t fieldCount;

    const Il2CppPropertyDefinition* propertyDefs;
    size_t propertyCount;

    const Il2CppEventDefinition* eventDefs;
    size_t eventCount;

    const Il2CppStringLiteral* stringLiterals;
    size_t stringLiteralCount;

    const Il2CppGenericContainer* genericContainers;
    size_t genericContainerCount;

    const Il2CppGenericParameter* genericParameters;
    size_t genericParameterCount;

    const Il2CppFieldRef* fieldRefs;
    size_t fieldRefCount;

    const uint32_t* interfaceIndices;
    size_t interfaceIndexCount;

    const uint32_t* nestedTypeIndices;
    size_t nestedTypeIndexCount;

    const uint32_t* vtableMethods;
    size_t vtableMethodCount;

    const Il2CppCustomAttributeDataRange* attributeDataRanges;
    size_t attributeDataRangeCount;
};

bool DumpMetaFile(const std::string& path);

void GetCodeRegistration();