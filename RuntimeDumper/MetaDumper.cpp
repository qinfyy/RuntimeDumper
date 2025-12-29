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

            for (size_t i = 0; i + 4 < size; i++)
            {
                if (base[i] == 175 && base[i + 1] == 27 && base[i + 2] == 177 &&base[i + 3] == 250)
                {
                    auto* header = (Il2CppGlobalMetadataHeader*)(base + i);

                    if (header->version >= 10 && header->version <= 50 &&
                        header->stringOffset > 0 &&
                        header->typeDefinitionsOffset > 0 &&
                        header->methodsOffset > 0 &&
                        header->imagesOffset > 0)
                    {
                        printf("[MetaDumper] Found GlobalMetadata: %p version=%d\n", (void*)header, header->version);
                        return header;
                    }
                }
            }
        }

        addr += mbi.RegionSize;
    }

    return nullptr;
}

void* GetGlobalMetadata()
{
#if METADATA_MANUAL_ADDRESS
    void** pGlobalMetadata = (void**)(GetModuleBase() + S_GLOBAL_METADATA_PTR);

    __try
    {
        if (!pGlobalMetadata || !*pGlobalMetadata)
            return nullptr;

        Il2CppGlobalMetadataHeader* header = (Il2CppGlobalMetadataHeader*)(*pGlobalMetadata);

        DebugPrintA("[MetaDumper] Using manual s_GlobalMetadata: var=%p value=%p version=%d\n",
            (void*)pGlobalMetadata, (void*)header, header->version);

        return header;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrintA("[ERROR] Manual s_GlobalMetadata pointer invalid\n");
        return nullptr;
    }
#else
    return ScanGlobalMetadata();
#endif
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

    DebugPrintA("[MetaDumper] Metadata size = 0x%zx\n", metaSize);

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

    DebugPrintA("[MetaDumper] Dumped global-metadata.dat (%zu bytes)\n", metaSize);

    return true;
}
