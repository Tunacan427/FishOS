#include "mem/vmm.hpp"
#include <acpi/tables.hpp>
#include <klib/cstdlib.hpp>
#include <klib/cstdio.hpp>
#include <klib/vector.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <cpu/interrupts/apic.hpp>
#include <sched/timer/hpet.hpp>

namespace acpi {
    bool do_checksum(uptr table, usize size) {
        u8 sum = 0; // intentionally overflowing because the sum has to be mod 0x100
        for (usize i = 0; i < size; i++)
            sum += ((u8*)table)[i];
        return sum == 0;
    }

    void parse_rsdp(uptr rsdp_addr) {
        u64 hhdm = mem::vmm::get_hhdm();
        auto rsdp1 = (RSDP1*)rsdp_addr;
        if (klib::memcmp(rsdp1->signature, "RSD PTR ", 8))
            panic("RSDP signature incorrect");
        if (!do_checksum(rsdp_addr, sizeof(RSDP1)))
            panic("RSDP1 checksum incorrect");
        klib::printf("ACPI: RSDP OEM ID: %.*s\n", 6, rsdp1->oem_id);
        if (rsdp1->revision == 0) {
            auto rsdt = (RSDT*)(rsdp1->rsdt_addr + hhdm);
            if (klib::memcmp(rsdt->signature, "RSDT", 4))
                panic("RSDT signature incorrect");
            if (!do_checksum((uptr)rsdt, rsdt->size))
                panic("RSDT checksum incorrect");
            klib::printf("ACPI: RSDT OEM ID: %.*s\n", 6, rsdt->oem_id);

            for (usize i = 0; i < (rsdt->size - sizeof(SDT)) / 4; i++) {
                auto sdt = (SDT*)(rsdt->sdt_array[i] + hhdm);
                parse_sdt(sdt);
            }
        }
        if (rsdp1->revision == 2) {
            auto rsdp2 = (RSDP2*)rsdp1;
            if (!do_checksum(rsdp_addr + sizeof(RSDP1), rsdp2->size - sizeof(RSDP1)))
                panic("RSDP2 checksum incorrect");

            auto xsdt = (XSDT*)(rsdp2->xsdt_addr + hhdm);
            if (klib::memcmp(xsdt->signature, "XSDT", 4))
                panic("XSDT signature incorrect");
            if (!do_checksum((uptr)xsdt, xsdt->size))
                panic("XSDT checksum incorrect");
            klib::printf("ACPI: XSDT OEM ID: %.*s\n", 6, xsdt->oem_id);

            for (usize i = 0; i < (xsdt->size - sizeof(SDT)) / 8; i++) {
                auto sdt = (SDT*)(xsdt->sdt_array[i] + hhdm);
                parse_sdt(sdt);
            }
        }
    }

    void parse_sdt(SDT *sdt) {
        klib::printf("ACPI: Found table with signature: %.*s\n", 4, sdt->signature);
        if (!do_checksum((uptr)sdt, sdt->size))
            panic("ACPI table with signature %.*s has incorrect checksum", 4, sdt->signature);
        
        if (!klib::memcmp(sdt->signature, "APIC", 4))
            return parse_madt((MADT*)sdt);
        
        if (!klib::memcmp(sdt->signature, "HPET", 4))
            return parse_hpet((HPET*)sdt);
    }

    void parse_madt(MADT *madt) {
        using namespace cpu::interrupts;
        uptr hhdm = mem::vmm::get_hhdm();
        klib::printf("ACPI: Parsing MADT, LAPIC phy addr: %#X (might be overridden)\n", madt->lapic_addr);

        for (auto entry = (MADT::Entry*)((uptr)madt + sizeof(MADT)); (uptr)entry < (uptr)madt + madt->size; entry = (MADT::Entry*)((uptr)entry + entry->size)) {
            if (entry->type == MADT::LAPIC) {
                auto lapic_entry = (MADT::EntryLAPIC*)entry;
                klib::printf("ACPI: LAPIC | ID: %d, Processor ID: %d", lapic_entry->processor_id, lapic_entry->lapic_id);
                if (lapic_entry->flags & 0b1)
                    klib::printf(", enabled");
                else
                    klib::printf(", disabled");
                if (lapic_entry->flags & 0b11)
                    klib::printf(", online capable");
                klib::putchar('\n');
                if (lapic_entry->flags & 0b1)
                    lapics().push_back((LAPIC) { lapic_entry->lapic_id, lapic_entry->processor_id });
            } else if (entry->type == MADT::IOAPIC) {
                auto ioapic_entry = (MADT::EntryIOAPIC*)entry;
                klib::printf("ACPI: IOAPIC | ID: %d, Phy Addr: %#X, GSI base: %d\n", ioapic_entry->ioapic_id, ioapic_entry->ioapic_addr, ioapic_entry->gsi_base);
                mem::vmm::get_kernel_pagemap()->map_page(ioapic_entry->ioapic_addr, ioapic_entry->ioapic_addr + hhdm, PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE | PAGE_CACHE_DISABLE);
                IOAPIC ioapic(ioapic_entry->ioapic_id, ioapic_entry->ioapic_addr, ioapic_entry->gsi_base);
                ioapics().push_back(ioapic);
            } else if (entry->type == MADT::IOAPIC_IRQ_SOURCE_OVERRIDE) {
                auto override_entry = (MADT::EntryIOAPICIRQSourceOverride*)entry;
                klib::printf("ACPI: IOAPIC IRQ Source Override | IRQ: %d, GSI: %d, Flags: %x\n", override_entry->irq_source, override_entry->gsi, override_entry->flags);
                override_irq_source(override_entry->irq_source, override_entry->gsi, override_entry->flags);
            } else if (entry->type == MADT::LAPIC_NMI) {
                auto nmi_entry = (MADT::EntryLAPICNMI*)entry;
                klib::printf("ACPI: LAPIC NMI | Processor ID: %d, LINT%d, Flags: %x\n", nmi_entry->processor_id, nmi_entry->lint, nmi_entry->flags);
            } else if (entry->type == MADT::LAPIC_ADDR_OVERRIDE) {
                auto override_entry = (MADT::EntryLAPICAddressOverride*)entry;
                klib::printf("ACPI: LAPIC Addr Override | Phy Addr: %#lX\n", override_entry->lapic_addr);
            } else {
                klib::printf("ACPI: Found unknown entry, type: %d\n", entry->type);
            }
        }

        LAPIC::prepare();

        // set the NMI LINT
        for (auto entry = (MADT::Entry*)((uptr)madt + sizeof(MADT)); (uptr)entry < (uptr)madt + madt->size; entry = (MADT::Entry*)((uptr)entry + entry->size)) {
            if (entry->type == MADT::LAPIC_NMI) {
                auto nmi_entry = (MADT::EntryLAPICNMI*)entry;
                LAPIC bsp {};
                for (auto &lapic : lapics()) {
                    if (lapic.id == LAPIC::read_id()) {
                        bsp = lapic;
                        break;
                    }
                }

                if (nmi_entry->processor_id == 0xFF || nmi_entry->processor_id == bsp.acpi_id) {
                    bool active_low = (nmi_entry->flags & 0b11) == 0b11;
                    bool level_trigger = (nmi_entry->flags & 0b1100) == 0b1100;
                    // set the NMI LINT to vector 0xFE
                    LAPIC::set_vector(nmi_entry->lint ? LAPIC::LVT_LINT1 : LAPIC::LVT_LINT0, 0xFE, true, active_low, level_trigger, false);
                    // mask the other LINT
                    LAPIC::set_vector(nmi_entry->lint ? LAPIC::LVT_LINT0 : LAPIC::LVT_LINT1, 0, false, false, false, true);
                }
            }
        }

        // mask the other lvt interrupts
        LAPIC::set_vector(LAPIC::LVT_CMCI, 0, false, false, false, true);
        LAPIC::set_vector(LAPIC::LVT_TIMER, 0, false, false, false, true);
        LAPIC::set_vector(LAPIC::LVT_PERF, 0, false, false, false, true);
        LAPIC::set_vector(LAPIC::LVT_THERMAL, 0, false, false, false, true);
        LAPIC::set_vector(LAPIC::LVT_ERROR, 0, false, false, false, true);

        LAPIC::enable();
    }

    void parse_hpet(HPET *table) {
        klib::printf("ACPI: Parsing HPET\n");
        sched::timer::hpet::init(table);
    }
}
