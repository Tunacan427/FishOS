#pragma once

#include <klib/types.hpp>
#include <acpi/tables.hpp>

namespace sched::timer::hpet {
    void stall_ms(usize ms);
    void stall_us(usize us);
    void stall_ns(usize ns);
    bool is_initialized();
    void init(acpi::HPET *table);
}
