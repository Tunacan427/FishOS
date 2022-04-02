#include "stivale2.h"
#include <mem/pmm.hpp>
#include <kstd/lock.hpp>
#include <kstd/cstdio.hpp>

namespace mem::pmm {
    static volatile kstd::Spinlock pmm_lock;
    static usize page_bitmap_index = 0;

    static usize total_mem_size = 0;
    static usize total_usable_size = 0;

    void init(uptr hhdm, stivale2_struct_tag_memmap *tag_mmap) {
        auto page_bitmap = get_bitmap();

        for (usize i = 0; i < tag_mmap->entries; i++) {
            auto entry = tag_mmap->memmap[i];
            total_mem_size += entry.length;
            if (entry.type == STIVALE2_MMAP_USABLE) 
                total_usable_size += entry.length;
        }
        kstd::printf("[INFO] Total usable memory size: %ld KiB\n", total_usable_size / 1024);

        page_bitmap->size = total_mem_size / 0x1000;

        for (usize i = 0; i < tag_mmap->entries; i++) {
            auto entry = tag_mmap->memmap[i];
            if (entry.type == STIVALE2_MMAP_USABLE && entry.length >= (page_bitmap->size / 8)) {
                page_bitmap->buffer = (u8*)(entry.base + hhdm);
                kstd::printf("[INFO] Bitmap virt addr: %#lX, size: %#lX\n", (uptr)page_bitmap->buffer, page_bitmap->size);
                break;
            }
        }

        for (usize i = 0; i < page_bitmap->size; i++)
            page_bitmap->set(i, true);
        
        for (usize e = 0; e < tag_mmap->entries; e++) {
            auto entry = tag_mmap->memmap[e];
            for (usize i = entry.base / 0x1000; i <= (entry.base / 0x1000) + (entry.length / 0x1000); i++)
                page_bitmap->set(i, entry.type != STIVALE2_MMAP_USABLE);
        }
        
        usize bitmap_self_index = ((uptr)page_bitmap->buffer - hhdm) / 0x1000;
        for (usize i = bitmap_self_index; i < bitmap_self_index + page_bitmap->size / 0x8000; i++)
            page_bitmap->set(i, true);

        // for (usize i = 0; i < page_bitmap->size; i++)
        //    kstd::putchar(page_bitmap->get(i) + '0');
    }

    kstd::Bitmap* get_bitmap() {
        static kstd::Bitmap page_bitmap;
        return &page_bitmap;
    }

    usize get_total_mem() {
        return total_mem_size;
    }

    void *alloc_page() {
        pmm_lock.lock();
        auto page_bitmap = get_bitmap();
        void *result = nullptr;
        for (; page_bitmap_index < page_bitmap->size; page_bitmap_index++) {
            if (page_bitmap->get(page_bitmap_index)) continue;
            page_bitmap->set(page_bitmap_index, true);
            result = (void*)(page_bitmap_index * 0x1000);
            break;
        }
        pmm_lock.unlock();
        return result;
    }

    void free_page(void *phy) {
        pmm_lock.lock();
        auto page_bitmap = get_bitmap();
        usize i = ((uptr)phy) / 0x1000;
        if (i < page_bitmap_index)
            page_bitmap_index = i;
        page_bitmap->set(i, false);
        pmm_lock.unlock();
    }
}
