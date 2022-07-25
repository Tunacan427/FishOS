#pragma once

#include <kstd/types.hpp>

namespace sched::timer::apic_timer {
    void stop();
    void oneshot(usize us);
    void init();
}
