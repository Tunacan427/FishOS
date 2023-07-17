#include <cpu/cpu.hpp>
#include <klib/cstdio.hpp>
#include <sched/sched.hpp>
#include <userland/fd.hpp>

namespace cpu::syscall {
    extern "C" { void *__syscall_table[5]; }
    
    void init_syscall_table() {
        __syscall_table[0] = (void*)&sched::syscall_exit;
        __syscall_table[1] = (void*)&userland::syscall_read;
        __syscall_table[2] = (void*)&userland::syscall_pread;
        __syscall_table[3] = (void*)&userland::syscall_write;
        __syscall_table[4] = (void*)&userland::syscall_pwrite;
    }
}
