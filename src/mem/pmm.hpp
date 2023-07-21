#pragma once

#include <klib/types.hpp>
#include <klib/bitmap.hpp>
#include <limine.hpp>

namespace mem::pmm {
    void init(uptr hhdm, limine_memmap_response *memmap_res);
    klib::Bitmap* get_bitmap();
    uptr alloc_pages(usize num_pages);
    void free_pages(uptr phy, usize num_pages);
    usize get_total_allocated();
}
