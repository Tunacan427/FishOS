#include <cpu/cpu.hpp>
#include <klib/cstdio.hpp>
#include <sched/sched.hpp>
#include <fs/vfs.hpp>

namespace cpu::syscall {
    extern "C" { void *__syscall_table[11]; }
    
    void init_syscall_table() {
        __syscall_table[0]  = (void*)&sched::syscall_exit;
        __syscall_table[1]  = (void*)&fs::vfs::syscall_open;
        __syscall_table[2]  = (void*)&fs::vfs::syscall_openat;
        __syscall_table[3]  = (void*)&fs::vfs::syscall_close;
        __syscall_table[4]  = (void*)&fs::vfs::syscall_read;
        __syscall_table[5]  = (void*)&fs::vfs::syscall_pread;
        __syscall_table[6]  = (void*)&fs::vfs::syscall_write;
        __syscall_table[7]  = (void*)&fs::vfs::syscall_pwrite;
        __syscall_table[8]  = (void*)&fs::vfs::syscall_seek;
        __syscall_table[9]  = (void*)&fs::vfs::syscall_getcwd;
        __syscall_table[10] = (void*)&fs::vfs::syscall_chdir;
    }
}
