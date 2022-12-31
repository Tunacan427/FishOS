#include <cpu/interrupts/interrupts.hpp>
#include <cpu/interrupts/apic.hpp>
#include <cpu/interrupts/idt.hpp>
#include <klib/vector.hpp>

namespace cpu::interrupts {
    static u32 irq_to_gsi[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    static u16 irq_flags[16];

    klib::Vector<IOAPIC>& ioapics() {
        static klib::Vector<IOAPIC> ioapics;
        return ioapics;
    }

    klib::Vector<LAPIC>& lapics() {
        static klib::Vector<LAPIC> lapics;
        return lapics;
    }

    void override_irq_source(u8 irq, u32 gsi, u16 flags) {
        irq_to_gsi[irq] = gsi;
        irq_flags[irq] = flags;
    }

    void register_gsi(u32 gsi, bool active_low, bool level_trigger, IDTHandler handler) {
        u8 vector = allocate_vector();
        load_idt_handler(vector, handler);
        for (auto &ioapic : ioapics()) {
            if (gsi >= ioapic.gsi_base && gsi < ioapic.gsi_base + ioapic.max_entries) {
                ioapic.set_redir_entry(gsi, vector, 0, false, active_low, level_trigger, false, LAPIC::read_id()); 
                break;
            }
        }
    }

    void register_irq(u8 irq, IDTHandler handler) {
        u32 gsi = irq_to_gsi[irq];
        u16 flags = irq_flags[irq];
        bool active_low = (flags & 0b11) == 0b11;
        bool level_trigger = (flags & 0b1100) == 0b1100;
        register_gsi(gsi, active_low, level_trigger, handler);
    }

    void eoi() {
        LAPIC::eoi();
    }
}
