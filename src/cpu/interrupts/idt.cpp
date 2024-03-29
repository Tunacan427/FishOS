#include <cpu/interrupts/idt.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/cpu.hpp>
#include <mem/vmm.hpp>
#include <klib/cstdio.hpp>
#include <klib/bitmap.hpp>
#include <sched/sched.hpp>
#include <panic.hpp>

namespace cpu::interrupts {
    [[gnu::aligned(16)]] static IDTEntry idt[256];
    static IDTR idtr;
    static IDTHandler idt_handlers[256];
    static u8 idt_raw_bitmap[256 / 8];
    static klib::Bitmap idt_bitmap;

    extern "C" void (*__idt_wrappers[256])();

    u8 allocate_vector() {
        for (u8 i = 32; i; i++) {
            if (idt_bitmap.get(i) == false) {
                idt_bitmap.set(i, true);
                return i;
            }
        }
        panic("Failed to allocate interrupt");
    }

    void load_idt_entry(u8 index, void (*wrapper)(), IDTType type) {
        auto entry = &idt[index];
        entry->offset1 = (u64)wrapper & 0xFFFF;
        entry->offset2 = ((u64)wrapper & 0xFFFF0000) >> 16;
        entry->offset3 = ((u64)wrapper & 0xFFFFFFFF00000000) >> 32;
        entry->selector = u16(GDTSegment::KERNEL_CODE_64);
        entry->attributes = (1 << 15) | (int(type) << 8); // set present bit and type
        entry->reserved = 0;
    }

    void load_idt_handler(u8 index, IDTHandler handler) {
        idt_handlers[index] = handler;
    }

    const char *exception_strings[] = {
        "Division by 0",
        "Debug",
        "NMI",
        "Breakpoint",
        "Overflow",
        "Bound range exceeded",
        "Invalid opcode",
        "Device not available",
        "Double fault",
        "???",
        "Invalid TSS",
        "Segment not present",
        "Stack-segment fault",
        "General protection fault",
        "Page fault",
        "???",
        "x87 exception",
        "Alignment check",
        "Machine check",
        "SIMD exception",
        "Virtualisation",
        "???",
        "???",
        "???",
        "???",
        "???",
        "???",
        "???",
        "???",
        "???",
        "Security"
    };

    static void exception_handler(u64 vec, InterruptState *state) {
        const char *err_name = vec < 19 ? exception_strings[vec] : "Reserved";
        if ((state->cs & 3) == 3) {
            auto *task = (sched::Task*)cpu::read_gs_base();
            klib::printf("\nUser task (tid: %d) crashed (%s)\n", task->tid, err_name);
            sched::dequeue_and_die();
            return;
        }
        klib::printf("\nCPU Exception: %s (%#lX)\n", err_name, vec);
        if (state->err) klib::printf("Error code: %#04lX\n", state->err);
        if (vec == 0xE) klib::printf("CR2=%016lX\n",  cpu::read_cr2());
        klib::printf("RAX=%016lX RBX=%016lX RCX=%016lX RDX=%016lX\n", state->rax, state->rbx, state->rcx, state->rdx);
        klib::printf("RSI=%016lX RDI=%016lX RBP=%016lX RSP=%016lX\n", state->rsi, state->rdi, state->rbp, state->rsp);
        klib::printf(" R8=%016lX  R9=%016lX R10=%016lX R11=%016lX\n", state->r8,  state->r9,  state->r10, state->r11);
        klib::printf("R12=%016lX R13=%016lX R14=%016lX R15=%016lX\n", state->r12, state->r13, state->r14, state->r15);
        klib::printf("RIP=%016lX RFLAGS=%016lX\n", state->rip, state->rflags);
        panic("Cannot recover from CPU exception that happened in the kernel");
    }

    static void page_fault_handler(u64 vec, InterruptState *state) {
        u64 cr2 = cpu::read_cr2();
        mem::vmm::Pagemap *pagemap;
        if ((state->cs & 3) == 3)
            pagemap = ((sched::Task*)cpu::read_gs_base())->pagemap;
        else
            pagemap = mem::vmm::get_kernel_pagemap();
        if (pagemap->handle_page_fault(cr2))
            exception_handler(vec, state);
        // else
        //     klib::printf("Demand paged %#lX\n", cr2);
    }

    extern "C" void __idt_handler_common(u64 vec, InterruptState *state) {
        idt_handlers[vec](vec, state);
    }

    void load_idt() {
        idt_bitmap.m_buffer = idt_raw_bitmap;
        idt_bitmap.m_size = 256;

        for (int i = 0; i < 256; i++)
            load_idt_entry(i, __idt_wrappers[i], IDTType::INTERRUPT);
        
        for (int i = 0; i < 32; i++) {
            load_idt_handler(i, i == 0xE ? page_fault_handler : exception_handler);
            idt_bitmap.set(i, true);
        }

        idtr.limit = sizeof(idt) - 1;
        idtr.base = (u64)&idt;
        asm volatile("cli");
        asm volatile("lidt %0" : : "m" (idtr));
        asm volatile("sti");
    }
}
