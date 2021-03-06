#include <kstd/types.hpp>
#include <kstd/cstring.hpp>
#include <kstd/cstdio.hpp>
#include <kstd/bitmap.hpp>
#include <kstd/cstdlib.hpp>
#include <limine.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/interrupts/idt.hpp>
#include <cpu/interrupts/pic.hpp>
#include <ps2/kbd/keyboard.hpp>
#include <gfx/framebuffer.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/allocator.hpp>
#include <panic.hpp>
#include <acpi/tables.hpp>
#include <cpu/cpu.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <sched/timer/pit.hpp>
#include <sched/timer/apic_timer.hpp>
#include <terminal.hpp>
#include <kstd/functional.hpp>

static volatile limine_hhdm_request hhdm_req = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static volatile limine_smp_request smp_req = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .flags = 0
};

static volatile limine_memmap_request memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile limine_rsdp_request rsdp_req = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static volatile limine_kernel_address_request kernel_addr_req = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

static volatile limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

[[noreturn]] void panic(const char *format, ...) {
    kstd::printf("\nKernel Panic: ");
    va_list list;
    va_start(list, format);
    kstd::vprintf(format, list);
    va_end(list);
    kstd::putchar('\n');
    abort();
}

extern "C" [[noreturn]] void _start() {
    if (!fb_req.response || fb_req.response->framebuffer_count == 0 || !memmap_req.response 
        || !kernel_addr_req.response || !hhdm_req.response || !rsdp_req.response || !smp_req.response) 
    {
        panic("Did not receive requested Limine features");
    }

    uptr hhdm = hhdm_req.response->offset;

    auto &fb = gfx::main_fb();
    auto fb_res = fb_req.response->framebuffers[0];
    fb = {
        .addr = (u8*)fb_res->address,
        .width = fb_res->width,
        .height = fb_res->height,
        .depth = fb_res->bpp,
        .pitch = fb_res->pitch,
        .pixel_width = (u32)fb_res->bpp / 8
    };
    /*
    fb.fill_rect(0, 0, fb.width, fb.height, 0xf49b02);
    fb.fill_rect(fb.width / 2 - 50, fb.height / 2 - 25, 100, 50, 0xDB880D);
    terminal::set_width(fb.width / 8 - 2);
    terminal::set_height(fb.height / 16 - 1);
    */
    terminal::init(terminal_request.response);
    kstd::printf("[ OK ] Initialized the framebuffer\n");
    
    cpu::load_gdt();
    kstd::printf("[ OK ] Loaded new GDT\n");

    cpu::interrupts::load_idt();
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

    mem::pmm::init(hhdm, memmap_req.response);
    kstd::printf("[ OK ] Initialized the PMM\n");

    mem::vmm::init(hhdm, memmap_req.response, kernel_addr_req.response);
    kstd::printf("[ OK ] Initialized the VMM\n");

    auto alloc = mem::BuddyAlloc::get();
    const u64 heap_size = 1024 * 1024 * 1024;
    alloc->init(~(u64)0 - heap_size - 0x1000 + 1, heap_size);
    kstd::printf("[ OK ] Initialized the memory allocator, base: %#lX\n", (uptr)alloc->head);

    kstd::printf("[INFO] Parsing ACPI tables and enabling APIC\n");
    acpi::parse_rsdp((uptr)rsdp_req.response->address);

    auto smp_res = smp_req.response;
    kstd::printf("[INFO] SMP | x2APIC: %s\n", (smp_res->flags & 1) ? "yes" : "no");
    for (u32 i = 0; i < smp_res->cpu_count; i++) {
        auto cpu = smp_res->cpus[i];
        auto is_bsp = cpu->lapic_id == smp_res->bsp_lapic_id ? " (BSP)" : "";
        kstd::printf("       Core %d%s | Processor ID: %d, LAPIC ID: %d\n", i, is_bsp, cpu->processor_id, cpu->lapic_id);
    }
    
    ps2::kbd::init();
    kstd::printf("[ OK ] Initialized PS/2 keyboard\n");
/*
    sched::timer::pit::init();
    kstd::printf("[ OK ] Initialized PIT\n");
*/
    sched::timer::apic_timer::init();
    kstd::printf("[ OK ] Initialized APIC timer\n");

    kstd::printf("[ .. ] Fake hanging to test keyboard\n");
    asm("sti");
    while (true) asm("hlt");

    panic("Reached end of _start");
}
