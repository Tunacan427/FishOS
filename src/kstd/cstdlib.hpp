#pragma once

#include <types.hpp>
#include <mem/allocator.hpp>
#include <kstd/cstring.hpp>

namespace kstd {
    void* malloc(usize size);
    void* calloc(usize size);
    void* realloc(void *ptr, usize size);
    void free(void *ptr);
}
