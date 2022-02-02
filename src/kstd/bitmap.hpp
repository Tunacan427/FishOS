#pragma once

#include <types.hpp>

namespace kstd {
    struct Bitmap {
        u8 *buffer;
        usize size;
        
        Bitmap() {}
        Bitmap(u8 *buffer, usize size) : buffer(buffer), size(size) {}

        inline bool operator[](usize index) const {
            return (this->buffer[index / 8] >> (index % 8)) & 1;
        }

        void set(usize index, bool value);
    };
}
