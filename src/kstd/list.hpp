#pragma once

namespace kstd {
    template<typename T>
    struct LinkedList {
        class Element {
            Element *next, *prev;
            T data;
        };

        Element *first, *last;
    };
}
