#pragma once

#include <types.hpp>

namespace kstd {

template<typename T>
class Vector {
    T *buffer;
    u64 size;
public:
    Vector(u64 reserve = 1) : size(reserve) {
        
    }

    void push_back(T elem) {

    }
};

}
