#pragma once

#include <kstd/types.hpp>
#include <kstd/cstring.hpp>

namespace kstd {
    struct Bitmap {
        u8 *buffer;
        usize size;
        
        Bitmap() {}
        Bitmap(u8 *buffer, usize size) : buffer(buffer), size(size) {}

        inline bool get(usize index) const {
            return (this->buffer[index / 8] >> (index % 8)) & 1;
        }

        inline void set(usize index, bool value) {
            usize d = index / 8;
            usize r = index % 8;
            this->buffer[d] &= ~(1 << r);
            this->buffer[d] |= value << r;
        }
    };
}
