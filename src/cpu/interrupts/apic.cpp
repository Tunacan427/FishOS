#include <cpu/interrupts/apic.hpp>
#include <cpu/interrupts/idt.hpp>
#include <cpu/cpu.hpp>
#include <klib/cstdio.hpp>
#include <mem/vmm.hpp>

namespace cpu::interrupts {
    static uptr reg_base;

    static void spurious(u64 vec, InterruptState *state) {
        klib::printf("\n[WARN] APIC spurious interrupt fired\n");
        LAPIC::eoi();
    }

    void LAPIC::prepare() {
        uptr phy_base = MSR::read(MSR::IA32_APIC_BASE) & ~(u64)0xFFF;
        reg_base = phy_base + mem::vmm::get_hhdm();
        mem::vmm::get_kernel_pagemap()->map_page(phy_base, reg_base, PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE | PAGE_CACHE_DISABLE);
    }

    void LAPIC::enable() {
        MSR::write(MSR::IA32_APIC_BASE, MSR::read(MSR::IA32_APIC_BASE) | (1 << 11)); // set the global enable flag
        u8 spurious_vector = allocate_vector();
        load_idt_handler(spurious_vector, spurious);
        write_reg(SPURIOUS, spurious_vector | (1 << 8)); // set spurious interrupt and set bit 8 to start getting interrupts
    }

    void LAPIC::write_reg(R reg, u32 val) {
        *(volatile u32*)(reg_base + reg) = val;
    }

    u32 LAPIC::read_reg(R reg) {
        return *(volatile u32*)(reg_base + reg);
    }

    u32 LAPIC::read_id() {
        return read_reg(ID);
    }

    void LAPIC::eoi() {
        write_reg(EOI, 0);
    }

    void LAPIC::set_vector(R reg, u8 vector, bool nmi, bool active_low, bool level_trigger, bool mask) {
        write_reg(reg, vector | (nmi << 10) | (active_low << 13) | (level_trigger << 15) | (mask << 16));
    }

    void LAPIC::mask_vector(R reg) {
        write_reg(reg, read_reg(reg) | (1 << 16));
    }

    void LAPIC::unmask_vector(R reg) {
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
