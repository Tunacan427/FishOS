#pragma once

#include <klib/types.hpp>

namespace cpu {
    struct [[gnu::packed]] GDTR {
        u16 limit;
        u64 base;
    };

    struct [[gnu::packed]] GDTEntry {
        u16 limit;
        u16 base_low;
        u8 base_mid;
        u8 access;
        u8 granularity;
        u8 base_high;
    };

    enum class GDTSegment {
        KERNEL_CODE_16 = 0x8,
        KERNEL_DATA_16 = 0x10,
        KERNEL_CODE_32 = 0x18,
        KERNEL_DATA_32 = 0x20,
        KERNEL_CODE_64 = 0x28,
        KERNEL_DATA_64 = 0x30,
        USER_DATA_64 = 0x38 | 3,
        USER_CODE_64 = 0x40 | 3,
        TSS = 0x48
    };

    extern "C" void __flush_gdt(GDTR *gdtr);
    void load_gdt();
    void load_tss_gdt_entry();
}
