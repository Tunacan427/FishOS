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

    extern "C" void __flush_gdt(GDTR *gdtr);
    void load_gdt();
}
