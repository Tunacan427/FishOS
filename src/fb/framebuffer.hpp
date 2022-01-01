#pragma once

#include <types.hpp>

namespace fb {

struct Framebuffer {
    u8 *addr;
    u32 width, height, depth, pitch, pixel_width;

    void put_pixel(u16 x, u16 y, u32 color);
    void fill_rect(u16 x, u16 y, u16 w, u16 h, u32 color);
};

}
