#pragma once

#include <types.hpp>

namespace cpu {

struct [[gnu::packed]] GDTR {
    u16 limit;
    u64 base;
};

extern "C" void __flush_gdt(GDTR *gdtr);
void load_gdt();

}
