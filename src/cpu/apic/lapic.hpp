#pragma once

#include <types.hpp>

namespace cpu::apic {
    class LAPIC {
    public:
        usize id;
        usize processor_id;
        uptr reg_base;

        inline void write_reg(usize reg, u32 val) {
            *(volatile u32*)(reg + reg_base) = val;
        }

        inline u32 read_reg(usize reg) {
            return *(volatile u32*)(reg + reg_base);
        }
    };
}
