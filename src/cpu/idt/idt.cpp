#include <cpu/idt/idt.hpp>
#include <kstd/cstdio.hpp>

namespace cpu {

[[gnu::aligned(16)]] static IDTEntry idt[256] = { 0 };
static IDTR idtr;
static IDTHandler idt_handlers[256];

extern "C" void (*__idt_wrappers[256])();

void load_idt_entry(u16 index, void (*wrapper)(), IDTType type) {
    auto entry = &idt[index];
    entry->offset1 = (u64)wrapper & 0xFFFF;
    entry->offset2 = ((u64)wrapper & 0xFFFF0000) >> 16;
    entry->offset3 = ((u64)wrapper & 0xFFFFFFFF00000000) >> 32;
    entry->selector = 0x8; // kernel code segment
    entry->attributes = (1 << 15) | (int(type) << 8); // set present bit and type
    entry->reserved = 0;
}

void load_idt_handler(u16 index, IDTHandler handler) {
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
    kstd::printf("RAX=%016lX RBX=%016lX RCX=%016lX RDX=%016lX\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
    kstd::printf("RSI=%016lX RDI=%016lX RBP=%016lX RSP=%016lX\n", frame->rsi, frame->rdi, frame->rbp, frame->rsp);
    kstd::printf(" R8=%016lX  R9=%016lX R10=%016lX R11=%016lX\n", frame->r8, frame->r9, frame->r10, frame->r11);
    kstd::printf("R12=%016lX R13=%016lX R14=%016lX R15=%016lX\n", frame->r12, frame->r13, frame->r14, frame->r15);
    kstd::printf("RIP=%016lX RFLAGS=%016lX\n", frame->rip, frame->rflags);
    kstd::printf("[ .. ] Cannot recover, hanging\n");
    for (;;) asm("hlt");
}

extern "C" void __idt_handler_common(InterruptFrame *frame) {
    idt_handlers[frame->vec](frame);
}

void load_idt() {
    for (int i = 0; i < 256; i++)
        load_idt_entry(i, __idt_wrappers[i], IDTType::INTERRUPT);
    
    for (int i = 0; i < 32; i++)
        load_idt_handler(i, exception_handler);

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (u64)&idt;
    asm volatile("cli;"
                 "lidt %0;"
                 "sti;" : : "m" (idtr));
}

}
