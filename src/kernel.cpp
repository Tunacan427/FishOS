#include <klib/types.hpp>
#include <klib/cstring.hpp>
#include <klib/cstdio.hpp>
#include <klib/bitmap.hpp>
#include <klib/cstdlib.hpp>
#include <limine.hpp>
#include <cpu/cpu.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/interrupts/idt.hpp>
#include <cpu/interrupts/pic.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <cpu/syscall/syscall.hpp>
#include <ps2/kbd/keyboard.hpp>
#include <gfx/framebuffer.hpp>
#include <gfx/terminal.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <mem/allocator.hpp>
#include <panic.hpp>
#include <acpi/tables.hpp>
#include <sched/timer/pit.hpp>
#include <sched/timer/hpet.hpp>
#include <sched/timer/apic_timer.hpp>
#include <sched/sched.hpp>
#include <userland/elf.hpp>
#include <fs/vfs.hpp>
#include <fs/initramfs.hpp>

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

static volatile limine_module_request module_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

struct StackFrame {
    StackFrame *next;
    uptr ip;
};

[[noreturn]] void panic(const char *format, ...) {
    klib::panic_printf("\nKernel Panic: ");
    va_list list;
    va_start(list, format);
    klib::panic_vprintf(format, list);
    va_end(list);
    StackFrame *frame = (StackFrame*)__builtin_frame_address(0);
    klib::panic_printf("\nStacktrace: ");
    while (true) {
        if (frame == nullptr || frame->ip == 0)
            break;
        
        klib::panic_printf("%#lX\n            ", frame->ip);
        frame = frame->next;
    }
    abort();
}

[[noreturn]] void kernel_thread();

extern "C" [[noreturn]] void _start() {
    if (!fb_req.response || fb_req.response->framebuffer_count == 0 || !hhdm_req.response)
        panic("Did not receive enough limine features to initialise early boot terminal");

    uptr hhdm = hhdm_req.response->offset;

    auto &fb = gfx::screen_fb();
    auto fb_res = fb_req.response->framebuffers[0];
    fb = {
        .addr = (u8*)fb_res->address,
        .width = (u32)fb_res->width,
        .height = (u32)fb_res->height,
        .pitch = (u32)fb_res->pitch,
        .pixel_width = (u32)fb_res->bpp / 8
    };

    if (!memmap_req.response) panic("Did not receive Limine memmap feature response");
    if (!module_req.response) panic("Did not receive Limine module feature response");
    if (!kernel_addr_req.response) panic("Did not receive Limine kernel address feature response");
    if (!rsdp_req.response) panic("Did not receive Limine RSDP feature response");
    if (!smp_req.response) panic("Did not receive Limine SMP feature response");

    mem::pmm::init(hhdm, memmap_req.response);
    klib::printf("PMM: Initialized\n");
    
    cpu::early_init();

    mem::vmm::init(hhdm, memmap_req.response, kernel_addr_req.response);
    klib::printf("VMM: Initialized\n");
    
    gfx::kernel_terminal();
    gfx::set_kernel_terminal_ready();
    klib::printf("Terminal: Initialized\n");

    auto alloc = mem::BuddyAlloc::get();
    const u64 heap_size = 1024 * 1024 * 1024;
    alloc->init(~(u64)0 - heap_size - 0x1000 + 1, heap_size);
    klib::printf("Allocator: Initialized, base: %#lX\n", (uptr)alloc->head);
    
    cpu::smp_init(smp_req.response);

    klib::printf("ACPI: Parsing ACPI tables and enabling APIC\n");
    acpi::parse_rsdp((uptr)rsdp_req.response->address);

    cpu::syscall::init_syscall_table();
    
    fs::vfs::init();
    klib::printf("VFS: Initialized\n");

    sched::timer::apic_timer::init();
    klib::printf("APIC Timer: Initialized\n");

    sched::init();
    klib::printf("Scheduler: Initialized\n");

    sched::new_kernel_task(uptr(kernel_thread), true);
    sched::start();

    asm("sti");
    while (true) asm("hlt");

    panic("Reached end of _start");
}

[[noreturn]] void kernel_thread() {
    ps2::kbd::init();
    klib::printf("PS/2 Keyboard: Initialized\n");

    auto module_res = module_req.response;
    if (module_res->module_count == 0) panic("No initramfs Limine module loaded");
    if (module_res->module_count > 1) panic("Too many Limine modules loaded");

    auto initramfs_module = module_res->modules[0];
    klib::printf("Loading initramfs file %s (size: %ld KiB)\n", initramfs_module->path, initramfs_module->size / 1024);
    fs::initramfs::load_into(fs::vfs::root_dir(), initramfs_module->address, initramfs_module->size);

    auto result = fs::vfs::path_to_node("/bin/test");
    if (result.target == nullptr)
        panic("Failed to find /bin/test");
    delete[] result.basename;
    
    klib::printf("Loading executable /bin/test\n");
    sched::Task *test_task = sched::new_user_task((fs::vfs::FileNode*)result.target, true);
    while (true) {
        if (test_task->sched_list.next == nullptr) {
            klib::printf("Test task died, rebooting in 3 seconds\n");
            sched::timer::hpet::stall_ms(3000);
            cpu::write_cr3(0);
        }
        asm("hlt");
    }
    // sched::dequeue_and_die();
}
