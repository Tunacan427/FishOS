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
    u64 virt, phy, size;
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

    u64 num_regions;
    Region *regions;

    u64 mem_size, heap_size;
    u64 heap_virt_base;
    u64 hhdm;

    kstd::Bitmap page_bitmap;
    [[gnu::aligned(0x1000)]] u64 PML4[512];

    void* request_page();
    void free_page(void *virt);

    u64 heap_virt_to_phy(u64 virt);
    u64 heap_phy_to_virt(u64 phy);
    
    void map_page(u64 phy, u64 virt, u64 flags);
    void map_pages(u64 phy, u64 virt, u64 size, u64 flags);
};

static inline void invlpg(u64 addr) {
    asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

}
