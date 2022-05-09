#include <cpu/interrupts/pic.hpp>
#include <cpu/cpu.hpp>

namespace cpu::pic {
    void remap(u8 offset1, u8 offset2) {
        u8 m1 = in<u8>(PIC1_DATA); // save masks
        u8 m2 = in<u8>(PIC2_DATA);
    
        out<u8>(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
        io_wait();
        out<u8>(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();

        out<u8>(PIC1_DATA, offset1); // ICW2: master PIC vector offset
        io_wait();
        out<u8>(PIC2_DATA, offset2); // ICW2: slave PIC vector offset
        io_wait();

        out<u8>(PIC1_DATA, 4); // ICW3: tell master PIC that there is a slave PIC at IRQ2 (0000 0100)
        io_wait();
        out<u8>(PIC2_DATA, 2); // ICW3: tell slave PIC its cascade identity (0000 0010)
        io_wait();

        out<u8>(PIC1_DATA, ICW4_8086);
        io_wait();
        out<u8>(PIC2_DATA, ICW4_8086);
        io_wait();

        out<u8>(PIC1_DATA, m1); // restore saved masks
        out<u8>(PIC2_DATA, m2);
    }

    void send_eoi(u8 irq) {
        if (irq >= 8) out<u8>(PIC2_COMMAND, PIC_EOI);
        out<u8>(PIC1_COMMAND, PIC_EOI);
    }

    void set_mask(u8 irq) {
        u16 port = irq < 8 ? PIC1_DATA : PIC2_DATA;
        out<u8>(port, in<u8>(port) | (1 << (irq & 7)));
    }
    
    void clear_mask(u8 irq) {
        u16 port = irq < 8 ? PIC1_DATA : PIC2_DATA;
        out<u8>(port, in<u8>(port) & ~(1 << (irq & 7)));
    }
}
