#pragma once

#include <kstd/types.hpp>

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

    struct [[gnu::packed]] InterruptFrame {
        u64 r15, r14, r13, r12, r11, r10, r9, r8;
        u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
        u64 vec; // vector number, set by the wrapper
        u64 err; // might be pushed by the cpu, set to 0 by the wrapper if not
        u64 rip, cs, rflags, rsp, ss; // all pushed by the cpu
    };

    typedef void (*IDTHandler)(InterruptFrame*);

    u8 allocate_vector();
    void load_idt_entry(u8 index, void (*wrapper)(), IDTType type);
    void load_idt_handler(u8 index, IDTHandler handler);
    void load_idt();
}
