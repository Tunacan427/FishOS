#include <terminal.hpp>
#include <cpu/cpu.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <gfx/framebuffer.hpp>
#include <klib/cstring.hpp>

namespace terminal {
    static bool initialized = false;
    static char *buffer; // represents the currently displayed characters

    // pixel area of the terminal
    static usize terminal_width, terminal_height;
    static usize terminal_x, terminal_y;

    // size of terminal in chars
    static usize terminal_width_chars, terminal_height_chars;

    // color of the terminal
    static u32 terminal_fg, terminal_bg;

    // size of the used font
    static usize font_width, font_height;

    // where to write chars at
    static usize cursor_x, cursor_y;

    static bool cursor_needs_undrawing = true;

    // x and y are the position in chars
    static inline void set_char_at(usize x, usize y, char c) {
        char *p = &buffer[y * terminal_width_chars + x];

        if (*p == c) {
            return;
        }

        *p = c;
        gfx::screen_fb().draw_psf_char(c, x, y, terminal_x, terminal_y, terminal_fg, terminal_bg);
        cursor_needs_undrawing = false;
    }

    static inline void set_char_at_cursor(char c) {
        set_char_at(cursor_x, cursor_y, c);
    }

    static inline char get_char_at(usize x, usize y) {
        return buffer[y * terminal_width_chars + x];
    }

    // scrolls down by 1 line
    static void scroll() {
        for (usize y = 0; y < terminal_height_chars - 1; y++)
            for (usize x = 0; x < terminal_width_chars; x++)
                set_char_at(x, y, get_char_at(x, y + 1));

        for (usize x = 0; x < terminal_width_chars; x++)
            set_char_at(x, terminal_height_chars - 1, ' ');

        cursor_needs_undrawing = true;
    }

    static void redraw_cursor() {
        static usize old_x = cursor_x, old_y = cursor_y;
        
        if (cursor_needs_undrawing)
            gfx::screen_fb().fill_rect(old_x * font_width + 1, old_y * font_height, 1, font_height, terminal_bg);
        
        gfx::screen_fb().fill_rect(cursor_x * font_width + 1, cursor_y * font_height, 1, font_height, terminal_fg);

        old_x = cursor_x;
        old_y = cursor_y;
    }

    void init() {
        auto fb = gfx::screen_fb();
        terminal_x = 0;
        terminal_y = 0;
        terminal_width = fb.width / 2; // use first half of the screen (other half is reserved for funny graphics tests) 
        terminal_height = fb.height;

        terminal_fg = 0xFCFCFC;
        terminal_bg = 0x232627;

        font_width = 8;
        font_height = 16;

        terminal_width_chars = terminal_width / font_width;
        terminal_height_chars = terminal_height / font_height;

        usize buffer_size = terminal_width_chars * terminal_height_chars;
        buffer = (char*)(mem::vmm::get_hhdm() + mem::pmm::alloc_pages((buffer_size + 0x1000 - 1) / 0x1000));
        klib::memset(buffer, ' ', buffer_size);

        cursor_x = 0;
        cursor_y = 0;

        fb.fill_rect(terminal_x, terminal_y, terminal_width, terminal_height, terminal_bg);

        initialized = true;

        redraw_cursor();
    }

    void write_char(char c) {
        cpu::out<u8>(0x3F8, c); // output to qemu serial console
        if (!initialized) return;

        switch (c) {
            case '\b':
                if (cursor_x > 0) {
                    cursor_x--;
                    set_char_at_cursor(' ');
                    cursor_needs_undrawing = true;
                    redraw_cursor();
                } else cursor_needs_undrawing = false;
                return;
            case '\r':
                cursor_x = 0;
                break;
            case '\n':
                cursor_x = 0;
                cursor_y++;
                cursor_needs_undrawing = true;
                break;
            default:
                set_char_at_cursor(c);
                cursor_x++;
        }

        if (cursor_x >= terminal_width_chars) {
            cursor_x = 0;
            cursor_y++;
        }
        
        if (cursor_y >= terminal_height_chars) {
            scroll();
            cursor_y--;
        }

        redraw_cursor();
        cursor_needs_undrawing = true;
    }
}
