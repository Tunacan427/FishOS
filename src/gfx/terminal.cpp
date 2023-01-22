#include <gfx/terminal.hpp>
#include <gfx/framebuffer.hpp>
#include <cpu/cpu.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <klib/cstring.hpp>

namespace gfx {
    static bool terminal_ready = false;

    Terminal& kernel_terminal() {
        static Terminal term;
        return term;
    }

    bool is_kernel_terminal_ready() {
        return terminal_ready;
    }

    void set_kernel_terminal_ready() {
        terminal_ready = true;
    }
    
    void Terminal::set_char_at(usize c_x, usize c_y, char c) {
        char *p = &buffer[c_y * width_chars + c_x];

        if (*p == c) {
            return;
        }

        *p = c;
        gfx::screen_fb().draw_psf_char(c, c_x, c_y, actual_x, actual_y, fg, bg);
        cursor_needs_undrawing = false;
    }

    void Terminal::set_char_at_cursor(char c) {
        set_char_at(cursor_x, cursor_y, c);
    }

    char Terminal::get_char_at(usize c_x, usize c_y) {
        return buffer[c_y * width_chars + c_x];
    }

    void Terminal::scroll() {
        for (usize y = 0; y < height_chars - 1; y++)
            for (usize x = 0; x < width_chars; x++)
                set_char_at(x, y, get_char_at(x, y + 1));

        for (usize x = 0; x < width_chars; x++)
            set_char_at(x, height_chars - 1, ' ');

        cursor_needs_undrawing = true;
    }

    void Terminal::redraw_cursor() {
        if (cursor_needs_undrawing) {
            gfx::screen_fb().fill_rect(
                old_cursor_x * font_width + 1 + actual_x, 
                old_cursor_y * font_height + actual_y, 
                1, font_height, bg
            );
        }
        
        gfx::screen_fb().fill_rect(
            cursor_x * font_width + 1 + actual_x, 
            cursor_y * font_height + actual_y, 
            1, font_height, fg
        );

        old_cursor_x = cursor_x;
        old_cursor_y = cursor_y;
    }

    Terminal::Terminal() {
        auto fb = gfx::screen_fb();
        x = 0;
        y = 0;
        width = fb.width / 2; // use first half of the screen (other half is reserved for funny graphics tests) 
        height = fb.height;

        padding_top = 4;
        padding_bottom = 4;
        padding_left = 4;
        padding_right = 4;

        actual_width = width - padding_left - padding_right;
        actual_height = height - padding_top - padding_bottom;
        actual_x = x + padding_left;
        actual_y = y + padding_top;

        fg = 0xFCFCFC;
        bg = 0x232627;
        border = 0x232627;

        font_width = 8;
        font_height = 16;

        width_chars = actual_width / font_width;
        height_chars = actual_height / font_height;

        usize buffer_size = width_chars * height_chars;
        buffer = (char*)(mem::vmm::get_hhdm() + mem::pmm::alloc_pages((buffer_size + 0x1000 - 1) / 0x1000));
        klib::memset(buffer, ' ', buffer_size);

        cursor_x = 0;
        cursor_y = 0;

        fb.fill_rect(actual_x, actual_y, actual_width, actual_height, bg);

        // draw the border
        fb.fill_rect(x, y, width, padding_top, border);
        fb.fill_rect(x, height - padding_bottom, width, padding_bottom, border);
        fb.fill_rect(x, padding_top, padding_left, actual_height, border);
        fb.fill_rect(width - padding_right, padding_top, padding_right, actual_height, border);

        redraw_cursor();
    }

    void Terminal::write_char(char c) {
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

        if (cursor_x >= width_chars) {
            cursor_x = 0;
            cursor_y++;
        }
        
        if (cursor_y >= height_chars) {
            scroll();
            cursor_y--;
        }

        redraw_cursor();
        cursor_needs_undrawing = true;
    }
}
