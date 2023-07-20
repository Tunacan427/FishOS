#pragma once

#include <klib/types.hpp>
#include <klib/cstring.hpp>
#include <mem/allocator.hpp>

namespace klib {
    void* malloc(usize size);
    void* calloc(usize size);
    void* realloc(void *ptr, usize size);
    void free(void *ptr);
}
