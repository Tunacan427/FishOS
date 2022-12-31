#pragma once

#include <klib/types.hpp>
#include <acpi/tables.hpp>

namespace sched::timer::hpet {
    void sleep_ms(usize ms);
    void sleep_us(usize us);
    void sleep_ns(usize ns);
    bool is_initialized();
    void init(acpi::HPET *table);
}
