#pragma once

#include <cpu/cpu.hpp>

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
        KERNEL_CODE_64 = 0x8,
        KERNEL_DATA_64 = 0x10,
        USER_DATA_64 = 0x18,
        USER_CODE_64 = 0x20,
        TSS = 0x28
    };

    extern "C" void __flush_gdt(GDTR *gdtr);
    void load_gdt();
    void reload_gdt();
    void load_tss(TSS *tss);
}
