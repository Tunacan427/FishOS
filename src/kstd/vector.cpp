#include <kstd/vector.hpp>

namespace kstd {
    template<typename T>
    Vector<T>::Vector(usize reserve) : size(0), reserved(reserve) {
        buffer = reserve ? kstd::malloc(reserve * sizeof(T)) : nullptr;
    }

    template<typename T>
    void Vector<T>::push_back(T elem) {
        
    }
}
