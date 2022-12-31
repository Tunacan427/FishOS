#pragma once

#include <klib/types.hpp>
#include <klib/cstring.hpp>

namespace klib {
    struct Bitmap {
        u8 *m_buffer;
        usize m_size;
        
        Bitmap() {}
        Bitmap(u8 *buffer, usize size) : m_buffer(buffer), m_size(size) {}

        inline bool get(usize index) const {
            return (m_buffer[index / 8] >> (index % 8)) & 1;
        }

        inline void set(usize index, bool value) {
            usize d = index / 8;
            usize r = index % 8;
            m_buffer[d] &= ~(1 << r);
            m_buffer[d] |= value << r;
        }

        inline void fill(bool value) {
            klib::memset(m_buffer, ~(u8)0, m_size / 8);

            for (usize i = 0; i < m_size % 8; i++) {
                set(m_size - i, true);
            }
        }
    };
}
