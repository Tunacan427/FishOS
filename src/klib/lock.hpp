#pragma once

#include <klib/types.hpp>
#include <panic.hpp>

namespace klib {
    struct Spinlock {
        // int i = 0;
        volatile bool locked = false;

        inline void lock() {
            asm volatile("cli");
            while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE)) {
                // i++;
                // if (i == 10000000)
                //     panic("Spinlock spun too much");
                asm volatile("pause");
            }
        }

        inline void unlock() {
            // i = 0;
            __atomic_clear(&this->locked, __ATOMIC_RELEASE);
            asm volatile("sti");
        }
    };

    template<class T>
    concept BasicLockable = requires(T l) {
        l.lock();
        l.unlock();
    };

    template<BasicLockable L>
    class LockGuard {
        L &lock;

    public:
        explicit LockGuard(L &l) : lock(l) {
            lock.lock();
        }

        ~LockGuard() {
            lock.unlock();
        }

        LockGuard(const LockGuard&) = delete;
        LockGuard& operator =(const LockGuard&) = delete;
    };
}
