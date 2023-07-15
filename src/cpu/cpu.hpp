#pragma once

#include <klib/types.hpp>
#include <limine.hpp>
#include <panic.hpp>

namespace cpu {
    void early_init();
    void smp_init(limine_smp_response *smp_res);
    void init(limine_smp_info *info);

    struct [[gnu::packed]] TSS {
        u32 reserved0;
        u64 rsp0, rsp1, rsp2;
        u64 reserved1;
        u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
        u64 reserved2;
        u16 reserved3;
        u16 io_map_base;
    };

    struct [[gnu::packed]] Local {
        u64 cpu_number;
        bool is_bsp;
        TSS tss;
        u64 lapic_id;
        u64 lapic_timer_freq;
    };
    
    struct [[gnu::packed]] InterruptState {
        u64 ds, es;
        u64 r15, r14, r13, r12, r11, r10, r9, r8;
        u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
        u64 err; // might be pushed by the cpu, set to 0 by the interrupt wrapper if not
        u64 rip, cs, rflags, rsp, ss; // all pushed by the cpu
    };

    namespace MSR {
        enum R : u32 {
            IA32_APIC_BASE = 0x1B,
            IA32_PAT = 0x277,
            IA32_EFER = 0xC0000080,
            IA32_STAR = 0xC0000081,
            IA32_LSTAR = 0xC0000082,
            IA32_FMASK = 0xC0000084,
            IA32_FS_BASE = 0xC0000100,
            IA32_GS_BASE = 0xC0000101,
            IA32_KERNEL_GS_BASE = 0xC0000102
        };

        static inline u64 read(R msr) {
            volatile u32 lo, hi;
            asm volatile("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));
            return ((u64)hi << 32) | lo;
        }
        
        static inline void write(R msr, u64 val) {
            volatile u32 lo = val & 0xFFFFFFFF;
            volatile u32 hi = val >> 32;
            asm volatile("wrmsr" : : "a" (lo), "d" (hi), "c" (msr));
        }
    };

    static inline void cli() {
        asm volatile("cli");
    }

    static inline void sti() {
        asm volatile("sti");
    }

    static inline void cpuid(u32 leaf, u32 subleaf, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
        asm volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) : "a" (leaf), "c" (subleaf));
    }
    
    static inline void write_cr3(u64 cr3) {
        asm volatile("mov %0, %%cr3" : : "r" (cr3));
    }

    static inline void write_cr4(u64 cr4) {
        asm volatile("mov %0, %%cr4" : : "r" (cr4));
    }

    static inline u64 read_cr2() {
        volatile u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r" (cr2));
        return cr2;
    }

    static inline u64 read_cr3() {
        volatile u64 cr3;
        asm volatile("mov %%cr3, %0" : "=r" (cr3));
        return cr3;
    }

    static inline u64 read_cr4() {
        volatile u64 cr4;
        asm volatile("mov %%cr4, %0" : "=r" (cr4));
        return cr4;
    }

    static inline void write_gs_base(u64 gs) {
        MSR::write(MSR::IA32_GS_BASE, gs);
    }

    static inline void write_kernel_gs_base(u64 gs) {
        MSR::write(MSR::IA32_KERNEL_GS_BASE, gs);
    }

    static inline void write_fs_base(u64 fs) {
        MSR::write(MSR::IA32_FS_BASE, fs);
    }

    static inline u64 read_gs_base() {
        return MSR::read(MSR::IA32_GS_BASE);
    }

    static inline u64 read_kernel_gs_base() {
        return MSR::read(MSR::IA32_KERNEL_GS_BASE);
    }

    static inline u64 read_fs_base() {
        return MSR::read(MSR::IA32_FS_BASE);
    }

    static inline void invlpg(void *m) {
        asm volatile("invlpg (%0)" : : "r" (m) : "memory");
    }
    
    template<klib::Integral T> 
    static inline T bswap(T val) {
        volatile T result;
        asm volatile("bswap %0" : "=r" (result) : "r" (val));
        return result;
    }
    
    template<klib::Integral T> static inline void out(const u16 port, const T val) {
        panic("cpu::out must be used with u8, u16, or u32");
    }

    template<>
    inline void out<u8>(const u16 port, const u8 val) {
        asm volatile("outb %0, %1" : : "a" (val), "Nd" (port));
    }

    template<>
    inline void out<u16>(const u16 port, const u16 val) {
        asm volatile("outw %0, %1" : : "a" (val), "Nd" (port));
    }

    template<>
    inline void out<u32>(const u16 port, const u32 val) {
        asm volatile("outl %0, %1" : : "a" (val), "Nd" (port));
    }
    
    template<klib::Integral T> static inline T in(const u16 port) {
        panic("cpu::in must be used with u8, u16, or u32");
    }

    template<>
    inline u8 in<u8>(const u16 port) {
        volatile u8 ret;
        asm volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
        return ret;
    }

    template<>
    inline u16 in<u16>(const u16 port) {
        volatile u16 ret;
        asm volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
        return ret;
    }

    template<>
    inline u32 in<u32>(const u16 port) {
        volatile u32 ret;
        asm volatile("inl %1, %0" : "=a" (ret) : "Nd" (port));
        return ret;
    }
}
