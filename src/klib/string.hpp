#pragma once

#include "cstdio.hpp"
#include "types.hpp"
#include "cstdlib.hpp"
#include "cstring.hpp"

namespace klib {
    class String {
        char *m_buffer;
        usize m_length;
        
    public:
        struct iterator {
            using value_type = char;
            using pointer = char*;
            using reference = char&;

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

        String() : m_buffer(nullptr), m_length(0) {}

        String(const String &str) : m_buffer((char*)malloc(str.m_length + 1)), m_length(str.m_length) {
            memcpy(m_buffer, str.m_buffer, str.m_length + 1);
        }

        String(String &&str) : m_buffer(move(str.m_buffer)), m_length(move(str.m_length)) {}

        String(const char *cstr) {
            m_length = strlen(cstr);
            m_buffer = (char*)malloc(m_length + 1);
            memcpy(m_buffer, cstr, m_length + 1);
        }
        
        ~String() {
            free(m_buffer);
        }

        inline constexpr usize size() const noexcept { return m_length; }
        inline constexpr usize length() const noexcept { return m_length; }

        char operator [](usize index) const {
            return index < m_length ? m_buffer[index] : 0;
        }
        
        constexpr auto operator ==(klib::String &rhs) {
            if (m_length != rhs.m_length) return false;
            return memcmp(m_buffer, rhs.m_buffer, m_length) == 0;
        }

        iterator begin() noexcept { return iterator(m_buffer); }
        iterator end() noexcept { return iterator(m_buffer + m_length + 1); }

        constexpr const char& front() const { return m_buffer[0]; }
        constexpr const char& back() const { return m_buffer[m_length - 1]; }
        constexpr const char* c_str() const { return m_buffer; }
    };
}
