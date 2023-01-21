#pragma once

#include <cpu/cpu.hpp>
#include <klib/types.hpp>
#include <mem/vmm.hpp>

namespace sched {
    struct Task {
        // fixed members, do not move !!!!!
        usize running_on;
        uptr stack;

        // movable members
        u16 tid;
        mem::vmm::Pagemap *pagemap;
        cpu::GPRState *gpr_state;
        u64 gs_base, fs_base;
        bool blocked;

        Task(u16 tid) : tid(tid), pagemap(new mem::vmm::Pagemap()), gpr_state(new cpu::GPRState()), blocked(false) {}
    };

    void init();
    void start();
    Task* new_kernel_task(uptr ip, bool enqueue);
    [[noreturn]] void dequeue_and_die();
    
    void scheduler_isr(u64 vec, cpu::GPRState *gpr_state);

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
