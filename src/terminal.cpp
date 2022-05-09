#include <terminal.hpp>
#include <cpu/cpu.hpp>
#include <gfx/framebuffer.hpp>
#include <kstd/lock.hpp>
#include <panic.hpp>

namespace terminal {
    /*
    static char buffer_internal[8192];
    static kstd::RingBuffer<char> term_buf;

    static usize terminal_x = 0;
    static usize terminal_y = 0;
    static usize terminal_width = 0;
    static usize terminal_height = 0;

    void init() {
        term_buf.buffer = buffer_internal;
        term_buf.size = sizeof(buffer_internal);
    }

    void set_width(usize width) {
        terminal_width = width;
    }

    void set_height(usize height) {
        terminal_height = height;
    }

    void write_char(char c) {
        cpu::out<u8>(0x3F8, c); // output to qemu serial console
        if (c == '\n') {
            terminal_x = 0;
            terminal_y++;
            return;
        }
        if (c == '\b') {
            terminal_x--;
            return;
        }
        if (terminal_x >= terminal_width) {
            terminal_x = 0;
            terminal_y++;
        }
        if (terminal_y >= terminal_height) {
            terminal_x = 0;
            terminal_y = 0;
        }
        term_buf.put(c);
        gfx::main_fb().draw_psf_char(c, terminal_x, terminal_y, 8, 8, 0, 0xf49b02);
        terminal_x++;
    }
    */

    static kstd::Spinlock terminal_lock;
    static limine_terminal_write terminal_write;
    static limine_terminal *main_terminal;

    void init(limine_terminal_response *terminal_res) {
        if (terminal_res->terminal_count == 0)
            panic("No Limine terminal provided");

        terminal_write = terminal_res->write;
        main_terminal = terminal_res->terminals[0];
    }

    void write_char(char c) {
        cpu::out<u8>(0x3F8, c); // output to qemu serial console
        terminal_write(main_terminal, &c, 1);
    }
}
