#pragma once

#include <kstd/types.hpp>
#include <kstd/cstdlib.hpp>
#include <kstd/cstring.hpp>

namespace kstd {
    template<typename T>
    class BasicString {
        T *buffer;
        usize len;
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

        BasicString() : buffer(nullptr), len(0) {}

        BasicString(const char *cstr) {
            len = kstd::strlen(cstr);
            buffer = kstd::malloc(len + 1);
            kstd::memcpy(buffer, cstr, len + 1);
        }
        
        ~BasicString() {
            kstd::free(buffer);
        }

        constexpr usize size() const noexcept { return len; }
        constexpr usize length() const noexcept { return len; }

        T& operator[](usize index) const {
            return index < len ? buffer[index] : nullptr;
        }
        
        iterator begin() noexcept { return iterator(buffer); }
        iterator end() noexcept { return iterator(buffer + len + 1); }

        constexpr const T& front() const { return buffer[0]; }
        constexpr const T& back() const { return buffer[len - 1]; }
    };

    using String = BasicString<char>;
    using U8String = BasicString<char8_t>;
}
