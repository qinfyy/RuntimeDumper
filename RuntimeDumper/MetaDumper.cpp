#include "pch.h"
#include "MetaDumper.h"
#include <filesystem>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vm/GlobalMetadataFileInternals.h>
#include "Il2CppFunctions.h"
#include "PrintHelper.h"

#define METADATA_MANUAL_ADDRESS 0

#if METADATA_MANUAL_ADDRESS
#define S_GLOBAL_METADATA_PTR 0x7213798
#endif

static void* s_GlobalMetadata = nullptr;
static std::once_flag flag;

void* ScanGlobalMetadata()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

    MEMORY_BASIC_INFORMATION mbi{};

    while (addr < maxAddr)
    {
        if (!VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)))
            break;

        if (mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_NOACCESS) && !(mbi.Protect & PAGE_GUARD))
        {
            uint8_t* base = (uint8_t*)mbi.BaseAddress;
            size_t size = mbi.RegionSize;

            for (size_t i = 0; i + 0x20 < size; i++)
            {
                uint8_t* p = base + i;

                __try
                {
                    if (*(int32_t*)p != 4205910959u)
                        continue;

                    int32_t version = *(int32_t*)(p + 4);
                    if (version < 20 || version > 40)
                        continue;

                    int32_t headerSize = *(int32_t*)(p + 8);
                    if (headerSize < 0x80 || headerSize > 0x400)
                        continue;

                    bool ok = true;
                    int32_t nextOffset = headerSize;

                    for (int32_t off = 0x8; off + 4 < headerSize; off += 8)
                    {
                        int32_t curOffset = *(int32_t*)(p + off);
                        int32_t curSize = *(int32_t*)(p + off + 4);

                        if (curOffset < headerSize)
                            continue;

                        if (curSize < 0)
                        {
                            ok = false;
                            break;
                        }

                        if (curOffset < nextOffset)
                        {
                            ok = false;
                            break;
                        }

                        if (curSize > 0)
                            nextOffset = curOffset + curSize;
                    }

                    if (!ok)
                        continue;

                    return p;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    continue;
                }
            }
        }

        addr += mbi.RegionSize;
    }

    return nullptr;
}

void* GetGlobalMetadata()
{
    std::call_once(flag, []() {
#if METADATA_MANUAL_ADDRESS
        void** pGlobalMetadata = (void**)(GetModuleBase() + S_GLOBAL_METADATA_PTR);

        __try
        {
            if (pGlobalMetadata && *pGlobalMetadata)
            {
                cachedHeader = (Il2CppGlobalMetadataHeader*)(*pGlobalMetadata);
                DebugPrintA("[MetaDumper] Using manual s_GlobalMetadata: var=%p value=%p version=%d\n",
                    (void*)pGlobalMetadata, (void*)cachedHeader, cachedHeader->version);
            }
            else
            {
                DebugPrintA("[ERROR] Manual s_GlobalMetadata pointer invalid\n");
                cachedHeader = nullptr;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintA("[ERROR] Manual s_GlobalMetadata pointer invalid (exception)\n");
            cachedHeader = nullptr;
        }
#else
        s_GlobalMetadata = ScanGlobalMetadata();
        if (s_GlobalMetadata)
            DebugPrintA("[MetaDumper] Found GlobalMetadata: %p version=%d\n", s_GlobalMetadata, ((Il2CppGlobalMetadataHeader*)s_GlobalMetadata)->version);
        else
            DebugPrintA("[MetaDumper] Failed to find GlobalMetadata\n");
#endif
        });

    return s_GlobalMetadata;
}

size_t CalculateMetadataSize(const Il2CppGlobalMetadataHeader* header)
{
    size_t maxEnd = 0;

#define UPDATE_END(offset, size) \
    if ((offset) > 0 && (size) > 0) \
        maxEnd = max(maxEnd, (size_t)(offset) + (size_t)(size))

    UPDATE_END(header->stringLiteralOffset, header->stringLiteralSize);
    UPDATE_END(header->stringLiteralDataOffset, header->stringLiteralDataSize);
    UPDATE_END(header->stringOffset, header->stringSize);
    UPDATE_END(header->eventsOffset, header->eventsSize);
    UPDATE_END(header->propertiesOffset, header->propertiesSize);
    UPDATE_END(header->methodsOffset, header->methodsSize);
    UPDATE_END(header->parameterDefaultValuesOffset, header->parameterDefaultValuesSize);
    UPDATE_END(header->fieldDefaultValuesOffset, header->fieldDefaultValuesSize);
    UPDATE_END(header->fieldAndParameterDefaultValueDataOffset, header->fieldAndParameterDefaultValueDataSize);
    UPDATE_END(header->fieldMarshaledSizesOffset, header->fieldMarshaledSizesSize);
    UPDATE_END(header->parametersOffset, header->parametersSize);
    UPDATE_END(header->fieldsOffset, header->fieldsSize);
    UPDATE_END(header->genericParametersOffset, header->genericParametersSize);
    UPDATE_END(header->genericParameterConstraintsOffset, header->genericParameterConstraintsSize);
    UPDATE_END(header->genericContainersOffset, header->genericContainersSize);
    UPDATE_END(header->nestedTypesOffset, header->nestedTypesSize);
    UPDATE_END(header->interfacesOffset, header->interfacesSize);
    UPDATE_END(header->vtableMethodsOffset, header->vtableMethodsSize);
    UPDATE_END(header->interfaceOffsetsOffset, header->interfaceOffsetsSize);
    UPDATE_END(header->typeDefinitionsOffset, header->typeDefinitionsSize);
    UPDATE_END(header->imagesOffset, header->imagesSize);
    UPDATE_END(header->assembliesOffset, header->assembliesSize);
    UPDATE_END(header->fieldRefsOffset, header->fieldRefsSize);
    UPDATE_END(header->referencedAssembliesOffset, header->referencedAssembliesSize);
    UPDATE_END(header->attributeDataOffset, header->attributeDataSize);
    UPDATE_END(header->attributeDataRangeOffset, header->attributeDataRangeSize);
    UPDATE_END(header->unresolvedVirtualCallParameterTypesOffset, header->unresolvedVirtualCallParameterTypesSize);
    UPDATE_END(header->unresolvedVirtualCallParameterRangesOffset, header->unresolvedVirtualCallParameterRangesSize);
    UPDATE_END(header->windowsRuntimeTypeNamesOffset, header->windowsRuntimeTypeNamesSize);
    UPDATE_END(header->windowsRuntimeStringsOffset, header->windowsRuntimeStringsSize);
    UPDATE_END(header->exportedTypeDefinitionsOffset, header->exportedTypeDefinitionsSize);

#undef UPDATE_END

    return maxEnd;
}

bool DumpMetaFile(const std::string& path)
{
    auto* header = (Il2CppGlobalMetadataHeader*)GetGlobalMetadata();
    if (!header)
    {
        DebugPrintA("[MetaDumper] GlobalMetadata not found\n");
        return false;
    }

    size_t metaSize = CalculateMetadataSize(header);

    DebugPrintA("[MetaDumper] Metadata size = %zu bytes\n", metaSize);

    std::filesystem::path filePath(path);
    std::filesystem::path directory = filePath.parent_path();
    if (!directory.empty() && !std::filesystem::exists(directory))
    {
        std::filesystem::create_directories(directory);
    }

    std::ofstream file(path, std::ios::binary | std::ios::out);
    if (!file.is_open())
    {
        DebugPrintA("[ERROR] Failed to open file for writing: %s\n", path.c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(header), metaSize);
    file.close();

    DebugPrintA("[MetaDumper] Dumped global-metadata.dat\n");

    return true;
}

inline const uint8_t* MetaBase(const Il2CppGlobalMetadataHeader* h)
{
    return reinterpret_cast<const uint8_t*>(h);
}

Metadata ParseMetadata()
{
    Metadata m{};

    auto* header = reinterpret_cast<const Il2CppGlobalMetadataHeader*>(GetGlobalMetadata());

    m.header = header;
    auto base = MetaBase(header);

    // ImageDefinitions
    m.imageDefs = reinterpret_cast<const Il2CppImageDefinition*>(base + header->imagesOffset);
    m.imageCount = header->imagesSize / sizeof(Il2CppImageDefinition);

    // AssemblyDefinitions
    m.assemblyDefs = reinterpret_cast<const Il2CppAssemblyDefinition*>(base + header->assembliesOffset);
    m.assemblyCount = header->assembliesSize / sizeof(Il2CppAssemblyDefinition);

    // TypeDefinitions
    m.typeDefs = reinterpret_cast<const Il2CppTypeDefinition*>(base + header->typeDefinitionsOffset);
    m.typeCount = header->typeDefinitionsSize / sizeof(Il2CppTypeDefinition);

    // MethodDefinitions
    m.methodDefs = reinterpret_cast<const Il2CppMethodDefinition*>(base + header->methodsOffset);
    m.methodCount = header->methodsSize / sizeof(Il2CppMethodDefinition);

    // ParameterDefinitions
    m.parameterDefs = reinterpret_cast<const Il2CppParameterDefinition*>(base + header->parametersOffset);
    m.parameterCount = header->parametersSize / sizeof(Il2CppParameterDefinition);

    // FieldDefinitions
    m.fieldDefs = reinterpret_cast<const Il2CppFieldDefinition*>(base + header->fieldsOffset);
    m.fieldCount = header->fieldsSize / sizeof(Il2CppFieldDefinition);

    // PropertyDefinitions
    m.propertyDefs = reinterpret_cast<const Il2CppPropertyDefinition*>(base + header->propertiesOffset);
    m.propertyCount = header->propertiesSize / sizeof(Il2CppPropertyDefinition);

    // EventDefinitions
    m.eventDefs = reinterpret_cast<const Il2CppEventDefinition*>(base + header->eventsOffset);
    m.eventCount = header->eventsSize / sizeof(Il2CppEventDefinition);

    // StringLiterals
    m.stringLiterals = reinterpret_cast<const Il2CppStringLiteral*>(base + header->stringLiteralOffset);
    m.stringLiteralCount = header->stringLiteralSize / sizeof(Il2CppStringLiteral);

    // GenericContainers
    m.genericContainers = reinterpret_cast<const Il2CppGenericContainer*>(base + header->genericContainersOffset);
    m.genericContainerCount = header->genericContainersSize / sizeof(Il2CppGenericContainer);

    // GenericParameters
    m.genericParameters = reinterpret_cast<const Il2CppGenericParameter*>(base + header->genericParametersOffset);
    m.genericParameterCount = header->genericParametersSize / sizeof(Il2CppGenericParameter);

    // FieldRefs
    m.fieldRefs = reinterpret_cast<const Il2CppFieldRef*>(base + header->fieldRefsOffset);
    m.fieldRefCount = header->fieldRefsSize / sizeof(Il2CppFieldRef);

    // Interfaces
    m.interfaceIndices = reinterpret_cast<const uint32_t*>(base + header->interfacesOffset);
    m.interfaceIndexCount = header->interfacesSize / sizeof(uint32_t);

    // NestedTypes
    m.nestedTypeIndices = reinterpret_cast<const uint32_t*>(base + header->nestedTypesOffset);
    m.nestedTypeIndexCount = header->nestedTypesSize / sizeof(uint32_t);

    // VTableMethods
    m.vtableMethods = reinterpret_cast<const uint32_t*>(base + header->vtableMethodsOffset);
    m.vtableMethodCount = header->vtableMethodsSize / sizeof(uint32_t);

    // CustomAttributeDataRanges (v29+)
    m.attributeDataRanges =
        reinterpret_cast<const Il2CppCustomAttributeDataRange*>(base + header->attributeDataRangeOffset);
    m.attributeDataRangeCount =
        header->attributeDataRangeSize / sizeof(Il2CppCustomAttributeDataRange);

    return m;
}


enum class SearchSectionType
{
    Exec,
    Data,
    Bss
};

struct SearchSection
{
    uintptr_t address;     // VA start
    uintptr_t addressEnd;  // VA end
    uintptr_t offset;      // RVA start
    uintptr_t offsetEnd;   // RVA end
};

struct SectionHelper
{
    std::vector<SearchSection> exec;
    std::vector<SearchSection> data;
    std::vector<SearchSection> bss;
};

PIMAGE_NT_HEADERS GetNtHeaders(uintptr_t moduleBase)
{
    auto* dos = (PIMAGE_DOS_HEADER)moduleBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    auto* nt = (PIMAGE_NT_HEADERS)(moduleBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    return nt;
}

SectionHelper GetSectionHelper(uintptr_t moduleBase)
{
    SectionHelper helper;

    auto* nt = GetNtHeaders(moduleBase);
    if (!nt)
        return helper;

    auto* section = IMAGE_FIRST_SECTION(nt);
    WORD sectionCount = nt->FileHeader.NumberOfSections;

    for (WORD i = 0; i < sectionCount; i++, section++)
    {
        DWORD ch = section->Characteristics;

        SearchSection sec{};
        sec.address = moduleBase + section->VirtualAddress;
        sec.addressEnd = sec.address + section->Misc.VirtualSize;
        sec.offset = section->VirtualAddress;
        sec.offsetEnd = section->VirtualAddress + section->Misc.VirtualSize;

        // Il2CppDumper 的 switch (Characteristics)
        if (ch == 0x60000020) // IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE
        {
            helper.exec.push_back(sec);
        }
        else if (ch == 0x40000040 || ch == 0xC0000040)
        {
            helper.data.push_back(sec);
            helper.bss.push_back(sec);
        }
    }

    return helper;
}


const std::vector<uint8_t> featureBytes = { 0x6D, 0x73, 0x63, 0x6F, 0x72, 0x6C, 0x69, 0x62, 0x2E, 0x64, 0x6C, 0x6C, 0x00 }; //mscorlib.dll

std::vector<uintptr_t> FindBytesInSection(const SearchSection& sec, const std::vector<uint8_t>& pattern) {
    std::vector<uintptr_t> results;
    uint8_t* base = (uint8_t*)sec.address;
    size_t size = sec.addressEnd - sec.address;

    for (size_t i = 0; i + pattern.size() <= size; i++) {
        bool match = true;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (base[i + j] != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            results.push_back((uintptr_t)(base + i));
        }
    }
    return results;
}

bool CheckPointerRange(const std::vector<SearchSection>& secs, uintptr_t ptr) {
    for (auto& sec : secs) {
        if (ptr >= sec.address && ptr < sec.addressEnd) {
            return true;
        }
    }
    return false;
}

bool CheckPointerRangeArray(const std::vector<SearchSection>& secs, uintptr_t* ptrArray, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (!CheckPointerRange(secs, ptrArray[i]))
            return false;
    }
    return true;
}

std::vector<uintptr_t> FindReference(const std::vector<SearchSection>& secs, uintptr_t target) {
    std::vector<uintptr_t> refs;
    for (auto& sec : secs) {
        uintptr_t* base = (uintptr_t*)sec.address;
        size_t count = (sec.addressEnd - sec.address) / sizeof(uintptr_t);
        for (size_t i = 0; i < count; i++) {
            if (base[i] == target) {
                refs.push_back((uintptr_t)base + i * sizeof(uintptr_t));
            }
        }
    }
    return refs;
}

uintptr_t FindCodeRegistration2019(const SectionHelper& helper, int imageCount, int il2cppVersion) {
    constexpr size_t PTR = sizeof(uintptr_t);

    for (auto& sec : helper.data) {
        auto hits = FindBytesInSection(sec, featureBytes);

        for (auto dllva : hits) {
            for (auto refva : FindReference(helper.data, dllva)) {
                for (auto refva2 : FindReference(helper.data, refva)) {

                    if (il2cppVersion < 27)
                        continue;

                    for (int i = imageCount - 1; i >= 0; i--) {
                        uintptr_t target = refva2 - i * PTR;

                        for (auto refva3 : FindReference(helper.data, target)) {

                            uintptr_t countAddr = refva3 - PTR;

                            if (!CheckPointerRange(helper.data, countAddr))
                                continue;

                            auto* pCount = (uintptr_t*)countAddr;

                            if (*pCount == (uintptr_t)imageCount) {
                                if (il2cppVersion >= 29)
                                    return refva3 - PTR * 14;
                                else
                                    return refva3 - PTR * 13;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

void DumpSections(const SectionHelper& s)
{
    auto dump = [](const char* name, const std::vector<SearchSection>& v)
        {
            DebugPrintA("[%s]\n", name);
            for (auto& sec : v)
            {
                DebugPrintA("  VA: %p - %p\n", (void*)sec.address, (void*)sec.addressEnd);
            }
        };

    dump("EXEC", s.exec);
    dump("DATA", s.data);
    dump("BSS", s.bss);
}

void GetCodeRegistration() {
    auto* header = reinterpret_cast<const Il2CppGlobalMetadataHeader*>(GetGlobalMetadata());

    Metadata metadata = ParseMetadata();
    size_t methodCount = metadata.methodCount;
    size_t typeCount = metadata.typeCount;
    size_t imageCount = metadata.imageCount;

    printf("MethodDefs count: %zu\n", methodCount);
    printf("TypeDefs count: %zu\n", typeCount);
    printf("ImageDefs count: %zu\n", imageCount);

    uintptr_t moduleBase = GetModuleBase();
    SectionHelper helper = GetSectionHelper(moduleBase);

    uintptr_t codeReg = FindCodeRegistration2019(helper, imageCount, header->version);

    if (codeReg)
    {
        uintptr_t rva = codeReg - moduleBase;
        printf("CodeRegistration found at: VA = 0x%zx, RVA = 0x%zx\n", codeReg, rva);
    }
    else
    {
        printf("CodeRegistration not found!\n");
    }
}

