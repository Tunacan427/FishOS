#include <cpu/pic/pic.hpp>
#include <ioports.hpp>

using namespace io;

namespace cpu::pic {
    void remap(u8 offset1, u8 offset2) {
        u8 m1 = inb(PIC1_DATA); // save masks
        u8 m2 = inb(PIC2_DATA);
    
        outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
        wait();
        outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        wait();

        outb(PIC1_DATA, offset1); // ICW2: master PIC vector offset
        wait();
        outb(PIC2_DATA, offset2); // ICW2: slave PIC vector offset
        wait();

        outb(PIC1_DATA, 4); // ICW3: tell master PIC that there is a slave PIC at IRQ2 (0000 0100)
        wait();
        outb(PIC2_DATA, 2); // ICW3: tell slave PIC its cascade identity (0000 0010)
        wait();

        outb(PIC1_DATA, ICW4_8086);
        wait();
        outb(PIC2_DATA, ICW4_8086);
        wait();

        outb(PIC1_DATA, m1); // restore saved masks
        outb(PIC2_DATA, m2);
    }

    void send_eoi(u8 irq) {
        if (irq >= 8) outb(PIC2_COMMAND, PIC_EOI);
        outb(PIC1_COMMAND, PIC_EOI);
    }

    void set_mask(u8 irq) {
        u16 port = irq < 8 ? PIC1_DATA : PIC2_DATA;
        outb(port, inb(port) | (1 << (irq & 7)));
    }
    
    void clear_mask(u8 irq) {
        u16 port = irq < 8 ? PIC1_DATA : PIC2_DATA;
        outb(port, inb(port) & ~(1 << (irq & 7)));
    }
}
