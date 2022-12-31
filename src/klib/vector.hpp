#pragma once

#include "types.hpp"
#include "cstdlib.hpp"

namespace klib {
    template<typename T>
    class Vector {
        T *m_buffer;
        usize m_size;
        usize m_capacity;

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

        Vector(usize reserve = 0) : m_size(0), m_capacity(reserve) {
            m_buffer = (T*)(reserve ? klib::malloc(reserve * sizeof(T)) : nullptr);
        }
        
        ~Vector() {
            klib::free(m_buffer);
        }

        T* operator [](usize index) const {
            return index < m_size ? m_buffer[index] : nullptr;
        }

        void push_back(T elem) {
            if (m_size == m_capacity) {
                m_capacity = m_capacity ? m_capacity * 2 : 1;
                m_buffer = (T*)klib::realloc(m_buffer, m_capacity * sizeof(T));
            }
            m_buffer[m_size] = elem;
            m_size++;
        }

        inline constexpr usize size() const noexcept { return m_size; }
        inline constexpr usize capacity() const noexcept { return m_capacity; }
        
        iterator begin() noexcept { return iterator(m_buffer); }
        iterator end() noexcept { return iterator(m_buffer + m_size); }
    };
}
