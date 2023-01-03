#pragma once

#include <klib/types.hpp>

namespace klib {
    struct Spinlock {
        volatile bool locked = false;

        inline void lock() {
            // asm volatile("cli");
            while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE))
                asm volatile("pause");
        }

        inline void unlock() {
            __atomic_clear(&this->locked, __ATOMIC_RELEASE);
            // asm volatile("sti");
        }
    };

    template<class T>
    concept BasicLockable = requires(T l) {
        l.lock();
        l.unlock();
    };

    template<BasicLockable M>
    class LockGuard {
        M &mutex;

    public:
        explicit LockGuard(M &m) : mutex(m) {
            mutex.lock();
        }

        ~LockGuard() {
            mutex.unlock();
        }

        LockGuard(const LockGuard&) = delete;
        LockGuard& operator =(const LockGuard&) = delete;
    };
}
