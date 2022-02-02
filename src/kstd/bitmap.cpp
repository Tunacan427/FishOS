#include <kstd/bitmap.hpp>

namespace kstd {
    void Bitmap::set(usize index, bool value) {
        usize d = index / 8;
        usize r = index % 8;
        this->buffer[d] &= ~(1 << r);
        this->buffer[d] |= value << r;
    }
}
