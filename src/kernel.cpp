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
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/allocator.hpp>
#include <panic.hpp>
#include <acpi/tables.hpp>
#include <cpu/cpu.hpp>

static u8 stack[8192];

static stivale2_header_tag_framebuffer framebuffer_header_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = (u64)0
    },
    // set all these to 0 for the bootloader to pick the best resolution
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0,
    .unused = 0
};

static stivale2_header_tag_smp smp_header_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = (u64)&framebuffer_header_tag
    },
    .flags = 0 // set to 1 to use x2APIC if available
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
        if (!current_tag) return nullptr;
        if (current_tag->identifier == id) return current_tag;
        current_tag = (stivale2_tag*)current_tag->next;
    }
}

[[noreturn]] void panic(const char *format, ...) {
    kstd::printf("\nKernel Panic: ");
    va_list list;
    va_start(list, format);
    kstd::vprintf(format, list);
    va_end(list);
    kstd::putchar('\n');
    abort();
}

extern "C" [[noreturn]] void _start(stivale2_struct *stivale2_struct) {
    auto tag_fb = (stivale2_struct_tag_framebuffer*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    auto tag_mmap = (stivale2_struct_tag_memmap*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    auto tag_pmrs = (stivale2_struct_tag_pmrs*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
    auto tag_base_addr = (stivale2_struct_tag_kernel_base_address*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);
    auto tag_hhdm = (stivale2_struct_tag_hhdm*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_HHDM_ID);
    auto tag_rsdp = (stivale2_struct_tag_rsdp*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);
    auto tag_smp = (stivale2_struct_tag_smp*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);

    if (!tag_fb || !tag_mmap || !tag_pmrs || !tag_base_addr || !tag_hhdm || !tag_rsdp || !tag_smp) {
        panic("Couldn't find the requested stivale2 tags");
    }
    
    cpu::load_gdt();
    kstd::printf("[ OK ] Loaded new GDT\n");

    cpu::load_idt();
    kstd::printf("[ OK ] Loaded IDT\n");

    {
        u32 eax, ebx, ecx, edx;
        cpu::cpuid(0, 0, &eax, &ebx, &ecx, &edx);
        char vendor[12];
        *(u32*)&vendor[0] = ebx;
        *(u32*)&vendor[4] = edx;
        *(u32*)&vendor[8] = ecx;
        kstd::printf("[INFO] CPUID Vendor: %.*s\n", 12, vendor);
    }

    mem::pmm::init(tag_hhdm->addr, tag_mmap);
    kstd::printf("[ OK ] Initialized the PMM\n");

    mem::vmm::init(tag_hhdm->addr, tag_mmap, tag_pmrs, tag_base_addr);
    kstd::printf("[ OK ] Initialized the VMM\n");

    auto alloc = mem::BuddyAlloc::get();
    const u64 heap_size = 1024 * 1024 * 1024;
    alloc->init(~(u64)0 - heap_size - 0x1000 + 1, heap_size);
    kstd::printf("[ OK ] Initialized the memory allocator, base: %#lX\n", (uptr)alloc->head);

    {
        kstd::printf("[INFO] Testing malloc and free\n");
        auto string1 = (char*)kstd::malloc(24);
        kstd::strcpy(string1, "Hello world!");
        kstd::printf("[INFO] string1: %s\n", string1);
        kstd::printf("[INFO] string1 address: %#lX\n", (uptr)string1);
        kstd::free(string1);
        kstd::printf("[INFO] string1 freed\n");
        auto string2 = (char*)kstd::malloc(24);
        kstd::strcpy(string2, "Hello world!");
        kstd::printf("[INFO] string2: %s\n", string2);
        kstd::printf("[INFO] string2 address: %#lX\n", (uptr)string1);
        kstd::free(string2);
        kstd::printf("[INFO] string2 freed\n");
        kstd::printf("[INFO]\n");
    }

    {
        kstd::printf("[INFO] Testing new and delete\n");
        auto string1 = new char[24];
        kstd::strcpy(string1, "Hello world!");
        kstd::printf("[INFO] string1: %s\n", string1);
        kstd::printf("[INFO] string1 address: %#lX\n", (uptr)string1);
        delete[] string1;
        kstd::printf("[INFO] string1 deleted\n");
        auto string2 = new char[24];
        kstd::strcpy(string2, "Hello world!");
        kstd::printf("[INFO] string2: %s\n", string2);
        kstd::printf("[INFO] string2 address: %#lX\n", (uptr)string1);
        delete[] string2;
        kstd::printf("[INFO] string2 deleted\n");
        kstd::printf("[INFO]\n");
    }

    {
        kstd::printf("[INFO] Allocating 1024 128-byte objects and an array to keep track of them\n");
        u8 **array = new u8*[1024];
        for (int i = 0; i < 1024; i++)
            array[i] = new u8[128];
        for (int i = 0; i < 1024; i++)
            delete array[i];
        kstd::printf("[INFO]\n");
    }

    {
        kstd::printf("[INFO] Testing realloc\n");
        auto string = (char*)kstd::malloc(24);
        kstd::strcpy(string, "Hello world!");
        kstd::printf("[INFO] string: %s\n", string);
        kstd::printf("[INFO] string address: %#lX\n", (uptr)string);
        string = (char*)kstd::realloc(string, 25);
        kstd::printf("[INFO] string reallocated from 24 to 25 bytes\n");
        kstd::printf("[INFO] string: %s\n", string);
        kstd::printf("[INFO] string address: %#lX\n", (uptr)string);
        kstd::strcpy(string, "Hello, world!");
        kstd::printf("[INFO] new string: %s\n", string);
        kstd::free(string);
        kstd::printf("[INFO] string freed\n");
        kstd::printf("[INFO]\n");
    }

    {
        kstd::printf("[INFO] Testing new and delete again\n");
        auto string1 = new char[24];
        kstd::strcpy(string1, "Hello world!");
        kstd::printf("[INFO] string1: %s\n", string1);
        kstd::printf("[INFO] string1 address: %#lX\n", (uptr)string1);
        delete[] string1;
        kstd::printf("[INFO] string1 deleted\n");
        auto string2 = new char[24];
        kstd::strcpy(string2, "Hello world!");
        kstd::printf("[INFO] string2: %s\n", string2);
        kstd::printf("[INFO] string2 address: %#lX\n", (uptr)string1);
        delete[] string2;
        kstd::printf("[INFO] string2 deleted\n");
        kstd::printf("[INFO]\n");
    }

/*
    cpu::pic::remap(0x20, 0x28);
    kstd::printf("[ OK ] Initialized the PIC\n");

    ps2::kbd::setup();
    kstd::printf("[ OK ] Initialized PS/2 keyboard\n");
*/
    kstd::printf("[INFO] SMP | x2APIC: %s\n", (tag_smp->flags & 1) ? "yes" : "no");
    for (u32 i = 0; i < tag_smp->cpu_count; i++) {
        auto info = tag_smp->smp_info[i];
        auto is_bsp = info.lapic_id == tag_smp->bsp_lapic_id ? " (BSP)" : "";
        kstd::printf("       Core %d%s | Processor ID: %d, LAPIC ID: %d\n", i, is_bsp, info.processor_id, info.lapic_id);
    }

    gfx::Framebuffer fb = {
        .addr = (u8*)tag_fb->framebuffer_addr,
        .width = tag_fb->framebuffer_width,
        .height = tag_fb->framebuffer_height,
        .depth = tag_fb->framebuffer_bpp,
        .pitch = tag_fb->framebuffer_pitch,
        .pixel_width = (u32)tag_fb->framebuffer_bpp / 8
    };
    fb.fill_rect(0, 0, fb.width, fb.height, 0x0D7BDBFF);
    fb.fill_rect(fb.width / 2 - 50, fb.height / 2 - 25, 100, 50, 0xDB880DFF);
    kstd::printf("[ OK ] Initialized the framebuffer\n");

    kstd::printf("[INFO] Parsing ACPI tables\n");
    acpi::parse_rsdp(tag_rsdp->rsdp);

    kstd::printf("[ .. ] Fake hanging to test keyboard\n");
    asm("sti");
    while (true) asm("hlt");

    panic("Reached end of _start");
}
