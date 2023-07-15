#pragma once

#include <klib/types.hpp>

namespace gfx {
    class Terminal {
        char *buffer; // represents the currently displayed characters

        // pixel area of the terminal
        usize width, height;
        usize x, y;

        // size of terminal in chars
        usize width_chars, height_chars;

        // terminal padding in pixels
        usize padding_top, padding_bottom;
        usize padding_left, padding_right;

        // pixel area of the terminal, accounting for padding
        usize actual_width, actual_height;
        usize actual_x, actual_y;

        // color of the terminal
        u32 fg, bg, border;

        // size of the used font
        usize font_width, font_height;

        // where to write chars at
        usize cursor_x, cursor_y;
        usize old_cursor_x, old_cursor_y;

        bool cursor_needs_undrawing;

        // c_x and c_y are the position in chars
        void set_char_at(usize c_x, usize c_y, char c);
        void set_char_at_cursor(char c);
        char get_char_at(usize c_x, usize c_y);

        // scrolls down by 1 line
        void scroll();

        void redraw_cursor();

    public:
        Terminal();
        
        void write_char(char c);
    };

    Terminal& kernel_terminal();
    bool is_kernel_terminal_ready();
    void set_kernel_terminal_ready();
}
