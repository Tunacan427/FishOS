#pragma once

#include <types.hpp>

namespace cpu::apic {
    class IOAPIC {
    public:
        usize id;
        uptr ioregsel;
        uptr ioregwin;

        IOAPIC(usize id, uptr base) : id(id), ioregsel(base), ioregwin(base + 0x10) {}

        inline void write_reg(usize reg, u32 val) {
            *(volatile u32*)(ioregsel) = reg;
            *(volatile u32*)(ioregwin) = val;
        }

        inline u32 read_reg(usize reg) {
            *(volatile u32*)(ioregsel) = reg;
            return *(volatile u32*)(ioregwin);
        }
    };
}
