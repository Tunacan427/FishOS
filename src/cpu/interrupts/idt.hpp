#pragma once

#include <klib/types.hpp>
#include <cpu/cpu.hpp>

namespace cpu::interrupts {
    struct [[gnu::packed]] IDTR {
        u16 limit;
        u64 base;
    };

    enum class IDTType {
        INTERRUPT = 0b1110,
        TRAP = 0b1111
    };

    struct [[gnu::packed]] IDTEntry {
        u16 offset1;
        u16 selector;
        u16 attributes;
        u16 offset2;
        u32 offset3;
        u32 reserved;
    };

    typedef void (*IDTHandler)(u64 vec, GPRState* frame);

    u8 allocate_vector();
    void load_idt_entry(u8 index, void (*wrapper)(), IDTType type);
    void load_idt_handler(u8 index, IDTHandler handler);
    void load_idt();
}
