#pragma once

#include <klib/types.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <panic.hpp>

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

    struct [[gnu::packed]] GenericAddr {
        enum AddressSpace : u8 {
            SYSTEM_MEM, 
            SYSTEM_IO, 
            PCI_CONFIG,
            PCI_BAR = 6
        };

        AddressSpace address_space;
        u8 bit_width;
        u8 bit_offset;
        u8 access_size;
        u64 addr;

        template<typename T>
        void write(u64 offset, T val) {
            switch (this->address_space) {
            case SYSTEM_MEM:
                *(volatile T*)(addr + offset + mem::vmm::get_hhdm()) = val;
                break;
            case SYSTEM_IO:
                cpu::out<T>(addr + offset, val);
                break;
            default:
                panic("Unsupported GenericAddr address space: %#X", u8(address_space));
            }
        }

        template<typename T>
        T read(u64 offset) {
            switch (this->address_space) {
            case SYSTEM_MEM:
                return *(volatile T*)(addr + offset + mem::vmm::get_hhdm());
            case SYSTEM_IO:
                return cpu::in<T>(addr + offset);
            default:
                panic("Unsupported GenericAddr address space: %#X", u8(address_space));
            }
        }
    };
    
    void parse_sdt(SDT *sdt);
    
    struct [[gnu::packed]] RSDT : SDT {
        u32 sdt_array[];
    };
    
    struct [[gnu::packed]] XSDT : SDT {
        u64 sdt_array[];
    };
    
    struct [[gnu::packed]] HPET : SDT { 
        u8 hardware_rev_id;
        u8 info;
        u16 pci_vendor_id;
        GenericAddr address;
        u8 hpet_number;
        u16 minimum_tick;
        u8 page_protection;
    };

    void parse_hpet(HPET *hpet);
    
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

    void parse_madt(MADT *madt);
}