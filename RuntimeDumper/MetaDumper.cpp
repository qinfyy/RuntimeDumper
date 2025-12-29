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
