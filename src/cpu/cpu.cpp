#include <cpu/cpu.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/interrupts/idt.hpp>
#include <klib/cstdio.hpp>

namespace cpu {
    void init() {
        load_gdt();
        cpu::interrupts::load_idt();

        // enable syscall/sysret instructions
        MSR::write(MSR::IA32_EFER, MSR::read(MSR::IA32_EFER) | 1);

        u64 star = 0 | (u64(GDTSegment::KERNEL_CODE_64) << 32) | ((u64(GDTSegment::USER_CODE_64) - 16) << 48);
        MSR::write(MSR::IA32_STAR, star);
        MSR::write(MSR::IA32_LSTAR, 0); // TODO: replace this with actual syscall handler
    }
}
