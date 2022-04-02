#pragma once

#include "stivale2.h"
#include <types.hpp>

#define PAGE_PRESENT 1 << 0
#define PAGE_WRITABLE 1 << 1
#define PAGE_USER 1 << 2
#define PAGE_WRITE_THROUGH 1 << 3
#define PAGE_CACHE_DISABLE 1 << 4
#define PAGE_ACCESSED 1 << 5
#define PAGE_DIRTY 1 << 6
#define PAGE_ATTRIBUTE_TABLE 1 << 7
#define PAGE_GLOBAL 1 << 8
#define PAGE_WRITE_COMBINING PAGE_ATTRIBUTE_TABLE | PAGE_CACHE_DISABLE
#define PAGE_NO_EXECUTE (u64)1 << 63

#define PAGE_DEMAND 1 << 9

namespace mem::vmm {
    void init(uptr hhdm_base, stivale2_struct_tag_memmap *tag_mmap, stivale2_struct_tag_pmrs *tag_pmrs, stivale2_struct_tag_kernel_base_address *tag_base_addr);

    uptr get_hhdm();

    void map_page(uptr phy, uptr virt, u64 flags);
    void map_pages(uptr phy, uptr virt, usize size, u64 flags);
    
    bool try_demand_page(uptr virt);
}
