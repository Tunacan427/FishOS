#include <kstd/cstdio.hpp>
#include <mem/manager.hpp>
#include <kstd/cstring.hpp>
#include <kstd/mutex.hpp>

static volatile kstd::Mutex vmm_mutex;
static volatile kstd::Mutex pmm_mutex;

namespace mem {
    const char *region_type_strings[] = {
        "Usable",
        "Reserved",
        "ACPI Reclaimable",
        "ACPI NVS",
        "Bad Memory",
        "Bootloader Reclaimable",
        "Kernel and Modules",
        "Framebuffer"
    };

    const char *region_type_string(RegionType type) {
        return region_type_strings[u8(type)];
    }

    void Manager::boot(
        stivale2_struct_tag_memmap *tag_mmap,
        stivale2_struct_tag_pmrs *tag_pmrs,
        stivale2_struct_tag_kernel_base_address *tag_base_addr,
        stivale2_struct_tag_hhdm *tag_hhdm
    ) {
        kstd::memset(this->PML4, 0, 0x1000);
        this->hhdm = tag_hhdm->addr;
        bool is_first_heap = true;
        uptr first_heap_phy;
        for (usize i = 0; i < tag_mmap->entries; i++) {
            auto entry = tag_mmap->memmap[i];
            if (entry.type == STIVALE2_MMAP_USABLE) {
                if (is_first_heap) {
                    is_first_heap = false;
                    first_heap_phy = entry.base;
                }
                this->heap_size += entry.length;
            }
            this->mem_size += entry.length;
        }
        this->heap_virt_base = first_heap_phy + this->hhdm + 0x100000000;
        this->regions = (Region*)first_heap_phy;
        uptr regions_virt_base = this->heap_virt_base;
        uptr next_virt_base = this->heap_virt_base + 0x1000;
        uptr virt_heap_end = this->heap_virt_base;
        for (int i = 0; i < tag_mmap->entries; i++) {
            auto entry = tag_mmap->memmap[i];
            RegionType type;
            uptr virt = this->hhdm + entry.base;
            switch (entry.type) {
            case STIVALE2_MMAP_USABLE:
                type = RegionType::USABLE;
                virt = virt_heap_end;
                virt_heap_end += entry.length;
                break;
            case STIVALE2_MMAP_RESERVED: type = RegionType::RESERVED; break;
            case STIVALE2_MMAP_ACPI_RECLAIMABLE: type = RegionType::ACPI_RECLAIMABLE; break;
            case STIVALE2_MMAP_ACPI_NVS: type = RegionType::ACPI_NVS; break;
            case STIVALE2_MMAP_BAD_MEMORY: type = RegionType::BAD_MEMORY; break;
            case STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE: type = RegionType::BOOTLOADER_RECLAIMABLE; break;
            case STIVALE2_MMAP_KERNEL_AND_MODULES:
                type = RegionType::KERNEL_AND_MODULES; 
                virt = tag_base_addr->virtual_base_address;
                break;
            case STIVALE2_MMAP_FRAMEBUFFER: type = RegionType::FRAMEBUFFER; break;
            }
            Region region = {
                .virt = virt,
                .phy = entry.base,
                .size = entry.length,
                .type = type
            };
            this->num_regions++;
            kstd::memcpy(&this->regions[i], &region, sizeof(Region));
        }
        /*
        kstd::printf("\n[INFO] Memory regions (should be same as the memory map)\n");
        for (int i = 0; i < this->num_regions; i++) {
            auto region = &this->regions[i];
            kstd::printf("       %s phy: %#lX, size: %ld KiB, v start: %#lX, end: %#lX\n", 
                region_type_string(region->type), region->phy, region->size / 1024, region->virt, region->virt + region->size);
        }
        */
        uptr bitmap_virt_base = next_virt_base;
        this->page_bitmap = kstd::Bitmap((u8*)(first_heap_phy + 0x1000), this->heap_size / 0x1000);
        usize bitmap_pages = ((this->page_bitmap.size / 8) / 0x1000) + 1;
        next_virt_base += bitmap_pages * 0x1000;
        for (int i = 0; i < 1 + bitmap_pages; i++)
            this->page_bitmap.set(i, true);
        for (int i = 0; i < this->num_regions; i++) {
            auto region = &this->regions[i];
            if (region->type != RegionType::BAD_MEMORY) {
                // TODO: not every page should be executable
                this->map_pages(region->phy, region->virt, region->size, PAGE_WRITABLE | PAGE_PRESENT);
            }
        }
        this->page_bitmap.buffer = (u8*)bitmap_virt_base;
        this->regions = (Region*)regions_virt_base;
        this->heap_virt_base = this->hhdm + 0x100000000;
        asm volatile("mov %0, %%cr3" : : "r" ((u64)this->PML4 - tag_base_addr->virtual_base_address + tag_base_addr->physical_base_address));
    }

    Manager* Manager::get() {
        static Manager mm;
        return &mm;
    }

    static usize page_bitmap_index = 0;

    void* Manager::request_page() {
        pmm_mutex.lock();
        for (; page_bitmap_index < this->page_bitmap.size; page_bitmap_index++) {
            if (this->page_bitmap[page_bitmap_index]) continue;
            this->page_bitmap.set(page_bitmap_index, true);
            pmm_mutex.unlock();
            return (void*)(this->heap_virt_base + (page_bitmap_index * 0x1000));
        }
        pmm_mutex.unlock();
        return nullptr; // oh no
    }

    void* Manager::request_pages(usize pages_requested) {
        pmm_mutex.lock();
        u64 pages = 0;
        u64 index = page_bitmap_index;
        for (; index < this->page_bitmap.size; index++) {
            if (this->page_bitmap[index]) {
                pages = 0;
                continue;
            }
            pages++;
            if (pages == pages_requested) break;
        }
        u64 first_index = index - pages;
        for (u64 i = first_index; i < index; i++)
            this->page_bitmap.set(i, true);
        pmm_mutex.unlock();
        return (void*)(this->heap_virt_base + (first_index * 0x1000));
    }

    void Manager::free_page(void *virt) {
        pmm_mutex.lock();
        usize i = ((uptr)virt - this->heap_virt_base) / 0x1000;
        if (i < page_bitmap_index) page_bitmap_index = i;
        this->page_bitmap.set(i, false);
        pmm_mutex.unlock();
    }

    void Manager::free_pages(void *virt, usize pages) {
        pmm_mutex.lock();
        usize i = ((uptr)virt - this->heap_virt_base) / 0x1000;
        if (i < page_bitmap_index) page_bitmap_index = i;
        for (; i < i + pages; i++) 
            this->page_bitmap.set(i, false);
        pmm_mutex.unlock();
    }

    uptr Manager::heap_virt_to_phy(uptr virt) {
        for (usize i = 0; i < this->num_regions; i++) {
            auto region = this->regions[i];
            if (region.type == RegionType::USABLE) {
                if (virt >= region.virt && virt <= region.virt + region.size) {
                    return virt - region.virt + region.phy;
                }
            }
        }
        return 0;
    }

    uptr Manager::heap_phy_to_virt(uptr phy) {
        for (usize i = 0; i < this->num_regions; i++) {
            auto region = this->regions[i];
            if (region.type == RegionType::USABLE)
                if (phy >= region.phy && phy <= region.virt + region.size)
                    return phy - region.phy + region.virt;
        }
        return 0;
    }

    void Manager::map_page(uptr phy, uptr virt, u64 flags) {
        vmm_mutex.lock();
        u64 *current_table = this->PML4;
        //kstd::printf("Map page phy %#lX virt %#lX\n", phy, virt);
        for (usize i = 39; i >= 21; i -= 9) { // go through the PML4, PDP and PD
            u64 current_entry = current_table[(virt >> i) & 0x1FF];
            if (current_entry & PAGE_PRESENT) {
                current_table = (u64*)((current_entry & 0xFFFFFFFFFFFFF000) + this->hhdm); 
                //kstd::printf("Page table exists, next table at %#lX\n", (u64)current_table);
            } else {
                u64 new_table = (u64)this->request_page();
                current_entry = heap_virt_to_phy(new_table) | PAGE_WRITABLE | PAGE_PRESENT;
                current_table[(virt >> i) & 0x1FF] = current_entry;
                current_table = (u64*)(new_table - 0x100000000);
                //kstd::printf("Page table created at %#lX, current entry %#lX\n", (u64)current_table, current_entry);
            }
        }
        
        // at this point current_table is the PT
        current_table[virt >> 12 & 0x1FF] = (phy & 0xFFFFFFFFFFFFF000) | flags;
        vmm_mutex.unlock();
    }

    void Manager::map_pages(uptr phy, uptr virt, usize size, u64 flags) {
        usize num_pages = size / 0x1000;
        if (size % 0x1000) num_pages++;
        //kstd::printf("Mapping %ld pages phy %#lX virt %#lX\n", num_pages, phy, virt);
        for (usize i = 0; i < num_pages; i++)
            this->map_page(phy + (i * 0x1000), virt + (i * 0x1000), flags);
    }
}
