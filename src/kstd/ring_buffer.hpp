#pragma once

#include <kstd/types.hpp>

namespace kstd {
    template<typename T>
    struct RingBuffer {
        T *buffer;
        usize size, start = 0, end = 0;
        
        RingBuffer() {}
        RingBuffer(T *buffer, usize size) : buffer(buffer), size(size) {}

        inline void put(T elem) {
            buffer[end++] = elem;
            end %= size;
        }

        inline T get() {
            T item = buffer[start++];
            start %= size;
            return item;
        }
    };
}
