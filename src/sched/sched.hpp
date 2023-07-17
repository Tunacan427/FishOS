#pragma once

#include <cpu/cpu.hpp>
#include <mem/vmm.hpp>
#include <klib/types.hpp>
#include <klib/vector.hpp>
#include <klib/list.hpp>
#include <userland/fd.hpp>

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
        klib::Vector<userland::FileDescriptor> file_descriptors;

        Task(u16 tid) : tid(tid), gpr_state(new cpu::InterruptState()), blocked(false) {}
    };

    void init();
    void start();
    Task* new_kernel_task(uptr ip, bool enqueue);
    Task* new_user_task(void *elf_file, bool enqueue);
    [[noreturn]] void dequeue_and_die();
    
    [[noreturn]] void syscall_exit(int status);
    
    void scheduler_isr(u64 vec, cpu::InterruptState *gpr_state);
}
