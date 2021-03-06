#include <mem/vmm.hpp>
#include <mem/pmm.hpp>
#include <panic.hpp>
#include <kstd/lock.hpp>
#include <kstd/cstdio.hpp>
#include <kstd/cstring.hpp>
#include <cpu/cpu.hpp>
#include <limine.hpp>

static inline usize align_page(usize i) {
    return i % 0x1000 ? ((i / 0x1000) + 1) * 0x1000 : i;
}

namespace mem::vmm {
    const usize heap_size = 1024 * 1024 * 1024;
    const uptr heap_begin = ~(uptr)0 - heap_size - 0x1000;
    const uptr heap_end = heap_begin + heap_size;
    
    static kstd::Spinlock vmm_lock;
    [[gnu::aligned(0x1000)]] static u64 PML4[512];
    static uptr hhdm;

    void init(uptr hhdm_base, limine_memmap_response *memmap_res, limine_kernel_address_response *kernel_addr_res) {
        hhdm = hhdm_base;
        uptr kernel_phy_base = kernel_addr_res->physical_base;
        uptr kernel_virt_base = kernel_addr_res->virtual_base;

        u64 cr4 = cpu::read_cr4();
        kstd::printf("[INFO] Original CR4: %#lX\n", cr4);

        u32 eax, ebx, ecx, edx;
        cpu::cpuid(7, 0, &eax, &ebx, &ecx, &edx);
        kstd::printf("[INFO] CPUID Leaf 7: EAX: %#X, EBX: %#X, ECX: %#X, EDX: %#X\n", eax, ebx, ecx, edx);

        // enable SMAP if available
        if (ebx & (1 << 20))
            cr4 |= 1 << 21;
        
        // enable SMEP if available
        if (ebx & (1 << 7))
            cr4 |= 1 << 20;
        
        // enable UMIP if available
        if (ecx & (1 << 2))
            cr4 |= 1 << 11;

        kstd::printf("[INFO] New CR4: %#lX\n", cr4);
        cpu::write_cr4(cr4);

        cpu::cpuid(1, 0, &eax, &ebx, &ecx, &edx);
        kstd::printf("[INFO] CPUID Leaf 1: EAX: %#X, EBX: %#X, ECX: %#X, EDX: %#X\n", eax, ebx, ecx, edx);

        // panic if PAT is not available
        if (!(edx & (1 << 16)))
            panic("CPU does not have PAT");

        // hardcode PAT
        // 0: WB  1: WT  2: UC-  3: UC  4: WB  5: WT  6: WC  7: WP
        cpu::write_msr(0x0277, (6 << 0) | (4 << 8) | (7 << 16) | (0 << 24) | (6l << 32) | (4l << 40) | (1l << 48) | (5l << 56));
        
        usize kernel_size = 0;

        kstd::printf("[INFO] Physical memory map:\n");
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
            case LIMINE_MEMMAP_KERNEL_AND_MODULES: entry_name = "Kernel and Modules"; kernel_size = entry->length; break;
            case LIMINE_MEMMAP_FRAMEBUFFER: entry_name = "Framebuffer"; flags |= PAGE_WRITE_COMBINING; break;
            default: entry_name = "Unknown";
            }

            kstd::printf("       %s | base: %#lX, size: %ld KiB\n", entry_name, entry->base, entry->length / 1024);

            if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE || entry->type == LIMINE_MEMMAP_FRAMEBUFFER || entry->type == LIMINE_MEMMAP_USABLE) {
                map_pages(entry->base, entry->base + hhdm, entry->length, flags);
                if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE || entry->type == LIMINE_MEMMAP_FRAMEBUFFER)
                    map_pages(entry->base, entry->base, entry->length, flags);
            }
        }

        kstd::printf("[INFO] Kernel base addresses | phy: %#lX, virt: %#lX\n", kernel_phy_base, kernel_virt_base);
        
        /*
        kstd::printf("[INFO] Kernel PMRs:\n");
        for (usize i = 0; i < tag_pmrs->entries; i++) {
            auto pmr = tag_pmrs->pmrs[i];
            u64 flags = PAGE_PRESENT;
            if (pmr.permissions & STIVALE2_PMR_WRITABLE) flags |= PAGE_WRITABLE;
            if (!(pmr.permissions & STIVALE2_PMR_EXECUTABLE)) flags |= PAGE_NO_EXECUTE;

            map_pages((pmr.base - kernel_virt_base) + kernel_phy_base, pmr.base, pmr.length, flags);
            
            kstd::printf("       base %#lX, size: %ld KiB, permissions: %ld\n", pmr.base, pmr.length / 1024, pmr.permissions);
        }
        */

        map_pages(kernel_phy_base, kernel_virt_base, kernel_size, PAGE_PRESENT | PAGE_WRITABLE);
/*
        map_pages(0, ~(u64)0 - heap_size - 0x1000, heap_size + 0x1000, PAGE_DEMAND);
*/
        cpu::write_cr3((uptr)&PML4[0] - kernel_virt_base + kernel_phy_base);
    }

    uptr get_hhdm() {
        return hhdm;
    }

    static inline u64* page_table_next_level(u64 *current_table, usize index) {
        u64 *next_table = nullptr;
        u64 current_entry = current_table[index];
        if (current_entry & PAGE_PRESENT) {
            next_table = (u64*)((current_entry & 0x000FFFFFFFFFF000) + hhdm);
            // kstd::printf("Page table exists, next table at %#lX\n", (u64)current_table);
        } else {
            next_table = (u64*)pmm::alloc_page();
            current_table[index] = (u64)next_table | PAGE_PRESENT | PAGE_WRITABLE;
            next_table = (u64*)((uptr)next_table + hhdm);
            kstd::memset(next_table, 0, 0x1000);
            // kstd::printf("Page table created at %#lX, current entry %#lX\n", (u64)next_table, current_entry);
        }
        return next_table;
    }

    void map_page(uptr phy, uptr virt, u64 flags) {
        kstd::LockGuard<kstd::Spinlock> guard(vmm_lock);
        u64 *current_table = (u64*)PML4;
        // kstd::printf("Map page phy %#lX virt %#lX\n", phy, virt);

        current_table = page_table_next_level(current_table, (virt >> 39) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 30) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 21) & 0x1FF);

        u64 *entry = &current_table[virt >> 12 & 0x1FF];
        bool replaced = *entry != 0;
        *entry = (phy & 0x000FFFFFFFFFF000) | flags;
        if (replaced) cpu::invlpg((void*)virt);
    }

    void map_pages(uptr phy, uptr virt, usize size, u64 flags) {
        usize num_pages = align_page(size) / 0x1000;
        // kstd::printf("Mapping %ld pages phy %#lX virt %#lX\n", num_pages, phy, virt);
        for (usize i = 0; i < num_pages; i++)
            map_page(phy + (i * 0x1000), virt + (i * 0x1000), flags);
    }

    bool try_demand_page(uptr virt) {
        kstd::LockGuard<kstd::Spinlock> guard(vmm_lock);
        u64 *current_table = (u64*)PML4;

        current_table = page_table_next_level(current_table, (virt >> 39) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 30) & 0x1FF);
        current_table = page_table_next_level(current_table, (virt >> 21) & 0x1FF);

        u64 *entry = &current_table[virt >> 12 & 0x1FF];

        if (!(*entry & PAGE_PRESENT)) {
            if (virt >= heap_begin && virt <= heap_end) // kernel heap, allocate a new page
                *entry = ((u64)pmm::alloc_page() & 0x000FFFFFFFFFF000) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE;
            else // direct map
                *entry = ((virt - hhdm) & 0x000FFFFFFFFFF000) | PAGE_PRESENT | PAGE_WRITABLE;
            return false;
        }

        return true;
    }
}
