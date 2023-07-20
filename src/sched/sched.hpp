#pragma once

#include <cpu/cpu.hpp>
#include <mem/vmm.hpp>
#include <klib/types.hpp>
#include <klib/vector.hpp>
#include <klib/list.hpp>
#include <fs/vfs.hpp>

namespace sched {
    struct Task {
        // fixed fields   do not move !!!!!
        usize running_on;
        uptr kernel_stack; // used as the stack during syscalls
        uptr user_stack; // used to preserve the user stack during syscalls
        // the rest are movable

        u16 tid;
        klib::ListHead sched_list;
        mem::vmm::Pagemap *pagemap;
        cpu::InterruptState *gpr_state;
        u64 gs_base, fs_base;
        uptr stack; // the actual stack
        bool blocked;
        klib::Vector<fs::vfs::FileDescriptor*> file_descriptors;
        usize num_file_descriptors; // the actual number
        usize first_free_fdnum; // used for allocating file descriptor numbers
        fs::vfs::DirectoryNode *cwd; // current working directory

        Task();
        int allocate_fdnum();
    };

    void init();
    void start();
    Task* new_kernel_task(uptr ip, bool enqueue);
    Task* new_user_task(fs::vfs::FileNode *elf_file, bool enqueue);
    [[noreturn]] void dequeue_and_die();
    
    [[noreturn]] void syscall_exit(int status);
    
    void scheduler_isr(u64 vec, cpu::InterruptState *gpr_state);
}
