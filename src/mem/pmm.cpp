#include "mem/vmm.hpp"
#include <mem/pmm.hpp>
#include <klib/lock.hpp>
#include <klib/cstdio.hpp>
#include <klib/cstring.hpp>
#include <panic.hpp>
#include <limine.hpp>

namespace mem::pmm {
    static klib::Spinlock pmm_lock;
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
        klib::printf("[INFO] Total usable memory size: %ld KiB\n", total_usable_size / 1024);

        page_bitmap->m_size = total_mem_size / 0x1000;

        for (usize i = 0; i < memmap_res->entry_count; i++) {
            auto entry = memmap_res->entries[i];
            if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= (page_bitmap->m_size / 8)) {
                page_bitmap->m_buffer = (u8*)(entry->base + hhdm);
                klib::printf("[INFO] Bitmap virt addr: %#lX, bits: %#lX\n", (uptr)page_bitmap->m_buffer, page_bitmap->m_size);
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
                for (usize i = page_start; i <= page_end; i++)
                    page_bitmap->set(i, false);
            }
        }
        
        usize bitmap_self_index = ((uptr)page_bitmap->m_buffer - hhdm) / 0x1000;
        for (usize i = bitmap_self_index; i < bitmap_self_index + page_bitmap->m_size / 0x8000; i++)
            page_bitmap->set(i, true);
    }

    klib::Bitmap* get_bitmap() {
        static klib::Bitmap page_bitmap;
        return &page_bitmap;
    }

    usize get_total_mem() {
        return total_mem_size;
    }

    void* inner_alloc(usize num_pages, usize limit) {
        auto page_bitmap = get_bitmap();
        usize pages = 0;

        while (page_bitmap_index < limit) {
            if (!page_bitmap->get(page_bitmap_index)) {
                page_bitmap_index++;
                pages++;
                if (pages == num_pages) {
                    usize beginning = page_bitmap_index - num_pages;
                    for (usize i = beginning; i < page_bitmap_index; i++) {
                        page_bitmap->set(i, true);
                    }
                    return (void*)(beginning * 0x1000);
                }
            } else {
                page_bitmap_index++;
                pages = 0;
            }
        }

        return nullptr;
    }

    void* alloc_pages(usize num_pages) {
        klib::LockGuard<klib::Spinlock> guard(pmm_lock);

        usize last = page_bitmap_index;
        void *result = inner_alloc(num_pages, get_bitmap()->m_size);

        if (result == nullptr) {
            page_bitmap_index = 0;
            result = inner_alloc(num_pages, last);
            if (result == nullptr) {
                panic("Out of memory");
            }
        }

        return result;
    }

    void* calloc_pages(usize num_pages) {
        void *pages = alloc_pages(num_pages);
        klib::memset((void*)((uptr)pages + vmm::get_hhdm()), 0, num_pages * 0x1000);
        return pages;
    }

    void free_pages(void *phy, usize num_pages) {
        klib::LockGuard<klib::Spinlock> guard(pmm_lock);
        auto page_bitmap = get_bitmap();

        usize i = ((uptr)phy) / 0x1000;
        if (i < page_bitmap_index)
            page_bitmap_index = i;
        
        for (; i < num_pages + i; i++)
            page_bitmap->set(i, false);
    }
}
