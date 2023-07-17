#pragma once

#include <klib/types.hpp>

namespace ps2::kbd {
    constexpr usize buffer_size = 256;

    void init();

    // returns the number of bytes read. will block until keyboard buffer is flushed
    usize read(void *buf, usize count);
}
