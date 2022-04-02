#pragma once

namespace kstd {
    struct Spinlock {
        volatile bool locked = false;

        inline void lock() volatile {
            while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE))
                asm("pause");
        }

        inline void unlock() volatile {
            __atomic_clear(&this->locked, __ATOMIC_RELEASE);
        }
    };
}
