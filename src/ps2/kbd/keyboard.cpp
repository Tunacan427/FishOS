#include <ps2/kbd/keyboard.hpp>
#include <cpu/cpu.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <klib/cstdio.hpp>

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

    // ring buffer
    static char buffer[buffer_size];
    static usize buffer_write_index = 0;
    static usize buffer_read_index = 0;

    static bool left_shift = false, right_shift = false;
    static bool caps_lock = false;

    static inline bool is_caps() {
        return caps_lock ^ (left_shift || right_shift);
    }

    static void irq(u64 vec, cpu::InterruptState *state) {
        u8 scancode = cpu::in<u8>(0x60);

        if (scancode & 128) { // release scancode
            switch (scancode & 127) {
            case 42:
                left_shift = false;
                break;
            case 54:
                right_shift = false;
                break;
            case 58:
                caps_lock = !caps_lock;
                break;
            }
        } else {
            switch (scancode) {
            case 42:
                left_shift = true;
                break;
            case 54:
                right_shift = true;
                break;
            default:
                char c = map[scancode];
                if (c) {
                    if (is_caps() && c >= 'a' && c <= 'z')
                        c = c - 'a' + 'A';
                    
                    if ((buffer_write_index + 1) % buffer_size != buffer_read_index) {
                        buffer[buffer_write_index] = c;
                        buffer_write_index = (buffer_write_index + 1) % buffer_size;
                        klib::putchar(c);
                    }
                }
            }
        }

        cpu::interrupts::eoi();
    }

    void init() {
        cpu::interrupts::register_irq(1, irq);
        cpu::in<u8>(0x60); // drain ps2 buffer
        klib::memset(buffer, 0, buffer_size);
    }

    usize read(void *buf, usize count) {
        asm volatile("sti");
        for (usize i = 0; i < count;) {
            while (buffer_read_index == buffer_write_index)
                asm volatile("pause");
            char c = buffer[buffer_read_index];
            ((u8*)buf)[i++] = c;
            buffer_read_index = (buffer_read_index + 1) % buffer_size;
            if (c == '\n')
                return i;
        }
        return count;
    }
}
