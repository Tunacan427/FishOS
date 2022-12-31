#pragma once

#include <klib/types.hpp>

namespace klib {
    template<Integral T> 
    const T min(const T a, const T b) {
        return (b < a) ? b : a;
    }

    template<Integral T> 
    const T max(const T a, const T b) {
        return (a < b) ? b : a;
    }
}
