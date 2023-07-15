#include <sched/timer/apic_timer.hpp>
#include <sched/timer/hpet.hpp>
#include <sched/timer/pit.hpp>
#include <sched/sched.hpp>
#include <klib/lock.hpp>
#include <klib/cstdio.hpp>
#include <cpu/interrupts/interrupts.hpp>
#include <cpu/interrupts/apic.hpp>
#include <cpu/interrupts/idt.hpp>
#include <cpu/cpu.hpp>

using namespace cpu::interrupts;

namespace sched::timer::apic_timer {
    usize freq = 0;
    u8 vector = 0;

    static void interrupt(u64 vec, cpu::InterruptState *state) {
        sched::scheduler_isr(vec, state);
    }

    void stop() {
        LAPIC::write_reg(LAPIC::TIMER_INITIAL, 0);
        LAPIC::mask_vector(LAPIC::LVT_TIMER);
    }

    void oneshot(usize us) {
        stop();

        u32 ticks = us * (freq / 1000000);
        LAPIC::set_vector(LAPIC::LVT_TIMER, vector, false, false, false, false);
        LAPIC::write_reg(LAPIC::TIMER_DIVIDE, 0);
        LAPIC::write_reg(LAPIC::TIMER_INITIAL, ticks);
    }

    void init() {
        vector = allocate_vector();
        load_idt_handler(vector, interrupt);

        stop();

        LAPIC::set_vector(LAPIC::LVT_TIMER, vector, false, false, false, true);
        LAPIC::write_reg(LAPIC::TIMER_DIVIDE, 3); // divide by 16

        if (hpet::is_initialized()) {
            klib::printf("APIC Timer: Using HPET for calibration\n");
            LAPIC::write_reg(LAPIC::TIMER_INITIAL, 0xFFFFFFFF);
            hpet::sleep_ms(100);
        } else {
            klib::printf("APIC Timer: Using PIT for calibration\n");
            pit::prepare_sleep(100);
            LAPIC::write_reg(LAPIC::TIMER_INITIAL, 0xFFFFFFFF);
            pit::perform_sleep();
        }

        freq = (0xFFFFFFFF - LAPIC::read_reg(LAPIC::TIMER_CURRENT)) * 10 * 16; 

        stop();
        
        klib::printf("APIC Timer: Freq: %ld\n", freq);
/*
        LAPIC::write_reg(LAPIC::TIMER_DIVIDE, 3); // divide by 16
        LAPIC::write_reg(LAPIC::TIMER_INITIAL, freq * 4 / 16); // try to sleep 4 seconds with apic timer

        while (LAPIC::read_reg(LAPIC::TIMER_CURRENT))
            asm("pause");
*/
    }
}