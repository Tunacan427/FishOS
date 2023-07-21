#include <sched/sched.hpp>
#include <sched/timer/apic_timer.hpp>
#include <mem/pmm.hpp>
#include <mem/vmm.hpp>
#include <cpu/cpu.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <cpu/syscall/syscall.hpp>
#include <klib/cstring.hpp>
#include <klib/cstdio.hpp>
#include <userland/elf.hpp>
#include <gfx/framebuffer.hpp>

namespace sched {
    const usize stack_size = 64 * 1024; // 64 KiB
    static klib::ListHead sched_list_head;

    int Task::allocate_fdnum() {
        num_file_descriptors++;
        for (usize i = first_free_fdnum; i < file_descriptors.size(); i++) {
            if (file_descriptors[i] == nullptr) {
                first_free_fdnum = i;
                return i;
            }
        }
        first_free_fdnum = file_descriptors.size();
        file_descriptors.push_back(nullptr);
        return file_descriptors.size() - 1;
    }
    
    static u16 last_tid = 0;
    
    Task::Task() {
        tid = last_tid++;
        gpr_state = new cpu::InterruptState();
        blocked = false;
        num_file_descriptors = 0;
        first_free_fdnum = 0;
    }

    Task* new_kernel_task(uptr ip, bool enqueue) {
        Task *task = new Task();

        uptr stack_phy = mem::pmm::alloc_pages(stack_size / 0x1000);
        task->stack = stack_phy + stack_size + mem::vmm::get_hhdm();

        task->running_on = 0;
        task->pagemap = mem::vmm::get_kernel_pagemap();
        task->gpr_state->cs = u64(cpu::GDTSegment::KERNEL_CODE_64);
        task->gpr_state->ds = u64(cpu::GDTSegment::KERNEL_DATA_64);
        task->gpr_state->es = u64(cpu::GDTSegment::KERNEL_DATA_64);
        task->gpr_state->ss = u64(cpu::GDTSegment::KERNEL_DATA_64);
        task->gpr_state->rflags = 0x202; // only set the interrupt flag 
        task->gpr_state->rip = ip;
        task->gpr_state->rsp = task->stack;

        if (enqueue)
            sched_list_head.add_before(&task->sched_list);

        return task;
    }

    Task* new_user_task(fs::vfs::FileNode *elf_file, bool enqueue) {
        Task *task = new Task();

        task->pagemap = new mem::vmm::Pagemap();
        task->pagemap->pml4 = (u64*)(mem::pmm::alloc_pages(1) + mem::vmm::get_hhdm());
        klib::memset(task->pagemap->pml4, 0, 0x1000);
        task->pagemap->map_kernel();
        task->pagemap->range_list_head.init();

        uptr kernel_stack_phy = mem::pmm::alloc_pages(stack_size / 0x1000);
        task->kernel_stack = kernel_stack_phy + stack_size + mem::vmm::get_hhdm();
        
        uptr ip = userland::elf::load(task->pagemap, elf_file, &task->mmap_anon_base);

        task->stack = task->mmap_anon_base;
        for (usize i = 0; i < stack_size / 0x1000; i++) {
            uptr page_phy = mem::pmm::alloc_pages(1);
            klib::memset((void*)(page_phy + mem::vmm::get_hhdm()), 0, 0x1000);
            task->pagemap->map_page(page_phy, task->mmap_anon_base, PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE | PAGE_NO_EXECUTE);
            task->mmap_anon_base += 0x1000;
        }

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

        task->file_descriptors.push_back(new fs::vfs::FileDescriptor()); // stdin
        task->file_descriptors.push_back(new fs::vfs::FileDescriptor()); // stdout
        task->file_descriptors.push_back(new fs::vfs::FileDescriptor()); // stderr
        task->num_file_descriptors = 3;
        task->first_free_fdnum = 3;
        task->cwd = fs::vfs::root_dir();

        if (enqueue)
            sched_list_head.add_before(&task->sched_list);

        return task;
    }

    [[noreturn]] static void test_task_1() {
        klib::printf("Hello from task 1!\n");
        auto fb = gfx::screen_fb();
        i64 x = fb.width / 2, y = 0, old_x = x, old_y = y, x_inc = true, y_inc = true;
        while (true) {
            fb.fill_rect(x, y, 16, 16, 0x0000FF);
            x = x_inc ? x + 1 : x - 1;
            y = y_inc ? y + 1 : y - 1;
            if (x + 16 >= fb.width) x_inc = false;
            if (x <= fb.width / 2) x_inc = true;
            if (y + 16 >= fb.height) y_inc = false;
            if (y <= 0) y_inc = true;
            
            asm volatile("hlt");
            
            fb.fill_rect(old_x, old_y, 16, 16, 0x6565CE);
            old_x = x;
            old_y = y;
        }
    }

    [[noreturn]] static void test_task_2() {
        klib::printf("Hello from task 2!\n");
        auto fb = gfx::screen_fb();
        i64 x = fb.width, y = fb.height, old_x = x, old_y = y, x_inc = false, y_inc = false;
        while (true) {
            fb.fill_rect(x, y, 16, 16, 0xFF0000);
            x = x_inc ? x + 1 : x - 1;
            y = y_inc ? y + 1 : y - 1;
            if (x + 16 >= fb.width) x_inc = false;
            if (x <= fb.width / 2) x_inc = true;
            if (y + 16 >= fb.height) y_inc = false;
            if (y <= 0) y_inc = true;

            asm volatile("hlt");
            
            fb.fill_rect(old_x, old_y, 16, 16, 0xBC5151);
            old_x = x;
            old_y = y;
        }
    }

    void init() {
        sched_list_head.init();
        new_kernel_task(uptr(test_task_1), true);
        new_kernel_task(uptr(test_task_2), true);
    }

    void start() {
        sched::timer::apic_timer::oneshot(1000);
    }

    [[noreturn]] void dequeue_and_die() {
        asm("cli");

        Task *current_task = (Task*)cpu::read_gs_base();
        current_task->sched_list.remove();

        asm("sti");
        while (true) asm("hlt");
    }

    [[noreturn]] void syscall_exit(int status) {
#if SYSCALL_TRACE
        klib::printf("exit(%d)\n", status);
#endif
        dequeue_and_die();
    }

    void scheduler_isr(u64 vec, cpu::InterruptState *gpr_state) {
        timer::apic_timer::stop();

        Task *current_task = (Task*)cpu::read_gs_base();
        if (current_task) {
            // copy the saved registers into the current task
            klib::memcpy(current_task->gpr_state, gpr_state, sizeof(cpu::InterruptState));
            current_task->gs_base = cpu::read_kernel_gs_base(); // this was the regular gs base before the swapgs of the interrupt (if it was a kernel thread then the kernel gs base is the same anyway)
            current_task->fs_base = cpu::read_fs_base();
        }

        // switch to the next task
        if (current_task && current_task->sched_list.next && current_task->sched_list.next != &sched_list_head)
            current_task = LIST_ENTRY(current_task->sched_list.next, Task, sched_list);
        else if (!sched_list_head.empty())
            current_task = LIST_ENTRY(sched_list_head.next, Task, sched_list);
        else
            panic("No tasks in scheduler list");
        
        if ((current_task->gpr_state->cs & 3) == 3) // user thread
            cpu::write_kernel_gs_base(current_task->gs_base); // will be swapped to be the regular gs base
        else
            cpu::write_kernel_gs_base((u64)current_task);
        cpu::write_gs_base((u64)current_task); // will be swapped to be the kernel gs base
        cpu::write_fs_base(current_task->fs_base);

        current_task->pagemap->activate();

        // load the task's registers
        klib::memcpy(gpr_state, current_task->gpr_state, sizeof(cpu::InterruptState));

        cpu::interrupts::eoi();
        timer::apic_timer::oneshot(1000000 / 300); // 300 Hz
    }
}
