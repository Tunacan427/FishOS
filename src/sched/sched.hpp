#pragma once

#include <cpu/cpu.hpp>
#include <mem/vmm.hpp>
#include <klib/types.hpp>

namespace sched {
    struct Task {
        // fixed fields   do not move !!!!!
        usize running_on;
        Task *self;
        uptr kernel_stack; // used as the stack during syscalls
        uptr user_stack; // used to preserve the user stack during syscalls
        // the rest are movable

        u16 tid;
        mem::vmm::Pagemap pagemap;
        cpu::InterruptState *gpr_state;
        u64 gs_base, fs_base;
        uptr stack; // the actual stack
        bool blocked;

        Task(u16 tid) : tid(tid), pagemap(), gpr_state(new cpu::InterruptState()), blocked(false) {}
    };

    void init();
    void start();
    Task* new_kernel_task(uptr ip, bool enqueue);
    Task* new_user_task(void *elf_file, bool enqueue);
    [[noreturn]] void dequeue_and_die();
    
    [[noreturn]] void syscall_exit(int status);
    
    void scheduler_isr(u64 vec, cpu::InterruptState *gpr_state);

    struct ScheduleQueue {
        struct QueuedTask {
            Task *task;
            QueuedTask *next;
        };
        
        QueuedTask *head;

        void insert(Task *task);
    };

    struct SleepQueue {
        struct QueuedTask {
            Task *task;
            QueuedTask *next;
            usize delta_time_ns;
        };

        QueuedTask *head;

        void insert(Task *task, usize time_ns);
    };
}
