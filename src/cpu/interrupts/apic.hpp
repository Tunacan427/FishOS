#pragma once

#include <kstd/types.hpp>

namespace cpu::interrupts {
    struct LAPIC {
        enum class Reg {
            ID = 0x20,
            VER = 0x30,
            EOI = 0xB0,
            SPURIOUS = 0xF0,
            ICR = 0x300,
            LVT_CMCI = 0x2F0,
            LVT_TIMER = 0x320,
            LVT_THERMAL = 0x330,
            LVT_PERF = 0x340,
            LVT_LINT0 = 0x350,
            LVT_LINT1 = 0x360,
            LVT_ERROR = 0x370,
            TIMER_INITIAL = 0x380,
            TIMER_CURRENT = 0x390,
            TIMER_DIVIDE = 0x3E0
        };

        u8 id, acpi_id;

        static void prepare();
        static void enable();
        
        static void write_reg(Reg reg, u32 val);
        static u32 read_reg(Reg reg);

        static u32 read_id();
        static void eoi();

        static void set_vector(Reg reg, u8 vector, bool nmi, bool active_low, bool level_trigger, bool mask);
        static void mask_vector(Reg reg);
        static void unmask_vector(Reg reg);
    };

    class IOAPIC {
    public:
        u8 id;
        u32 gsi_base, max_entries;
        volatile u32 *ioregsel;
        volatile u32 *ioregwin;

        IOAPIC(usize id, uptr addr, usize gsi_base);

        void write_reg(usize reg, u32 val);
        u32 read_reg(usize reg);

        void set_redir_entry(usize n, u8 vector, u8 delivery_mode, bool logic_dest, bool active_low, bool level_trigger, bool mask, u8 dest);
    };
}
