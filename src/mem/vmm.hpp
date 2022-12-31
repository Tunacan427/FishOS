#pragma once

#include <klib/types.hpp>
#include <limine.hpp>

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_WRITE_THROUGH (1 << 3)
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_ACCESSED (1 << 5)
#define PAGE_DIRTY (1 << 6)
#define PAGE_ATTRIBUTE_TABLE (1 << 7)
#define PAGE_GLOBAL (1 << 8)
#define PAGE_WRITE_COMBINING (PAGE_ATTRIBUTE_TABLE | PAGE_CACHE_DISABLE)
#define PAGE_NO_EXECUTE ((u64)1 << 63)

namespace mem::vmm {
    struct Pagemap {
        u64 *pml4;
        bool active;

        void map_page(uptr phy, uptr virt, u64 flags);
        void map_pages(uptr phy, uptr virt, usize size, u64 flags);
    };

    void init(uptr hhdm_base, limine_memmap_response *memmap_res, limine_kernel_address_response *kernel_addr_res);
    void activate_pagemap(Pagemap *pagemap);

    uptr get_hhdm();
    Pagemap *get_kernel_pagemap();

    bool try_demand_page(uptr virt);
}
