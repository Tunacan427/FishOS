#include <mem/vmm.hpp>
#include <mem/pmm.hpp>
#include <panic.hpp>
#include <klib/cstdio.hpp>
#include <klib/cstring.hpp>
#include <cpu/cpu.hpp>
#include <limine.hpp>

static inline usize align_page(usize i) {
    return i % 0x1000 ? ((i / 0x1000) + 1) * 0x1000 : i;
}

namespace mem::vmm {
    const usize heap_size = 1024 * 1024 * 1024;
    const uptr heap_begin = ~(uptr)0 - heap_size - 0x1000;
    const uptr heap_end = heap_begin + heap_size;
    
    static uptr hhdm;
    static uptr kernel_phy_base;
    static uptr kernel_virt_base;

    static Pagemap kernel_pagemap;

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
        usize num_pages = align_page(size) / 0x1000;
        // klib::printf("Mapping %ld pages phy %#lX virt %#lX\n", num_pages, phy, virt);
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
        klib::LockGuard guard(this->lock);
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

    bool try_demand_page(uptr virt) {
        klib::LockGuard guard(kernel_pagemap.lock);
        u64 *current_table = (u64*)kernel_pagemap.pml4;

        current_table = page_table_next_level(current_table, (virt >> 39) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 30) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 21) & 0x1FF);

        u64 *entry = &current_table[virt >> 12 & 0x1FF];

        if (!(*entry & PAGE_PRESENT)) {
            if (virt >= heap_begin && virt <= heap_end) {
                // kernel heap, allocate a new page
                uptr new_page = pmm::alloc_pages(1);
                klib::memset((void*)(new_page + hhdm), 0, 0x1000);
                *entry = new_page | PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE;
            } else // direct map
                *entry = ((virt - hhdm) & 0x000FFFFFFFFFF000) | PAGE_PRESENT | PAGE_WRITABLE;
            return false;
        }

        return true;
    }
}
