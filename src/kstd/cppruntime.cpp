#include <kstd/cstdlib.hpp>
#include <kstd/cstdio.hpp>
#include <kstd/mutex.hpp>

static volatile kstd::Mutex static_init_mutex;

using __guard = u64;

static inline void set_guard_in_use(__guard *g) {
    *g |= 2;
}

static inline bool guard_in_use(__guard *g) {
    return *g & 2;
}

static inline void set_guard_initialized(__guard *g) {
    *g |= 1;
}

static inline bool guard_initialized(__guard *g) {
    return *g & 1;
}

namespace __cxxabiv1  {
    extern "C" i32 __cxa_guard_acquire(__guard *g) {
        if (*g)
            return 0;
        static_init_mutex.lock();
        if (guard_initialized(g)) {
            static_init_mutex.unlock();
            return 0;
        }

        if (guard_in_use(g)) {
            kstd::printf("\n__cxa_guard_acquire: guard in use\n");
            return 0;
        }

        set_guard_in_use(g);
        return 1;
    }

    extern "C" void __cxa_guard_release(__guard *g) {
        set_guard_initialized(g);
        static_init_mutex.unlock();
    }

    extern "C" void __cxa_guard_abort(__guard *g) {
        kstd::printf("\n__cxa_guard_abort called\n");
    }
}

extern "C" void __cxa_pure_virtual() {
    kstd::printf("\n__cxa_pure_virtual called\n");
}
 
void* operator new(usize size) {
    return kstd::malloc(size);
}
 
void* operator new[](usize size) {
    return kstd::malloc(size);
}
 
void operator delete(void *ptr) {
    kstd::free(ptr);
}
 
void operator delete[](void *ptr) {
    kstd::free(ptr);
}

void operator delete(void *ptr, usize size) {
    ::operator delete(ptr);
}

void operator delete[](void *ptr, usize size) {
    ::operator delete[](ptr);
}
