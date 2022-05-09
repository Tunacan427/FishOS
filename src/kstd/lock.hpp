#pragma once

#include "cpu/cpu.hpp"
#include <kstd/types.hpp>

namespace kstd {
    struct Spinlock {
        volatile bool locked = false;

        inline void lock() {
            asm volatile("cli");
            while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE))
                asm volatile("pause");
        }

        inline void unlock() {
            __atomic_clear(&this->locked, __ATOMIC_RELEASE);
            asm volatile("sti");
        }
    };

    template<class> struct IsMutex : public False {};
    template<> struct IsMutex<Spinlock> : public True {};
    
    template<class T> concept Mutex = IsMutex<T>::value;

    template<Mutex M>
    class LockGuard {
        M &mutex;
    public:
        explicit LockGuard(M &m) : mutex(m) {
            //cpu::out<u8>(0x3f8, 'L');
            mutex.lock();
        }

        ~LockGuard() {
            //cpu::out<u8>(0x3f8, 'U');
            mutex.unlock();
        }

        LockGuard(const LockGuard&) = delete;
        LockGuard& operator =(const LockGuard&) = delete;
    };
}
