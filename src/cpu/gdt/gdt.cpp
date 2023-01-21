#include <cpu/gdt/gdt.hpp>

namespace cpu {
    [[gnu::aligned(8)]] static GDTEntry gdt[7];
    static GDTR gdtr;

    void load_gdt() {
        // null descriptor
        gdt[0] = { 0 };

        // 64-bit kernel code
        gdt[1] = {
            .limit = 0,
            .base_low = 0,
            .base_mid = 0,
            .access = 0b10011010,
            .granularity = 0b00100000,
            .base_high = 0
        };

        // 64-bit kernel data
        gdt[2] = {
            .limit = 0,
            .base_low = 0,
            .base_mid = 0,
            .access = 0b10010010,
            .granularity = 0,
            .base_high = 0
        };

        // 64-bit user data
        gdt[3] = {
            .limit = 0,
            .base_low = 0,
            .base_mid = 0,
            .access = 0b11110010,
            .granularity = 0,
            .base_high = 0
        };

        // 64-bit user code
        gdt[4] = {
            .limit = 0,
            .base_low = 0,
            .base_mid = 0,
            .access = 0b11111010,
            .granularity = 0b00100000,
            .base_high = 0
        };

        reload_gdt();
    }

    void reload_gdt() {
        gdtr.limit = sizeof(gdt) - 1;
        gdtr.base = u64(&gdt);
        __flush_gdt(&gdtr);
    }

    void load_tss(uptr tss_addr) {
        gdt[5] = {
            .limit = 0x67,
            .base_low = u16(tss_addr),
            .base_mid = u8(tss_addr >> 16),
            .access = 0b10001001,
            .granularity = 0b00000000,
            .base_high = u8(tss_addr >> 24)
        };

        gdt[6] = {
            .limit = u16(tss_addr >> 32),
            .base_low = u16(tss_addr >> 48)
        };

        asm volatile("ltr %0" : : "r" (u16(GDTSegment::TSS)) : "memory");
    }
}
