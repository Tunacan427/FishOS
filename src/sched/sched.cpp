#include "cpu/interrupts/interrupts.hpp"
#include <sched/sched.hpp>
#include <sched/timer/apic_timer.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <kstd/cstring.hpp>
#include <kstd/cstdio.hpp>

namespace sched {
    const usize stack_size = 0x200000; // 2 MiB
    static ScheduleQueue schedule_queue;
    static ScheduleQueue::QueuedThread *current_thread;

    void ScheduleQueue::insert(Thread *thread) {
        if (!head) {
            head = new QueuedThread { thread, nullptr };
            return;
        }

        for (QueuedThread **current = &head; *current; current = &(*current)->next) {
            (*current) = new QueuedThread { thread, *current };
            break;
        }
    }

    void SleepQueue::insert(Thread *thread, usize time_ns) {
        if (!head) {
            head = new QueuedThread { thread, nullptr, time_ns };
            return;
        }

        for (QueuedThread **current = &head; ; current = &(*current)->next) {
            if (*current == nullptr) goto add;
            if (time_ns >= (*current)->delta_time_ns) {
                time_ns -= (*current)->delta_time_ns;
                continue;
            }
        add:
            (*current) = new QueuedThread { thread, *current, time_ns };
            if (auto next = (*current)->next) next->delta_time_ns -= time_ns;
            break;
        }
    }

    Thread* new_kernel_thread(void *pc, bool enqueue) {
        static u16 tid = 0;
        Thread *thread = new Thread(tid++);

        uptr user_stack_phy = (uptr)mem::pmm::alloc_pages(stack_size / 0x1000);
        thread->user_stack = user_stack_phy + mem::vmm::get_hhdm();
        // uptr kernel_stack_phy = (uptr)mem::pmm::alloc_pages(stack_size / 0x1000);
        // thread->kernel_stack = kernel_stack_phy + mem::vmm::get_hhdm();

        thread->running_on = 0;
        thread->pagemap->pml4 = mem::vmm::get_kernel_pagemap()->pml4;
        thread->gpr_state->cs = 40; // 64-bit kernel code 
        thread->gpr_state->ds = 48; // 64-bit kernel data
        thread->gpr_state->es = 48; // 64-bit kernel data
        thread->gpr_state->ss = 48; // 64-bit kernel data
        thread->gpr_state->rflags = 0x202; // only set the interrupt flag 
        thread->gpr_state->rip = (u64)pc;
        thread->gpr_state->rsp = thread->user_stack;

        if (enqueue)
            schedule_queue.insert(thread);

        return thread;
    }

    [[noreturn]] void test_thread_1() {
        while (true)
            kstd::printf("hello from thread 1\n");
    }

    [[noreturn]] void test_thread_2() {
        while (true)
            kstd::printf("hello from thread 2\n");
    }

    void init() {
        new_kernel_thread((void*)test_thread_1, true);
        new_kernel_thread((void*)test_thread_2, true);
        timer::apic_timer::oneshot(10000);
    }

    void scheduler_isr(u64 vec, cpu::GPRState *gpr_state) {
        timer::apic_timer::stop();

        if (current_thread) {
            auto t = current_thread->thread;

            // copy the saved registers into the current thread
            kstd::memcpy(t->gpr_state, gpr_state, sizeof(cpu::GPRState));
            t->gs_base = cpu::read_kernel_gs_base();
            t->fs_base = cpu::read_fs_base();
        }
        
        // switch to the next thread
        if (current_thread && current_thread->next) 
            current_thread = current_thread->next;
        else 
            current_thread = schedule_queue.head;

        cpu::write_gs_base((u64)current_thread->thread);
        cpu::write_fs_base(current_thread->thread->fs_base);
        
        if (!current_thread->thread->pagemap->active)
            mem::vmm::activate_pagemap(current_thread->thread->pagemap);
        
        // load the thread's registers  
        kstd::memcpy(gpr_state, current_thread->thread->gpr_state, sizeof(cpu::GPRState));

        cpu::interrupts::eoi();
        timer::apic_timer::oneshot(10000);
    }
}
