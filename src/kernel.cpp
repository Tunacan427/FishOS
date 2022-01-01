#include <types.hpp>
#include <kstd/cstring.hpp>
#include <kstd/cstdio.hpp>
#include <stivale2.h>
#include <ioports.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/idt/idt.hpp>
#include <cpu/pic/pic.hpp>
#include <ps2/keyboard/keyboard.hpp>
#include <fb/framebuffer.hpp>

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

[[gnu::section(".stivale2hdr"), gnu::used]]
static stivale2_header stivale_hdr = {
    .entry_point = 0,
    .stack = (u64)stack + sizeof(stack),
    .flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
    .tags = (u64)&framebuffer_header_tag
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
    auto tag_rsdp = (stivale2_struct_tag_rsdp*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);

    if (tag_fb == 0 || tag_mmap == 0 || tag_pmrs == 0 || tag_base_addr == 0 || tag_rsdp == 0) {
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
    ps2::keyboard::setup();
    kstd::printf("\r[ OK ]\n");

    kstd::printf("[ .. ] Setting up the framebuffer");
    fb::Framebuffer fb = {
        .addr = (u8*)tag_fb->framebuffer_addr,
        .width = tag_fb->framebuffer_width,
        .height = tag_fb->framebuffer_height,
        .depth = tag_fb->framebuffer_bpp,
        .pitch = tag_fb->framebuffer_pitch,
        .pixel_width = (u32)tag_fb->framebuffer_bpp / 8
    };
    kstd::printf("\r[ OK ]\n");

    fb.fill_rect(10, 10, 100, 100, 0xFFFFFFFF);

    kstd::printf("[ .. ] Fake hanging to test keyboard \n");
    for (;;) io::wait();

    kstd::printf("[ .. ] Reached end of _start, hanging\n");
    for (;;) asm("hlt");
}
