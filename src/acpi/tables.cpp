#include <acpi/tables.hpp>
#include <panic.hpp>
#include <kstd/cstdlib.hpp>
#include <kstd/cstdio.hpp>
#include <mem/vmm.hpp>

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
        if (kstd::memcmp(rsdp1->signature, "RSD PTR ", 8))
            panic("RSDP signature incorrect");
        if (!do_checksum(rsdp_addr, sizeof(RSDP1)))
            panic("RSDP1 checksum incorrect");
        kstd::printf("[INFO] RSDP OEM ID: %.*s\n", 6, rsdp1->oem_id);
        if (rsdp1->revision == 0) {
            auto rsdt = (RSDT*)(rsdp1->rsdt_addr + hhdm);
            if (kstd::memcmp(rsdt->signature, "RSDT", 4))
                panic("RSDT signature incorrect");
            if (!do_checksum((uptr)rsdt, rsdt->size))
                panic("RSDT checksum incorrect");
            kstd::printf("[INFO] RSDT OEM ID: %.*s\n", 6, rsdt->oem_id);

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
            if (kstd::memcmp(xsdt->signature, "XSDT", 4))
                panic("XSDT signature incorrect");
            if (!do_checksum((uptr)xsdt, xsdt->size))
                panic("XSDT checksum incorrect");
            kstd::printf("[INFO] XSDT OEM ID: %.*s\n", 6, xsdt->oem_id);

            for (usize i = 0; i < (xsdt->size - sizeof(SDT)) / 8; i++) {
                auto sdt = (SDT*)(xsdt->sdt_array[i] + hhdm);
                parse_sdt(sdt);
            }
        }
    }

    void parse_sdt(SDT *sdt) {
        u64 hhdm = mem::vmm::get_hhdm();
        kstd::printf("[INFO] Found table with signature: %.*s\n", 4, sdt->signature);
        if (!do_checksum((uptr)sdt, sdt->size))
            panic("ACPI table with signature %.*s has incorrect checksum", 4, sdt->signature);
        if (!kstd::memcmp(sdt->signature, "APIC", 4)) {
            auto madt = (MADT*)sdt;
            kstd::printf("[INFO] Parsing MADT, LAPIC addr: %#lX (might be overridden)\n", madt->lapic_addr + hhdm);
            auto entry = (MADT::Entry*)((uptr)madt + sizeof(MADT));
            for (; (uptr)entry < (uptr)sdt + sdt->size; entry = (MADT::Entry*)((uptr)entry + entry->size)) {
                switch (entry->type) {
                case MADT::LAPIC: {
                    auto lapic = (MADT::EntryLAPIC*)entry;
                    kstd::printf("[INFO] LAPIC | ID: %d, Processor ID: %d\n", lapic->processor_id, lapic->lapic_id);
                    break;
                }
                case MADT::IOAPIC: {
                    auto ioapic = (MADT::EntryIOAPIC*)entry;
                    kstd::printf("[INFO] IOAPIC | ID: %d, Addr: %#lX, GSI base: %d\n", ioapic->ioapic_id, ioapic->ioapic_addr + hhdm, ioapic->gsi_base);
                    break;
                }
                case MADT::IOAPIC_IRQ_SOURCE_OVERRIDE: {
                    auto override = (MADT::EntryIOAPICIRQSourceOverride*)entry;
                    kstd::printf("[INFO] IOAPIC IRQ Source Override | IRQ: %d, Bus: %d, GSI: %d\n", override->irq_source, override->bus_source, override->gsi);
                    break;
                }
                case MADT::LAPIC_NMI: {
                    auto nmi = (MADT::EntryLAPICNMI*)entry;
                    kstd::printf("[INFO] LAPIC NMI | Processor ID: %d, LINT#: %d\n", nmi->processor_id, nmi->lint);
                    break;
                }
                default:
                    kstd::printf("[INFO] Found unknown entry, type: %d\n", entry->type);
                }
            }
        }
    }
}
