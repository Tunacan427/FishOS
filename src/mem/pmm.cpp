#include <mem/pmm.hpp>
#include <kstd/lock.hpp>
#include <kstd/cstdio.hpp>
#include <limine.hpp>

namespace mem::pmm {
    static kstd::Spinlock pmm_lock;
    static usize page_bitmap_index = 0;

    static usize total_mem_size = 0;
    static usize total_usable_size = 0;

    void init(uptr hhdm, limine_memmap_response *memmap_res) {
        auto page_bitmap = get_bitmap();

        for (usize i = 0; i < memmap_res->entry_count; i++) {
            auto entry = memmap_res->entries[i];
            if (i == memmap_res->entry_count - 1) {
                total_mem_size = entry->base + entry->length;
            }
            if (entry->type == LIMINE_MEMMAP_USABLE) 
                total_usable_size += entry->length;
        }
        kstd::printf("[INFO] Total usable memory size: %ld KiB\n", total_usable_size / 1024);

        page_bitmap->size = total_mem_size / 0x1000;

        for (usize i = 0; i < memmap_res->entry_count; i++) {
            auto entry = memmap_res->entries[i];
            if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= (page_bitmap->size / 8)) {
                page_bitmap->buffer = (u8*)(entry->base + hhdm);
                kstd::printf("[INFO] Bitmap virt addr: %#lX, bits: %#lX\n", (uptr)page_bitmap->buffer, page_bitmap->size);
                break;
            }
        }

        for (usize i = 0; i < page_bitmap->size; i++)
            page_bitmap->set(i, true);
        
        for (usize e = 0; e < memmap_res->entry_count; e++) {
            auto entry = memmap_res->entries[e];
            if (entry->type == LIMINE_MEMMAP_USABLE) {
                usize page_start = entry->base / 0x1000;
                usize page_end = entry->base / 0x1000 + entry->length / 0x1000;
                if (entry->base % 0x1000)
                    page_start++;
                if (entry->length % 0x1000)
                    page_end--;
                for (usize i = page_start; i <= page_end; i++)
                    page_bitmap->set(i, false);
            }
        }
        
        usize bitmap_self_index = ((uptr)page_bitmap->buffer - hhdm) / 0x1000;
        for (usize i = bitmap_self_index; i < bitmap_self_index + page_bitmap->size / 0x8000; i++)
            page_bitmap->set(i, true);
/*
        for (usize i = 0; i < page_bitmap->size; i++)
            kstd::putchar(page_bitmap->get(i) + '0');
*/  
    }

    kstd::Bitmap* get_bitmap() {
        static kstd::Bitmap page_bitmap;
        return &page_bitmap;
    }

    usize get_total_mem() {
        return total_mem_size;
    }

    void *alloc_page() {
        kstd::LockGuard<kstd::Spinlock> guard(pmm_lock);
        auto page_bitmap = get_bitmap();
        for (; page_bitmap_index < page_bitmap->size; page_bitmap_index++) {
            if (page_bitmap->get(page_bitmap_index)) continue;
            page_bitmap->set(page_bitmap_index, true);
            return (void*)(page_bitmap_index * 0x1000);
        }
        return nullptr;
    }

    void free_page(void *phy) {
        kstd::LockGuard<kstd::Spinlock> guard(pmm_lock);
        auto page_bitmap = get_bitmap();
        usize i = ((uptr)phy) / 0x1000;
        if (i < page_bitmap_index)
            page_bitmap_index = i;
        page_bitmap->set(i, false);
    }
}
