#include <cpu/interrupts/idt.hpp>
#include <cpu/cpu.hpp>
#include <mem/vmm.hpp>
#include <kstd/cstdio.hpp>
#include <kstd/bitmap.hpp>
#include <panic.hpp>

namespace cpu::interrupts {
    [[gnu::aligned(16)]] static IDTEntry idt[256];
    static IDTR idtr;
    static IDTHandler idt_handlers[256];
    static u8 idt_raw_bitmap[256 / 8];
    static kstd::Bitmap idt_bitmap;

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
        entry->selector = 40; // kernel code segment
        entry->attributes = (1 << 15) | (int(type) << 8); // set present bit and type
        entry->reserved = 0;
    }

    void load_idt_handler(u8 index, IDTHandler handler) {
        idt_handlers[index] = handler;
    }

    const char *exception_strings[] = {
        "Division By Zero",
        "Debug",
        "Non Maskable Interrupt",
        "Breakpoint",
        "Into Detected Overflow",
        "Out of Bounds",
        "Invalid Opcode",
        "No Coprocessor",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Bad TSS",
        "Segment Not Present",
        "Stack Fault",
        "General Protection Fault",
        "Page Fault",
        "Unknown Interrupt",
        "Coprocessor Fault",
        "Alignment Check",
        "Machine Check"
    };

    static void exception_handler(InterruptFrame *frame) {
        const char *err_name = frame->vec < 19 ? exception_strings[frame->vec] : "Reserved";
        kstd::printf("\nCPU Exception: %s (%#lX)\n", err_name, frame->vec);
        if (frame->err) kstd::printf("Error code: %#04lX\n", frame->err);
        if (frame->vec == 0xE) kstd::printf("CR2=%016lX\n",  cpu::read_cr2());
        kstd::printf("RAX=%016lX RBX=%016lX RCX=%016lX RDX=%016lX\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
        kstd::printf("RSI=%016lX RDI=%016lX RBP=%016lX RSP=%016lX\n", frame->rsi, frame->rdi, frame->rbp, frame->rsp);
        kstd::printf(" R8=%016lX  R9=%016lX R10=%016lX R11=%016lX\n", frame->r8, frame->r9, frame->r10, frame->r11);
        kstd::printf("R12=%016lX R13=%016lX R14=%016lX R15=%016lX\n", frame->r12, frame->r13, frame->r14, frame->r15);
        kstd::printf("RIP=%016lX RFLAGS=%016lX\n", frame->rip, frame->rflags);
        panic("Cannot recover from CPU exception that happened in the kernel");
    }

    static void page_fault_handler(InterruptFrame *frame) {
        u64 cr2 = cpu::read_cr2();
        if (mem::vmm::try_demand_page(cr2))
            exception_handler(frame);
        // else
            // kstd::printf("Demand paged %#lX\n", cr2);
    }

    extern "C" void __idt_handler_common(InterruptFrame *frame) {
        idt_handlers[frame->vec](frame);
    }

    void load_idt() {
        idt_bitmap.buffer = idt_raw_bitmap;
        idt_bitmap.size = 256;

        for (int i = 0; i < 256; i++)
            load_idt_entry(i, __idt_wrappers[i], IDTType::INTERRUPT);
        
        for (int i = 0; i < 32; i++) {
            load_idt_handler(i, i == 0xE ? page_fault_handler : exception_handler);
            idt_bitmap.set(i, true);
        }

        idtr.limit = sizeof(idt) - 1;
        idtr.base = (u64)&idt;
        asm volatile("cli;"
                     "lidt %0;"
                     "sti;" : : "m" (idtr));
    }
}
