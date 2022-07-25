#pragma once

#include <cpu/cpu.hpp>
#include <kstd/types.hpp>
#include <mem/vmm.hpp>

namespace sched {
    struct Thread {
        // fixed members, do not move !!!!!
        usize running_on;
        uptr user_stack;
        uptr kernel_stack;

        // movable members
        u16 tid;
        mem::vmm::Pagemap *pagemap;
        cpu::GPRState *gpr_state;
        u64 gs_base, fs_base;
        bool blocked;

        Thread(u16 tid) : tid(tid), pagemap(new mem::vmm::Pagemap()), gpr_state(new cpu::GPRState()), blocked(false) {}
    };

    void init();
    Thread* new_kernel_thread(void *pc, bool enqueue);
    
    void scheduler_isr(u64 vec, cpu::GPRState *gpr_state);

    struct ScheduleQueue {
        struct QueuedThread {
            Thread *thread;
            QueuedThread *next;
        };
        
        QueuedThread *head;

        void insert(Thread *thread);
    };

    struct SleepQueue {
        struct QueuedThread {
            Thread *thread;
            QueuedThread *next;
            usize delta_time_ns;
        };

        QueuedThread *head;

        void insert(Thread *thread, usize time_ns);
    };
}
