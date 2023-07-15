#include <sched/sched.hpp>
#include <sched/timer/apic_timer.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <klib/cstring.hpp>
#include <klib/cstdio.hpp>
#include <elf/elf.hpp>

namespace sched {
    const usize stack_size = 0x10000; // 64 KiB
    static ScheduleQueue schedule_queue;
    static ScheduleQueue::QueuedTask *current_task;

    void ScheduleQueue::insert(Task *task) {
        if (!head) {
            head = new QueuedTask { task, nullptr };
            return;
        }

        for (QueuedTask **current = &head; *current; current = &(*current)->next) {
            (*current) = new QueuedTask { task, *current };
            break;
        }
    }

    void SleepQueue::insert(Task *task, usize time_ns) {
        if (!head) {
            head = new QueuedTask { task, nullptr, time_ns };
            return;
        }

        for (QueuedTask **current = &head; ; current = &(*current)->next) {
            if (*current == nullptr) goto add;

            if (time_ns >= (*current)->delta_time_ns) {
                time_ns -= (*current)->delta_time_ns;
                continue;
            }

        add:
            (*current) = new QueuedTask { task, *current, time_ns };
            if (auto next = (*current)->next) next->delta_time_ns -= time_ns;
            break;
        }
    }

    Task* new_kernel_task(uptr ip, bool enqueue) {
        static u16 tid = 0;
        Task *task = new Task(tid++);

        uptr stack_phy = mem::pmm::alloc_pages(stack_size / 0x1000);
        task->stack = stack_phy + stack_size + mem::vmm::get_hhdm();

        task->running_on = 0;
        task->pagemap.pml4 = mem::vmm::get_kernel_pagemap()->pml4;
        task->gpr_state->cs = u64(cpu::GDTSegment::KERNEL_CODE_64);
        task->gpr_state->ds = u64(cpu::GDTSegment::KERNEL_DATA_64);
        task->gpr_state->es = u64(cpu::GDTSegment::KERNEL_DATA_64);
        task->gpr_state->ss = u64(cpu::GDTSegment::KERNEL_DATA_64);
        task->gpr_state->rflags = 0x202; // only set the interrupt flag 
        task->gpr_state->rip = ip;
        task->gpr_state->rsp = task->stack;

        if (enqueue)
            schedule_queue.insert(task);

        return task;
    }

    Task* new_user_task(void *elf_file, bool enqueue) {
        static u16 tid = 0;
        Task *task = new Task(tid++);

        task->pagemap.pml4 = (u64*)(mem::pmm::alloc_pages(1) + mem::vmm::get_hhdm());
        klib::memset(task->pagemap.pml4, 0, 0x1000);
        task->pagemap.map_kernel();

        uptr stack_phy = mem::pmm::alloc_pages(stack_size / 0x1000);
        task->stack = stack_phy + stack_size;
        task->pagemap.map_pages(stack_phy, stack_phy, stack_size, PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE | PAGE_NO_EXECUTE);

        uptr kernel_stack_phy = mem::pmm::alloc_pages(stack_size / 0x1000);
        task->kernel_stack = kernel_stack_phy + stack_size + mem::vmm::get_hhdm();

        uptr ip = elf::load(&task->pagemap, (uptr)elf_file);

        // task->pagemap->activate();

        task->running_on = 0;
        task->gpr_state->cs = u64(cpu::GDTSegment::USER_CODE_64) | 3;
        task->gpr_state->ds = u64(cpu::GDTSegment::USER_DATA_64) | 3;
        task->gpr_state->es = u64(cpu::GDTSegment::USER_DATA_64) | 3;
        task->gpr_state->ss = u64(cpu::GDTSegment::USER_DATA_64) | 3;
        task->gpr_state->rflags = 0x202;
        task->gpr_state->rip = ip;
        task->gpr_state->rsp = task->stack;
        task->gs_base = 0;
        task->fs_base = 0;

        if (enqueue)
            schedule_queue.insert(task);

        return task;
    }

    static const i64 test_speed = 1;

    [[noreturn]] static void test_task_1() {
        klib::printf("Hello from task 1!\n");
        auto fb = gfx::screen_fb();
        i64 x = fb.width / 2, y = 0, old_x = x, old_y = y, frames = 0, x_inc = true, y_inc = true;
        while (true) {
            fb.fill_rect(x, y, 16, 16, 0x0000FF);
            x = x_inc ? x + 1 : x - 1;
            y = y_inc ? y + 1 : y - 1;
            frames++;
            if (x + 16 >= fb.width) x_inc = false;
            if (x <= fb.width / 2) x_inc = true;
            if (y + 16 >= fb.height) y_inc = false;
            if (y <= 0) y_inc = true;
            if (frames == test_speed) {
                frames = 0;
                asm volatile("hlt");
            }
            fb.fill_rect(old_x, old_y, 16, 16, 0x6565CE);
            old_x = x;
            old_y = y;
        }
    }

    [[noreturn]] static void test_task_2() {
        klib::printf("Hello from task 2!\n");
        auto fb = gfx::screen_fb();
        i64 x = fb.width, y = fb.height, old_x = x, old_y = y, frames = 0, x_inc = false, y_inc = false;
        while (true) {
            fb.fill_rect(x, y, 16, 16, 0xFF0000);
            x = x_inc ? x + 1 : x - 1;
            y = y_inc ? y + 1 : y - 1;
            frames++;
            if (x + 16 >= fb.width) x_inc = false;
            if (x <= fb.width / 2) x_inc = true;
            if (y + 16 >= fb.height) y_inc = false;
            if (y <= 0) y_inc = true;
            if (frames == test_speed) {
                frames = 0;
                asm volatile("hlt");
            }
            fb.fill_rect(old_x, old_y, 16, 16, 0xBC5151);
            old_x = x;
            old_y = y;
        }
    }

    void init() {
        new_kernel_task(uptr(test_task_1), true);
        new_kernel_task(uptr(test_task_2), true);
    }

    void start() {
        sched::timer::apic_timer::oneshot(1000);
    }

    [[noreturn]] void dequeue_and_die() {
        cpu::cli();
        
        ScheduleQueue::QueuedTask *prev = nullptr;
        for (auto current = schedule_queue.head; ; current = current->next) {
            if (current == current_task) {
                if (prev)
                    prev->next = current->next;
                else
                    schedule_queue.head = current->next;
                
                break;
            }

            prev = current;
        }

        cpu::sti();
        while (true) asm("hlt");
    }

    [[noreturn]] void syscall_exit(int status) {
        klib::printf("exit(status: %d)\n", status);
        dequeue_and_die();
    }

    void scheduler_isr(u64 vec, cpu::InterruptState *gpr_state) {
        timer::apic_timer::stop();

        if (current_task) {
            auto t = current_task->task;

            // copy the saved registers into the current task
            klib::memcpy(t->gpr_state, gpr_state, sizeof(cpu::InterruptState));
            t->gs_base = cpu::read_kernel_gs_base(); // this was the regular gs base before the swapgs of the interrupt (if it was a kernel thread then the kernel gs base is the same anyway)
            t->fs_base = cpu::read_fs_base();
        }
        
        // switch to the next task
        if (current_task && current_task->next)
            current_task = current_task->next;
        else
            current_task = schedule_queue.head;

        cpu::write_kernel_gs_base((u64)current_task->task);

        current_task->task->pagemap.activate();
        
        if ((current_task->task->gpr_state->cs & 3) == 3) // user thread
            cpu::write_kernel_gs_base(current_task->task->gs_base); // will be swapped to be the regular gs base
        else
            cpu::write_kernel_gs_base((u64)current_task->task);
        cpu::write_gs_base((u64)current_task->task); // will be swapped to be the kernel gs base
        cpu::write_fs_base(current_task->task->fs_base);

        // load the task's registers  
        klib::memcpy(gpr_state, current_task->task->gpr_state, sizeof(cpu::InterruptState));

        cpu::interrupts::eoi();
        timer::apic_timer::oneshot(1000);
    }
}
