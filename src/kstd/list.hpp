#pragma once

namespace kstd {
    template<typename T>
    class LinkedList {
    public:
        class Element {
            Element *next, *prev;
            T data;
        };

        Element *first, *last;
    };
}
