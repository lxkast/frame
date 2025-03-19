#include <ntifs.h>
#include <ntimage.h>
#include "helpers.hpp"

namespace helpers {
    uintptr_t resolve_address(uintptr_t instruction, ULONG offset_into_instruction, ULONG instruction_size) {
        LONG rip_offset = *(PLONG)(instruction + offset_into_instruction);
        return instruction + instruction_size + rip_offset;
    }

    uintptr_t get_ntos_base_address() {
        typedef unsigned char uint8_t;
        auto idt_base = reinterpret_cast<uintptr_t>(KeGetPcr()->IdtBase);
        auto align_page = *reinterpret_cast<uintptr_t*>(idt_base + 4) >> 0xc << 0xc;

        for (; align_page; align_page -= PAGE_SIZE)
        {
            for (int index = 0; index < PAGE_SIZE - 0x7; index++)
            {
                auto current_address = static_cast<intptr_t>(align_page) + index;

                if (*reinterpret_cast<uint8_t*>(current_address) == 0x48
                    && *reinterpret_cast<uint8_t*>(current_address + 1) == 0x8D
                    && *reinterpret_cast<uint8_t*>(current_address + 2) == 0x1D
                    && *reinterpret_cast<uint8_t*>(current_address + 6) == 0xFF) //48 8d 1D ?? ?? ?? FF
                {
                    auto Ntosbase = resolve_address(current_address, 3, 7);
                    if (!((UINT64)Ntosbase & 0xfff))
                    {
                        return Ntosbase;
                    }
                }
            }
        }
        return 0;
    }

    uintptr_t get_pattern(uintptr_t base, size_t range, const char* pattern, const char* mask) {
        const auto check_mask = [](const char* base, const char* pattern, const char* mask) -> bool
        {
            for (; *mask; ++base, ++pattern, ++mask)
            {
                if (*mask == 'x' && *base != *pattern)
                {
                    return false;
                }
            }

            return true;
        };

        range = range - strlen(mask);

        for (size_t i = 0; i < range; ++i)
        {
            if (check_mask((const char*)base + i, pattern, mask))
            {
                return base + i;
            }
        }

        return NULL;
    }

    uintptr_t find_pattern(uintptr_t Base, CHAR* Pattern, CHAR* Mask) {
        PIMAGE_NT_HEADERS Headers{ (PIMAGE_NT_HEADERS)(Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew) };
        PIMAGE_SECTION_HEADER Sections{ IMAGE_FIRST_SECTION(Headers) };

        for (auto i = 0; i < Headers->FileHeader.NumberOfSections; ++i)
        {
            IMAGE_SECTION_HEADER* Section{ &Sections[i] };

            if (!memcmp(Section->Name, ".text", 5) || !memcmp(Section->Name, "PAGE", 4))
            {
                const auto match = get_pattern(Base + Section->VirtualAddress, Section->Misc.VirtualSize, Pattern, Mask);

                if (match) {
                    return (match);
                }
            }
        }

        return 0;
    }
}