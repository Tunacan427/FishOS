#pragma once

#include <types.hpp>
#include <kstd/bitmap.hpp>
#include <stivale2.h>

#define PAGE_PRESENT 1 << 0
#define PAGE_WRITABLE 1 << 1
#define PAGE_USER 1 << 2
#define PAGE_WRITE_THROUGH 1 << 3
#define PAGE_CACHE_DISABLE 1 << 4
#define PAGE_ACCESSED 1 << 5
#define PAGE_DIRTY 1 << 6
#define PAGE_GLOBAL 1 << 8
#define PAGE_NO_EXECUTE (u64)1 << 63

namespace mem {
    enum class RegionType : u8 {
        USABLE,
        RESERVED,
        ACPI_RECLAIMABLE,
        ACPI_NVS,
        BAD_MEMORY,
        BOOTLOADER_RECLAIMABLE,
        KERNEL_AND_MODULES,
        FRAMEBUFFER
    };

    const char *region_type_string(RegionType type);

    struct [[gnu::packed]] Region {
        uptr virt, phy;
        usize size;
        RegionType type;
    };

    class Manager {
        Manager() {}
    public:
        void boot(
            stivale2_struct_tag_memmap *tag_mmap,
            stivale2_struct_tag_pmrs *tag_pmrs,
            stivale2_struct_tag_kernel_base_address *tag_base_addr,
            stivale2_struct_tag_hhdm *tag_hhdm
        );

        static Manager* get();

        usize num_regions;
        Region *regions;

        usize mem_size, heap_size;
        uptr heap_virt_base;
        usize hhdm;

        kstd::Bitmap page_bitmap;
        [[gnu::aligned(0x1000)]] u64 PML4[512];

        void* request_page();
        void* request_pages(usize pages_requested);
        void free_page(void *virt);
        void free_pages(void *virt, usize pages);

        uptr heap_virt_to_phy(uptr virt);
        uptr heap_phy_to_virt(uptr phy);
        
        void map_page(uptr phy, uptr virt, u64 flags);
        void map_pages(uptr phy, uptr virt, usize size, u64 flags);
    };

    static inline void invlpg(uptr addr) {
        asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
    }
}
