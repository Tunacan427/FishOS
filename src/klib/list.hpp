#pragma once

#include <klib/types.hpp>

// ptr: pointer to a ListHead, type: type of struct that the ListHead is in, member: the name of the ListHead in the struct
#define LIST_ENTRY(ptr, type, member) (type*)((uptr)ptr - offsetof(type, member))

namespace klib {
    struct ListHead {
        ListHead *next, *prev;

        // init a list
        inline void init() {
            next = this;
            prev = this;
        }

        // add entry after this
        inline void add(ListHead *entry) {
            next->prev = entry;
            entry->next = next;
            entry->prev = this;
            this->next = entry;
        }

        // add entry before this
        inline void add_before(ListHead *entry) {
            prev->next = entry;
            entry->prev = prev;
            entry->next = this;
            this->prev = entry;
        }

        // remove this entry from the list
        inline void remove() {
            next->prev = prev;
            prev->next = next;
            this->next = nullptr;
            this->prev = nullptr;
        }

        // is this list empty
        inline bool empty() {
            return next == this;
        }
    };
}
