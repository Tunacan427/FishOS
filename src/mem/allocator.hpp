#pragma once

#include <types.hpp>

namespace mem {
    struct BuddyBlock {
        usize size; // includes the header
        bool is_free;

        inline void* data() const {
            return (void*)((uptr)this + sizeof(*this));
        }

        inline BuddyBlock* next() const {
            return (BuddyBlock*)((uptr)this + this->size);
        }

        BuddyBlock* split_until(usize size);
    };

    // based on https://www.gingerbill.org/article/2021/12/02/memory-allocation-strategies-006/
    class BuddyAlloc {
        BuddyAlloc() {}
    public:
        void boot(uptr base, usize size);
        static BuddyAlloc* get();

        BuddyBlock *head;
        BuddyBlock *tail;

        static constexpr usize alignment = 16;

        BuddyBlock* find_best(usize size);
        void coalescence();

        void* malloc(usize size);
        void* realloc(void *ptr, usize size);
        void free(void *ptr);
    };
}
