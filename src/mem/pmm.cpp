#include <mem/pmm.hpp>
#include <klib/lock.hpp>
#include <klib/cstdio.hpp>
#include <klib/cstring.hpp>
#include <klib/algorithm.hpp>
#include <panic.hpp>
#include <limine.hpp>

namespace mem::pmm {
    static klib::Spinlock pmm_lock;
    static usize page_bitmap_index = 0;

    static uptr last_usable_mem = 0;
    static usize total_usable_size = 0;
    static usize total_allocated = 0;

    void init(uptr hhdm, limine_memmap_response *memmap_res) {
        auto page_bitmap = get_bitmap();

        for (usize i = 0; i < memmap_res->entry_count; i++) {
            auto entry = memmap_res->entries[i];
            uptr new_usable_mem = entry->base + entry->length;
            if (new_usable_mem > last_usable_mem && entry->type == LIMINE_MEMMAP_USABLE)
                last_usable_mem = new_usable_mem;
        }

        page_bitmap->m_size = last_usable_mem / 0x1000;
        klib::printf("PMM: Bitmap range: %ld KiB\n", last_usable_mem);

        for (usize i = 0; i < memmap_res->entry_count; i++) {
            auto entry = memmap_res->entries[i];
            if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= (page_bitmap->m_size / 8)) {
                page_bitmap->m_buffer = (u8*)(entry->base + hhdm);
                klib::printf("PMM: Bitmap virt addr: %#lX, bits: %#lX\n", (uptr)page_bitmap->m_buffer, page_bitmap->m_size);
                break;
            }
        }

        page_bitmap->fill(true);
        
        for (usize e = 0; e < memmap_res->entry_count; e++) {
            auto entry = memmap_res->entries[e];
            if (entry->type == LIMINE_MEMMAP_USABLE) {
                usize page_start = entry->base / 0x1000;
                usize page_end = entry->base / 0x1000 + entry->length / 0x1000;
                if (entry->base % 0x1000)
                    page_start++;
                if (entry->length % 0x1000)
                    page_end--;
                for (usize i = page_start; i < page_end; i++) {
                    page_bitmap->set(i, false);
                    total_usable_size += 0x1000;
                }
            }
        }
        
        usize bitmap_self_index = (uptr(page_bitmap->m_buffer) - hhdm) / 0x1000;
        for (usize i = bitmap_self_index; i < bitmap_self_index + klib::align_up<usize, 0x1000>(page_bitmap->m_size / 8) / 0x1000; i++) {
            page_bitmap->set(i, true);
            total_usable_size -= 0x1000;
        }
    }

    klib::Bitmap* get_bitmap() {
        static klib::Bitmap page_bitmap;
        return &page_bitmap;
    }

    uptr inner_alloc(usize num_pages, usize limit) {
        auto page_bitmap = get_bitmap();
        usize pages = 0;

        while (page_bitmap_index < limit) {
            if (!page_bitmap->get(page_bitmap_index)) {
                page_bitmap_index++;
                pages++;
                if (pages == num_pages) {
                    usize beginning = page_bitmap_index - num_pages;
                    for (usize i = beginning; i < page_bitmap_index; i++)
                        page_bitmap->set(i, true);
                    return beginning * 0x1000;
                }
            } else {
                page_bitmap_index++;
                pages = 0;
            }
        }

        return 0;
    }

    uptr alloc_pages(usize num_pages) {
        klib::LockGuard guard(pmm_lock);

        usize last = page_bitmap_index;
        uptr result = inner_alloc(num_pages, get_bitmap()->m_size);

        if (result == 0) {
            page_bitmap_index = 0; // try again from the beginning just in case
            result = inner_alloc(num_pages, last);
            if (result == 0)
                panic("Out of physical memory (%ld KiB has been allocated)", total_allocated / 1024);
        }

        total_allocated += num_pages * 0x1000;
        return result;
    }

    void free_pages(uptr phy, usize num_pages) {
        klib::LockGuard guard(pmm_lock);
        auto page_bitmap = get_bitmap();

        usize i = phy / 0x1000;
        if (i < page_bitmap_index)
            page_bitmap_index = i;
        
        for (; i < num_pages + i; i++)
            page_bitmap->set(i, false);

        total_allocated -= num_pages * 0x1000;
    }

    usize get_total_allocated() {
        return total_allocated;
    }
}
