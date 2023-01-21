#include "klib/cstdio.hpp"
#include <gfx/framebuffer.hpp>

#define PSF_FONT_MAGIC 0x864ab572

namespace gfx {
    Framebuffer& screen_fb() {
        static Framebuffer screen_fb;
        return screen_fb;
    }

    void Framebuffer::put_pixel(u16 x, u16 y, u32 rgb) {
        if (x > width || y > height) return;
        u64 where = x * pixel_width + y * pitch;
        addr[where] = rgb & 255; // blue
        addr[where + 1] = (rgb >> 8) & 255; // green
        addr[where + 2] = (rgb >> 16) & 255; // red
    }

    void Framebuffer::fill_rect(u16 x, u16 y, u16 w, u16 h, u32 rgb) {
        u8 *where;
        u32 pw = pixel_width;
        for (u16 i = y; i < h + y; i++) {
            if (i >= height) continue;
            where = (u8*)(u64(addr) + pitch * i);
            for (u16 j = x; j < w + x; j++) {
                if (j >= width) continue;
                where[j * pw] = rgb & 255; // blue
                where[j * pw + 1] = (rgb >> 8) & 255; // green
                where[j * pw + 2] = (rgb >> 16) & 255; // red
            }
        }
    }

    struct [[gnu::packed]] PSF {
        u32 magic;
        u32 version;
        u32 header_size;
        u32 flags; // 0 if there's no unicode table
        u32 num_glyph;
        u32 bytes_per_glyph;
        u32 height;
        u32 width;
    };
    
    // these are linked using objcopy
    extern "C" u8 _binary_font_psfu_start[];
    extern "C" u8 _binary_font_psfu_end[];

    // c is a unicode character, cx and cy are cursor position in characters, offx and offy are offsets in pixels
    void Framebuffer::draw_psf_char(u32 c, u16 cx, u16 cy, u16 offx, u16 offy, u32 fg, u32 bg) {
        PSF *font = (PSF*)_binary_font_psfu_start;

        // we need to know how many bytes encode one row
        u32 bytes_per_line = (font->width + 7) / 8;

        // get the glyph for the character. if there's no
        // glyph for a given character, we'll display the first glyph.
        u8 *glyph = _binary_font_psfu_start + font->header_size + (c > 0 && c < font->num_glyph ? c : 0) * font->bytes_per_glyph;

        // calculate the upper left corner on screen where we want to display.
        // we only do this once, and adjust the whereet later. This is faster.
        u64 where = ((cy * font->height + offy) * pitch) + ((cx * font->width + offx) * 4);

        // finally display pixels according to the bitmap
        usize line, mask;
        for (usize y = 0; y < font->height; y++) {
            // save the starting position of the line
            line = where;
            mask = 1 << (font->width - 1);

            // display a row
            for (usize x = 0; x < font->width; x++) {
                *((u32*)(addr + line)) = *glyph & mask ? fg : bg;

                // adjust to the next pixel
                mask >>= 1;
                line += pixel_width;
            }

            // adjust to the next line
            glyph += bytes_per_line;
            where += pitch;
        }
    }
}
