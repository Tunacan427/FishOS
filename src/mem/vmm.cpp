#include <mem/vmm.hpp>
#include <mem/pmm.hpp>
#include <panic.hpp>
#include <klib/cstdio.hpp>
#include <klib/cstring.hpp>
#include <klib/algorithm.hpp>
#include <klib/posix.hpp>
#include <cpu/cpu.hpp>
#include <cpu/syscall/syscall.hpp>
#include <sched/sched.hpp>
#include <limine.hpp>

namespace mem::vmm {
    const usize heap_size = 1024 * 1024 * 1024;
    const uptr heap_begin = ~(uptr)0 - heap_size - 0x1000;
    const uptr heap_end = heap_begin + heap_size;
    
    static uptr hhdm;
    static uptr kernel_phy_base;
    static uptr kernel_virt_base;

    static Pagemap kernel_pagemap;
    static MappedRange kernel_hhdm_range;
    static MappedRange kernel_heap_range;

    void init(uptr hhdm_base, limine_memmap_response *memmap_res, limine_kernel_address_response *kernel_addr_res) {
        hhdm = hhdm_base;
        kernel_phy_base = kernel_addr_res->physical_base;
        kernel_virt_base = kernel_addr_res->virtual_base;

        kernel_pagemap.pml4 = (u64*)(pmm::alloc_pages(1) + hhdm);
        klib::memset(kernel_pagemap.pml4, 0, 0x1000);

        usize kernel_size = 0;

        klib::printf("VMM: Physical memory map:\n");
        for (u64 i = 0; i < memmap_res->entry_count; i++) {
            auto entry = memmap_res->entries[i];
            const char *entry_name;
            u64 flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE;

            switch (entry->type) {
            case LIMINE_MEMMAP_USABLE: entry_name = "Usable"; break;
            case LIMINE_MEMMAP_RESERVED: entry_name = "Reserved"; break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE: entry_name = "ACPI Reclaimable"; break;
            case LIMINE_MEMMAP_ACPI_NVS: entry_name = "ACPI NVS"; break;
            case LIMINE_MEMMAP_BAD_MEMORY: entry_name = "Bad Memory"; break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: entry_name = "Bootloader Reclaimable"; flags &= ~PAGE_NO_EXECUTE; break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES: entry_name = "Kernel and Modules"; kernel_size += entry->length; break;
            case LIMINE_MEMMAP_FRAMEBUFFER: entry_name = "Framebuffer"; flags |= PAGE_WRITE_COMBINING; break;
            default: entry_name = "Unknown";
            }

            klib::printf("    %s | base: %#lX, size: %ld KiB\n", entry_name, entry->base, entry->length / 1024);

            if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE || entry->type == LIMINE_MEMMAP_FRAMEBUFFER || entry->type == LIMINE_MEMMAP_USABLE)
                kernel_pagemap.map_pages(entry->base, entry->base + hhdm, entry->length, flags);
        }

        klib::printf("VMM: Kernel base addresses | phy: %#lX, virt: %#lX\n", kernel_phy_base, kernel_virt_base);
        
/*
        klib::printf("VMM: Kernel PMRs:\n");
        for (usize i = 0; i < tag_pmrs->entries; i++) {
            auto pmr = tag_pmrs->pmrs[i];
            u64 flags = PAGE_PRESENT;
            if (pmr.permissions & STIVALE2_PMR_WRITABLE) flags |= PAGE_WRITABLE;
            if (!(pmr.permissions & STIVALE2_PMR_EXECUTABLE)) flags |= PAGE_NO_EXECUTE;

            map_pages((pmr.base - kernel_virt_base) + kernel_phy_base, pmr.base, pmr.length, flags);
            
            klib::printf("       base %#lX, size: %ld KiB, permissions: %ld\n", pmr.base, pmr.length / 1024, pmr.permissions);
        }
*/

        kernel_pagemap.map_pages(kernel_phy_base, kernel_virt_base, kernel_size, PAGE_PRESENT | PAGE_WRITABLE);
        kernel_pagemap.range_list_head.init();
        kernel_hhdm_range = {
            .base = hhdm_base,
            .length = (u64)1024 * 1024 * 1024 * 1024,
            .page_flags = PAGE_PRESENT | PAGE_WRITABLE,
            .type = MappedRange::Type::DIRECT
        };
        kernel_pagemap.range_list_head.add(&kernel_hhdm_range.range_list);
        kernel_heap_range = { 
            .base = heap_begin,
            .length = heap_size,
            .page_flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE,
            .type = MappedRange::Type::ANONYMOUS
        };
        kernel_pagemap.range_list_head.add(&kernel_heap_range.range_list);
        kernel_pagemap.activate();
    }

    uptr get_hhdm() {
        return hhdm;
    }

    Pagemap *get_kernel_pagemap() {
        return &kernel_pagemap;
    }

    static u64* page_table_next_level(u64 *current_table, usize index) {
        u64 *next_table = nullptr;
        u64 current_entry = current_table[index];
        if (current_entry & PAGE_PRESENT) {
            next_table = (u64*)((current_entry & 0x000FFFFFFFFFF000) + hhdm);
        } else {
            uptr new_page = pmm::alloc_pages(1);
            klib::memset((void*)(new_page + hhdm), 0, 0x1000);
            current_table[index] = new_page | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
            next_table = (u64*)(new_page + hhdm);
        }
        return next_table;
    }

    void Pagemap::map_page(uptr phy, uptr virt, u64 flags) {
        klib::LockGuard guard(this->lock);
        u64 *current_table = this->pml4;

        // klib::printf("Phy: %#lX, Virt: %#lX, Flags: %#lX\n", phy, virt, flags);

        current_table = page_table_next_level(current_table, (virt >> 39) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 30) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 21) & 0x1FF);

        u64 *entry = &current_table[virt >> 12 & 0x1FF];
        bool replaced = *entry != 0;
        *entry = (phy & 0x000FFFFFFFFFF000) | flags;
        if (replaced) cpu::invlpg((void*)virt); // TODO: find a better way to do this crap
    }

    void Pagemap::map_pages(uptr phy, uptr virt, usize size, u64 flags) {
        usize num_pages = klib::align_up<usize, 0x1000>(size) / 0x1000;
        // klib::printf("Mapping %ld pages phy %#lX virt %#lX end %#lX\n", num_pages, phy, virt, phy + num_pages * 0x1000);
        for (usize i = 0; i < num_pages; i++)
            map_page(phy + (i * 0x1000), virt + (i * 0x1000), flags);
    }

    void Pagemap::map_kernel() {
        klib::LockGuard guard(this->lock);
        auto kernel_pagemap = get_kernel_pagemap();
        for (usize i = 256; i < 512; i++) {
            pml4[i] = kernel_pagemap->pml4[i];
        }
    }

    void Pagemap::activate() {
        // bool is_pagemap_empty = true;
        // for (usize i = 0; i < 512; i++)
        //     if (pml4[i] != 0) { is_pagemap_empty = false; break; }
        // if (is_pagemap_empty)
        //     panic("Tried to activate pagemap %#lX (pml4: %#lX) but its completely empty", (uptr)this, (uptr)pml4);
        cpu::write_cr3(uptr(pml4) - hhdm);
    }

    uptr Pagemap::physical_addr(uptr virt) {
        u64 *current_table = pml4;

        // TODO: make these not allocate if they cant find
        current_table = page_table_next_level(current_table, (virt >> 39) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 30) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 21) & 0x1FF);

        u64 *entry = &current_table[virt >> 12 & 0x1FF];
        return *entry & 0x000FFFFFFFFFF000;
    }

    // TODO: maybe optimize somehow
    MappedRange* Pagemap::addr_to_range(uptr virt) {
        klib::ListHead *current = &this->range_list_head;
        while (true) {
            if (current->next == &this->range_list_head)
                return nullptr;
            current = current->next;
            MappedRange *range = LIST_ENTRY(current, MappedRange, range_list);
            if (virt >= range->base && virt <= range->base + range->length)
                return range;
        }
    }

    // returns true if the page fault couldnt be handled
    bool Pagemap::handle_page_fault(uptr virt) {
        klib::LockGuard guard(this->lock);
        u64 *current_table = this->pml4;

        current_table = page_table_next_level(current_table, (virt >> 39) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 30) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 21) & 0x1FF);

        u64 *entry = &current_table[virt >> 12 & 0x1FF];

        if (!(*entry & PAGE_PRESENT)) {
            MappedRange *range = addr_to_range(virt);
            if (range == nullptr)
                return true;
            
            switch (range->type) {
            case MappedRange::Type::ANONYMOUS: {
                // allocate a new page
                uptr new_page = pmm::alloc_pages(1);
                klib::memset((void*)(new_page + hhdm), 0, 0x1000);
                *entry = new_page | range->page_flags;
                return false;
            }
            case MappedRange::Type::DIRECT:
                *entry = ((virt - hhdm) & 0x000FFFFFFFFFF000) | range->page_flags;
                return false;
            default:
                klib::printf("Unknown mapped range type: %#lX\n", u64(range->type));
                return true;
            }
        } else return true;
    }
    
    isize syscall_mmap(void *hint, usize length, int prot, int flags, int fd, usize offset) {
#if SYSCALL_TRACE
        klib::printf("mmap(%#lX, %ld, %d, %d, %d, %ld)\n", (uptr)hint, length, prot, flags, fd, offset);
#endif
        auto *task = (sched::Task*)cpu::read_gs_base();
        if (!(flags & MAP_PRIVATE) || (flags & MAP_SHARED) || !(flags & MAP_ANONYMOUS))
            return -ENOSYS; // only private anonymous mapping is supported
        
        u64 page_flags = PAGE_PRESENT | PAGE_USER;
        if (prot & PROT_WRITE)
            page_flags |= PAGE_WRITABLE;
        if (!(prot & PROT_EXEC))
            page_flags |= PAGE_NO_EXECUTE;
        
        uptr base = task->mmap_anon_base;
        usize aligned_size = klib::align_up<usize, 0x1000>(length);

        MappedRange *range = new MappedRange();
        range->base = base;
        range->length = aligned_size;
        range->page_flags = page_flags;
        range->type = MappedRange::Type::ANONYMOUS;
        task->pagemap->range_list_head.add(&range->range_list);

        task->mmap_anon_base += aligned_size;
        return base;
    }
}
