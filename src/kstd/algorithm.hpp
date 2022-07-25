#pragma once

#include <kstd/types.hpp>

namespace kstd {
    template<Integral T> 
    const T min(const T a, const T b) {
        return (b < a) ? b : a;
    }

    template<Integral T> 
    const T max(const T a, const T b) {
        return (a < b) ? b : a;
    }
}
