#include <kstd/lock.hpp>
#include <kstd/cstdio.hpp>
#include <cpu/idt/idt.hpp>
#include <cpu/pic/pic.hpp>
#include <ioports.hpp>

static volatile kstd::Spinlock pit_lock;

namespace sched::pit {
    static void irq(cpu::InterruptFrame *frame) {
        kstd::putchar('h');
        cpu::pic::send_eoi(0);
    }

    void setup() {
        cpu::pic::clear_mask(0);
        cpu::load_idt_handler(32, irq);
    }
}