#pragma once

namespace helpers {
    _declspec(noinline) auto resolve_address(
        uintptr_t Instruction,
        ULONG OffsetOffset,
        ULONG InstructionSize)->uintptr_t;

    _declspec(noinline) auto get_ntos_base_address()->uintptr_t;

    INT kmemcmp(const void* s1, const void* s2, size_t n);

    auto get_pattern(uintptr_t base, size_t range, const char* pattern, const char* mask)->uintptr_t;

    auto find_pattern(uintptr_t Base, CHAR* Pattern, CHAR* Mask)->uintptr_t;
}