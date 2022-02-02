#include <types.hpp>
#include <kstd/cstring.hpp>
#include <kstd/cstdio.hpp>
#include <kstd/bitmap.hpp>
#include <kstd/cstdlib.hpp>
#include <stivale2.h>
#include <ioports.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/idt/idt.hpp>
#include <cpu/pic/pic.hpp>
#include <ps2/kbd/keyboard.hpp>
#include <gfx/framebuffer.hpp>
#include <mem/manager.hpp>
#include <mem/allocator.hpp>

static u8 stack[8192];

static stivale2_header_tag_framebuffer framebuffer_header_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = (u64)0
    },
    // set all these to 0 for the bootloader to pick the best resolution
    .framebuffer_width  = 800,
    .framebuffer_height = 600,
    .framebuffer_bpp    = 32
};

static stivale2_header_tag_smp smp_header_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = (u64)&framebuffer_header_tag
    },
    .flags = 1 // use x2APIC if available
};

static stivale2_tag unmap_null_header_tag = {
    .identifier = STIVALE2_HEADER_TAG_UNMAP_NULL_ID,
    .next = (u64)&smp_header_tag
};

[[gnu::section(".stivale2hdr"), gnu::used]]
static stivale2_header stivale_hdr = {
    .entry_point = 0,
    .stack = (u64)stack + sizeof(stack),
    .flags = 0b11110,
    .tags = (u64)&unmap_null_header_tag
};

void *stivale2_get_tag(stivale2_struct *stivale2_struct, u64 id) {
    auto current_tag = (stivale2_tag*)stivale2_struct->tags;
    for (;;) {
        if (current_tag == NULL)
            return NULL;

        if (current_tag->identifier == id)
            return current_tag;
 
        current_tag = (stivale2_tag*)current_tag->next;
    }
}

extern "C" void _start(stivale2_struct *stivale2_struct) {
    auto tag_fb = (stivale2_struct_tag_framebuffer*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    auto tag_mmap = (stivale2_struct_tag_memmap*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    auto tag_pmrs = (stivale2_struct_tag_pmrs*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
    auto tag_base_addr = (stivale2_struct_tag_kernel_base_address*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);
    auto tag_hhdm = (stivale2_struct_tag_hhdm*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_HHDM_ID);
    auto tag_rsdp = (stivale2_struct_tag_rsdp*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);
    auto tag_smp = (stivale2_struct_tag_smp*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);

    if (!tag_fb || !tag_mmap || !tag_pmrs || !tag_base_addr || !tag_hhdm || !tag_rsdp || !tag_smp) {
        kstd::printf("[ .. ] Couldn't find the requested stivale2 tags, hanging\n");
        for (;;) asm("hlt");
    }
    
    kstd::printf("[ .. ] Loading new GDT");
    cpu::load_gdt();
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Loading IDT");
    cpu::load_idt();
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Initializing the PIC");
    cpu::pic::remap(0x20, 0x28);
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Setting up PS/2 keyboard");
    ps2::kbd::setup();
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[INFO] SMP | x2APIC: %s\n", (tag_smp->flags & 1) ? "yes" : "no");
    for (int i = 0; i < tag_smp->cpu_count; i++) {
        auto info = tag_smp->smp_info[i];
        auto is_bsp = info.lapic_id == tag_smp->bsp_lapic_id ? " (BSP)" : "";
        kstd::printf("       Core %d%s | Processor ID: %d, LAPIC ID: %d\n", i, is_bsp, info.processor_id, info.lapic_id);
    }

    kstd::printf("[INFO] Printing memory information\n");
    kstd::printf("[INFO] Physical memory map:\n");
    for (int i = 0; i < tag_mmap->entries; i++) {
        auto entry = tag_mmap->memmap[i];
        const char *entry_name;
        switch (entry.type) {
        case STIVALE2_MMAP_USABLE: entry_name = "Usable"; break;
        case STIVALE2_MMAP_RESERVED: entry_name = "Reserved"; break;
        case STIVALE2_MMAP_ACPI_RECLAIMABLE: entry_name = "ACPI Reclaimable"; break;
        case STIVALE2_MMAP_ACPI_NVS: entry_name = "ACPI NVS"; break;
        case STIVALE2_MMAP_BAD_MEMORY: entry_name = "Bad Memory"; break;
        case STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE: entry_name = "Bootloader Reclaimable"; break;
        case STIVALE2_MMAP_KERNEL_AND_MODULES: entry_name = "Kernel and Modules"; break;
        case STIVALE2_MMAP_FRAMEBUFFER: entry_name = "Framebuffer"; break;
        default: entry_name = "Unknown";
        }
        kstd::printf("       %s | base: %#lX, size: %ld KiB\n", entry_name, entry.base, entry.length / 1024);
    }
    kstd::printf("[INFO] Kernel base addresses | phy: %#lX, virt: %#lX\n", tag_base_addr->physical_base_address, tag_base_addr->virtual_base_address);
    kstd::printf("[INFO] Kernel PMRs:\n");
    for (usize i = 0; i < tag_pmrs->entries; i++) {
        auto pmr = tag_pmrs->pmrs[i];
        kstd::printf("       base %#lX, size: %ld KiB, permissions: %ld\n", pmr.base, pmr.length / 1024, pmr.permissions);
    }

    kstd::printf("[ .. ] Initializing the memory manager");
    auto mm = mem::Manager::get();
    mm->boot(tag_mmap, tag_pmrs, tag_base_addr, tag_hhdm);
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Initializing the memory allocator");
    auto alloc = mem::BuddyAlloc::get();
    usize kernel_heap_size = 2 * 1024 * 1024;
    alloc->boot((uptr)mm->request_pages(kernel_heap_size / 0x1000), kernel_heap_size);
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Setting up the framebuffer");
    gfx::Framebuffer fb = {
        .addr = (u8*)tag_fb->framebuffer_addr,
        .width = tag_fb->framebuffer_width,
        .height = tag_fb->framebuffer_height,
        .depth = tag_fb->framebuffer_bpp,
        .pitch = tag_fb->framebuffer_pitch,
        .pixel_width = (u32)tag_fb->framebuffer_bpp / 8
    };
    fb.fill_rect(10, 10, 100, 100, 0xFFFFFFFF);
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Fake hanging to test keyboard\n");
    for (;;) io::wait();

    kstd::printf("[ .. ] Reached end of _start, hanging\n");
    for (;;) asm("hlt");
}
 
