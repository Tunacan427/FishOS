#include <cpu/idt/idt.hpp>
#include <kstd/cstdio.hpp>

namespace cpu {

static IDTEntry idt[256] [[gnu::aligned(16)]] = { 0 };
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

static void exception_handler(InterruptFrame *frame) {
    kstd::printf("\nCPU Exception %d\n", frame->vec);
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
