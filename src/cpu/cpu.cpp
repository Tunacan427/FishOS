#include "limine.hpp"
#include <cpu/cpu.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/interrupts/idt.hpp>
#include <klib/cstdio.hpp>

namespace cpu {
    void smp_init(limine_smp_response *smp_res) {
        klib::printf("CPU: SMP | x2APIC: %s\n", (smp_res->flags & 1) ? "yes" : "no");
        for (u32 i = 0; i < smp_res->cpu_count; i++) {
            auto cpu = smp_res->cpus[i];
            auto is_bsp = cpu->lapic_id == smp_res->bsp_lapic_id;
            klib::printf("\tCore %d%s | Processor ID: %d, LAPIC ID: %d\n", i, is_bsp ? " (BSP)" : "", cpu->processor_id, cpu->lapic_id);
            if (!is_bsp)
                __atomic_store_n(&cpu->goto_address, &init, __ATOMIC_SEQ_CST);
            else
                init(cpu);
        }
    }

    void early_init() {
        load_gdt();
        
        interrupts::load_idt();

        // hardcode PAT
        // 0: WB  1: WT  2: UC-  3: UC  4: WB  5: WT  6: WC  7: WP
        MSR::write(MSR::IA32_PAT, 0x501040600070406);
        
        // enable syscall/sysret instructions
        MSR::write(MSR::IA32_EFER, MSR::read(MSR::IA32_EFER) | 1);

        u64 star = 0 | (u64(GDTSegment::KERNEL_CODE_64) << 32) | ((u64(GDTSegment::USER_CODE_64) - 16) << 48);
        MSR::write(MSR::IA32_STAR, star);
        MSR::write(MSR::IA32_LSTAR, 0); // TODO: replace this with actual syscall handler
    }

    void init(limine_smp_info *info) {
        reload_gdt();
        interrupts::load_idt();

        // hardcode PAT
        // 0: WB  1: WT  2: UC-  3: UC  4: WB  5: WT  6: WC  7: WP
        MSR::write(MSR::IA32_PAT, 0x501040600070406);
        
        // enable syscall/sysret instructions
        MSR::write(MSR::IA32_EFER, MSR::read(MSR::IA32_EFER) | 1);

        u64 star = 0 | (u64(GDTSegment::KERNEL_CODE_64) << 32) | ((u64(GDTSegment::USER_CODE_64) - 16) << 48);
        MSR::write(MSR::IA32_STAR, star);
        MSR::write(MSR::IA32_LSTAR, 0); // TODO: replace this with actual syscall handler
    }
}
