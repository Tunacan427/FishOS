#include <kstd/cstdlib.hpp>
#include <kstd/lock.hpp>
#include <panic.hpp>

static volatile kstd::Spinlock static_init_lock;

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
        static_init_lock.lock();
        if (guard_initialized(g)) {
            static_init_lock.unlock();
            return 0;
        }

        if (guard_in_use(g)) {
            panic("__cxa_guard_acquire: guard in use");
            return 0;
        }

        set_guard_in_use(g);
        return 1;
    }

    extern "C" void __cxa_guard_release(__guard *g) {
        set_guard_initialized(g);
        static_init_lock.unlock();
    }

    extern "C" void __cxa_guard_abort(__guard *g) {
        panic("__cxa_guard_abort called");
    }
}

extern "C" void __cxa_pure_virtual() {
    panic("__cxa_pure_virtual called");
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
