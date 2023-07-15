#include <cpu/cpu.hpp>
#include <klib/cstdio.hpp>
#include <sched/sched.hpp>

namespace cpu::syscall {
    extern "C" { void *__syscall_table[2]; }

    u64 syscall_debug(char *string, u64 length) {
        klib::printf("debug(string: %#lX, length: %ld)\n", (uptr)string, length);
        for (u64 i = 0; i < length; i++)
            klib::putchar(string[i]);
        return 0;
    }

    void init_syscall_table() {
        __syscall_table[0] = (void*)&syscall_debug;
        __syscall_table[1] = (void*)&sched::syscall_exit;
    }
}
