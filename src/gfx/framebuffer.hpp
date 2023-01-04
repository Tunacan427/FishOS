#pragma once

#include <klib/types.hpp>

namespace gfx {
    struct Framebuffer {
        u8 *m_addr;
        u32 m_width, m_height, m_depth, m_pitch, m_pixel_width;

        void put_pixel(u16 x, u16 y, u32 color);
        void fill_rect(u16 x, u16 y, u16 w, u16 h, u32 color);
        void draw_psf_char(u32 c, u16 cx, u16 cy, u16 offx, u16 offy, u32 fg, u32 bg);
    };

    Framebuffer& main_fb();
}
