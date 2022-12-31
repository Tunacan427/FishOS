#pragma once

#include <klib/types.hpp>

namespace sched::timer::pit {
    const usize freq = 1193182;
    
    void set_reload(u16 count);
    u16 get_current_count();
    void prepare_sleep(usize ms);
    void perform_sleep();
    void init();
}
