#include <cpu/interrupts/apic.hpp>
#include <cpu/interrupts/idt.hpp>
#include <cpu/cpu.hpp>
#include <kstd/cstdio.hpp>
#include <mem/vmm.hpp>

namespace cpu::interrupts {
    static uptr reg_base;

    void LAPIC::prepare() {
        uptr phy_base = read_msr(0x1B) & ~(u64)0xFFF;
        reg_base = phy_base + mem::vmm::get_hhdm();
        mem::vmm::map_page(phy_base, reg_base, PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE | PAGE_CACHE_DISABLE);
    }

    static void spurious(InterruptFrame *frame) {
        kstd::printf("\n[WARN] APIC spurious interrupt fired\n");
        LAPIC::eoi();
    }

    void LAPIC::enable() {
        write_msr(0x1B, read_msr(0x1B) | (1 << 11)); // set the global enable flag
        u8 spurious_vector = allocate_vector();
        load_idt_handler(spurious_vector, spurious);
        write_reg(Reg::SPURIOUS, spurious_vector | (1 << 8)); // set spurious interrupt and set bit 8 to start getting interrupts
    }

    void LAPIC::write_reg(Reg reg, u32 val) {
        *(volatile u32*)(reg_base + u32(reg)) = val;
    }

    u32 LAPIC::read_reg(Reg reg) {
        return *(volatile u32*)(reg_base + u32(reg));
    }

    u32 LAPIC::read_id() {
        return read_reg(Reg::ID);
    }

    void LAPIC::eoi() {
        write_reg(Reg::EOI, 0); // write 0 to end of interrupt register
    }

    void LAPIC::set_vector(Reg reg, u8 vector, bool nmi, bool active_low, bool level_trigger, bool mask) {
        write_reg(reg, vector | (nmi << 10) | (active_low << 13) | (level_trigger << 15) | (mask << 16));
    }

    void LAPIC::mask_vector(Reg reg) {
        write_reg(reg, read_reg(reg) | (1 << 16));
    }

    void LAPIC::unmask_vector(Reg reg) {
        write_reg(reg, read_reg(reg) & ~(1 << 16));
    }

    IOAPIC::IOAPIC(usize id, uptr addr, usize gsi_base) : id(id), gsi_base(gsi_base) {
        uptr hhdm = mem::vmm::get_hhdm();
        ioregsel = (volatile u32*)(addr + hhdm);
        ioregwin = (volatile u32*)(addr + 0x10 + hhdm);
        max_entries = (read_reg(1) >> 16) + 1;
    }

    void IOAPIC::write_reg(usize reg, u32 val) {
        *ioregsel = reg;
        *ioregwin = val;
    }

    u32 IOAPIC::read_reg(usize reg) {
        *ioregsel = reg;
        return *ioregwin;
    }

    void IOAPIC::set_redir_entry(usize n, u8 vector, u8 delivery_mode, bool logic_dest, bool active_low, bool level_trigger, bool mask, u8 dest) {
        write_reg(n * 2 + 0x10, vector | (delivery_mode << 8) | (logic_dest << 11) | (active_low << 13) | (level_trigger << 15) | (mask << 16));
        write_reg(n * 2 + 0x11, dest << 24);
    }
}
