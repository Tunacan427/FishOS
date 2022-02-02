#include <kstd/cstdlib.hpp>

namespace kstd {
    void* malloc(usize size) {
        return mem::BuddyAlloc::get()->malloc(size);
    }

    void* calloc(usize size) {
        auto ptr = mem::BuddyAlloc::get()->malloc(size);
        memset(ptr, 0, size);
        return ptr;
    }

    void* realloc(void *ptr, usize size) {
        return mem::BuddyAlloc::get()->realloc(ptr, size);
    }

    void free(void *ptr) {
        mem::BuddyAlloc::get()->free(ptr);
    }
}
