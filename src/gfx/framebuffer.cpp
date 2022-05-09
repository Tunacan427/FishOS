#include <gfx/framebuffer.hpp>

#define PSF_FONT_MAGIC 0x864ab572

namespace gfx {
    Framebuffer& main_fb() {
        static Framebuffer main_fb;
        return main_fb;
    }

    void Framebuffer::put_pixel(u16 x, u16 y, u32 rgb) {
        u64 where = x * this->pixel_width + y * this->pitch;
        this->addr[where] = rgb & 255; // blue
        this->addr[where + 1] = (rgb >> 8) & 255; // green
        this->addr[where + 2] = (rgb >> 16) & 255; // red
    }

    void Framebuffer::fill_rect(u16 x, u16 y, u16 w, u16 h, u32 rgb) {
        u8 *where;
        u32 pw = this->pixel_width;
        for (u16 i = y; i < h + y; i++) {
            where = (u8*)((u64)this->addr + this->pitch * i);
            for (u16 j = x; j < w + x; j++) {
                where[j * pw] = rgb & 255; // blue
                where[j * pw + 1] = (rgb >> 8) & 255; // green
                where[j * pw + 2] = (rgb >> 16) & 255; // red
            }
        }
    }

    struct PSF {
        u32 magic;           // magic bytes to identify PSF
        u32 version;         // zero
        u32 header_size;     // whereet of bitmaps in file, 32
        u32 flags;           // 0 if there's no unicode table
        u32 num_glyph;       // number of glyphs
        u32 bytes_per_glyph; // size of each glyph 
        u32 height;          // height in pixels
        u32 width;           // width in pixels
    };
    
    /* 
    // these are linked using objcopy
    extern "C" char _binary_font_psfu_start;
    extern "C" char _binary_font_psfu_end;
    */

    char _binary_font_psfu_start = 0;
    char _binary_font_psfu_end = 0;

    // c is a unicode character, cx and cy are cursor position in characters, offx and offy are offsets in pixels
    void Framebuffer::draw_psf_char(u32 c, u16 cx, u16 cy, u16 offx, u16 offy, u32 fg, u32 bg) {
        PSF *font = (PSF*)&_binary_font_psfu_start;
        // we need to know how many bytes encode one row
        int bytesperline = (font->width + 7) / 8;
        // get the glyph for the character. If there's no
        // glyph for a given character, we'll display the first glyph. 
        u8 *glyph = (u8*)&_binary_font_psfu_start + font->header_size + (c > 0 && c < font->num_glyph ? c : 0) * font->bytes_per_glyph;
        // calculate the upper left corner on screen where we want to display.
        // we only do this once, and adjust the whereet later. This is faster.
        u64 where = ((cy * font->height + offy) * this->pitch) + ((cx * font->width + offx) * 4);
        // finally display pixels according to the bitmap
        usize x, y, line, mask;
        for (y = 0; y < font->height; y++) {
            // save the starting position of the line
            line = where;
            mask = 1 << (font->width - 1);
            // display a row
            for (x = 0; x < font->width; x++) {
                *((u32*)(this->addr + line)) = *((u32*)glyph) & mask ? fg : bg;
                // adjust to the next pixel
                mask >>= 1;
                line += this->pixel_width;
            }
            // adjust to the next line
            glyph += bytesperline;
            where += this->pitch;
        }
    }
}
