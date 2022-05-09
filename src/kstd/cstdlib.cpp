#include <kstd/cstdlib.hpp>
#include <kstd/cstdio.hpp>

namespace kstd {
    void* malloc(usize size) {
        auto ptr = mem::BuddyAlloc::get()->malloc(size);
        printf("malloc(%ld): %#lX\n", size, (uptr)ptr);
        return ptr;
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
        printf("free(): %#lX\n", (uptr)ptr);
        mem::BuddyAlloc::get()->free(ptr);
    }
    
    // required for weird shit
    extern "C" int atexit(void (*func)()) noexcept {
        return 0;
    }
}
