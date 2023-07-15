#pragma once

#include <klib/types.hpp>

namespace mem {
    // based on https://www.gingerbill.org/article/2021/12/02/memory-allocation-strategies-006/
    class BuddyAlloc {
        BuddyAlloc() {}
        
    public:
        static constexpr usize alignment = 16;

        static constexpr usize align(usize i) {
            return i % alignment ? ((i / alignment) + 1) * alignment : i;
        }

        static constexpr usize size_required(usize size) {
            usize actual_size = alignment;
            size = align(size); 
            while (size > actual_size)
                actual_size *= 2;
            return actual_size;
        }

        struct Block {
            usize size; // size of the actual data stored including header, use size_required() to find the actual block size
            bool is_free;

            inline void* data() const {
                return (void*)((uptr)this + sizeof(*this));
            }

            inline Block* next() const {
                return (Block*)((uptr)this + size_required(this->size));
            }

            Block* split_until(usize size);
        };

        void init(uptr base, usize size);
        static BuddyAlloc* get();

        Block *head;
        Block *tail;

        Block* find_best(usize size);
        void coalescence();

        void* malloc(usize size);
        void* realloc(void *ptr, usize size);
        void free(void *ptr);
    };
}
