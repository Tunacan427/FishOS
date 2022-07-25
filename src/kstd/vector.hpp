#pragma once

#include <kstd/types.hpp>
#include <kstd/cstdlib.hpp>

namespace kstd {
    template<typename T>
    class Vector {
        T *buffer;
        usize size;
        usize reserved;
        
    public:
        struct iterator {
            using value_type = T;
            using pointer = T*;
            using reference = T&;

            explicit iterator(pointer ptr) : ptr(ptr) {}
            reference operator *() const { return *ptr; }
            pointer operator ->() const { return ptr; }

            iterator& operator ++() { ++ptr; return *this; }  
            iterator operator ++(int) { iterator tmp = *this; ++ptr; return tmp; }
            iterator& operator --() { --ptr; return *this; }  
            iterator operator --(int) { iterator tmp = *this; --ptr; return tmp; }

            friend bool operator ==(const iterator &a, const iterator &b) { return a.ptr == b.ptr; };
            friend bool operator !=(const iterator &a, const iterator &b) { return a.ptr != b.ptr; };
            friend size_t operator -(const iterator &a, const iterator &b) { return a.ptr - b.ptr; };
            friend bool operator <(const iterator &a, const iterator &b) { return a.ptr < b.ptr; };

        private:
            pointer ptr;
        };

        Vector(usize reserve = 0) : size(0), reserved(reserve) {
            buffer = (T*)(reserve ? kstd::malloc(reserve * sizeof(T)) : nullptr);
        }
        
        ~Vector() {
            kstd::free(buffer);
        }

        T* operator[](usize index) const {
            return index < size ? buffer[index] : nullptr;
        }

        void push_back(T elem) {
            if (size == reserved) {
                reserved++;
                buffer = (T*)kstd::realloc(buffer, reserved * sizeof(T));
            }
            buffer[size] = elem;
            size++;
        }
        
        iterator begin() noexcept { return iterator(buffer); }
        iterator end() noexcept { return iterator(buffer + size); }
    };
}
