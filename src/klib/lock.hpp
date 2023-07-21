#pragma once

#include <klib/types.hpp>
#include <panic.hpp>

#define DETECT_DEADLOCK 1

namespace klib {
    struct Spinlock {
        volatile bool locked = false;
#if DETECT_DEADLOCK
        u32 i = 0;
#endif

        inline void lock() {
            asm volatile("cli");
            while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE)) {
#if DETECT_DEADLOCK
                i++;
                if (i == 10000000)
                    panic("Spinlock spun too much");
#endif
                asm volatile("pause");
            }
        }

        inline void unlock() {
#if DETECT_DEADLOCK
            i = 0;
#endif
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
