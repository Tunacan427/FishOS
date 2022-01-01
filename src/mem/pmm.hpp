#pragma once

#include <types.hpp>
#include <stivale2.h>

namespace mem {
namespace pmm {

struct MemMap {
    u64 entries;
    stivale2_mmap_entry internal[];


};

}
}
