#pragma once

#include <kstd/types.hpp>
#include <limine.hpp>

namespace terminal {
    void init(limine_terminal_response *terminal_res);

    /*
    void set_width(usize width);
    void set_height(usize height);
    */

    void write_char(char c);
}
