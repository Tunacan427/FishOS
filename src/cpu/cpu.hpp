#pragma once

#include <kstd/types.hpp>
#include <panic.hpp>

namespace cpu {
    static inline void cli() {
        asm volatile("cli");
    }

    static inline void sti() {
        asm volatile("sti");
    }

    static inline void cpuid(u32 leaf, u32 subleaf, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
        asm volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) : "a" (leaf), "c" (subleaf));
    }

    static inline u64 read_msr(u32 msr) {
        volatile u32 lo, hi;
        asm volatile("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));
        return ((u64)hi << 32) | lo;
    }
    
    static inline void write_msr(u32 msr, u64 val) {
        volatile u32 lo = val & 0xFFFFFFFF;
        volatile u32 hi = val >> 32;
        asm volatile("wrmsr" : : "a" (lo), "d" (hi), "c" (msr));
    }

    static inline void write_cr3(u64 cr3) {
        asm volatile("mov %0, %%cr3" : : "r" (cr3));
    }

    static inline void write_cr4(u64 cr4) {
        asm volatile("mov %0, %%cr4" : : "r" (cr4));
    }

    static inline u64 read_cr4() {
        volatile u64 cr4;
        asm volatile("mov %%cr4, %0" : "=r" (cr4));
        return cr4;
    }

    static inline u64 read_cr2() {
        volatile u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r" (cr2));
        return cr2;
    }

    static inline void invlpg(void *m) {
        asm volatile("invlpg (%0)" : : "r" (m) : "memory");
    }
    
    template<kstd::Integral T> 
    static inline T bswap(T val) {
        volatile T result;
        asm volatile("bswap %0" : "=r" (result) : "r" (val));
        return result;
    }
    
    template<kstd::Integral T> static inline void out(const u16 port, const T val) {
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
    
    template<kstd::Integral T> static inline T in(const u16 port) {
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

    static inline void io_wait() {
        out<u8>(0x80, 0); 
    }
}
