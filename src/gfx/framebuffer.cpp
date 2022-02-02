#include <gfx/framebuffer.hpp>

namespace gfx {
    void Framebuffer::put_pixel(u16 x, u16 y, u32 color) {
        if ((color & 255) == 0) return; // ignore if alpha is 0
        u64 where = x * this->pixel_width + y * this->pitch;
        this->addr[where] = (color >> 8) & 255; // blue
        this->addr[where + 1] = (color >> 16) & 255; // green
        this->addr[where + 2] = (color >> 24) & 255; // red
    }

    void Framebuffer::fill_rect(u16 x, u16 y, u16 w, u16 h, u32 color) {
        if ((color & 255) == 0) return; // ignore if alpha is 0
        u8 *where;
        u32 pw = this->pixel_width;
        for (u16 i = y; i < h + y; i++) {
            where = (u8*)((u64)this->addr + this->pitch * i);
            for (u16 j = x; j < w + x; j++) {
                where[j * pw] = (color >> 8) & 255; // blue
                where[j * pw + 1] = (color >> 16) & 255; // green
                where[j * pw + 2] = (color >> 24) & 255; // red
            }
        }
    }
}
