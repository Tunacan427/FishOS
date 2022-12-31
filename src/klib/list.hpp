#pragma once

#include <klib/types.hpp>

namespace klib {
    template<typename T>
    class LinkedList {
        struct Entry {
            Entry *next;
            T data;
        };

    public:
        Entry *head, *tail;

        void insert(T data) {
            if (!head) {
                head = new Entry { nullptr, data };
                tail = head;
                return;
            }

            auto e = new Entry { nullptr, data };
            tail->next = e;
            tail = e;
        }
    };
}
