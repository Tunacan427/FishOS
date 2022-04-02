#pragma once

#include <types.hpp>
#include <kstd/cstdlib.hpp>

namespace kstd {
    template<typename T>
    class Vector {
        T *buffer;
        usize size;
        usize reserved;
    public:
        Vector(usize reserve = 0);

        void push_back(T elem);
    };
}
