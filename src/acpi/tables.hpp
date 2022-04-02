#pragma once

#include <types.hpp>

namespace acpi {
    bool do_checksum(uptr table, usize size);
    void parse_rsdp(uptr rsdp_addr);

    struct [[gnu::packed]] RSDP1 {
        char signature[8];
        u8 checksum;
        char oem_id[6];
        u8 revision;
        u32 rsdt_addr;
    };

    struct [[gnu::packed]] RSDP2 : RSDP1 {
        u32 size;
        u64 xsdt_addr;
        u8 ext_checksum;
        u8 reserved[3];
    };
    
    // generic acpi table header
    struct [[gnu::packed]] SDT {
        char signature[4];
        u32 size;
        u8 revision;
        u8 checksum;
        char oem_id[6];
        char oem_table_id[8];
        u32 oem_revision;
        char creator_id[4];
        u32 creator_revision;
    };
    
    void parse_sdt(SDT *sdt);
    
    struct [[gnu::packed]] RSDT : SDT {
        u32 sdt_array[];
    };
    
    struct [[gnu::packed]] XSDT : SDT {
        u64 sdt_array[];
    };
    
    struct [[gnu::packed]] MADT : SDT {
        u32 lapic_addr;
        u32 flags;

        enum EntryType : u8 {
            LAPIC, 
            IOAPIC, 
            IOAPIC_IRQ_SOURCE_OVERRIDE,
            IOAPIC_NMI_SOURCE,
            LAPIC_NMI,
            LAPIC_ADDR_OVERRIDE,
            LX2APIC = 9
        };

        struct [[gnu::packed]] Entry {
            EntryType type;
            u8 size;
        };

        struct [[gnu::packed]] EntryLAPIC : Entry {
            u8 processor_id;
            u8 lapic_id;
            u32 flags;
        };

        struct [[gnu::packed]] EntryIOAPIC : Entry {
            u8 ioapic_id;
            u8 reserved;
            u32 ioapic_addr;
            u32 gsi_base;
        };

        struct [[gnu::packed]] EntryIOAPICIRQSourceOverride : Entry {
            u8 bus_source;
            u8 irq_source;
            u32 gsi;
            u16 flags;
        };

        struct [[gnu::packed]] EntryIOAPICNMISource : Entry {
            u8 nmi_source;
            u8 reserved;
            u16 flags;
            u32 gsi;
        };

        struct [[gnu::packed]] EntryLAPICNMI : Entry {
            u8 processor_id;
            u16 flags;
            u8 lint;
        };

        struct [[gnu::packed]] EntryLAPICAddressOverride : Entry {
            u16 reserved;
            u64 lapic_addr;
        };

        struct [[gnu::packed]] EntryLX2APIC : Entry {
            u16 reserved;
            u32 lx2apic_id;
            u32 flags;
            u32 acpi_id;
        };
    };
}