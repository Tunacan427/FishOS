#pragma once

#include <types.hpp>
#include <stivale2.h>
#include <kstd/bitmap.hpp>

namespace mem::pmm {
    void init(uptr hhdm, stivale2_struct_tag_memmap *tag_mmap);

    kstd::Bitmap* get_bitmap();
    usize get_total_mem();

    void* alloc_page();
    void free_page(void *phy);
}
