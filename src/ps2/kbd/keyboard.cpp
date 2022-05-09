#include <ps2/kbd/keyboard.hpp>
#include <cpu/cpu.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <kstd/cstdio.hpp>

namespace ps2::kbd {
    const char map[128] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '-', '=', '\b',
        '\t',
        'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, // 29 ctrl
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, // 42 left shift
        '\\', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, // 54 right shift
        '*',
        0, // 56 alt
        ' ',
        0, // 58 caps lock
        0, // 59 F1 key
        0, 0, 0, 0, 0, 0, 0, 0,
        0, // 68 F10 key
        0, // 69 num lock
        0, // 70 scroll lock
        0, // 71 home key
        0, // 72 up arrow
        0, // 73 page up
        '-',
        0, // 75 left arrow
        0,
        0, // 77 right arrow
        '+',
        0, // 79 end key
        0, // 80 down arrow
        0, // 81 page down
        0, // 82 insert key
        0, // 83 delete key
        0, 0, 0,
        0, // 87 F11 key
        0, // 88 F12 key
        0
    };

    static void irq(cpu::interrupts::InterruptFrame *frame) {
        u8 scancode = cpu::in<u8>(0x60);

        if (scancode & 128) // release scancode, ignored
            goto end;
        
        kstd::putchar(map[scancode]);
    end:
        cpu::interrupts::eoi();
    }

    void init() {
        cpu::interrupts::register_irq(1, irq);
        cpu::in<u8>(0x60); // drain buffer
    }
}
