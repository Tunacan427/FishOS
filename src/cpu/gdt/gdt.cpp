#include <cpu/gdt/gdt.hpp>

namespace cpu {
    
static u64 gdt[3] [[gnu::aligned(8)]] = { 0 };
static GDTR gdtr;

void load_gdt() {
    gdt[0] = 0; // null entry
    gdt[1] = ((u64)0b00100000 << 48) | ((u64)0b10011010 << 40); // ring 0 code 64 entry
    gdt[2] = ((u64)0b10010011 << 40); // ring 0 data 64 entry

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (u64)&gdt;
    __flush_gdt(&gdtr);
}

}
