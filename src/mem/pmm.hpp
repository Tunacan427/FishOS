#pragma once

#include <kstd/types.hpp>
#include <kstd/bitmap.hpp>
#include <limine.hpp>

namespace mem::pmm {
    void init(uptr hhdm, limine_memmap_response *memmap_res);

    kstd::Bitmap* get_bitmap();
    usize get_total_mem();

    void* alloc_page();
    void free_page(void *phy);
}
