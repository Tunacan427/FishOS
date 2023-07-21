#pragma once

#include <klib/types.hpp>
#include <klib/lock.hpp>
#include <klib/list.hpp>
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
    struct MappedRange {
        enum class Type {
            DIRECT,
            ANONYMOUS
        };

        klib::ListHead range_list;
        uptr base;
        uptr length;
        u64 page_flags;
        Type type;
    };

    struct Pagemap {
        u64 *pml4;
        klib::Spinlock lock;
        klib::ListHead range_list_head;

        void activate();
        uptr physical_addr(uptr virt);

        void map_page(uptr phy, uptr virt, u64 flags);
        void map_pages(uptr phy, uptr virt, usize size, u64 flags);
        void map_kernel(); // for user pagemaps

        MappedRange* addr_to_range(uptr virt);
        bool handle_page_fault(uptr virt);
    };

    void init(uptr hhdm_base, limine_memmap_response *memmap_res, limine_kernel_address_response *kernel_addr_res);

    uptr get_hhdm();
    Pagemap* get_kernel_pagemap();

    isize syscall_mmap(void *hint, usize length, int prot, int flags, int fd, usize offset);
}
