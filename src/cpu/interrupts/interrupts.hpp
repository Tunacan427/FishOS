#pragma once

#include <kstd/types.hpp>
#include <cpu/interrupts/idt.hpp>
#include <cpu/interrupts/apic.hpp>
#include <kstd/vector.hpp>

namespace cpu::interrupts {
    kstd::Vector<IOAPIC>& ioapics();
    kstd::Vector<LAPIC>& lapics();

    void override_irq_source(u8 irq, u32 gsi, u16 flags);
    void register_gsi(u32 gsi, bool active_low, bool level_trigger, IDTHandler handler);
    void register_irq(u8 irq, IDTHandler handler);
    void eoi();
}
